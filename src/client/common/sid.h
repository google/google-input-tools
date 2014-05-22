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

#ifndef GOOPY_COMMON_SID_H__
#define GOOPY_COMMON_SID_H__

#include <windows.h>
#include "base/basictypes.h"

namespace ime_goopy {

// This class is used to hold and generate SIDS.
class Sid {
 public:
  // Constructors initializing the object with the SID passed.
  // This is a converting constructor. It is not explicit.
  Sid(const SID *sid);
  Sid(WELL_KNOWN_SID_TYPE type);

  // Returns sid_.
  const SID *GetPSID() const;

 private:
  BYTE sid_[SECURITY_MAX_SID_SIZE];
};

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_SID_H__
