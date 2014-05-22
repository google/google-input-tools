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

#include <string>
#include "base/basictypes.h"
#include "imm/debug.h"
#include "imm/immdev.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
struct TestStruct {
  DWORD flag;
  wchar_t* name;
};

static const TestStruct kBitTestData[] = {
  NULL, L"NULL",
  ISC_SHOWUIALL, L"ISC_SHOWUIALL | ",
  ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUICANDIDATEWINDOW,
  L"ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUICANDIDATEWINDOW | ",
  ISC_SHOWUICANDIDATEWINDOW << 1, L"ISC_SHOWUICANDIDATEWINDOW << 1 | ",
};

static const TestStruct kEnumTestData[] = {
  IMN_OPENSTATUSWINDOW, L"IMN_OPENSTATUSWINDOW",
  IMN_SETSTATUSWINDOWPOS, L"IMN_SETSTATUSWINDOWPOS",
  0x100, L"UNKNOWN_FLAG: 100!",
};

TEST(Debug, BitTestISC) {
  for (int i = 0; i < arraysize(kBitTestData); i++) {
    EXPECT_STREQ(kBitTestData[i].name,
                 Debug::ISC_String(kBitTestData[i].flag).c_str());
  }
}

TEST(Debug, EnumTestIMN) {
  for (int i = 0; i < arraysize(kEnumTestData); i++) {
    EXPECT_STREQ(kEnumTestData[i].name,
                 Debug::IMN_String(kEnumTestData[i].flag).c_str());
  }
}

}  // namespace imm
}  // namespace ime_goopy
