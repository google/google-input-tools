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

#include "common/shellutils.h"
#ifdef OS_WINDOWS
#include <Aclapi.h>
#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#else
#include <unistd.h>
#include "common/windows_types.h"
#endif
#include "base/logging.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {
#ifdef OS_WINDOWS
static const HINSTANCE kShellExecuteSucceeded =
  reinterpret_cast<HINSTANCE>(32);
struct ExecutionInfo {
  const WCHAR* file;
  const WCHAR* cmdline;
  DWORD show_cmd;
};
static const wchar_t kConsentFileName[] = L"consent.exe";
#endif

#ifdef OS_WINDOWS
static DWORD WINAPI ExecuteThread(LPVOID parameter) {
  ExecutionInfo* info = reinterpret_cast<ExecutionInfo*>(parameter);
  BOOL retval = ShellExecute(
      NULL,
      L"open",
      info->file,
      info->cmdline,
      NULL,
      info->show_cmd) > kShellExecuteSucceeded;
  LocalFree(HLOCAL(info->cmdline));
  LocalFree(HLOCAL(info->file));
  delete info;
  ExitThread(retval);
  return TRUE;
}

BOOL ShellUtils::Execute(const WCHAR* file, const WCHAR* cmdline,
                         DWORD show_cmd, BOOL is_block) {
  if (IsSystemAccount()) return FALSE;

  ExecutionInfo* info = new ExecutionInfo;
  info->file = StrDup(file);
  info->cmdline = StrDup(cmdline);
  info->show_cmd = show_cmd;
  HANDLE handle = CreateThread(NULL, 0, ExecuteThread,
                               reinterpret_cast<LPVOID>(info), 0, NULL);
  if (handle == NULL) {
    LocalFree(HLOCAL(info->file));
    LocalFree(HLOCAL(info->cmdline));
    delete info;
    return FALSE;
  }

  // non-block mode, return directly
  if (!is_block)
    return handle != NULL;

  // wait for return value
  if (WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0)
    return FALSE;
  DWORD retval = FALSE;
  if (!GetExitCodeThread(handle, &retval))
    retval = FALSE;
  CloseHandle(handle);
  return retval;
}

void ShellUtils::LaunchProcess(const WCHAR *path,
                               const WCHAR *params,
                               bool wait) {
  if (!path)
    return;
  SHELLEXECUTEINFO ShExecInfo = {0};
  ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.hwnd = NULL;
  ShExecInfo.lpVerb = NULL;
  ShExecInfo.lpFile = path;
  ShExecInfo.lpParameters = params;
  ShExecInfo.lpDirectory = NULL;
  ShExecInfo.nShow = SW_SHOW;
  ShExecInfo.hInstApp = NULL;
  ShellExecuteEx(&ShExecInfo);
  if (wait)
    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
}

// Util to set low integrity to an object handle.
//
// NOTE(ygwang): this is similar to common/vistautil LowIntegritySecurityDesc
// class but without atl dependency, used in several cases (like sandbox,
// sharedfile, sharedmem) which does not work well with ATL classes.
//
// See http://msdn.microsoft.com/en-us/library/bb250462(VS.85).aspx for more
// details on setting low integrity to system objects.
bool ShellUtils::SetHandleLowIntegrity(HANDLE handle, SE_OBJECT_TYPE type) {
  static LPCWSTR kLowIntegritySddlSacl = L"S:(ML;;NW;;;LW)";
  PSECURITY_DESCRIPTOR security_desc = NULL;
  PACL acl = NULL;
  BOOL acl_present = FALSE;
  BOOL acl_defaulted = FALSE;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
          kLowIntegritySddlSacl, SDDL_REVISION_1, &security_desc, NULL)) {
    DLOG(ERROR) << "fail to convert security descriptor: " << GetLastError();
    return false;
  }
  bool ret = true;
  if (!GetSecurityDescriptorSacl(security_desc, &acl_present,
                                 &acl, &acl_defaulted)) {
    DLOG(ERROR) << "fail to get security descriptor sacl: " << GetLastError();
    ret = false;
  }
  if (ret && SetSecurityInfo(handle, type, LABEL_SECURITY_INFORMATION,
                      NULL, NULL, NULL, acl) != ERROR_SUCCESS) {
    DLOG(ERROR) << "fail to set security info: " << GetLastError();
    ret = false;
  }
  LocalFree(security_desc);
  return ret;
}
#endif

BOOL ShellUtils::CheckWindowsVista() {
#ifdef OS_WINDOWS
  return GetOS() >= OS_WINDOWSVISTA;
#else
  return FALSE;
#endif
}

BOOL ShellUtils::CheckWindowsXPOrLater() {
#ifdef OS_WINDOWS
  return GetOS() >= OS_WINDOWSXPSP2;
#else
  return FALSE;
#endif
}

BOOL ShellUtils::CheckWindows8() {
#ifdef OS_WINDOWS
  return GetOS() == OS_WINDOWS8;
#else
  return FALSE;
#endif
}

ShellUtils::OS ShellUtils::GetOS() {
#ifdef OS_WINDOWS
  OSVERSIONINFO osvi = { 0 };
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  if (!GetVersionEx(&osvi))
    return OS_NOTSUPPORTED;
  switch (osvi.dwMajorVersion) {
    case 5:
      if (osvi.dwMinorVersion >= 1)
        return OS_WINDOWSXPSP2;
      break;
    case 6:
      switch (osvi.dwMinorVersion) {
        case 0:
          return OS_WINDOWSVISTA;
        case 1:
          return OS_WINDOWS7;
        default:
          return OS_WINDOWS8;
      }
      break;
    default:
      return OS_NOTSUPPORTED;
  }
  return OS_NOTSUPPORTED;
#else
  return OS_NOTSUPPORTED;
#endif
}

