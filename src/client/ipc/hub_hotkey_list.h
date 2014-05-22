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

#ifndef GOOPY_IPC_HUB_HOTKEY_LIST_H_
#define GOOPY_IPC_HUB_HOTKEY_LIST_H_
#pragma once

#include <map>
#include <utility>

#include "base/basictypes.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

// A wrapper class of ipc::proto::HotkeyList, which provides hotkey matching
// functionality.
class HotkeyList {
 public:
  // The content of |hotkeys| object will be copied.
  explicit HotkeyList(const proto::HotkeyList& hotkeys);
  ~HotkeyList();

  uint32 id() const {
    return hotkeys_.id();
  }

  void set_id(uint32 id) {
    hotkeys_.set_id(id);
  }

  uint32 owner() const {
    return hotkeys_.owner();
  }

  void set_owner(uint32 owner) {
    hotkeys_.set_owner(owner);
  }

  const proto::HotkeyList& hotkeys() const { return hotkeys_; }

  // Matches a hotkey in this hotkey list.
  // Returns the pointer to the matched hotkey, or NULL if nothing is matched.
  const proto::Hotkey* Match(const proto::KeyEvent& previous,
                             const proto::KeyEvent& current) const;
 private:
  proto::HotkeyList hotkeys_;

  // Key is <keycode, modifiers>
  typedef std::map<std::pair<uint32, uint32>, int> HotkeyMap;

  HotkeyMap hotkey_map_;

  DISALLOW_COPY_AND_ASSIGN(HotkeyList);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_HOTKEY_LIST_H_
