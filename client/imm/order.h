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

#ifndef GOOPY_IMM_ORDER_H_
#define GOOPY_IMM_ORDER_H_

#include "base/basictypes.h"

namespace ime_goopy {
namespace imm {
// Order class is used to change IME order in Vista. This class can not work
// in XP and will fail silently.
class Order {
 public:
  static void SetFirstChineseIME(HKL hkl);
};
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_ORDER_H_
