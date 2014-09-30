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

#ifndef GOOPY_COMMON_APP_UTILS_H_
#define GOOPY_COMMON_APP_UTILS_H_

#include <algorithm>
#include <string>
#include "base/basictypes.h"

namespace ime_goopy {
class RegistryKey;
class AppUtils {
 public:
  // Binary path is usually under \Program Files\Google\[product name]
  static wstring GetBinaryFilePath(const wstring& filename);
  static wstring GetUserDataFilePath(const wstring& filename);
  static wstring GetSystemDataFilePath(const wstring& filename);
  // The path where we store our minidump file when there is a crash for the
  // current user. It's usually under UserDataFilePath.
  static wstring GetDumpPath();
  // Obtains a list of regular files in given directory and sub directories. An
  // optional predicate function can be provided for additional checking.
  static bool GetFileList(const char* dir,
                          vector<string>* files,
                          bool (*check)(const string&));
  static bool GetFileList(const wchar_t* dir,
                          vector<wstring>* files,
                          bool (*check)(const wstring&));
  static bool IsInstalledForCurrentUser();
  static void LaunchInputManager();
  // Caller is responsible to delete the created object.
  static RegistryKey* OpenUserRegistry();
  static RegistryKey* OpenSystemRegistry(bool readonly);

  static wstring PrefixedName(const wstring& id_string, const wstring& prefix) {
    wstring name(prefix + id_string);
    replace(name.begin(), name.end(), L'\\', L'/');
    return name.substr(0, MAX_PATH - 1);
  }
  // Convert a path to OS full path in short 8.3 format, lower case.
  static bool GetShortLowerFullPath(const wstring& path,
                                    wstring* full_path,
                                    wstring* file_name);

  // Performs a depth first recursive delete operation on the directory tree
  // rooted at 'root_dir'.
  // Doesn't follow symbolic links during deletion.
  static void RecursiveDelete(const wchar_t* root_dir);

  // Tries to copy 'source_dir/file_name' to 'target_dir/file_name' if the file
  // does not exist under target_dir.
  static void CopyFileWhenMissing(const wstring& source_dir,
                                  const wstring& target_dir,
                                  const wstring& filename);
  // Tries to copy file from 'source_path' to 'target_path' if the file
  // does not exist in 'target_path'.
  static void CopyFileWhenMissing(const wstring& source_path,
                                  const wstring& target_path);

  // Obtains the file name of the current binary.
  static wstring GetBinaryFileName();

  // Returns true if succeeds with installed package names.
  static bool GetInstalledPackages(std::vector<std::wstring>* packages);

  // Returns current system date. e.g. "20120701".
  static DWORD GetSystemDate();
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_APP_UTILS_H_
