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

#ifndef GOOPY_INSTALLER_GOOPY_UTILS_H_
#define GOOPY_INSTALLER_GOOPY_UTILS_H_

#include <atlpath.h>
#include <atlstr.h>

#include "base/basictypes.h"

namespace ime_goopy {

extern const wchar_t kOemName[];
extern const GUID kOptionsElevationPolicyGuid;
extern const GUID kDashboardElevationPolicyGuid;
extern const wchar_t kUninstallKey[];
extern const wchar_t kAutoRunRegistryKey[];
extern const wchar_t kAutoRunRegistryName[];
extern const wchar_t kTextServiceRegistryKey[];

enum TargetFolder {
  TARGET_BINARY = 0,
  TARGET_SYSTEM_DATA,
  TARGET_SYSTEM_X86,
  TARGET_SYSTEM_X64,
  TARGET_USER_DATA,
};

class GoopyUtils {
 public:
  static CPath GetTargetFolder(TargetFolder target);
  static CPath GetTargetPath(TargetFolder target, const CString& filename);
  // Creates shell associations for Goopy-specific file extensions.
  static bool AssociateFileExtensions();
  // Removes shell associations for Goopy-specific file extensions.
  static bool DissociateFileExtensions();
  // Tests if the file extensions are already registered as shell associations.
  static bool AreFileExtensionsAssociated();
  // Extract brandcode from installer/downloader filename
  static bool ExtractBrandCodeFromFileName(CString* brand);
};

}  // namespace ime_goopy

#endif  // GOOPY_INSTALLER_GOOPY_UTILS_H_
