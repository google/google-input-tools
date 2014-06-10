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

#ifndef GOOPY_COMMON_APP_CONST_H_
#define GOOPY_COMMON_APP_CONST_H_

namespace ime_goopy {
// TODO(jiangwei): Remove/rename goopy related const strings.
extern const wchar_t kInputRegistryKey[];
extern const wchar_t kInputToolsUninstallKey[];
extern const wchar_t kInputToolsUninstallKeyPrefix[];
extern const wchar_t kInputFrameworkUninstallKey[];
extern const wchar_t kInputImeFilename[];
extern const wchar_t kPacksSubKey[];
extern const wchar_t kKeyboardLayoutKey[];
extern const wchar_t kPackKeyboardLayoutValue[];
extern const wchar_t kIPCConsoleQuitEventName[];
extern const wchar_t kRegistrarFile[];
extern const wchar_t kPackLanguageValue[];
extern const wchar_t kPackLanguageId[];
extern const wchar_t kFrameworkPackName[];
extern const wchar_t kInputToolsUninstallerFilename[];
extern const wchar_t kPackVersionValue[];
extern const wchar_t kNotifierKey[];
extern const wchar_t kNotifierCounter[];
extern const wchar_t kPacksChangedKey[];
extern const wchar_t kPacksChangedCounterValue[];
extern const wchar_t kImeFrameworkPackName[];
extern const wchar_t kInstallTimeName[];
extern const wchar_t kInputToolsDisplayName[];
extern const wchar_t kIPCServiceName[];
extern const wchar_t kIPCConsoleName[];
extern const wchar_t kIPCServiceModuleName[];
extern const wchar_t kIPCConsoleModuleName[];
extern const wchar_t kRegisterIPCServerFlag[];
extern const wchar_t kInstallIPCServiceFlag[];
extern const wchar_t kUnRegisterIPCServerFlag[];
extern const char kPluginsSubFolder[];
extern const wchar_t kSessionManagerKey[];
extern const wchar_t kPendingFileRenameOperations[];

extern const wchar_t kInputToolsSubFolder[];
extern const wchar_t kRunOnceKey[];
extern const wchar_t kExtensionRegistryKey[];
extern const wchar_t kSandboxFilename[];
extern const wchar_t kMultilineText[];
extern const wchar_t kTranServiceTime[];
extern const wchar_t kTranServiceRequestTime[];
extern const wchar_t kInfoWindowSize[];
extern const wchar_t kItemDownloader[];
extern const wchar_t kPipePrefix[];
extern const wchar_t kDefaultSkinName[];
extern const wchar_t kActiveSkinRegkeyName[];
extern const char kSkinLocaleName[];
extern const wchar_t kFileItemsManifest[];
extern const wchar_t kPluginRegistryKey[];
extern const char kSettingsIPCConsolePID[];

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_APP_CONST_H_
