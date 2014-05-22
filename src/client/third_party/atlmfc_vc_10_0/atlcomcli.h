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
// This file works around the compiling error in ATLMFC v10
// atlcomcli: error C2338: CVarTypeInfo<char> cannot be compiled with
// /J or _CHAR_UNSIGNED flag enabled.
//
// Background
//
// Blaze specifies that char shall be unsigned in google3 code. Some google3
// libraries have already depended on the assumption that unsigned char
// and char have the same range.
//
// On the other hand in ATLMFC v10, an assertion is introduced in atlcomcli.h
// that char should be treated as signed to compile CVarTypeInfo<char>
// and CVarTypeInfo<char*>. (The assertion does not exists in former ATLMFC
// versions.
//
// As suggested by http://support.microsoft.com/kb/983233, the following code
// temporarily disables the assertion.

#ifndef GOOPY_THIRD_PARTY_ATLMFC_VC_10_0_ATLCOMCLI_H_
#define GOOPY_THIRD_PARTY_ATLMFC_VC_10_0_ATLCOMCLI_H_

#if !defined(_MSC_VER) || (_MSC_VER != 1600)
#error "This walk around is for MSVC V10.0 (_MSC_VER == 1600) only."
#endif

#include <atldef.h>  // ATLSTATIC_ASSERT is defined here.
#pragma push_macro("ATLSTATIC_ASSERT")
#undef ATLSTATIC_ASSERT
#define ATLSTATIC_ASSERT(expr,comment)

#include "third_party/atlmfc_vc_10_0/files/include/atlcomcli.h"
#undef ATLSTATIC_ASSERT
#pragma pop_macro("ATLSTATIC_ASSERT")

#endif  // GOOPY_THIRD_PARTY_ATLMFC_VC_10_0_ATLCOMCLI_H_
