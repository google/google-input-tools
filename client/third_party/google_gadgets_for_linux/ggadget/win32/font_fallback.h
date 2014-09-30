/*
  Copyright 2012 Google Inc.

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

#ifndef GGADGET_WIN32_FONT_FALLBACK_H__
#define GGADGET_WIN32_FONT_FALLBACK_H__

#include <windows.h>
#include <string>
#include "ggadget/common.h"

namespace ggadget {
namespace win32 {

// This class is a utility class that helps to find a propriate font for
// rendering the specified text.
class FontFallback {
 public:
  // Returns true if the current font in dc CANNOT render the given character.
  static bool ShouldFallback(HDC dc, wchar_t character);
  // Returns the font face that can render the given character.
  static LOGFONT GetFallbackFont(HDC dc, wchar_t character);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FontFallback);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_FONT_FALLBACK_H__
