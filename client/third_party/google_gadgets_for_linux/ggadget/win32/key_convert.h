/*
  Copyright 2008 Google Inc.

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

// This file contains some utility functions that convert windows key codes to
// ggadget codes.
#ifndef GGADGET_WIN32_KEY_CONVERT_H__
#define GGADGET_WIN32_KEY_CONVERT_H__

#include <windows.h>
#include <ggadget/common.h>
#include <ggadget/event.h>

namespace ggadget {
namespace win32 {

// Converts windows virtual key code to ggadget key code.
KeyboardEvent::KeyCode ConvertVirtualKeyCodeToKeyCode(WPARAM keycode);

// Gets the current key modifier and converts it to ggadget key modifier.
int GetCurrentKeyModifier();

// Converts windows key modifier to ggadget key modifier.
int ConvertWinKeyModiferToGGadgetKeyModifer(WPARAM key_modifier_state);

// Converts windows mouse buttion flag to ggadget button flag.
int ConvertWinButtonFlagToGGadgetButtonFlag(WPARAM button_flag);

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_KEY_CONVERT_H__
