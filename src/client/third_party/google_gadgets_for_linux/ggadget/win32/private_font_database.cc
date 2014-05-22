/*
  Copyright 2011 Google Inc.

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

#include "private_font_database.h"
#include <windows.h>
#include <algorithm>
using std::min;
using std::max;
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
#include <gdiplus.h>
#include <string>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/system_utils.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {
namespace win32 {

class PrivateFontDatabase::Impl {
 public:
  Impl() {
  }

  ~Impl() {
    for (size_t i = 0; i < font_handles_.size(); ++i)
      ::RemoveFontMemResourceEx(font_handles_[i]);
  }

  bool AddPrivateFont(const wchar_t* font_file) {
    if (font_collection_.AddFontFile(font_file) != Gdiplus::Ok)
      return false;
    // Call AddFontMemResourceEx so that gdi apis can use the font.
    std::string data;
    std::string file_name_utf8;
    ConvertStringUTF16ToUTF8(font_file, wcslen(font_file), &file_name_utf8);
    if (!ReadFileContents(file_name_utf8.c_str(), &data))
      return false;
    DWORD font_count = 0;
    HANDLE handle = ::AddFontMemResourceEx(
        const_cast<char*>(data.c_str()),  data.size(), 0, &font_count);
    if (!handle)
      return false;
    font_handles_.push_back(handle);
    return true;
  }

  Gdiplus::FontFamily* CreateFontFamilyByName(const wchar_t* family_name) {
    int family_count = font_collection_.GetFamilyCount();
    scoped_array<Gdiplus::FontFamily> families(
        new Gdiplus::FontFamily[family_count]);
    int family_found = 0;
    if (font_collection_.GetFamilies(family_count, families.get(),
                                     &family_found) != Gdiplus::Ok ||
        family_found != family_count) {
      return NULL;
    }
    for (int i = 0; i < family_count; ++i) {
      wchar_t name[LF_FACESIZE] = {0};
      families[i].GetFamilyName(name);
      if (wcscmp(name, family_name) == 0) {
        return families[i].Clone();
      }
    }
    return NULL;
  }

  Gdiplus::PrivateFontCollection font_collection_;
  std::vector<HANDLE> font_handles_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

PrivateFontDatabase::PrivateFontDatabase() : impl_(new Impl) {
}

PrivateFontDatabase::~PrivateFontDatabase() {
}

Gdiplus::FontFamily* PrivateFontDatabase::CreateFontFamilyByName(
    const wchar_t* family_name) const {
  return impl_->CreateFontFamilyByName(family_name);
}

bool PrivateFontDatabase::AddPrivateFont(const wchar_t* font_file) {
  return impl_->AddPrivateFont(font_file);
}

}  // namespace win32
}  // namespace ggadget
