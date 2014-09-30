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

#include "common/install_tools.h"

#include <atlbase.h>
#include <atlwin.h>
#include <shlobj.h>
#include <psapi.h>

#include "common/app_const.h"
#include "common/shellutils.h"

namespace ime_goopy {
namespace installer {
static const wchar_t kElevationPolicyRegsitryKey[] =
  L"Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\";

bool FileUtils::DelayedDeleteFile(const CString& filename) {
  if (!DeleteFile(filename)) {
    if (GetLastError() != ERROR_FILE_NOT_FOUND) {
      MoveFileEx(filename, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
      return false;
    }
  }
  return true;
}

bool FileUtils::DelayedMoveFile(const CString& source, const CString& target) {
  if (!MoveFileEx(source, target, MOVEFILE_REPLACE_EXISTING)) {
    MoveFileEx(source, target,
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT);
    return false;
  }
  return true;
}

bool FileUtils::RecursiveDeleteFolder(const CString& path) {
  bool delayed = false;
  WIN32_FIND_DATA find_data = {0};
  wchar_t pattern[MAX_PATH] = {0};
  PathCombine(pattern, path, L"*");
  HANDLE find_handle = FindFirstFile(pattern, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) return true;

  wchar_t filename[MAX_PATH] = {0};
  do {
    if (!PathCombine(filename, path, find_data.cFileName)) continue;
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (find_data.cFileName[0] != L'.') {
        if (!RecursiveDeleteFolder(filename)) delayed = true;
      }
    } else {
      if (!DelayedDeleteFile(filename)) delayed = true;
    }
  } while (FindNextFile(find_handle, &find_data));
  FindClose(find_handle);
  if (!RemoveDirectory(path)) {
    MoveFileEx(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    delayed = true;
  }
  return !delayed;
}

CPath FileUtils::GetShellSubFolder(int csidl, const CString& sub_folder) {
  CPath result;
  wchar_t path[MAX_PATH] = {0};
  if (csidl == CSIDL_SYSTEMX86 &&
      ShellUtils::Is64BitOS() &&
      !ShellUtils::CheckWindowsVista()) {
    // Fix bug 5811480. In 64bit windows xp,
    // installer::FileUtils::GetShellSubFolder(CSIDL_SYSTEMX86, L"") will
    // return "%windir%\\system32", which cause 32bit ime file can not be
    // copy to syswow64 folder and failed to install.
    if (!GetSystemWow64Directory(path, MAX_PATH))
      return NULL;
  } else {
    if (FAILED(SHGetFolderPath(NULL, csidl , NULL, SHGFP_TYPE_CURRENT, path)))
      return NULL;
  }
  result = path;
  if (!sub_folder.IsEmpty()) result.Append(sub_folder);
  return result;
}

bool FileUtils::CreateFolderIfNotPresent(const CPath& folder) {
  if (!folder.FileExists()) {
    if (SHCreateDirectoryEx(NULL, folder, NULL) != ERROR_SUCCESS)
      return false;
  }
  return true;
}

struct CheckLockerData {
  const set<CString> check_set;
  set<CString>* locker_set;
  void InsertTitle(CWindow window) {
    CString name;
    window.GetWindowText(name);
    if (!name.IsEmpty()) locker_set->insert(name);
  }
};

// A wrapper for EnumProcessModules/EnumProcessModulesEx.
// Because EnumProcessModulesEx only exists in psapi.lib in vista or later, the
// wrapper will firstly check the existance and fall back to the
// EnumProcessModules if not available.
static BOOL EnumProcessModules64(HANDLE process,
                                 HMODULE* modules,
                                 DWORD bytes,
                                 DWORD* bytes_needed) {
  HMODULE psapi = LoadLibrary(L"psapi.dll");
  BOOL has_module = FALSE;
  if (psapi) {
    typedef BOOL (CALLBACK* EnumProcessModulesExFunc)(HANDLE process,
                                                      HMODULE* modules,
                                                      DWORD bytes,
                                                      DWORD* bytes_needed,
                                                      DWORD flags);
    EnumProcessModulesExFunc enum_process_modules_ex =
        reinterpret_cast<EnumProcessModulesExFunc>(
            GetProcAddress(psapi, "EnumProcessModulesEx"));
    if (enum_process_modules_ex &&
        enum_process_modules_ex(process, modules, bytes, bytes_needed,
                                LIST_MODULES_ALL))
      has_module = TRUE;
    FreeLibrary(psapi);
  }
  if (!has_module) {
    if (EnumProcessModules(process, modules, bytes, bytes_needed))
      has_module = TRUE;
  }
  return has_module;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
  assert(hwnd);
  assert(lParam);
  CWindow window(hwnd);
  if (window.GetParent() != NULL) return TRUE;
  if (!window.IsWindowVisible()) return TRUE;

  CheckLockerData* data = reinterpret_cast<CheckLockerData*>(lParam);
  DWORD process_id = 0;
  GetWindowThreadProcessId(hwnd, &process_id);

  HANDLE process = OpenProcess(
      PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
      FALSE,
      process_id);
  if (process == NULL) return TRUE;
  HMODULE modules[1024];
  DWORD module_bytes = 0;
  if (!EnumProcessModules64(process, modules, sizeof(modules), &module_bytes))
    return TRUE;
  for (int j = 0; j < module_bytes / sizeof(modules[0]); j++) {
    wchar_t module_name[MAX_PATH] = {0};
    if (!GetModuleFileNameEx(process, modules[j], module_name, MAX_PATH))
      return TRUE;
    CharLower(module_name);
    if (data->check_set.find(module_name) != data->check_set.end()) {
      data->InsertTitle(hwnd);
      break;
    }
  }
  CloseHandle(process);
  return TRUE;
}

void InstallUtils::GetLockerTitleList(const set<CString> check_set,
                                      set<CString>* locker_set) {
  assert(locker_set);
  locker_set->clear();
  CheckLockerData data = {check_set, locker_set};
  EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
}

// Copy and modified from MSDN sample.
bool InstallUtils::SystemReboot() {
  HANDLE token;
  TOKEN_PRIVILEGES tkp;

  // Get a token for this process.
  if (!OpenProcessToken(GetCurrentProcess(),
      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    return false;

  // Get the LUID for the shutdown privilege.
  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

  tkp.PrivilegeCount = 1;  // one privilege to set
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  // Get the shutdown privilege for this process.
  AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0);
  if (GetLastError() != ERROR_SUCCESS) return false;

  // Shut down the system and force all applications to close.
  if (!ExitWindowsEx(EWX_REBOOT,
                    SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
                    SHTDN_REASON_MINOR_UPGRADE |
                    SHTDN_REASON_FLAG_PLANNED))
    return false;
  return true;
}

HRESULT InstallUtils::RunDll(const CString& file, const CStringA& function) {
  typedef HRESULT (*DLL_REGISTER_SERVER)();

  HMODULE module = LoadLibrary(file);
  if (!module) return ERROR_FILE_NOT_FOUND;

  DLL_REGISTER_SERVER proc = reinterpret_cast<DLL_REGISTER_SERVER>(
      GetProcAddress(module, function));
  if (!proc) {
    FreeLibrary(module);
    return ERROR_INVALID_FUNCTION;
  }

  CoInitialize(NULL);
  HRESULT result = proc();
  CoUninitialize();

  FreeLibrary(module);
  return result;
}

void InstallUtils::LaunchBrowser(const CString& url) {
#ifdef DEBUG
  // Don't bother the production ping url in debug mode.
  MessageBox(NULL, url, L"Fake Launch Browser in Debug Version", MB_OK);
#else
  ShellExecute(NULL, NULL, url, NULL, NULL, SW_SHOW);
#endif
}

void InstallUtils::KillProcess(const CString& path) {
  DWORD processes[1024];
  DWORD needed = 0;

  if (!EnumProcesses(processes, sizeof(processes), &needed)) return;

  wchar_t process_path[MAX_PATH] = {0};
  for (DWORD i = 0; i < needed / sizeof(processes[0]); i++) {
    HANDLE process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
        PROCESS_TERMINATE | SYNCHRONIZE, FALSE, processes[i]);
    if (process == NULL) continue;
    if (!GetModuleFileNameEx(process, NULL, process_path, MAX_PATH)) {
      // In 64bit system, 32 bit application cannot access 64 bit process
      // information by GetModuleFileNameEx. Instead we can use
      // QueryFullProcessImageNameW to get the full file name of the process.
      typedef BOOL (CALLBACK* LPFNPROC)(HANDLE, DWORD, LPTSTR, PDWORD);
      HINSTANCE kernel = LoadLibrary(L"kernel32");
      if (kernel == NULL)
        continue;
      LPFNPROC query_full_process_image_name = reinterpret_cast<LPFNPROC>(
          GetProcAddress(kernel, "QueryFullProcessImageNameW"));
      DWORD path_length = MAX_PATH;
      if (!query_full_process_image_name ||
          !query_full_process_image_name(process,
                                         0,
                                         process_path,
                                         &path_length)) {
        FreeLibrary(kernel);
        continue;
      }
      FreeLibrary(kernel);
    }
    if (path.CompareNoCase(process_path) == 0) {
      TerminateProcess(process, 0);
      WaitForSingleObject(process, INFINITE);
    }
    CloseHandle(process);
  }
}

int InstallUtils::ExecuteAndWait(const CString& file,
                                 const CString& parameters,
                                 DWORD show_cmd) {
  SHELLEXECUTEINFO execute_info = {0};
  execute_info.cbSize = sizeof(execute_info);
  execute_info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
  execute_info.lpVerb = TEXT("open");
  execute_info.lpFile = file;
  execute_info.lpParameters = parameters;
  execute_info.nShow = show_cmd;
  if (ShellExecuteEx(&execute_info)) {
    WaitForSingleObject(execute_info.hProcess, INFINITE);
  } else {
    return -1;
  }
  DWORD ret = -1;
  GetExitCodeProcess(execute_info.hProcess, &ret);
  CloseHandle(execute_info.hProcess);
  return ret;
}

void InstallUtils::ExecuteAfterReboot(const CString& file,
                                      const CString& parameters) {
  CString command = file;
  if (!parameters.IsEmpty())
    command.AppendFormat(L" %s", parameters);
  RegistryUtils::SetString(HKEY_CURRENT_USER, kRunOnceKey, file, command,
                           KEY_READ | KEY_WRITE | KEY_WOW64_64KEY);
}

CString RegistryUtils::GetString(HKEY root,
                                 const CString& key_name,
                                 const CString& value_name,
                                 REGSAM sam_flags) {
  // Use 64bit registry by default.
  if (!(sam_flags & KEY_WOW64_32KEY)) sam_flags |= KEY_WOW64_64KEY;
  CRegKey key;
  if (key.Open(root, key_name, KEY_READ | sam_flags) != ERROR_SUCCESS)
    return L"";

  ULONG chars = 0;
  if (key.QueryStringValue(value_name, NULL, &chars) != ERROR_SUCCESS)
    return L"";
  CString buffer;
  if (key.QueryStringValue(value_name, buffer.GetBuffer(chars), &chars) !=
      ERROR_SUCCESS)
    return L"";
  buffer.ReleaseBuffer();
  return buffer;
}

bool RegistryUtils::SetString(HKEY root,
                              const CString& key_name,
                              const CString& value_name,
                              const CString& value,
                              REGSAM sam_flags) {
  CRegKey key;
  // Use 64bit registry by default.
  if (!(sam_flags & KEY_WOW64_32KEY)) sam_flags |= KEY_WOW64_64KEY;
  REGSAM flags = KEY_READ | KEY_WRITE | sam_flags;
  if (key.Open(root, key_name, flags) != ERROR_SUCCESS) return false;
  key.SetStringValue(value_name, value);
  return true;
}

void RegistryUtils::RecurseDeleteKey(
    HKEY root, const CString& key, REGSAM sam_flags) {
  CString full_path(key);
  int pos = full_path.ReverseFind(L'\\');
  if (pos == -1) return;
  CString prefix = full_path.Left(pos);
  CString sub_key = full_path.Mid(pos + 1);
  CRegKey registry;
  // Use 64bit registry by default.
  if (!(sam_flags & KEY_WOW64_32KEY)) sam_flags |= KEY_WOW64_64KEY;
  if (registry.Open(root, prefix, KEY_READ | sam_flags) != ERROR_SUCCESS)
    return;
  registry.RecurseDeleteKey(sub_key);
}

void RegistryUtils::DeleteValue(HKEY root,
                                const CString& key_name,
                                const CString& value_name,
                                REGSAM sam_flags) {
  CRegKey key;
  // Use 64bit registry by default.
  if (!(sam_flags & KEY_WOW64_32KEY)) sam_flags |= KEY_WOW64_64KEY;
  if (key.Open(root, key_name, KEY_READ | KEY_WRITE | sam_flags) !=
      ERROR_SUCCESS) return;
  key.DeleteValue(value_name);
}

bool RegistryUtils::IsValueExisted(CRegKey* registry, const CString& name) {
  DWORD type = 0;
  ULONG length = 0;
  return (registry->QueryValue(name, &type, NULL, &length) == ERROR_SUCCESS);
}

bool RegistryUtils::WriteElevationPolicy(const GUID& guid,
                                         const CString& folder,
                                         const CString& name) {
  OLECHAR id[42] = {0};
  StringFromGUID2(guid, id, ARRAYSIZE(id));
  CString registry_key = kElevationPolicyRegsitryKey;
  registry_key.Append(id);
  CRegKey registry;
  if (registry.Create(HKEY_LOCAL_MACHINE,
                      registry_key,
                      REG_NONE,
                      REG_OPTION_NON_VOLATILE,
                      KEY_READ | KEY_WRITE | KEY_WOW64_64KEY) !=
      ERROR_SUCCESS) {
    return false;
  }
  registry.SetStringValue(L"AppName", name);
  registry.SetStringValue(L"AppPath", folder);
  registry.SetDWORDValue(L"Policy", 3);

  if (ShellUtils::Is64BitOS()) {
    // When it's Windows x64, we should also write the key to wow64 view.
    CRegKey wow64_registry;
    if (wow64_registry.Create(
        HKEY_LOCAL_MACHINE,
        registry_key,
        REG_NONE,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE | KEY_WOW64_32KEY) != ERROR_SUCCESS)
      return false;
    wow64_registry.SetStringValue(L"AppName", name);
    wow64_registry.SetStringValue(L"AppPath", folder);
    wow64_registry.SetDWORDValue(L"Policy", 3);
  }
  return true;
}

void RegistryUtils::DeleteElevationPolicy(const GUID& guid) {
  OLECHAR id[42];
  StringFromGUID2(guid, id, ARRAYSIZE(id));
  CString registry_key = kElevationPolicyRegsitryKey;
  registry_key.Append(id);
  RecurseDeleteKey(HKEY_LOCAL_MACHINE, registry_key, 0);
  if (ShellUtils::Is64BitOS())
    RecurseDeleteKey(HKEY_LOCAL_MACHINE, registry_key, KEY_WOW64_32KEY);
}

}  // namespace installer
}  // namespace ime_goopy
