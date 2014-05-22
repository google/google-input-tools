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

// This file mainly defines the SkinManager class.

#ifndef GOOPY_SKIN_SKIN_MANAGER_H__
#define GOOPY_SKIN_SKIN_MANAGER_H__

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {

namespace skin {
// This class manages skin files in the user's profile directory.
class SkinManager {
 public:
  // This structure contains the infomations of each skin, including skin file
  // name (file_name), skin file path (path), the display name, author and the
  // category of skin, and if it is the default skin.
  struct SkinInfo {
    SkinInfo();
    std::wstring file_name;
    std::wstring path;
    std::wstring display_name;
    std::wstring author;
    std::wstring category;
    bool is_default;
  };
  // Return installed skin file path list.
  static void GetInstalledSkinPathList(
      std::vector<std::wstring>* skin_path_list);

  // Return installed skin name list.
  static void GetInstalledSkinNameList(
      std::vector<std::wstring>* skin_name_list);

  // Return installed skin info list. The list is sorted firstly by the skin
  // category and secondly by the skin name, and the default skin is always
  // the first item.
  static void GetInstalledSkinInfoList(
      std::vector<SkinInfo>* skin_info_list);

  // Return active skin file path list.
  static void GetActiveSkinPathList(
      std::vector<std::wstring>* active_skin_path_list);

  // Return active skin name list.
  static void GetActiveSkinNameList(
      std::vector<std::wstring>* active_skin_name_list);

  // Set active skin names.
  static void SetActiveSkinNameList(
      const std::vector<std::wstring>& active_skin_name_list);

  // Get the full path for the skin file path in (typically)
  // %Appdata%\Google\Google Pinyin 2\Skin\skin_name
  static std::wstring GetSkinFilePath(const std::wstring& skin_name);

  // Get default skin name. If the default skin doesn't exist, it will return
  // the first valid installed skin.
  static std::wstring GetDefaultSkinName();

  // Get the current skin path, i.e. the first item in active skin, if the
  // current skin is invalid, try the rest active skins. If all the active
  // skins are invalid, try the default skin. If the default skin doesn't
  // work either, try all the installed skin. This function will modified
  // active skin registry if current skin is invalid.
  static std::wstring GetCurrentValidSkinPath();

  // Get the base directory of the skin files. Typically
  // %Appdata%\Google\Google Pinyin 2\Skins
  static std::wstring GetBaseDir();

  // Copy missing skins to user skins directory.
  static void CopyMissingSkin();

 private:
  static bool IsSkinFile(const std::wstring& filename);

  DISALLOW_COPY_AND_ASSIGN(SkinManager);
};
}  // namespace skin
}  // namespace ime_goopy
#endif  // GOOPY_SKIN_SKIN_MANAGER_H__
