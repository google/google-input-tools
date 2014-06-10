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

#include "common/app_utils.h"

#include <atlpath.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/file/file_path.h"
#include "base/file/file_util.h"
#include "base/scoped_handle_win.h"
#include "base/string_utils_win.h"
#include "common/app_const.h"
#include "common/registry.h"
#include "common/shellutils.h"

namespace ime_goopy {
static const wchar_t kDumpFolderName[] = L"Crash";
static const wchar_t kRegistryDataPathValue[] = L"DataPath";
static const wchar_t kRegistryPathValue[] = L"PATH";

static wstring CombinePath(const wstring& path,
                           const wstring& filename) {
  wchar_t result[MAX_PATH] = {0};
  PathCombine(result, path.c_str(), filename.c_str());
  return result;
}

static wstring GetBinaryPath() {
  scoped_ptr<RegistryKey> registry(AppUtils::OpenSystemRegistry(true));
  if (!registry.get()) return L"";
  wstring value;
  if (registry->QueryStringValue(kRegistryPathValue, &value) != ERROR_SUCCESS)
    return L"";
  return value;
}

static wstring GetDataPath() {
  scoped_ptr<RegistryKey> registry(AppUtils::OpenSystemRegistry(true));
  if (!registry.get()) return L"";
  wstring value;
  if (registry->QueryStringValue(kRegistryDataPathValue,
                                 &value) != ERROR_SUCCESS)
    return L"";
  return value;
}

wstring AppUtils::GetBinaryFilePath(const wstring& filename) {
  return CombinePath(GetBinaryPath(), filename);
}

wstring AppUtils::GetUserDataFilePath(const wstring& filename) {
  wchar_t app_data_path[MAX_PATH] = {0};
  if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL,
                             SHGFP_TYPE_CURRENT, app_data_path))) {
    return L"";
  }
  return CombinePath(
      CombinePath(app_data_path, kInputToolsSubFolder), filename);
}

wstring AppUtils::GetSystemDataFilePath(const wstring& filename) {
  return CombinePath(GetDataPath(), filename);
}

wstring AppUtils::GetDumpPath() {
  wstring dump_folder = GetUserDataFilePath(kDumpFolderName);
  if (dump_folder.empty()) return L"";

  // Make sure the folder exists.
  if (SHCreateDirectory(NULL, dump_folder.c_str()) != ERROR_SUCCESS) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) return L"";
  }

  return dump_folder;
}

bool AppUtils::IsInstalledForCurrentUser() {
  RegistryKey registry;
  return registry.Open(HKEY_CURRENT_USER,
                       kInputRegistryKey,
                       KEY_READ | KEY_WOW64_64KEY) == ERROR_SUCCESS;
}

void AppUtils::LaunchInputManager() {
  wchar_t sysdir[MAX_PATH] = { 0 };
  if (!GetSystemDirectory(sysdir, MAX_PATH))
    return;
  wchar_t control[MAX_PATH] = { 0 }, input_dll[MAX_PATH] = { 0 };
  if (!PathCombine(control, sysdir, L"control.exe") ||
      !PathCombine(input_dll, sysdir, L"input.dll"))
    return;
  ShellExecute(NULL,
               NULL,
               control,
               input_dll,
               NULL,
               SW_SHOW);
}

// Each login user has a corresponding winlogon.exe, however, winlogon.exe is
// running under SYSTEM account. To make sure we are opening the real user's
// registry key, instead of SYSTEM's, we should use RegOpenCurrentUser.
RegistryKey* AppUtils::OpenUserRegistry() {
  HKEY current_user = NULL;
  scoped_ptr<RegistryKey> registry(new RegistryKey());
  if (RegOpenCurrentUser(KEY_READ | KEY_WRITE | KEY_WOW64_64KEY,
                         &current_user) == ERROR_SUCCESS) {
    if (ERROR_SUCCESS !=
        registry->Create(current_user, kInputRegistryKey, 0, 0,
                         KEY_READ | KEY_WRITE | KEY_WOW64_64KEY)) {
      registry.reset(NULL);
    }
    RegCloseKey(current_user);
    return registry.release();
  }

  if (ERROR_SUCCESS !=
      registry->Create(HKEY_CURRENT_USER, kInputRegistryKey, 0, 0,
                       KEY_READ | KEY_WRITE | KEY_WOW64_64KEY)) {
    return NULL;
  }
  return registry.release();
}

