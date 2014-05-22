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

#include "imm/composition_string.h"
#include "imm/immdev.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
static const wchar_t kTestString[] = L"TEST";
static const wchar_t kTestStringLength = arraysize(kTestString) - 1;

TEST(CompositionString, Test) {
  CompositionString compstr;
  EXPECT_TRUE(compstr.Initialize());

  EXPECT_EQ(sizeof(CompositionString), compstr.info.dwSize);
  EXPECT_TRUE(compstr.info.dwCompStrOffset);
  EXPECT_TRUE(compstr.info.dwCompReadStrOffset);
  EXPECT_TRUE(compstr.info.dwResultStrOffset);

  uint8* string_start = reinterpret_cast<uint8*>(&compstr.info);
  uint8* composition = string_start + compstr.info.dwCompStrOffset;
  uint8* result = string_start + compstr.info.dwResultStrOffset;

  compstr.set_composition(kTestString);
  EXPECT_STREQ(kTestString, reinterpret_cast<wchar_t*>(composition));
  EXPECT_EQ(kTestStringLength, compstr.info.dwCompStrLen);
  EXPECT_EQ(kTestStringLength, compstr.info.dwCompReadStrLen);

  compstr.set_caret(kTestStringLength);
  EXPECT_EQ(kTestStringLength, compstr.info.dwCursorPos);

  compstr.set_result(kTestString);
  EXPECT_STREQ(kTestString, reinterpret_cast<wchar_t*>(result));
  EXPECT_EQ(kTestStringLength, compstr.info.dwResultStrLen);
}
}  // namespace imm
}  // namespace ime_goopy
