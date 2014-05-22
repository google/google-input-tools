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

#include "components/common/virtual_keyboard_and_character_picker_consts.h"

#include <algorithm>
#include "base/macros.h"

namespace ime_goopy {
namespace components {

static const std::string kCharacterPickerUnvailableLanguages[] = {
  "am", "ti"
};
bool IsCharacterPickerAvailable(const std::string& language) {
  const std::string* end = kCharacterPickerUnvailableLanguages +
      arraysize(kCharacterPickerUnvailableLanguages);
  return std::find(kCharacterPickerUnvailableLanguages, end, language) == end;
}

}  // namespace components
}  // namespace ime_goopy