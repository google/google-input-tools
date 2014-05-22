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

#include "components/common/keystroke_util.h"
#include "ipc/constants.h"

namespace ime_goopy {
namespace components {

KeyStroke KeyStrokeUtil::ConstructKeyStroke(const ipc::proto::KeyEvent& src) {
  uint8 key_state[256] = {0};
  bool down = (src.type() == ipc::proto::KeyEvent::DOWN);
  uint32 vkey = src.keycode();
  uint32 modifiers = src.modifiers();
  if (vkey == 0 && !src.text().empty()) {
    vkey = ::VkKeyScanA(src.text().at(0));
  }
  key_state[ipc::VKEY_SHIFT] |= ((modifiers & ipc::kShiftKeyMask) ? 0x80 : 0);
  key_state[ipc::VKEY_CONTROL] |=
    ((modifiers & ipc::kControlKeyMask) ? 0x80 : 0);
  key_state[ipc::VKEY_MENU] |= ((modifiers & ipc::kAltKeyMask) ? 0x80 : 0);
  key_state[ipc::VKEY_CAPITAL] |= ((modifiers & ipc::kCapsLockMask) ? 0x1 : 0);

  if (vkey != ipc::VKEY_CAPITAL)
    key_state[vkey] |= 0x80;

  if (vkey == ipc::VKEY_LSHIFT || vkey == ipc::VKEY_RSHIFT)
    vkey = ipc::VKEY_SHIFT;
  else if (vkey == ipc::VKEY_LCONTROL || vkey == ipc::VKEY_RCONTROL)
    vkey = ipc::VKEY_CONTROL;
  else if (vkey == ipc::VKEY_LMENU || vkey == ipc::VKEY_RMENU)
    vkey = ipc::VKEY_MENU;

  return KeyStroke(vkey, key_state, down);
}

}  // namespace components
}  // namespace ime_goopy
