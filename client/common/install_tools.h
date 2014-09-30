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

#ifndef GOOPY_INSTALLER_TOOLS_H_
#define GOOPY_INSTALLER_TOOLS_H_

#include <atlpath.h>
#include <atlstr.h>
#include <set>

#include "base/basictypes.h"

namespace ime_goopy {
namespace installer {

// TODO(ygwang): See ybo's TODO below. At least the class FileUtils and
// RegistryUtils should be merged to other common classes.
//
// TODO(ybo): Merge into common.
// TODO(ybo): Unit tests.
class FileUtils {
 public:
  // Delete file, do it after reboot if failed.
  // Return true if it's deleted now.
  static bool DelayedDeleteFile(const CString& filename);

  // Move file, do it after reboot if failed.
  // Return true if it's moved now.
  static bool DelayedMoveFile(const CString& source, const CString& target);

  // Try to delete folder and all folders and files in it.
  // If failed, delete it after the next reboot.
  // Return true if it's deleted now.
  static bool RecursiveDeleteFolder(const CString& path);
  static CPath GetShellSubFolder(int csidl, const CString& sub_folder);
  static bool CreateFolderIfNotPresent(const CPath& folder);
};

class InstallUtils {
 public:
  // Get a list of windows titles into locker_set, for they are currently
  // using the files in check_set.
  static void GetLockerTitleList(const set<CString> check_set,
                                 set<CString>* locker_set);
  static bool SystemReboot();
  // Run a function in the dll.
  static HRESULT RunDll(const CString& file, const CStringA& function);
  static void LaunchBrowser(const CString& url);
  static void KillProcess(const CString& name);
  static int ExecuteAndWait(const CString& file,
                            const CString& parameters,
                            DWORD show_cmd);
  static void ExecuteAfterReboot(const CString& file,
                                 const CString& parameters);
};

class RegistryUtils {
 public:
  static CString GetString(
      HKEY root,
      const CString& key_name,
      const CString& value_name,
      REGSAM sam_flags);
  static bool SetString(
      HKEY root,
      const CString& key_name,
      const CString& value_name,
      const CString& value,
      REGSAM sam_flags);
  static void RecurseDeleteKey(
      HKEY root, const CString& key, REGSAM sam_flags);
  static void DeleteValue(
      HKEY root,
      const CString& key_name,
      const CString& value_name,
      REGSAM sam_flags);
  static bool IsValueExisted(CRegKey* registry, const CString& name);

  static bool WriteElevationPolicy(
      const GUID& guid, const CString& folder, const CString& name);
  static void DeleteElevationPolicy(const GUID& guid);
};

}  // namespace installer
}  // namespace ime_goopy

#endif  // GOOPY_INSTALLER_TOOLS_H_
