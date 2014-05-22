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

const wchar_t kGoopyRegistryKey[] = L"Software\\Google\\Google Pinyin 2";
const wchar_t kGoopyAutoupdateRegisterKey[] =
    L"Software\\Google\\Google Pinyin 2\\Autoupdate";
const wchar_t kGoopyAutoupdateSysdictRegisterKey[] =
    L"Software\\Google\\Google Pinyin 2\\Autoupdate Sysdict";
const wchar_t kInstalledVersion[] = L"InstalledVersion";
const wchar_t kGoopySubFolder[] = L"Google\\Google Pinyin 2";
const wchar_t kGoopyDoodleSubFolder[] = L"Google\\Google Pinyin 2\\Doodles";
const wchar_t kGoopyDefaultDoodleSubFolder[] =
    L"Google\\Google Pinyin 2\\Doodles\\1001";
const wchar_t kGoopyExtensionSubFolder[] =
    L"Google\\Google Pinyin 2\\Extensions";
const wchar_t kImeFilename[] = L"GooglePinyin2.ime";
const wchar_t kOptionsFilename[] = L"GooglePinyinOptions.exe";
const wchar_t kSettingWizardFilename[] =
    L"GooglePinyinSettingWizard.exe";
const wchar_t kDashboardFilename[] = L"GooglePinyinDashboard.exe";
const wchar_t kReporterFilename[] = L"GooglePinyinReporter.exe";
const wchar_t kDaemonFilename[] = L"GooglePinyinDaemon.exe";
const wchar_t kUninstallerFilename[] = L"GooglePinyinUninstaller.exe";
const wchar_t kExperienceName[] = L"Experience";
const wchar_t kSetFilename[] = L"GooglePinyinSet.exe";
const wchar_t kLMTimestamp[] = L"LMTimestamp";
const wchar_t kDictionaryUpdaterFilename[] =
    L"GooglePinyinDictionary.exe";
const wchar_t kInstallTimeName[] = L"InstallTime";
const wchar_t kLastPing[] = L"LastPing";
const wchar_t kLastUseTimeName[] = L"ActiveTime";
const wchar_t kSentInstallPing[] = L"InstallSent";
const wchar_t kSentActivePing[] = L"ActivationSent";
const wchar_t kSendHpsFlag[] = L"SendHpsFlag";
const wchar_t kHpsFlag[] = L"HpsFlag";
const wchar_t kHomepageTrackParam[] = L"webhp?client=aff-ime";
const wchar_t k265TrackParam[] = L"?client=aff-ime";
const wchar_t k265HomepageSubString[] = L"265";
const wchar_t kLastDownloadTimestampKey[] = L"SyncLastDownload";
const wchar_t kAuthTokenRegistryKey[] = L"SyncAuthToken";
const wchar_t kMIDRegistryKey[] = L"SyncMID";
const wchar_t kEmailRegistryKey[] = L"SyncEmail";
const wchar_t kGaiaSignUpUrl[] =
    L"https://www.google.com/accounts/NewAccount?service=goopy&hl=zh-CN";
const wchar_t kGaiaForgetPasswordUrl[] =
    L"https://www.google.com/accounts/ForgotPasswd?service=goopy&hl=zh-CN";
const wchar_t kImeSourceName[] = L"ime-goopy";
const wchar_t kImeServiceName[] = L"goopy";
const wchar_t kV1RegistryKey[] = L"Software\\Google\\Google Pinyin";
const wchar_t kBrandCodeName[] = L"BrandCode";
const wchar_t kUserIdName[] = L"UserId";
const wchar_t kMachineGuidName[] = L"GUID";
const wchar_t kRlzName[] = L"RLZ";
const wchar_t kSettingWizardLaunchedName[] =
    L"SettingWizardEverLaunched";
