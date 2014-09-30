/*
  Copyright 2011 Google Inc.

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

#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <cstdio>
#include <stdlib.h>
#include <string>
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "ggadget/host_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

static const HostArgumentInfo kArgumentsInfo[] = {
  { 1, Variant::TYPE_BOOL, "-a1", "--argument-1" },
  { 2, Variant::TYPE_INT64, "-a2", NULL },
  { 3, Variant::TYPE_DOUBLE, "-a3", "--argument-3" },
  { 4, Variant::TYPE_STRING, NULL, "--argument-4" },
  { -1, Variant::TYPE_VOID, NULL, NULL }
};

static const char *kGoodArgv1[] = {
  "-a1=false", "-a2=12345", "-a3", "3.14", "--argument-4", "hello"
};
static const int kGoodArgc1 = arraysize(kGoodArgv1);

static const char *kGoodArgv2[] = {
  "-a1", "hello", "-a2", "12345", "world", "-a3=3.14", "test"
};
static const int kGoodArgc2 = arraysize(kGoodArgv2);

static const char *kGoodArgv3[] = {
  "-a1", "hello", "-a2", "12345", "world", "--argument-4=3.14", "test"
};
static const int kGoodArgc3 = arraysize(kGoodArgv3);

static const char *kBadArgv1[] = {
  "-a1=abc", "-a2=test", "--argument-3=0x64"
};
static const int kBadArgc1 = arraysize(kBadArgv1);

static const char *kBadArgv2[] = {
  "-a2", "test", "--argument-3", "0x64"
};
static const int kBadArgc2 = arraysize(kBadArgv2);

static const char *kBadArgv3[] = {
  "-a1=true", "-a2"
};
static const int kBadArgc3 = arraysize(kBadArgv3);

TEST(HostUtils, HostArgumentParserGood1) {
  HostArgumentParser parser(kArgumentsInfo);
  ASSERT_TRUE(parser.Start());
  EXPECT_FALSE(parser.Start());
  ASSERT_TRUE(parser.AppendArguments(kGoodArgc1, kGoodArgv1));
  ASSERT_TRUE(parser.Finish());
  EXPECT_FALSE(parser.Finish());

  Variant value;
  EXPECT_TRUE(parser.GetArgumentValue(1, &value));
  EXPECT_EQ(Variant::TYPE_BOOL, value.type());
  EXPECT_EQ(false, VariantValue<bool>()(value));
  EXPECT_TRUE(parser.GetArgumentValue(2, &value));
  EXPECT_EQ(Variant::TYPE_INT64, value.type());
  EXPECT_EQ(12345, VariantValue<int64_t>()(value));
  EXPECT_TRUE(parser.GetArgumentValue(3, &value));
  EXPECT_EQ(Variant::TYPE_DOUBLE, value.type());
  EXPECT_DOUBLE_EQ(3.14, VariantValue<double>()(value));
  EXPECT_TRUE(parser.GetArgumentValue(4, &value));
  EXPECT_EQ(Variant::TYPE_STRING, value.type());
  EXPECT_STREQ("hello", VariantValue<const char *>()(value));

  ASSERT_TRUE(parser.Start());
}

bool RemainedArgsCallback(const std::string &arg, int *count) {
  static const char *kRemainedArgs[] = { "hello", "world", "test" };
  EXPECT_GT(3, *count);
  EXPECT_STREQ(kRemainedArgs[*count], arg.c_str());
  ++ *count;
  return true;
}

TEST(HostUtils, HostArgumentParserGood2) {
  HostArgumentParser parser(kArgumentsInfo);
  ASSERT_TRUE(parser.Start());
  EXPECT_FALSE(parser.Start());
  ASSERT_TRUE(parser.AppendArguments(kGoodArgc2, kGoodArgv2));
  ASSERT_TRUE(parser.Finish());
  EXPECT_FALSE(parser.Finish());

  Variant value;
  EXPECT_TRUE(parser.GetArgumentValue(1, &value));
  EXPECT_EQ(Variant::TYPE_BOOL, value.type());
  EXPECT_EQ(true, VariantValue<bool>()(value));
  EXPECT_TRUE(parser.GetArgumentValue(2, &value));
  EXPECT_EQ(Variant::TYPE_INT64, value.type());
  EXPECT_EQ(12345, VariantValue<int64_t>()(value));
  EXPECT_TRUE(parser.GetArgumentValue(3, &value));
  EXPECT_EQ(Variant::TYPE_DOUBLE, value.type());
  EXPECT_DOUBLE_EQ(3.14, VariantValue<double>()(value));
  EXPECT_FALSE(parser.GetArgumentValue(4, &value));

  int count = 0;
  ASSERT_TRUE(parser.EnumerateRemainedArgs(
      NewSlot(RemainedArgsCallback, &count)));
}

bool RecognizedArgsCallback(const std::string &arg, int *count) {
  static const char *kRecognizedArgs[] = {
    "--argument-1=true", "-a2=12345", "--argument-4=3.14" };
  EXPECT_GT(3, *count);
  EXPECT_STREQ(kRecognizedArgs[*count], arg.c_str());
  ++ *count;
  return true;
}

TEST(HostUtils, HostArgumentParserGood3) {
  HostArgumentParser parser(kArgumentsInfo);
  ASSERT_TRUE(parser.Start());
  EXPECT_FALSE(parser.Start());
  ASSERT_TRUE(parser.AppendArguments(kGoodArgc3, kGoodArgv3));
  ASSERT_TRUE(parser.Finish());
  EXPECT_FALSE(parser.Finish());

  int count = 0;
  ASSERT_TRUE(parser.EnumerateRecognizedArgs(
      NewSlot(RecognizedArgsCallback, &count)));
}

TEST(HostUtils, HostArgumentParserBad1) {
  HostArgumentParser parser(kArgumentsInfo);

  for (int i = 0; i < kBadArgc1; ++i) {
    ASSERT_TRUE(parser.Start());
    EXPECT_FALSE(parser.AppendArgument(kBadArgv1[i]));
    ASSERT_FALSE(parser.Finish());
  }
}

TEST(HostUtils, HostArgumentParserBad2) {
  HostArgumentParser parser(kArgumentsInfo);

  for (int i = 0; i < kBadArgc2; i += 2) {
    ASSERT_TRUE(parser.Start());
    EXPECT_TRUE(parser.AppendArgument(kBadArgv2[i]));
    EXPECT_FALSE(parser.AppendArgument(kBadArgv2[i+1]));
    ASSERT_FALSE(parser.Finish());
  }
}

TEST(HostUtils, HostArgumentParserBad3) {
  HostArgumentParser parser(kArgumentsInfo);
  ASSERT_TRUE(parser.Start());
  ASSERT_TRUE(parser.AppendArguments(kBadArgc3, kBadArgv3));
  ASSERT_FALSE(parser.Finish());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
