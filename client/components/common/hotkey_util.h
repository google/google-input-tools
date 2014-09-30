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

#ifndef GOOPY_COMPONENTS_COMMON_HOTKEY_UTIL_H_
#define GOOPY_COMPONENTS_COMMON_HOTKEY_UTIL_H_

#include "base/basictypes.h"
#include "ipc/keyboard_codes.h"

namespace ipc {
namespace proto {
class HotkeyList;
}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace components {

class HotkeyUtil {
 public:
  static void AddHotKey(ipc::KeyboardCode keycode, uint32 modifiers,
                        const char* command_string_id, uint32 component_id,
                        ipc::proto::HotkeyList* hotkey_list);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HotkeyUtil);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_HOTKEY_UTIL_H_