RegistryKey* AppUtils::OpenSystemRegistry(bool readonly) {
  REGSAM flags = KEY_READ | KEY_WOW64_64KEY;
  if (!readonly) flags |= KEY_WRITE;
  return RegistryKey::OpenKey(HKEY_LOCAL_MACHINE, kInputRegistryKey, flags);
}

bool AppUtils::GetFileList(const char* dir,
                           vector<string>* files,
                           bool (*check)(const string&)) {
  char dir_pattern[MAX_PATH] = { 0 };
  char file[MAX_PATH] = { 0 };

  PathCombineA(dir_pattern, dir, "*");
  WIN32_FIND_DATAA file_data = {0};
  ScopedFindFileHandle find_file(FindFirstFileA(dir_pattern, &file_data));
  if (!find_file.IsValid())
    return false;

  do {
    PathCombineA(file, dir, file_data.cFileName);
    if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
        file_data.cFileName != NULL &&
        (check == NULL || check(file_data.cFileName))) {
      files->push_back(file);
    } else if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
               strcmp(file_data.cFileName, ".") != 0 &&
               strcmp(file_data.cFileName, "..") != 0) {
      GetFileList(file, files, check);
    }
  } while (FindNextFileA(find_file, &file_data) != 0);

  return true;
}

bool AppUtils::GetFileList(const wchar_t* dir,
                           vector<wstring>* files,
                           bool (*check)(const wstring&)) {
  wchar_t dir_pattern[MAX_PATH] = { 0 };
  wchar_t file[MAX_PATH] = { 0 };

  PathCombineW(dir_pattern, dir, L"*");
  WIN32_FIND_DATAW file_data = {0};
  ScopedFindFileHandle find_file(FindFirstFileW(dir_pattern, &file_data));
  if (!find_file.IsValid())
    return false;

  do {
    PathCombineW(file, dir, file_data.cFileName);
    if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
        file_data.cFileName != NULL &&
        (check == NULL || check(file_data.cFileName))) {
      files->push_back(file);
    } else if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
               wcscmp(file_data.cFileName, L".") != 0 &&
               wcscmp(file_data.cFileName, L"..") != 0) {
      GetFileList(file, files, check);
    }
  } while (FindNextFileW(find_file, &file_data) != 0);

  return true;
}

// Util to convert a path to OS full path in short 8.3 format, lower case.
// Returns the result path in full_path, the file name part in file_name.
bool AppUtils::GetShortLowerFullPath(const wstring& path,
                                     wstring* full_path,
                                     wstring* file_name) {
  wchar_t full_path_buf[MAX_PATH] = { 0 };
  wchar_t short_path_buf[MAX_PATH] = { 0 };
  wchar_t* file_name_part = NULL;
  DWORD ret = GetFullPathName(path.c_str(), MAX_PATH,
                              full_path_buf, &file_name_part);
  if (ret == 0 || ret > MAX_PATH) {
    return false;
  }
  ret = GetShortPathName(full_path_buf, short_path_buf,  MAX_PATH);
  if (ret == 0 || ret > MAX_PATH) {
    return false;
  }
  if (full_path) {
    *full_path = short_path_buf;
    transform(full_path->begin(), full_path->end(),
              full_path->begin(), tolower);
  }
  if (file_name) {
    *file_name = file_name_part;
    transform(file_name->begin(), file_name->end(),
              file_name->begin(), tolower);
  }
  return true;
}

