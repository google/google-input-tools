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
//
// Based on the source code of Chromium.
// Original code: chromimutrunk/sandbox/src/sid.*

#include "common/sid.h"
#include "base/logging.h"

namespace ime_goopy {

Sid::Sid(const SID *sid) {
  ::CopySid(SECURITY_MAX_SID_SIZE, sid_, const_cast<SID*>(sid));
};

Sid::Sid(WELL_KNOWN_SID_TYPE type) {
  DWORD size_sid = SECURITY_MAX_SID_SIZE;
  BOOL result = ::CreateWellKnownSid(type, NULL, sid_, &size_sid);
  DCHECK(result);
  DBG_UNREFERENCED_LOCAL_VARIABLE(result);
}

const SID *Sid::GetPSID() const {
  return reinterpret_cast<SID*>(const_cast<BYTE*>(sid_));
}

}  // namespace ime_goopy
