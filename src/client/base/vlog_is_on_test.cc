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

#include "base/vlog_is_on.h"

#include "base/commandlineflags.h"
#include "i18n/input/engine/lib/public/win/string_utils.h"
#include "testing/base/public/gunit.h"

DECLARE_int32(v);
DECLARE_string(vmodule);

namespace ime_shared {
static const wchar_t kTestFile1[] = L"TestFile1";
static const wchar_t kTestFile2[] = L"TestFile2";
static const wchar_t kTestFile3[] = L"TestFile3";
static const wchar_t kAllModule[] = L"TestFile1,TestFile2";
static const wchar_t kPartModule[] = L"Test";
static const wchar_t kComplexModule[] = L"TestFile1=2,TestFile2";
static const wchar_t kEnvModule[] = L"IME_TEST_VMODULE";
static const wchar_t kEnvLevel[] = L"IME_TEST_VLEVEL";

TEST(VLog, SetModule) {
  VLog::SetLevel(0);

  // All levels are off.
  VLog::SetModule(L"");
  EXPECT_EQ(0, VLog::GetVerboseLevel(kTestFile1));
  EXPECT_FALSE(VLog::IsOn(kTestFile1, 1));
  EXPECT_EQ(0, VLog::GetVerboseLevel(kTestFile2));
  EXPECT_FALSE(VLog::IsOn(kTestFile2, 1));

  // 1 module.
  VLog::SetModule(kTestFile1);
  EXPECT_TRUE(VLog::IsOn(kTestFile1, 1));
  EXPECT_FALSE(VLog::IsOn(kTestFile2, 1));

  // 2 modules.
  VLog::SetModule(kAllModule);
  EXPECT_TRUE(VLog::IsOn(kTestFile1, 1));
  EXPECT_TRUE(VLog::IsOn(kTestFile2, 1));

  // Partial match.
  VLog::SetModule(kPartModule);
  EXPECT_TRUE(VLog::IsOn(kTestFile1, 1));
  EXPECT_TRUE(VLog::IsOn(kTestFile2, 1));

  // Complex module.
  VLog::SetModule(kComplexModule);
  EXPECT_EQ(2, VLog::GetVerboseLevel(kTestFile1));
  EXPECT_EQ(1, VLog::GetVerboseLevel(kTestFile2));
  EXPECT_EQ(0, VLog::GetVerboseLevel(kTestFile3));
}

TEST(VLog, SetLevel) {
  VLog::SetModule(L"");

  // All levels are off.
  VLog::SetLevel(0);
  EXPECT_FALSE(VLog::IsOn(kTestFile1, 1));
  EXPECT_FALSE(VLog::IsOn(kTestFile1, 2));

  // Level 2.
  VLog::SetLevel(2);
  EXPECT_TRUE(VLog::IsOn(kTestFile1, 1));
  EXPECT_TRUE(VLog::IsOn(kTestFile1, 2));
  EXPECT_FALSE(VLog::IsOn(kTestFile1, 3));
}

TEST(VLog, SetFromEnvironment) {
  VLog::SetModule(kTestFile1);
  VLog::SetLevel(2);

  // Environment variable not existed, level not changed.
  SetEnvironmentVariable(kEnvModule, NULL);
  SetEnvironmentVariable(kEnvLevel, NULL);
  VLog::SetFromEnvironment(kEnvModule, kEnvLevel);
  EXPECT_STREQ(kTestFile1,
               i18n_input::engine::Utf8ToWide(FLAGS_vmodule).c_str());
  EXPECT_EQ(2, FLAGS_v);

  // Changed.
  SetEnvironmentVariable(kEnvModule, kTestFile2);
  SetEnvironmentVariable(kEnvLevel, L"1");
  VLog::SetFromEnvironment(kEnvModule, kEnvLevel);
  EXPECT_STREQ(kTestFile2,
               i18n_input::engine::Utf8ToWide(FLAGS_vmodule).c_str());
  EXPECT_EQ(1, FLAGS_v);

  // Clean up.
  SetEnvironmentVariable(kEnvModule, NULL);
  SetEnvironmentVariable(kEnvLevel, NULL);
}
}  // namespace ime_shared

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
