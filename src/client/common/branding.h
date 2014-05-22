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

#ifndef GOOPY_COMMON_BRANDING_H__
#define GOOPY_COMMON_BRANDING_H__

#include <atlstr.h>
#include <atlutil.h>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {
class RegistryKey;
class Branding {
 public:
  Branding();
  ~Branding();

  CString GetUserId();
  CString GetBrandCode();
  CString GetMachineGuid();
  CString GetOSVersion();
  // Gets rlz code for ping request.
  CString GetRlzCode();
  // Starting from v2.0.7, the rlz string will be obtained from pso server
  // instead of using a client-side generation. It is stored in a different
  // registry location (via rlz_lib). GetLegacyRlzCode() is used to obtain
  // the original rlz from versions before v2.0.7 for backward compatiblity.
  CString GetLegacyRlzCode();
  CString GetHashedMacAddress();

 private:
  int GetRlzWeeks();
  scoped_ptr<RegistryKey> user_registry_;
  scoped_ptr<RegistryKey> system_registry_;
  DISALLOW_EVIL_CONSTRUCTORS(Branding);
};
}  // namespace ime_goopy

#endif  // GOOPY_COMMON_BRANDING_H__
