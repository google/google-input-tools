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

#ifndef GOOPY_IMM_COMMON_H_
#define GOOPY_IMM_COMMON_H_

#include <imm.h>

namespace ime_goopy {
namespace imm {

// Payload for IMN_PRIVATE
struct ImmPrivateNotification {
  enum Type {
    FINISH_CONTEXT_UPDATE,
    PRIVATE_UI_MESSAGE,
  };
  Type type;
  union {
    int components;  // For FINISH_CONTEXT_UPDATE
    void *payload;   // For PRIVATE_UI_MESSAGE
  };
  explicit ImmPrivateNotification(int components_value)
      : type(FINISH_CONTEXT_UPDATE), components(components_value) {
  }
  explicit ImmPrivateNotification(void *payload_value)
      : type(PRIVATE_UI_MESSAGE), payload(payload_value) {
  }
};

// Escape code for importing user dictionary.
#ifndef IME_ESC_IMPORT_DICTIONARY
#define IME_ESC_IMPORT_DICTIONARY (IME_ESC_PRIVATE_FIRST)
#endif

}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_COMMON_H_