bool ShellUtils::IsCurrentProcessAdmin() {
  BOOL retval = FALSE;
  SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
  PSID administrators_group = NULL;

  retval = AllocateAndInitializeSid(&nt_authority, 2,
      SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
      &administrators_group);
  if (retval) {
    if (!CheckTokenMembership(NULL, administrators_group, &retval)) {
      retval = FALSE;
    }
    FreeSid(administrators_group);
  }
  return retval ? true : false;
}

// Check if current user is in admin group
bool ShellUtils::IsCurrentUserAdmin() {
  HANDLE hAccessToken = NULL;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken))
    return false;
  DWORD dwDefaultBufferSize = 1024;
  scoped_array<char> info_buffer(new char[dwDefaultBufferSize]);
  assert(info_buffer.get());

  DWORD dwInfoBufferSize = 0;
  BOOL bRet = GetTokenInformation(hAccessToken, TokenGroups,
                                  info_buffer.get(), dwDefaultBufferSize,
                                  &dwInfoBufferSize);
  if (bRet == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    // If the default size is not large enough to get the entire buffer into
    // it, we make it a larger buffer and try again.
    assert(dwInfoBufferSize > dwDefaultBufferSize);
    info_buffer.reset(new char[dwInfoBufferSize]);
    bRet = GetTokenInformation(hAccessToken, TokenGroups,
                               info_buffer.get(), dwInfoBufferSize,
                               &dwInfoBufferSize);
  }
  CloseHandle(hAccessToken);
  if (bRet) {
    PSID psidAdministrators = 0;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&siaNtAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &psidAdministrators))
      return false;
    bRet = FALSE;
    PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)info_buffer.get();
    for (size_t i = 0; i < ptgGroups->GroupCount; i++) {
      if (EqualSid(psidAdministrators, ptgGroups->Groups[i].Sid)) {
        bRet = TRUE;
        break;
      }
    }
    FreeSid(psidAdministrators);
  }

  return bRet == TRUE;
}

bool ShellUtils::IsSystemAccount() {
  wchar_t user_name[MAX_PATH] = {0};
  DWORD length = MAX_PATH;
  if (GetUserName(user_name, &length)) {
    if (_wcsicmp(user_name, L"SYSTEM") == 0)
      return true;
  }
  wchar_t module_file[MAX_PATH] = {0};
  if (::GetModuleFileName(NULL, module_file, MAX_PATH)) {
    wchar_t path[MAX_PATH] = {0};
    ::SHGetFolderPath(NULL, CSIDL_SYSTEM , NULL, SHGFP_TYPE_CURRENT, path);
    ::PathAppend(path, kConsentFileName);
    return ::PathMatchSpec(module_file, path);
  }
  return false;
}

bool ShellUtils::Is64BitOS() {
#ifdef OS_WINDOWS
  typedef BOOL (CALLBACK* LPFNPROC)(HANDLE, BOOL *);
  HINSTANCE kernel = LoadLibrary(L"kernel32");
  if (kernel == NULL) return false;

  BOOL is_wow64 = FALSE;
  LPFNPROC IsWow64ProcessProc = reinterpret_cast<LPFNPROC>(
      GetProcAddress(kernel, "IsWow64Process"));

  if (IsWow64ProcessProc)
    IsWow64ProcessProc(GetCurrentProcess(), &is_wow64);

  FreeLibrary(kernel);
  if (is_wow64)
    return true;
  SYSTEM_INFO info = {0};
  ::GetNativeSystemInfo(&info);
  return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
      info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64;
#else
#if UINTPTR_MAX == 0xffffffffffffffff
  return true;
#else
  return false;
#endif
#endif
}

int ShellUtils::GetPageSize() {
#ifdef OS_WINDOWS
  static int pagesize = 0;
  if (pagesize == 0) {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    pagesize = system_info.dwPageSize;
  }
  return pagesize;
#else
  return int(sysconf(_SC_PAGESIZE));
#endif
}

int ShellUtils::NumCPUs() {
#ifdef OS_WINDOWS
  static int num_cpus = 0;
  if (num_cpus == 0) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    num_cpus = info.dwNumberOfProcessors;
  }
  return num_cpus;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

bool ShellUtils::DisableWow64FsRedirection(PVOID* old_value) {
#ifdef OS_WINDOWS
  typedef BOOL (CALLBACK* LPFNPROC)(PVOID *);
  bool success = false;
  HINSTANCE kernel = LoadLibrary(L"kernel32");
  if (kernel != NULL) {
    LPFNPROC DisableWow64FsRedirectionProc = reinterpret_cast<LPFNPROC>(
        GetProcAddress(kernel, "Wow64DisableWow64FsRedirection"));
    if (DisableWow64FsRedirectionProc)
      success = DisableWow64FsRedirectionProc(old_value);
    FreeLibrary(kernel);
  }
  return success;
#else
  return false;
#endif
}

bool ShellUtils::RevertWow64FsRedirection(PVOID old_value) {
#ifdef OS_WINDOWS
  typedef BOOL (CALLBACK* LPFNPROC)(PVOID);
  bool success = false;
  HINSTANCE kernel = LoadLibrary(L"kernel32");
  if (kernel != NULL) {
    LPFNPROC RevertWow64FsRedirectionProc = reinterpret_cast<LPFNPROC>(
      GetProcAddress(kernel, "Wow64RevertWow64FsRedirection"));
    if (RevertWow64FsRedirectionProc)
      success = RevertWow64FsRedirectionProc(old_value);
    FreeLibrary(kernel);
  }
  return success;
#else
  return false;
#endif
}

bool ShellUtils::SupportTSF() {
  return GetOS() == OS_WINDOWS8;
}
}  // namespace ime_goopy
