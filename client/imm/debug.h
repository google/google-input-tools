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

#ifndef GOOPY_IMM_DEBUG_H__
#define GOOPY_IMM_DEBUG_H__

#include <string>
#include "base/basictypes.h"

namespace ime_goopy {
namespace imm {
class Debug {
 public:
  static wstring GCS_String(DWORD value);
  static wstring IMN_String(DWORD value);
  static wstring ISC_String(DWORD value);
  static wstring IMC_String(DWORD value);
  static wstring IME_SYSINFO_String(DWORD value);
  static wstring IME_CONFIG_String(DWORD value);
  static wstring NI_String(DWORD value);
  static wstring CPS_String(DWORD value);
  static wstring IME_ESC_String(DWORD value);
};
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_DEBUG_H__
