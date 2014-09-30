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

#include "ipc/hub_hotkey_list.h"

#include "ipc/constants.h"

namespace {

// Valid modifiers for a hotkey.
const uint32 kValidModifiersMask =
    ipc::kShiftKeyMask |
    ipc::kControlKeyMask |
    ipc::kAltKeyMask |
    ipc::kMetaKeyMask;

// A special mask to indicate an UP key event.
const uint32 kUpMask = 1 << 31;

}

namespace ipc {
namespace hub {

HotkeyList::HotkeyList(const proto::HotkeyList& hotkeys)
    : hotkeys_(hotkeys) {
  int size = hotkeys_.hotkey_size();
  for (int i = 0; i < size; ++i) {
    const proto::Hotkey& hotkey = hotkeys_.hotkey(i);
    int key_event_size = hotkey.key_event_size();
    for (int k = 0; k < key_event_size; ++k) {
      const proto::KeyEvent& key_event = hotkey.key_event(k);
      uint32 keycode = key_event.keycode();
      if (!keycode)
        continue;

      uint32 modifiers = (key_event.modifiers() & kValidModifiersMask);
      if (key_event.type() == proto::KeyEvent::UP)
        modifiers |= kUpMask;

      hotkey_map_[std::make_pair(keycode, modifiers)] = i;
    }
  }
}

HotkeyList::~HotkeyList() {
}

const proto::Hotkey* HotkeyList::Match(
    const proto::KeyEvent& previous,
    const proto::KeyEvent& current) const {
  uint32 keycode = current.keycode();
  uint32 modifiers = (current.modifiers() & kValidModifiersMask);

  // Only match key up events which meet certain criteria.
  if (current.type() == proto::KeyEvent::UP) {
    // The previous key event must be a key down.
    if (previous.type() != proto::KeyEvent::DOWN)
      return NULL;

    // The previous key event should have the same modifiers as the current one.
    if ((previous.modifiers() & kValidModifiersMask) != modifiers)
      return NULL;

    // The previous and current key event must have the same key code, unless
    // they are both modifiers.
    if (previous.keycode() != keycode &&
        (!previous.is_modifier() || !current.is_modifier()))
      return NULL;

    modifiers |= kUpMask;
  }

  HotkeyMap::const_iterator i =
      hotkey_map_.find(std::make_pair(keycode, modifiers));
  return (i != hotkey_map_.end()) ? &hotkeys_.hotkey(i->second) : NULL;
}

}  // namespace hub
}  // namespace ipc