const wchar_t kRunOnceKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
const wchar_t kExtensionRegistryKey[] = L"LoadExtensions";
const wchar_t kSandboxFilename[] = L"GooglePinyinService.exe";
const wchar_t kMultilineText[] = L"<\u5b57\u7b26\u753b>";  // ZiFuHua
const wchar_t kTranServiceTime[] = L"TransServiceTime";
const wchar_t kTranServiceRequestTime[] = L"TransServiceRequestTime";
const wchar_t kInfoWindowSize[] = L"InfoWindowSize";
const wchar_t kSettingsSharedMemoryName[] = L"GPY2SETTINGS";

// Migrated from goopy_utils.cc
const wchar_t kOemName[] = L"OEM";
// {BF9994A8-1840-47b2-9B14-4EF7C51F183E}
const GUID kOptionsElevationPolicyGuid =
{0xbf9994a8, 0x1840, 0x47b2, {0x9b, 0x14, 0x4e, 0xf7, 0xc5, 0x1f, 0x18, 0x3e}};
// {2AAEEAC6-B521-49c2-AC05-CEF715924F79}
const GUID kDashboardElevationPolicyGuid =
{0x2aaeeac6, 0xb521, 0x49c2, {0xac, 0x5, 0xce, 0xf7, 0x15, 0x92, 0x4f, 0x79}};
const wchar_t kUninstallKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\GooglePinyin2";
const wchar_t kAutoRunRegistryKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kAutoRunRegistryName[] = L"Google Pinyin 2 Autoupdater";
const wchar_t kUpdaterAutoRunName[] = L"Google Pinyin 2 Autoupdater";
const wchar_t kElevationPolicyRegsitryKey[] =
    L"Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\";
const wchar_t kEmptyValueName[] = L"";
const wchar_t kKeyDefaultIcon[] = L"DefaultIcon";
const wchar_t kKeyShell[] = L"shell";
const wchar_t kKeyOpen[] = L"open";
const wchar_t kKeyCommand[] = L"command";
const wchar_t kLuaFileExt[] = L".lua";
const wchar_t kLuaFileIdentifier[] = L"GooglePinyinIME.Extension.Lua";
const wchar_t kLuaFileDescription[] = L"Google Pinyin IME extension";
// Windows Explorer requires an English string as the key name with the Chinese
// string as the default value. Otherwise, the Windows XP Chinese version will
// mistakenly add an underline to the first character of the Chinese command
// name.
const wchar_t kInstallToGoopyActionKey[] = L"Install to Google IME";
const wchar_t kInstallToGoopyAction[] =
    // Chinese translation for "Install to Goopy IME"
    L"\u5B89\u88C5\u5230\u8C37\u6B4C\u62FC\u97F3\u8F93\u5165\u6CD5";

// Migrated from inputmethod.cc
const wchar_t kGoopyUIClassName[] = L"GOOGLEPINYIN2";
const wchar_t kGoopyDisplayName[] =
    L"\x8c37\x6b4c\x62fc\x97f3\x8f93\x5165\x6cd5 "  // GuGePinYinShuRuFa
    GOOPY_SHORT_VERSION;
const LANGID kGoopyLanguageId =
    MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

// {4966A555-1B67-45c0-B82F-627FD19AAD22}
const CLSID kGoopyTextServiceClsid =
{0x4966a555, 0x1b67, 0x45c0, {0xb8, 0x2f, 0x62, 0x7f, 0xd1, 0x9a, 0xad, 0x22}};
extern const wchar_t kGoopyTextServiceClsidString[] =
    L"CLSID\\{4966A555-1B67-45c0-B82F-627FD19AAD22}";
// {9EE1D8A6-6C8F-4104-BB8E-5563319247A8}
const GUID kGoopyProfileGuid =
{0x9ee1d8a6, 0x6c8f, 0x4104, {0xbb, 0x8e, 0x55, 0x63, 0x31, 0x92, 0x47, 0xa8}};
}  // namespace ime_goopy
