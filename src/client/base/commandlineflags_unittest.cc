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

#include "testing/base/public/gunit.h"

DEFINE_string(param_str, "string", "param string");
DEFINE_int32(param_int32, 44444, "param int64");
DEFINE_int64(param_int64, GG_LONGLONG(0x6666666666), "param int64");
DEFINE_bool(param_bool, false, "param bool");
DEFINE_double(param_double, 1.333, "param a4");

TEST(TestCommandlineFlags, TestString) {
  EXPECT_EQ("string", FLAGS_param_str);
}

TEST(TestCommandlineFlags, TestInt32) {
  EXPECT_EQ(44444, FLAGS_param_int32);
}

TEST(TestCommandlineFlags, TestInt64) {
  EXPECT_EQ(GG_LONGLONG(0x6666666666), FLAGS_param_int64);
}

TEST(TestCommandlineFlags, TestBool) {
  EXPECT_EQ(false, FLAGS_param_bool);
}

TEST(TestCommandlineFlags, TestDouble) {
  EXPECT_DOUBLE_EQ(1.333, FLAGS_param_double);
}
