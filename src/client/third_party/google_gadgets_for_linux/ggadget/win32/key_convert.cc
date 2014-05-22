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

#include "key_convert.h"

#include <algorithm>
#include <ggadget/common.h>
#include <ggadget/event.h>

using ggadget::KeyboardEvent;

namespace ggadget {
namespace win32 {

KeyboardEvent::KeyCode ConvertVirtualKeyCodeToKeyCode(WPARAM keyval) {
  return static_cast<KeyboardEvent::KeyCode>(keyval);
}

int GetCurrentKeyModifier() {
  int modifier = Event::MODIFIER_NONE;
  if (::GetKeyState(VK_SHIFT)) modifier |= Event::MODIFIER_SHIFT;
  if (::GetKeyState(VK_MENU)) modifier |= Event::MODIFIER_ALT;
  if (::GetKeyState(VK_CONTROL)) modifier |= Event::MODIFIER_CONTROL;
  return modifier;
}

int ConvertWinKeyModiferToGGadgetKeyModifer(WPARAM key_modifier_state) {
  int modifier = Event::MODIFIER_NONE;
  if (key_modifier_state & MK_CONTROL) modifier |= Event::MODIFIER_CONTROL;
  if (key_modifier_state & MK_SHIFT) modifier |= Event::MODIFIER_SHIFT;
  if (key_modifier_state & MK_ALT) modifier |= Event::MODIFIER_ALT;
  return modifier;
}

int ConvertWinButtonFlagToGGadgetButtonFlag(WPARAM button_flag) {
  int buttons = MouseEvent::BUTTON_NONE;
  if (button_flag & MK_LBUTTON) buttons |= MouseEvent::BUTTON_LEFT;
  if (button_flag & MK_MBUTTON) buttons |= MouseEvent::BUTTON_MIDDLE;
  if (button_flag & MK_RBUTTON) buttons |= MouseEvent::BUTTON_RIGHT;
  return buttons;
}

}  // namespace win32
}  // namespace ggadget