void AppUtils::RecursiveDelete(const wchar_t* root_dir) {
  // Stores <root_dir>\*
  wchar_t path_pattern[MAX_PATH] = { 0 };
  if (!PathCombine(path_pattern, root_dir, L"*"))
    return;

  WIN32_FIND_DATA find_data = { 0 };
  ScopedFindFileHandle handle(FindFirstFile(path_pattern, &find_data));

  if (!handle.IsValid())
    return;

  const int root_dir_length = wcslen(root_dir);
  do {
    CPath file_path(root_dir);
    if (!file_path.Append(find_data.cFileName))
      continue;
    // Guarantees special path segments like .. and . got removed.
    // Path like "seg1\seg2\.." will be canonicalized to "seg1".
    // Path like "seg1\." will be canonicalized to "seg1".
    file_path.Canonicalize();
    // Avoid deleting outside of the root_dir tree.
    CPath common_prefix_path = file_path.CommonPrefix(root_dir);
    if (common_prefix_path.m_strPath.GetLength() != root_dir_length ||
        file_path.m_strPath.GetLength() <= root_dir_length)
      continue;
    const wchar_t* file_path_str = file_path.m_strPath.GetString();
    if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        !(find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
      // The file path is a directory and not a symbolic link.
      RecursiveDelete(file_path_str);
    } else {
      DVLOG(2) << "Deleting file: " << WideToUtf8(file_path_str);
      DeleteFile(file_path_str);
    }
  } while (FindNextFile(handle, &find_data));

  DVLOG(2) << "Deleting dir : " << WideToUtf8(root_dir);
  RemoveDirectory(root_dir);
}

void AppUtils::CopyFileWhenMissing(const wstring& source_path,
                                   const wstring& target_path) {
  if (!PathFileExists(source_path.c_str()) ||
      PathFileExists(target_path.c_str())) {
    return;
  }

  // NOTE(haicsun): SHFileOperation should also create the directory if not
  // exist, but the behavior is not kept in some system, so we create the
  // directory before call it to make sure the file is copied.
  if (!file_util::CreateDirectory(FilePath(target_path.c_str()).DirName())) {
    DLOG(ERROR) << " Create parent directory failed from: "
                << WideToUtf8(source_path)
                << " to: " << WideToUtf8(target_path);
    return;
  }

  SHFILEOPSTRUCT fileop = { 0 };

  // SHFILEOPSTRUCT requires double NULL-terminated strings.
  const wstring double_null(2, L'\0');
  const wstring source_path_string(source_path + double_null);
  const wstring target_path_string(target_path + double_null);

  fileop.wFunc = FO_COPY;
  fileop.pFrom = source_path_string.c_str();
  fileop.pTo = target_path_string.c_str();
  fileop.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI |
                  FOF_NOCONFIRMMKDIR;

  if (SHFileOperation(&fileop) == 0) {
    DLOG(INFO) << "Copied file from: " << WideToUtf8(source_path)
               << " to: " << WideToUtf8(target_path);
  } else {
    DLOG(ERROR) << "Copied file failed from: " << WideToUtf8(source_path)
                << " to: " << WideToUtf8(target_path) << "error = "
                << ::GetLastError();
  }
}

void AppUtils::CopyFileWhenMissing(const wstring& source_dir,
                                   const wstring& target_dir,
                                   const wstring& file_name) {
  wchar_t source_path[MAX_PATH] = { 0 };
  PathCombine(source_path, source_dir.c_str(), file_name.c_str());
  wchar_t target_path[MAX_PATH] = { 0 };
  PathCombine(target_path, target_dir.c_str(), file_name.c_str());
  CopyFileWhenMissing(source_path, target_path);
}

wstring AppUtils::GetBinaryFileName() {
  wchar_t filename[MAX_PATH] = {0};
  if (GetModuleFileName(NULL, filename, MAX_PATH) == 0) {
    return L"";
  }
  return filename;
}

bool AppUtils::GetInstalledPackages(std::vector<std::wstring>* packages) {
  DCHECK(packages);
  std::wstring pack_list_key =
      WideStringPrintf(L"%s\\%s", kInputRegistryKey, kPacksSubKey);
  REGSAM flags = KEY_READ;
  if (ShellUtils::Is64BitOS())
    flags |= KEY_WOW64_64KEY;
  scoped_ptr<RegistryKey> registry(
      RegistryKey::OpenKey(HKEY_LOCAL_MACHINE,
                           pack_list_key.c_str(),
                           flags,
                           false));
  if (!registry.get())
    return false;

  int index = 0;
  while (true) {
    std::wstring pack_name;
    LONG ret = registry->EnumKey(index++, &pack_name);
    if (ret != ERROR_SUCCESS)
      break;
    packages->push_back(pack_name);
  }
  std::sort(packages->begin(), packages->end());
  return true;
}

DWORD AppUtils::GetSystemDate() {
  SYSTEMTIME systime = {0};
  ::GetSystemTime(&systime);
  return systime.wYear * 10000 + systime.wMonth * 100 + systime.wDay;
}

}  // namespace ime_goopy
