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

#include "skin/skin_manager.h"

#include <tchar.h>
#include <shlwapi.h>
#include <strsafe.h> // strssafe must be after tchar.h.

#include "base/logging.h"
#include "base/stringprintf.h"
#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/registry.h"
#include "common/string_utils.h"
#pragma push_macro("DLOG")
#pragma push_macro("LOG")
#include "skin/skin.h"
#include "skin/skin_consts.h"
#pragma pop_macro("LOG")
#pragma pop_macro("DLOG")
#include "third_party/google_gadgets_for_linux/ggadget/gadget_consts.h"

namespace ime_goopy {
namespace skin {

static const wchar_t kSkinFolder[] = L"Skins";
static const wchar_t kSkinFileSuffix[] = L".gskin";
static const int kSkinFileSuffixLen = arraysize(kSkinFileSuffix);

using ggadget::kManifestName;
using skin::kManifestCategory;
using std::vector;
using std::wstring;

SkinManager::SkinInfo::SkinInfo() : is_default(false) {
}

// Compares two skin info.
// Skins are ordered firstly by their category and secondly by their name.
// Default skin will always be the first skin.
static bool CompareSkinInfo(const SkinManager::SkinInfo& a,
                            const SkinManager::SkinInfo& b) {
  if (a.is_default && !b.is_default)
    return true;
  if (!a.is_default && b.is_default)
    return false;
  // StringMap::operator[] need non-const objects.
  return a.category < b.category ||
         (a.category == b.category && a.display_name < b.display_name);
}

void SkinManager::CopyMissingSkin() {
  wstring skin_system_path = AppUtils::GetSystemDataFilePath(kSkinFolder);
  wstring skin_user_path = GetBaseDir();
  AppUtils::CopyFileWhenMissing(skin_system_path, skin_user_path);
  vector<wstring> system_skin_list;
  vector<wstring> user_skin_list;
  AppUtils::GetFileList(skin_user_path.c_str(),
                        &user_skin_list,
                        IsSkinFile);
  AppUtils::GetFileList(skin_system_path.c_str(),
                        &system_skin_list,
                        IsSkinFile);
  if (system_skin_list != user_skin_list) {
    for (int i = 0; i < system_skin_list.size(); ++i) {
      wchar_t file_name[MAX_PATH] = {0};
      StringCchCopy(file_name, MAX_PATH, system_skin_list[i].c_str());
      PathStripPath(file_name);
      AppUtils::CopyFileWhenMissing(skin_system_path,
                                    skin_user_path,
                                    file_name);
    }
  }
}

void SkinManager::GetInstalledSkinPathList(vector<wstring>* skin_path_list) {
  DCHECK(skin_path_list);
  CopyMissingSkin();
  AppUtils::GetFileList(GetBaseDir().c_str(),
                        skin_path_list,
                        IsSkinFile);
  if (skin_path_list->empty()) {
    // Copies skins from the system data directory.
    // When goopy is newly installed, there's no skin in the user's home
    // directory, but the installer may install off-line skins in the system
    // directory.
    const wstring& base_dir = GetBaseDir();
    AppUtils::CopyFileWhenMissing(AppUtils::GetSystemDataFilePath(kSkinFolder),
                                  base_dir);
    AppUtils::GetFileList(base_dir.c_str(),
                          skin_path_list,
                          IsSkinFile);
  }
}

void SkinManager::GetInstalledSkinNameList(vector<wstring>* skin_name_list) {
  DCHECK(skin_name_list);
  vector<wstring> skin_path_list;
  GetInstalledSkinPathList(&skin_path_list);
  skin_name_list->reserve(skin_path_list.size());
  for (int i = 0; i < skin_path_list.size(); i++) {
    wchar_t file_name[MAX_PATH] = {0};
    StringCchCopy(file_name, MAX_PATH, skin_path_list[i].c_str());
    PathStripPath(file_name);
    PathRemoveExtension(file_name);
    skin_name_list->push_back(file_name);
  }
}

void SkinManager::GetInstalledSkinInfoList(
    std::vector<SkinInfo>* skin_info_list) {
  DCHECK(skin_info_list);
  vector<wstring> skin_path_list;
  GetInstalledSkinPathList(&skin_path_list);
  skin_info_list->reserve(skin_path_list.size());
  wstring default_skin_name = GetDefaultSkinName();
  for (size_t i = 0; i < skin_path_list.size(); i++) {
    wchar_t file_name[MAX_PATH] = {0};
    StringCchCopy(file_name, MAX_PATH, skin_path_list[i].c_str());
    PathStripPath(file_name);
    PathRemoveExtension(file_name);
    ggadget::StringMap manifest;
    if (skin::Skin::GetSkinManifestForLocale(
            WideToUtf8(skin_path_list[i]).c_str(), kSkinLocaleName,
            &manifest)) {
      SkinInfo skin_info;
      skin_info_list->push_back(skin_info);
      skin_info_list->back().path = skin_path_list[i];
      skin_info_list->back().file_name = file_name;
      skin_info_list->back().is_default = (file_name == default_skin_name);
      if (manifest[ggadget::kManifestName].empty()) {
        skin_info_list->back().display_name = file_name;
      } else {
        skin_info_list->back().display_name =
            Utf8ToWide(manifest[ggadget::kManifestName]);
      }
      skin_info_list->back().author =
          Utf8ToWide(manifest[ggadget::kManifestAuthor]);
      skin_info_list->back().category =
          Utf8ToWide(manifest[skin::kManifestCategory]);
    }
  }
  std::sort(skin_info_list->begin(), skin_info_list->end(), CompareSkinInfo);
}

wstring SkinManager::GetBaseDir() {
  return AppUtils::GetUserDataFilePath(kSkinFolder);
}

void SkinManager::GetActiveSkinPathList(
    vector<wstring>* active_skin_path_list) {
  DCHECK(active_skin_path_list);
  DCHECK(active_skin_path_list->empty());

  vector<wstring> skin_names;
  GetActiveSkinNameList(&skin_names);
  DCHECK(!skin_names.empty());
  for (int i = 0; i < skin_names.size(); i++) {
    active_skin_path_list->push_back(GetSkinFilePath(skin_names[i]));
  }
  DCHECK_EQ(active_skin_path_list->size(),
            skin_names.size());
}

void SkinManager::GetActiveSkinNameList(
    vector<wstring>* active_skin_name_list) {
  DCHECK(active_skin_name_list);
  DCHECK(active_skin_name_list->empty());

  scoped_ptr<RegistryKey> registry(AppUtils::OpenUserRegistry());
  if (registry.get()) {
    registry->QueryMultiStringValue(kActiveSkinRegkeyName,
                                    active_skin_name_list);
  }
  // Guarantee at least one active skin is returned.
  if (active_skin_name_list->empty())
    active_skin_name_list->push_back(GetDefaultSkinName());
}

void SkinManager::SetActiveSkinNameList(
    const vector<wstring>& active_skin_name_list) {
  DCHECK(!active_skin_name_list.empty());
  scoped_ptr<RegistryKey> registry(AppUtils::OpenUserRegistry());
  if (!registry.get()) return;
  registry->SetMultiStringValue(kActiveSkinRegkeyName, active_skin_name_list);
}

wstring SkinManager::GetSkinFilePath(const wstring& skin_name) {
  wstring skin_file =
      WideStringPrintf(L"%s%s", skin_name.c_str(), kSkinFileSuffix);
  wstring skin_system_path = AppUtils::GetSystemDataFilePath(kSkinFolder);
  wstring skin_user_path = GetBaseDir();
  AppUtils::CopyFileWhenMissing(skin_system_path, skin_user_path);
  wchar_t system_skin_path[MAX_PATH] = {0};
  wchar_t skin_path[MAX_PATH] = {0};
  PathCombine(system_skin_path, skin_system_path.c_str(), skin_file.c_str());
  PathCombine(skin_path, skin_user_path.c_str(), skin_file.c_str());
  // If skin_user_path exist but default skin is miss, copy it.
  AppUtils::CopyFileWhenMissing(system_skin_path, skin_path);
  if (!PathFileExists(skin_path)) {
    return L"";
  }
  return skin_path;
}

wstring SkinManager::GetDefaultSkinName() {
  // TODO(jiangrenkuan): Right now, we use file name as the default skin's
  // identify. In order to avoid the default skin is overwritten, in the
  // future, we should use skin GUID (within manifest) as the file name.
  // We don't need to verify if the default skin is available or not. It'll be
  // done in GetCurrentValidSkinPath().
  return kDefaultSkinName;
}

bool SkinManager::IsSkinFile(const wstring& filename) {
  wchar_t* extension = PathFindExtension(filename.c_str());
  return extension != NULL && _wcsicmp(extension, kSkinFileSuffix) == 0;
}

wstring SkinManager::GetCurrentValidSkinPath() {
  wstring skin_path;
  vector<wstring> active_skin;
  SkinManager::GetActiveSkinPathList(&active_skin);
  for (size_t i = 0; i < active_skin.size(); ++i) {
    if (active_skin[i].empty())
      continue;
    if (skin::Skin::ValidateSkinPackage(WideToUtf8(active_skin[i]).c_str(),
                                        kSkinLocaleName)) {
      vector<wstring> active_skin_names;
      GetActiveSkinNameList(&active_skin_names);
      active_skin_names.erase(active_skin_names.begin(),
                              active_skin_names.begin() + i);
      SetActiveSkinNameList(active_skin_names);
      skin_path = active_skin[i];
      break;
    }
  }
  // If all active skin doesn't work, try default skin;
  if (skin_path.empty()) {
    wstring default_skin_name = GetDefaultSkinName();
    if (skin::Skin::ValidateSkinPackage(
        WideToUtf8(GetSkinFilePath(default_skin_name)).c_str(),
        kSkinLocaleName)) {
      SetActiveSkinNameList(vector<wstring>(1, default_skin_name));
      skin_path = GetSkinFilePath(default_skin_name);
    }
  }
  // If default skin doesn't work, try installed skin.
  if (skin_path.empty()) {
    vector<wstring> installed_skin_names;
    GetInstalledSkinNameList(&installed_skin_names);
    for (size_t i = 0; i < installed_skin_names.size(); ++i) {
      wstring path = GetSkinFilePath(installed_skin_names[i]);
      if (skin::Skin::ValidateSkinPackage(
          WideToUtf8(path).c_str(), kSkinLocaleName)) {
        SetActiveSkinNameList(vector<wstring>(1, installed_skin_names[i]));
        skin_path = GetSkinFilePath(installed_skin_names[i]);
        break;
      }
    }
  }
  return skin_path;
}

}  // namespace skin
}  // namespace ime_goopy
