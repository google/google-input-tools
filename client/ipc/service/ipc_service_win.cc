/*
  Copyright 2014 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <windows.h>
#include <wtsapi32.h>
#include <atlbase.h>
#include <atlsecurity.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "common/app_const.h"
#include "components/win_frontend/ipc_console.h"
#include "ipc/service/resource.h"

// Service will check console process is running or not every 100 milliseconds.
static const DWORD kCheckProcessInterval = 100;

// Define scoped string allocated by |WTSFreeMemory|.
DEFINE_SCOPED_HANDLE(ScopedWTSSessionInfo, PWTS_SESSION_INFO, WTSFreeMemory);
// Define scoped service handle freed by |CloseServiceHandle|.
DEFINE_SCOPED_HANDLE(ScopedServiceHandle, SC_HANDLE, CloseServiceHandle);

namespace ipc {

class CAtlIPCServiceModule
    : public ATL::CAtlServiceModuleT<CAtlIPCServiceModule, IDS_SERVICENAME> {
 public:
  DECLARE_REGISTRY_APPID_RESOURCEID(
      IDR_ATL_IPC_SERVICE, "{F0E5712C-95FA-453C-8F40-DAB3A33846A0}")

  HRESULT InitializeSecurity() throw() {
    CAccessToken process_token;
    {
      HANDLE token = NULL;
      if (!::OpenProcessToken(::GetCurrentProcess(),
                              TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              &token)) {
        DLOG(ERROR) << "OpenProcessToken for adjust privilege failed"
                    << ::GetLastError();
        return E_FAIL;
      }
      process_token.Attach(token);
    }

    // Required by WTSQueryUserToken and CreateProcessAsUser.
    if (!process_token.EnablePrivilege(SE_TCB_NAME) ||
        !process_token.EnablePrivilege(SE_ASSIGNPRIMARYTOKEN_NAME) ||
        !process_token.EnablePrivilege(SE_INCREASE_QUOTA_NAME)) {
      DLOG(ERROR) << "Can't enable privilege" << ::GetLastError();
      return E_FAIL;
    }

    return S_OK;
  }

  // Overridden from CAtlServiceModuleT<xxx>:
  HRESULT PreMessageLoop(int show_cmd) throw() {
    quit_event_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    DCHECK(quit_event_);
    ATL::CAtlServiceModuleT<CAtlIPCServiceModule, IDS_SERVICENAME>::
        PreMessageLoop(show_cmd);
    return S_OK;
  }

  void RunMessageLoop() throw() {
    // Set service status to running in case |PreMessageLoop| doesn't do it.
    SetServiceStatus(SERVICE_RUNNING);
    while (true) {
      DWORD ret = ::WaitForSingleObject(quit_event_, kCheckProcessInterval);
      if (ret == WAIT_OBJECT_0)
        break;
      DCHECK_EQ(ret, WAIT_TIMEOUT);

      ForkProcessesIntoAllSessions();
    }
  }

  HRESULT PostMessageLoop() throw() {
    ::CloseHandle(quit_event_);
    quit_event_ = NULL;
    return CAtlServiceModuleT<CAtlIPCServiceModule, IDS_SERVICENAME>::
        PostMessageLoop();
  }

  void OnStop() throw() {
    // quit_event_ is not thread-safe, but it's ok for a simple logic service.
    if (quit_event_)
      ::SetEvent(quit_event_);
    CAtlServiceModuleT<CAtlIPCServiceModule, IDS_SERVICENAME>::
        OnStop();
  }

  // Override |CAtlServiceModuleT::RegisterAppId| to customize installation.
  HRESULT RegisterAppId(_In_ bool service = false) throw() {
    if (!Uninstall())
      return E_FAIL;

    HRESULT hr = UpdateRegistryAppId(TRUE);
    if (FAILED(hr))
      return hr;

    CRegKey key_app_id;
    LONG res = key_app_id.Open(HKEY_CLASSES_ROOT, L"AppID", KEY_WRITE);
    if (res != ERROR_SUCCESS)
      return AtlHresultFromWin32(res);

    CRegKey local_service_key;
    res = local_service_key.Create(key_app_id, GetAppIdT());
    if (res != ERROR_SUCCESS)
      return AtlHresultFromWin32(res);

    local_service_key.DeleteValue(L"LocalService");
    if (!service)
      return S_OK;
    local_service_key.SetStringValue(
        L"LocalService", ime_goopy::kIPCServiceName);

    // Create auto start service.
    return InstallAutoStartService();
  }

 private:
  HRESULT InstallAutoStartService() {
    if (IsInstalled())
      return S_OK;

    // Get the executable file path.
    ScopedServiceHandle service_manager;
    {
      SC_HANDLE sc_manager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
      if (!sc_manager) {
        DLOG(ERROR) << "OpenSCManager failed error = " << ::GetLastError();
        return E_FAIL;
      }
      service_manager.Reset(sc_manager);
    }

    ScopedServiceHandle service;
    {
      // Follow ATL's style, quote the file path with "".
      wchar_t file_path[MAX_PATH + 3] = {0};
      DWORD path_len = ::GetModuleFileName(NULL, file_path + 1, MAX_PATH);
      if (!path_len || path_len == MAX_PATH) {
        DLOG(ERROR) << "Binary path too long.";
        return E_FAIL;
      }
      file_path[0] = L'\"';
      file_path[path_len + 1] = L'\"';
      file_path[path_len + 2] = 0;

      // Create the service.
      SC_HANDLE service_handle = ::CreateService(
          service_manager.Get(),
          ime_goopy::kIPCServiceName,  // Service name.
          ime_goopy::kIPCServiceName,  // Display name.
          SERVICE_ALL_ACCESS,  // All access rights.
          SERVICE_WIN32_OWN_PROCESS,  // Service owns a process.
          SERVICE_AUTO_START,  // Service will auto start.
          SERVICE_ERROR_NORMAL,  // The error of startup will be logged in the
          // event log.
          file_path,  // The path of service executable file.
          NULL,  // The service is not a member of any group.
          NULL,  // Using default tag.
          NULL,  // No dependence.
          NULL,  // Start under LocalSystem account.
          NULL  // No password.
          );
      if (!service_handle) {
        DLOG(ERROR) << "CreateService failed error = " << ::GetLastError();
        return E_FAIL;
      }
      service.Reset(service_handle);
    }

    // Configure the service to auto-restart on failure.
    SERVICE_FAILURE_ACTIONS failure_actions = {0};
    failure_actions.dwResetPeriod = 0;  // Reset failure counter immediately.
    failure_actions.lpRebootMsg = NULL;  // Don't change reboot information.
    failure_actions.lpCommand = NULL;  // Don't change command on failure.
    failure_actions.cActions = 1;  // One action is enough since the counter is
    // always reset to 0.
    SC_ACTION actions[1] = { {SC_ACTION_RESTART, 1000} };  // Wait 1 second and
    // restart the service.
    failure_actions.lpsaActions = actions;
    if (!::ChangeServiceConfig2(
        service.Get(), SERVICE_CONFIG_FAILURE_ACTIONS, &failure_actions)) {
      DLOG(ERROR) << "ChangeServiceConfig2 failed error = " << ::GetLastError();
      return E_FAIL;
    }
    return S_OK;
  }

  void ForkProcessesIntoAllSessions() {
    // Start checking if ipc_console is started in every session.
    DWORD session_count = 0;

    PWTS_SESSION_INFO sessions_info = NULL;
    if (!::WTSEnumerateSessions(
        WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions_info, &session_count)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << "WTSEnumerateSessions failed error = " << error;
      return;
    }

    for (DWORD i = 0; i < session_count; ++i) {
      if (sessions_info[i].State != WTSActive ||
          AlreadyLaunchedInSession(sessions_info[i].SessionId)) {
        continue;
      }

      LaunchProcessInSession(sessions_info[i].SessionId);
    }
    ::WTSFreeMemory(sessions_info);
  }

  bool AlreadyLaunchedInSession(DWORD session_id) {
    // If the binary is already launched, it will create a event using
    // pre-defined name, so the service could check the name to determine its
    // existence. Launch twice will also work because the second process
    // will quit if it fails to create the event.
    wchar_t event_name[MAX_PATH] = {0};
    _snwprintf(event_name,
               MAX_PATH,
               L"%s%d",
               ime_goopy::kIPCConsoleEventNamePrefix,
               session_id);
    ScopedHandle process_event(
        ::OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, event_name));

    return process_event.Get() != NULL;
  }

  void LaunchProcessInSession(DWORD session_id) {
    // Construct binary path.
    wchar_t module_name[MAX_PATH] = {0};
    DWORD ret = ::GetModuleFileName(
        _AtlBaseModule.m_hInst, module_name, MAX_PATH);
    DCHECK(ret && ret != MAX_PATH);

    wchar_t executable_path[MAX_PATH] = {0};
    wchar_t drive[MAX_PATH] = {0};
    wchar_t dir[MAX_PATH] = {0};
    if (::_wsplitpath_s(
        module_name, drive, MAX_PATH, dir, MAX_PATH, NULL, 0, NULL, 0)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << "_wsplitpath_s failed error = " << error;
      return;
    }
    if (::_wmakepath_s(executable_path, MAX_PATH, drive, dir,
                       ime_goopy::kIPCConsoleModuleName, L"")) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << "wmakepath_s failed error = " << error;
      return;
    }

    // Get a copy of user access token.
    CAccessToken user_token_copy;
    user_token_copy.Attach(GetSessionUserToken(session_id));
    if (!user_token_copy.GetHandle())
      return;
    // Set UI_ACCESS to true to allow ipc service start ipc console whose
    // UI_ACCESS is true.
    DWORD ui_access = 1;
    ::SetTokenInformation(user_token_copy.GetHandle(), TokenUIAccess,
                          &ui_access, sizeof(ui_access));
    // Create default user process environment.
    LPVOID user_environment = NULL;
    if (!::CreateEnvironmentBlock(
        &user_environment, user_token_copy.GetHandle(), FALSE)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << "CreateEnvironmentBlock failed error = " << error;
      return;
    }

    // Create process using user's environment and token.
    PROCESS_INFORMATION process_information = {0};

    STARTUPINFO startup_info = {0};
    startup_info.cb = sizeof(startup_info);

    if (!::CreateProcessAsUser(
        user_token_copy.GetHandle(),
        executable_path,
        NULL,
        NULL,
        FALSE,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        user_environment,
        NULL,
        &startup_info,
        &process_information)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << "CreateProcessAsUser failed with error = " << error;
    }
    CloseHandle(process_information.hProcess);
    CloseHandle(process_information.hThread);
    ::DestroyEnvironmentBlock(user_environment);
  }

  HANDLE GetSessionUserToken(DWORD session_id) {
    DCHECK_NE(session_id, 0xFFFFFFFF);

    CAccessToken user_token;
    {
      HANDLE token = NULL;
      if (!::WTSQueryUserToken(session_id, &token))
        return NULL;
      user_token.Attach(token);
    }

    if (!user_token.ImpersonateLoggedOnUser())
      return NULL;

    HANDLE user_token_copy = NULL;
    if (!::DuplicateTokenEx(user_token.GetHandle(),
                            MAXIMUM_ALLOWED,
                            NULL,
                            SecurityDelegation,
                            TokenPrimary,
                            &user_token_copy)) {
      return NULL;
    }
    user_token.Revert();
    return user_token_copy;
  }

  HANDLE quit_event_;
};

}  // namespace ipc

ipc::CAtlIPCServiceModule _AtlModule;

extern "C" int WINAPI _tWinMain(HINSTANCE instance,
                                HINSTANCE prev_instance,
                                LPTSTR cmd_line,
                                int show_cmd) {
  return _AtlModule.WinMain(show_cmd);
}
