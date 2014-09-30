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

#include "common/app_const.h"

namespace ime_goopy {

// Google Input Tools installation const variables.
const wchar_t kInputRegistryKey[] = L"Software\\Google\\Google Input Tools";
const wchar_t kInputToolsUninstallKey[] = L"Software\\Microsoft\\Windows"
    L"\\CurrentVersion\\Uninstall\\GoogleInputTools";
const wchar_t kInputToolsUninstallKeyPrefix[] = L"Software\\Microsoft\\Windows"
    L"\\CurrentVersion\\Uninstall";
const wchar_t kInputFrameworkUninstallKey[] = L"GoogleInputFramework";
const wchar_t kInputImeFilename[] = L"GoogleInputTools.ime";
const wchar_t kPacksSubKey[] = L"Packs";
const wchar_t kKeyboardLayoutKey[] =
    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
const wchar_t kPackKeyboardLayoutValue[] = L"Keyboard Layout";
const wchar_t kIPCConsoleQuitEventName[] =
    L"Global\\google_ime_ipc_console_quit";
const wchar_t kRegistrarFile[] = L"GoogleInputRegistrar.exe";
const wchar_t kPackLanguageValue[] = L"Language";
const wchar_t kPackLanguageId[] = L"LANGID";
const wchar_t kFrameworkPackName[] = L"framework";
const wchar_t kInputToolsUninstallerFilename[] =
    L"GoogleInputUninstaller.exe";
const wchar_t kPackVersionValue[] = L"Version";
const wchar_t kNotifierKey[] = L"Notifier";
const wchar_t kNotifierCounter[] = L"Counter";
const wchar_t kPacksChangedKey[] = L"PacksChanged";
const wchar_t kPacksChangedCounterValue[] = L"PacksInstallCounter";
const wchar_t kImeFrameworkPackName[] = L"framework";
const wchar_t kInstallTimeName[] = L"InstallTime";
const wchar_t kInputToolsDisplayName[] = L"Google Input Tools";
const wchar_t kIPCServiceName[] = L"GoogleInputService";
const wchar_t kIPCServiceModuleName[] = L"GoogleInputService.exe";
const wchar_t kIPCConsoleModuleName[] = L"GoogleInputHandler.exe";
const wchar_t kRegisterIPCServerFlag[] = L"/RegServer";
const wchar_t kInstallIPCServiceFlag[] = L"/Service";
const wchar_t kUnRegisterIPCServerFlag[] = L"/UnregServer";
const char kPluginsSubFolder[] = "plugins";
const wchar_t kSessionManagerKey[] =
    L"System\\CurrentControlSet\\Control\\Session Manager";
const wchar_t kPendingFileRenameOperations[] = L"PendingFileRenameOperations";

const wchar_t kV2RegistryKey[] = L"Software\\Google\\Google Pinyin 2";
const wchar_t kV2AutoupdateRegisterKey[] =
    L"Software\\Google\\Google Pinyin 2\\Autoupdate";
const wchar_t kV2AutoupdateSysdictRegisterKey[] =
    L"Software\\Google\\Google Pinyin 2\\Autoupdate Sysdict";
const wchar_t kInstalledVersion[] = L"InstalledVersion";
const wchar_t kInputToolsSubFolder[] = L"Google\\Google Input Tools";
const wchar_t kRunOnceKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
const wchar_t kExtensionRegistryKey[] = L"LoadExtensions";
const wchar_t kSandboxFilename[] = L"GooglePinyinService.exe";
const wchar_t kMultilineText[] = L"<\u5b57\u7b26\u753b>";  // ZiFuHua
const wchar_t kTranServiceTime[] = L"TransServiceTime";
const wchar_t kTranServiceRequestTime[] = L"TransServiceRequestTime";
const wchar_t kInfoWindowSize[] = L"InfoWindowSize";
const wchar_t kPipePrefix[] = L"\\\\.\\pipe\\";
const wchar_t kDefaultSkinName[] = L"default";
const wchar_t kActiveSkinRegkeyName[] = L"ActiveSkin";
const char kSkinLocaleName[] = "zh-CN";
const wchar_t kFileItemsManifest[] = L"pack_file_items.manifest";
const wchar_t kPluginRegistryKey[] =  L"Plugins";
const char kSettingsIPCConsolePID[] = "IPCConsolePID";
}  // namespace ime_goopy
