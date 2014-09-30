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

#include "common/goopy_utils.h"

#include <atlbase.h>
#include <atlwin.h>
#include <shlobj.h>
#include <psapi.h>

#include "base/stringprintf.h"
#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/install_tools.h"
#include "common/registry.h"
#include "common/shellutils.h"

namespace ime_goopy {

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
const wchar_t kTextServiceRegistryKey[] =
  L"CLSID\\{4966A555-1B67-45c0-B82F-627FD19AAD22}";

static const wchar_t kElevationPolicyRegsitryKey[] =
  L"Software\\Microsoft\\Internet Explorer\\Low Rights\\ElevationPolicy\\";
static const wchar_t kEmptyValueName[] = L"";
static const wchar_t kKeyDefaultIcon[] = L"DefaultIcon";
static const wchar_t kKeyShell[] = L"shell";
static const wchar_t kKeyOpen[] = L"open";
static const wchar_t kKeyCommand[] = L"command";
static const wchar_t kLuaFileExt[] = L".lua";
static const wchar_t kLuaFileIdentifier[] = L"GooglePinyinIME.Extension.Lua";
static const wchar_t kLuaFileDescription[] = L"Google Pinyin IME extension";

// Windows Explorer requires an English string as the key name with the Chinese
// string as the default value. Otherwise, the Windows XP Chinese version will
// mistakenly add an underline to the first character of the Chinese command
// name.
static const wchar_t kInstallToGoopyActionKey[] = L"Install to Google IME";
static const wchar_t kInstallToGoopyAction[] =
    L"Install to Google Input Tools";

CPath GoopyUtils::GetTargetFolder(TargetFolder target) {
  switch (target) {
    case TARGET_SYSTEM_X86:
      if (ShellUtils::Is64BitOS())
        return installer::FileUtils::GetShellSubFolder(CSIDL_SYSTEMX86, L"");
      else
        return installer::FileUtils::GetShellSubFolder(CSIDL_SYSTEM, L"");
      break;
    case TARGET_SYSTEM_X64:
      if (ShellUtils::Is64BitOS()) {
        return installer::FileUtils::GetShellSubFolder(CSIDL_SYSTEM, L"");
      } else {
        DCHECK("0");
        return L"";
      }
      break;
    case TARGET_BINARY:
      return installer::FileUtils::GetShellSubFolder(CSIDL_PROGRAM_FILES,
                                                     kInputToolsSubFolder);
      break;
    case TARGET_SYSTEM_DATA:
      return installer::FileUtils::GetShellSubFolder(CSIDL_COMMON_APPDATA,
                                                     kInputToolsSubFolder);
      break;
    case TARGET_USER_DATA:
      return installer::FileUtils::GetShellSubFolder(CSIDL_APPDATA,
                                                     kInputToolsSubFolder);
      break;
  };
  return L"";
}

CPath GoopyUtils::GetTargetPath(TargetFolder target, const CString& filename) {
  CPath path = GetTargetFolder(target);
  path.Append(filename);
  return path;
}

// Associates a file extension with commands. extension should be something like
// ".ext", file_type is the identifier of the file type. specific_action_key,
// specific_action_name, and specific_action_command are used to register a new
// command other than the default "open" command, e.g., "install into IME".
//
// The current logic is a bit conservative: if the file extension is already
// registered, do not change the existing file type identifer, do not change the
// existing open command - only add the specific action into the registry.
static bool InternalAssociateFile(const wchar_t* extension,
                                  const wchar_t* file_type,
                                  const wchar_t* file_description,
                                  const wchar_t* icon_path,
                                  const wchar_t* open_command,
                                  const wchar_t* specific_action_key,
                                  const wchar_t* specific_action_name,
                                  const wchar_t* specific_action_command) {
  bool existed = false;
  wstring previous_value;
  if (!RegistryKey::CreateAndSetStringValueIfNotExisted(
          HKEY_CLASSES_ROOT, extension,
          kEmptyValueName, file_type,
          &existed, &previous_value)) {
    return false;
  }
  wstring new_file_type = existed ? previous_value : file_type;
  if (!RegistryKey::CreateAndSetStringValueIfNotExisted(
          HKEY_CLASSES_ROOT, new_file_type,
          kEmptyValueName, file_description,
          NULL, NULL)) {
    return false;
  }
  wstring key = WideStringPrintf(L"%s\\%s",
                                 new_file_type.c_str(),
                                 kKeyDefaultIcon);
  if (!RegistryKey::CreateAndSetStringValueIfNotExisted(
          HKEY_CLASSES_ROOT, key,
          kEmptyValueName, icon_path,
          NULL, NULL)) {
    return false;
  }
  key = WideStringPrintf(L"%s\\%s\\%s\\%s",
                         new_file_type.c_str(),
                         kKeyShell,
                         kKeyOpen,
                         kKeyCommand);
  if (!RegistryKey::CreateAndSetStringValueIfNotExisted(
          HKEY_CLASSES_ROOT, key,
          kEmptyValueName, open_command,
          NULL, NULL)) {
    return false;
  }
  key = WideStringPrintf(L"%s\\%s\\%s",
                         new_file_type.c_str(),
                         kKeyShell,
                         specific_action_key);
  if (!RegistryKey::CreateAndSetStringValueIfNotExisted(
          HKEY_CLASSES_ROOT, key,
          kEmptyValueName, specific_action_name,
          NULL, NULL)) {
    return false;
  }
  key = WideStringPrintf(L"%s\\%s\\%s\\%s",
                         new_file_type.c_str(),
                         kKeyShell,
                         specific_action_key,
                         kKeyCommand);
  if (!RegistryKey::CreateAndSetStringValueIfNotExisted(
          HKEY_CLASSES_ROOT, key,
          kEmptyValueName, specific_action_command,
          NULL, NULL)) {
    return false;
  }
  return true;
}

// Removes the association of a particular file extension. If the extension is
// associated to a different file type identifier(i.e., it's registered by other
// applications), only locates then removes the specific action registered by
// Goopy.
static bool InternalDissociateFile(const wchar_t* extension,
                                   const wchar_t* file_type,
                                   const wchar_t* specific_action_key,
                                   const wchar_t* specific_action) {
  LONG ret = 0;
  RegistryKey registry;
  ret = registry.Open(HKEY_CLASSES_ROOT, extension,
                      KEY_READ | KEY_WRITE | KEY_WOW64_64KEY);
  if (ret != ERROR_SUCCESS)
    return false;
  wstring previous_file_type;
  ret = registry.QueryStringValue(kEmptyValueName, &previous_file_type);
  if (ret != ERROR_SUCCESS)
    return false;
  if (previous_file_type == file_type) {
    RegistryKey::RecurseDeleteKey(HKEY_CLASSES_ROOT, extension, 0);
    RegistryKey::RecurseDeleteKey(HKEY_CLASSES_ROOT, file_type, 0);
  } else {
    ret = registry.Open(HKEY_CLASSES_ROOT, previous_file_type.c_str());
    if (ret != ERROR_SUCCESS)
      return false;
    wstring key = WideStringPrintf(L"%s\\%s",
                                   kKeyShell,
                                   specific_action_key);
    RegistryKey::RecurseDeleteKey(registry.m_hKey, key, 0);
    // Also deletes the Chinese action name which was used by old version as the
    // action key, so that the new logic can cover the old installs.
    key = WideStringPrintf(L"%s\\%s",
                           kKeyShell,
                           specific_action);
    RegistryKey::RecurseDeleteKey(registry.m_hKey, key, 0);
  }
  return true;
}

bool GoopyUtils::AreFileExtensionsAssociated() {
  // Only check if the IME-specific action of the lua file extension exists.
  LONG ret = 0;
  RegistryKey registry;
  ret = registry.Open(HKEY_CLASSES_ROOT, kLuaFileExt,
                      KEY_READ | KEY_WOW64_64KEY);
  if (ret != ERROR_SUCCESS)
    return false;
  wstring file_type;
  ret = registry.QueryStringValue(kEmptyValueName, &file_type);
  if (ret != ERROR_SUCCESS)
    return false;
  wstring key = WideStringPrintf(L"%s\\%s\\%s",
                                 file_type.c_str(),
                                 kKeyShell,
                                 kInstallToGoopyActionKey);
  ret = registry.Open(HKEY_CLASSES_ROOT, key.c_str(),
                      KEY_READ  | KEY_WOW64_64KEY);
  if (ret == ERROR_SUCCESS)
    return true;
  // Also checks the Chinese action name which was used by old version as the
  // action key, so that the new logic can cover the old installs.
  key = WideStringPrintf(L"%s\\%s\\%s",
                         file_type.c_str(),
                         kKeyShell,
                         kInstallToGoopyAction);
  ret = registry.Open(HKEY_CLASSES_ROOT, key.c_str(),
                      KEY_READ | KEY_WOW64_64KEY);
  return (ret == ERROR_SUCCESS);
}

bool GoopyUtils::AssociateFileExtensions() {
  CString command = GetTargetPath(TARGET_BINARY, kOptionsFilename);
  command = L"\"" + command + L"\" --import_ext_file=\"%1\"";
  CString icon_path = GetTargetPath(TARGET_SYSTEM_X86, kImeFilename);
  return InternalAssociateFile(
      kLuaFileExt,
      kLuaFileIdentifier,
      kLuaFileDescription,
      icon_path,
      command,
      kInstallToGoopyActionKey,
      kInstallToGoopyAction,
      command);
}

bool GoopyUtils::DissociateFileExtensions() {
  return InternalDissociateFile(
      kLuaFileExt,
      kLuaFileIdentifier,
      kInstallToGoopyActionKey,
      kInstallToGoopyAction);
}

bool GoopyUtils::ExtractBrandCodeFromFileName(CString* brand) {
  CPath path(AppUtils::GetBinaryFileName().c_str());
  path.StripPath();
  path.RemoveExtension();
  CString suffix = path;
  static const int kSuffixLength = 5;
  suffix = suffix.Right(kSuffixLength);
  if (suffix.GetLength() != kSuffixLength || suffix[0] != L'_')
    return false;
  *brand = suffix.Right(kSuffixLength - 1).MakeUpper();
  return true;
}

}  // namespace ime_goopy
