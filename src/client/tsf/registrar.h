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

#ifndef GOOPY_TSF_REGISTRAR_H_
#define GOOPY_TSF_REGISTRAR_H_

#include <atlbase.h>

#include "base/basictypes.h"

namespace ime_goopy {
namespace tsf {
// Register is used to register and unregister the text service in the system.
// These class can only be used in administrators account.
class Registrar {
 public:
  explicit Registrar(const CLSID& clsid);
  HRESULT Register(const wstring& base_filename,
                   LANGID language_id,
                   const GUID& profile_guid,
                   const wstring& display_name,
                   HKL hkl);
  HRESULT Unregister(LANGID language_id,
                     const GUID& profile_guid);
  bool IsInstalled();

 private:
  CLSID clsid_;
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_REGISTRAR_H_
