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

#include <ggadget/common.h>
#include <ggadget/scoped_ptr.h>

#ifndef GGADGET_WIN32_PRIVATE_FONT_DATABASE_H_
#define GGADGET_WIN32_PRIVATE_FONT_DATABASE_H_

namespace Gdiplus {
  class FontFamily;
}

namespace ggadget {
namespace win32 {

class PrivateFontDatabase {
 public:
  PrivateFontDatabase();
  ~PrivateFontDatabase();
  // Creates a FontFamily using the font family name.
  // returns NULL if the family name is not found in current database.
  // The caller should delete the object after using it.
  Gdiplus::FontFamily* CreateFontFamilyByName(const wchar_t* family_name) const;
  // Add a font to the private font database.
  // returns true if success.
  bool AddPrivateFont(const wchar_t* font_file);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(PrivateFontDatabase);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_PRIVATE_FONT_DATABASE_H_
