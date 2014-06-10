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

#include "skin/skin_host_win.h"

#include <string>
#include "skin/skin.h"
#include "third_party/google_gadgets_for_linux/ggadget/unicode_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/gadget_window.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/private_font_database.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/single_view_host.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/utilities.h"

using ggadget::ConvertStringUTF16ToUTF8;
using ggadget::GadgetInterface;
using ggadget::UTF16String;
using ggadget::ViewHostInterface;
using ggadget::win32::PrivateFontDatabase;
using ggadget::win32::SingleViewHost;


namespace ime_goopy {
namespace skin {

SkinHostWin::SkinHostWin() : private_font_database_() {
}

SkinHostWin::~SkinHostWin() {
}

ViewHostInterface* SkinHostWin::NewViewHost(GadgetInterface* gadget,
                                            ViewHostInterface::Type type) {
  return new SingleViewHost(
      type,
      1.0,
      0,
      &private_font_database_,
      CS_IME,
      WS_DISABLED,
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
}

bool SkinHostWin::LoadFont(const char* filename) {
  UTF16String filename_utf16;
  ggadget::ConvertStringUTF8ToUTF16(filename, strlen(filename),
                                    &filename_utf16);
  return private_font_database_.AddPrivateFont(
      reinterpret_cast<const wchar_t*>(filename_utf16.c_str()));
}

bool SkinHostWin::OpenURL(const GadgetInterface* gadget, const char* url) {
  return ggadget::win32::OpenURL(gadget, url);
}

SkinHost* SkinHost::NewSkinHost() {
  return new SkinHostWin;
}

}  // namespace skin
}  // namespace ime_goopy
