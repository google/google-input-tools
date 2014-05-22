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
// Since ATL headers has its fixed order of include. We create a common
// file to avoid problems.

#ifndef GOOPY_COMMON_ATL_H_
#define GOOPY_COMMON_ATL_H_

// This should be placed out of the warning push-pop block below.
// Otherwise code analysis warnings won't be resumed for some unknown
// reason.
#include <atlbase.h>
// Fixed order of headers. Do not change it unless you know what you are doing.
#pragma warning(push)
#include <codeanalysis/warnings.h>
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <atlstr.h>
#include <atltypes.h>

#ifdef NOMINMAX
#include <algorithm>
using std::min;
using std::max;
#endif

#include <atlapp.h>

#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atldlgs.h>
#include <atlimage.h>
#include <atlwin.h>
#pragma warning(pop)

#endif  // GOOPY_COMMON_ATL_H_
