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
#include "components/win_frontend/text_styles.h"

#include "base/basictypes.h"
namespace ime_goopy {
namespace components {
const TextStyle kStockTextStyles[] = {
  {  // Normal style.
    { 0x7880c693, 0xa6c, 0x4e7f,
      { 0x8e, 0xbd, 0xfc, 0xb6, 0x85, 0x5c, 0x12, 0xf6 } },
    TextStyle::FIELD_NONE,
    0, 0, 0, TextStyle::LINESTYLE_NONE, false
  }, {  // Underlined style.
    { 0xf2ef9e6c, 0x4bab, 0x454e,
      { 0xbf, 0x8a, 0x71, 0xc0, 0x19, 0x95, 0xde, 0xa4 } },
    TextStyle::FIELD_NONE,
    0, 0, 0, TextStyle::LINESTYLE_DOT, false
  }
};

COMPILE_ASSERT(arraysize(kStockTextStyles) == STOCKSTYLE_COUNT,
               StyleCountDoesNotMatch);
}  // namespace components
}  // namespace ime_goopy
