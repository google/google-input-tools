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

// The file contains the implementation of the graphics Interface on Gdi+.

#ifndef GGADGET_WIN32_GDIPLUS_FONT_H_
#define GGADGET_WIN32_GDIPLUS_FONT_H_

#include <string>
#include <ggadget/common.h>
#include <ggadget/font_interface.h>
#include <ggadget/scoped_ptr.h>

namespace Gdiplus {
class Font;
class FontFamily;
}  // namespace Gdiplus

namespace ggadget {
namespace win32 {

class PrivateFontDatabase;

// A Gdi+ implementation of the FontInterface.
class GdiplusFont : public FontInterface {
 public:
  GdiplusFont();
  bool Init(const std::string& font_name,
            double size,
            Style style,
            Weight weight,
            const PrivateFontDatabase* private_font_database_);
  virtual ~GdiplusFont();
  virtual Style GetStyle() const {
    return style_;
  }
  virtual Weight GetWeight() const {
    return weight_;
  }
  virtual double GetPointSize() const {
    return size_;
  }
  virtual void Destroy() {
    delete this;
  }
  // Creates a new Font object with respect to the font styles (underline and
  // strikeout). The caller should delete the object when done.
  Gdiplus::Font* CreateGdiplusFont(bool underline, bool strikeout) const;

  // Clear static variables defined in gdiplusheaders.h when they are no longer
  // available. This function should be call after calling GdiplusShutdown and
  // before calling GdiplusStartUp again.
  static void ClearStaticFonts();

  // Returns the font name.
  std::string GetFontName() const;

 private:
  scoped_ptr<Gdiplus::FontFamily> font_family_;
  std::string font_name_;
  double size_;
  Style style_;
  Weight weight_;

  DISALLOW_EVIL_CONSTRUCTORS(GdiplusFont);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_GDIPLUS_FONT_H_
