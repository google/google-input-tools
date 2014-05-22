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

#ifndef GOOPY_IMM_REGISTRAR_H__
#define GOOPY_IMM_REGISTRAR_H__

#include <atlbase.h>
#include "base/basictypes.h"

namespace ime_goopy {
namespace imm {
// Registrar is used to register and unregister the IME in the system.
// This class can only be used in administrators account.
class Registrar {
 public:
  explicit Registrar(const wstring& base_filename);
  // Registered HKL will be returned.
  HRESULT Register(const wstring& display_name, HKL* hkl);
  HRESULT Unregister();
  HKL GetHKL();

 private:
  wstring base_filename_;
};
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_REGISTRAR_H__
