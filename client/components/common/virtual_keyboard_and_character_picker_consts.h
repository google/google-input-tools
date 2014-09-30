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

#ifndef GOOPY_COMPONENTS_COMMON_VIRTUAL_KEYBOARD_AND_CHARACTER_PICKER_CONSTS_H_
#define GOOPY_COMPONENTS_COMMON_VIRTUAL_KEYBOARD_AND_CHARACTER_PICKER_CONSTS_H_

#include <string>

// This file defines some common constants that both virtualkeyboard and
// character picker will use.
namespace ime_goopy {
namespace components {

static const char kSettingsEnableVirtualKeyboard[] = "enable_vk";
static const char kSettingsOpenCharacterPicker[] = "open_char_picker";

bool IsCharacterPickerAvailable(const std::string& language);

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_COMMON_VIRTUAL_KEYBOARD_AND_CHARACTER_PICKER_CONSTS_H_
