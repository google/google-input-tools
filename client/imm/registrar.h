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
  explicit Registrar(const std::wstring& base_filename);
  // Registered HKL will be returned.
  HRESULT Register(const std::wstring& display_name, HKL* hkl);
  // Register the IME file for a specific language identifier.
  // Registered HKL will be returned.
  // See
  // http://msdn.microsoft.com/en-us/library/windows/desktop/dd318691(v=vs.85).aspx
  // for the reference of determining language identifiers.
  HKL Register(DWORD language_id, const std::wstring& display_name);
  HRESULT Unregister();
  // Unregister the IME of the given |language_id| with given |display_name|.
  // IMEs register will Register(const std::wstring& display_name, HKL* hkl)
  // should be Unregister by this method.
  HRESULT Unregister(DWORD language_id, const std::wstring& display_name);
  HRESULT Unregister(HKL hkl);
  HKL GetHKL();
  HKL GetHKL(DWORD language_id, const std::wstring& display_name);

 private:
  std::wstring base_filename_;
};
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_REGISTRAR_H__
