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

// This file content the implementation of class GdiplusFont, including
// initialization, and creating a Gdiplus Font object.

#include "gdiplus_font.h"

#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
using std::min;
using std::max;
#include <gdiplus.h>
#include <string>
#include <ggadget/unicode_utils.h>
#include "private_font_database.h"

namespace ggadget {
namespace win32 {

GdiplusFont::GdiplusFont()
    : size_(0),
      style_(STYLE_NORMAL),
      weight_(WEIGHT_NORMAL) {
}

GdiplusFont::~GdiplusFont() {
}

bool GdiplusFont::Init(const std::string& font_name,
                       double size,
                       Style style,
                       Weight weight,
                       const PrivateFontDatabase* private_font_database_) {
  size_ = size;
  style_ = style;
  weight_ = weight;
  font_name_ = font_name;
  UTF16String font_name_utf16;
  ConvertStringUTF8ToUTF16(font_name, &font_name_utf16);
  // Check private font database first.
  if (private_font_database_) {
    font_family_.reset(private_font_database_->CreateFontFamilyByName(
        reinterpret_cast<const wchar_t*>(font_name_utf16.c_str())));
  }
  // If font not found, check the system fonts.
  if (font_family_.get() == NULL || !font_family_->IsAvailable()) {
    font_family_.reset(new Gdiplus::FontFamily(
        reinterpret_cast<const wchar_t*>(font_name_utf16.c_str())));
  }
  // If font not found in system fonts either, return the default font.
  if (font_family_.get() == NULL || !font_family_->IsAvailable())
    font_family_.reset(Gdiplus::FontFamily::GenericSansSerif()->Clone());
  return font_family_.get() && font_family_->IsAvailable();
}

Gdiplus::Font* GdiplusFont::CreateGdiplusFont(bool underline,
                                              bool strikeout) const {
  if (font_family_.get() == NULL)
    return NULL;
  INT font_style = Gdiplus::FontStyleRegular;
  if (style_ == STYLE_ITALIC)
    font_style |= Gdiplus::FontStyleItalic;
  if (weight_ == WEIGHT_BOLD)
    font_style |= Gdiplus::FontStyleBold;
  if (underline)
    font_style |= Gdiplus::FontStyleUnderline;
  if (strikeout)
    font_style |= Gdiplus::FontStyleStrikeout;
  return new Gdiplus::Font(font_family_.get(),
                           static_cast<Gdiplus::REAL>(size_), font_style);
}

void GdiplusFont::ClearStaticFonts() {
  Gdiplus::GenericSansSerifFontFamily = NULL;
  Gdiplus::GenericSerifFontFamily = NULL;
  Gdiplus::GenericMonospaceFontFamily = NULL;
}

std::string GdiplusFont::GetFontName() const {
  return font_name_;
}

}  // namespace win32
}  // namespace ggadget
