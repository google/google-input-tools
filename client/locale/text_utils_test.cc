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

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "locale/text_utils.h"
#include <gtest/gunit.h>

namespace ime_goopy {

class TextManipulatorEnTest : public ::testing::Test {
 protected:
  void SetUp() {
    text_manipulator_.reset(new TextManipulatorEn);
  }
  scoped_ptr<TextManipulator> text_manipulator_;
};

TEST_F(TextManipulatorEnTest, TestExpandToWordBegin) {
  // Test a single word
  EXPECT_EQ(3, text_manipulator_->ExpandToWordBegin(L"hello", 3));
  // At the left boundary
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(L"hello", 0));
  // At the right boundary
  EXPECT_EQ(5, text_manipulator_->ExpandToWordBegin(L"hello", 5));
  EXPECT_EQ(4, text_manipulator_->ExpandToWordBegin(L"hello", 4));
  // The first character is empty
  EXPECT_EQ(2, text_manipulator_->ExpandToWordBegin(L" hello", 3));
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(L" hello", 1));
  // Test a short phrase
  EXPECT_EQ(3, text_manipulator_->ExpandToWordBegin(L"hello world", 3));
  EXPECT_EQ(1, text_manipulator_->ExpandToWordBegin(L"hello world", 7));
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(L"hello world", 6));
  // Test a Chinese phrase
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(L"\u5927\u5bb6", 1));
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(L"Test \u5927\u5bb6", 6));
  // Test special case.
  EXPECT_EQ(2, text_manipulator_->ExpandToWordBegin(L"I'm", 2));
}

TEST_F(TextManipulatorEnTest, TestExpandToWordEnd) {
  // Test a single word
  EXPECT_EQ(2, text_manipulator_->ExpandToWordEnd(L"hello", 3));
  // At the left boundary
  EXPECT_EQ(5, text_manipulator_->ExpandToWordEnd(L"hello", 0));
  // At the right boundary
  EXPECT_EQ(0, text_manipulator_->ExpandToWordEnd(L"hello", 5));
  // The last character is empty
  EXPECT_EQ(2, text_manipulator_->ExpandToWordEnd(L"hello ", 3));
  EXPECT_EQ(0, text_manipulator_->ExpandToWordEnd(L"hello ", 5));
  // Test a short phrase
  EXPECT_EQ(2, text_manipulator_->ExpandToWordEnd(L"hello world", 3));
  EXPECT_EQ(4, text_manipulator_->ExpandToWordEnd(L"hello world", 7));
  EXPECT_EQ(0, text_manipulator_->ExpandToWordEnd(L"hello world", 5));
  // Test a Chinese phrase
  EXPECT_EQ(0, text_manipulator_->ExpandToWordEnd(L"\u5927\u5bb6", 1));
  EXPECT_EQ(1, text_manipulator_->ExpandToWordEnd(L"\u5927\u5bb6 Test", 6));
  // Test special case.
  EXPECT_EQ(3, text_manipulator_->ExpandToWordEnd(L"I'll", 1));
}

class TextManipulatorZhCnTest : public ::testing::Test {
 protected:
  void SetUp() {
    text_manipulator_.reset(new TextManipulatorZhCn);
  }
  scoped_ptr<TextManipulator> text_manipulator_;
};

TEST_F(TextManipulatorZhCnTest, TestExpandToWordBegin) {
  wstring text =
      //   0     1     2     3     4     5     6     7     8     9     10
      //  bu    hao   yi     si    ,     wo   jin   tian  du    zi    tong
      L"\u4E0D\u597D\u610F\u601D\uFF0C\u6211\u4ECA\u5929\u809A\u5B50\u75DB";

  // Left end.
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(text, 0));
  // Right end
  EXPECT_EQ(6, text_manipulator_->ExpandToWordBegin(text, 11));
  // Normal
  EXPECT_EQ(2, text_manipulator_->ExpandToWordBegin(text, 2));
  EXPECT_EQ(1, text_manipulator_->ExpandToWordBegin(text, 6));
  // Left of commas.
  EXPECT_EQ(4, text_manipulator_->ExpandToWordBegin(text, 4));
  // Right of commas.
  EXPECT_EQ(0, text_manipulator_->ExpandToWordBegin(text, 5));
}

TEST_F(TextManipulatorZhCnTest, TestExpandToWordEnd) {
  wstring text =
      //   0     1     2     3     4     5     6     7     8     9     10
      //  bu    hao   yi     si    ,     wo   jin   tian  du    zi    tong
      L"\u4E0D\u597D\u610F\u601D\uFF0C\u6211\u4ECA\u5929\u809A\u5B50\u75DB";

  // Left end.
  EXPECT_EQ(4, text_manipulator_->ExpandToWordEnd(text, 0));
  // Right end
  EXPECT_EQ(0, text_manipulator_->ExpandToWordEnd(text, 11));
  // Normal
  EXPECT_EQ(2, text_manipulator_->ExpandToWordEnd(text, 2));
  EXPECT_EQ(5, text_manipulator_->ExpandToWordEnd(text, 6));
  // Left of commas.
  EXPECT_EQ(0, text_manipulator_->ExpandToWordEnd(text, 4));
  // Right of commas.
  EXPECT_EQ(6, text_manipulator_->ExpandToWordEnd(text, 5));
}
}  // namespace ime_goopy

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
#ifdef DEBUG
  logging::InitLogging(NULL,
                       logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                       logging::DONT_LOCK_LOG_FILE,
                       logging::APPEND_TO_OLD_LOG_FILE);
  logging::SetLogItems(false, true, true, false);
  ime_shared::VLog::SetFromEnvironment(L"GOOPY_VMODULE",
                                       L"GOOPY_VLEVEL");
#endif
  return RUN_ALL_TESTS();
}
