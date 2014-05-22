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

#include "imm/candidate_info.h"
#include "imm/immdev.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
static const wchar_t kTestString[] = L"TEST";
static const wchar_t kTestStringLength = arraysize(kTestString) - 1;

TEST(CandidateInfo, Test) {
  CandidateInfo candinfo;
  EXPECT_TRUE(candinfo.Initialize());
  EXPECT_EQ(sizeof(CandidateInfo), candinfo.info.dwSize);

  candinfo.set_count(2);
  EXPECT_EQ(2, candinfo.list.info.dwCount);

  candinfo.set_candidate(1, kTestString);
  uint8* list_start = reinterpret_cast<uint8*>(&candinfo.list);
  uint8* list1 = list_start + candinfo.list.info.dwOffset[1];
  EXPECT_STREQ(kTestString, reinterpret_cast<wchar_t*>(list1));

  candinfo.set_selection(1);
  EXPECT_EQ(1, candinfo.list.info.dwSelection);
}
}  // namespace imm
}  // namespace ime_goopy
