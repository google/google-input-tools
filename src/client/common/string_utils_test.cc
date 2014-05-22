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
﻿

#include "common/string_utils.h"
#include <gtest/gunit.h>

namespace ime_goopy {
TEST(StringUtilsTest, TestSplitWstring) {
  vector<wstring> result;
  SplitString(L"hello world", L" ", &result);
  EXPECT_EQ(result.size(), 2);
  EXPECT_TRUE(result[0] == L"hello");
  EXPECT_TRUE(result[1] == L"world");
}

TEST(StringUtilsTest, TestSplitSingleChar) {
  vector<wstring> result;
  SplitString(L"a", L" ", &result);
  EXPECT_EQ(result.size(), 1);
  EXPECT_TRUE(result[0] == L"a");
}

TEST(StringUtilsTest, SplitStringW) {
  vector<wstring> results;
  SplitString(L"", L",", &results);
  EXPECT_EQ(results.size(), 0);
  SplitString(L"123", L",", &results);
  EXPECT_EQ(1, results.size());
  EXPECT_STREQ(L"123", results[0].c_str());
  SplitString(L"1,2,3,12234234,234", L",", &results);
  EXPECT_EQ(5, results.size());
  EXPECT_STREQ(L"234", results[4].c_str());
  SplitString(L"1,2,3,12234234,234,", L",", &results);
  EXPECT_EQ(5, results.size());
  EXPECT_STREQ(L"234", results[4].c_str());
  SplitString(L"1::2::3::12234234::234", L"::", &results);
  EXPECT_EQ(5, results.size());
  EXPECT_STREQ(L"234", results[4].c_str());
}

TEST(StringUtilsTest, SplitString) {
  vector<string> results;
  SplitString("", ",", &results);
  EXPECT_EQ(results.size(), 0);
  SplitString("123", ",", &results);
  EXPECT_EQ(1, results.size());
  EXPECT_STREQ("123", results[0].c_str());
  SplitString("1,2,3,,12234234,234", ",", &results);
  EXPECT_EQ(5, results.size());
  EXPECT_STREQ("234", results[4].c_str());
  SplitString("1,2,3,12234234,234,", ",", &results);
  EXPECT_EQ(5, results.size());
  EXPECT_STREQ("234", results[4].c_str());
  SplitString("1::2::3::12234234::234", "::", &results);
  EXPECT_EQ(5, results.size());
  EXPECT_STREQ("234", results[4].c_str());
}

TEST(StringUtilsTest, StringEndsWith) {
  EXPECT_FALSE(StringEndsWith(NULL, L""));
  EXPECT_TRUE(StringEndsWith(L"", NULL));
  EXPECT_TRUE(StringEndsWith(L"", L""));
  EXPECT_TRUE(StringEndsWith(L"1", L"1"));
  EXPECT_TRUE(StringEndsWith(L"123", L"123"));
  EXPECT_TRUE(StringEndsWith(L"12345", L"45"));
  EXPECT_TRUE(StringEndsWith(L"12345", L"5"));
  EXPECT_TRUE(StringEndsWith(L"12345", L""));
  EXPECT_FALSE(StringEndsWith(L"12345", L"123"));
  EXPECT_FALSE(StringEndsWith(NULL, ""));
  EXPECT_TRUE(StringEndsWith("", NULL));
  EXPECT_TRUE(StringEndsWith("", ""));
  EXPECT_TRUE(StringEndsWith("1", "1"));
  EXPECT_TRUE(StringEndsWith("123", "123"));
  EXPECT_TRUE(StringEndsWith("12345", "45"));
  EXPECT_TRUE(StringEndsWith("12345", "5"));
  EXPECT_TRUE(StringEndsWith("12345", ""));
  EXPECT_FALSE(StringEndsWith("12345", "123"));
}

TEST(StringUtilsTest, StringBeginsWith) {
  EXPECT_FALSE(StringBeginsWith(NULL, L""));
  EXPECT_TRUE(StringBeginsWith(L"", NULL));
  EXPECT_TRUE(StringBeginsWith(L"", L""));
  EXPECT_TRUE(StringBeginsWith(L"1", L"1"));
  EXPECT_TRUE(StringBeginsWith(L"123", L"123"));
  EXPECT_TRUE(StringBeginsWith(L"12345", L"123"));
  EXPECT_TRUE(StringBeginsWith(L"12345", L"1"));
  EXPECT_TRUE(StringBeginsWith(L"12345", L""));
  EXPECT_FALSE(StringBeginsWith(L"12345", L"45"));
  EXPECT_FALSE(StringBeginsWith(NULL, ""));
  EXPECT_TRUE(StringBeginsWith("", NULL));
  EXPECT_TRUE(StringBeginsWith("", ""));
  EXPECT_TRUE(StringBeginsWith("1", "1"));
  EXPECT_TRUE(StringBeginsWith("123", "123"));
  EXPECT_TRUE(StringBeginsWith("12345", "123"));
  EXPECT_TRUE(StringBeginsWith("12345", "1"));
  EXPECT_TRUE(StringBeginsWith("12345", ""));
  EXPECT_FALSE(StringBeginsWith("12345", "45"));
}

TEST(StringUtilsTest, CombinePathUtf8) {
  EXPECT_EQ("test.txt", CombinePathUtf8("", "test.txt"));
  EXPECT_EQ("C:\\Users\\test.txt", CombinePathUtf8("C:\\Users\\", "test.txt"));
  EXPECT_EQ("C:\\Users\\test.txt", CombinePathUtf8("C:\\Users", "test.txt"));
  EXPECT_EQ("C:\\Users\\test", CombinePathUtf8("C:\\Users", "test"));
}

TEST(StringUtilsTest, ToWindowsCRLF) {
  EXPECT_EQ(L"\r\n", ToWindowsCRLF(L"\n"));
  EXPECT_EQ(L"\r\n", ToWindowsCRLF(L"\r\n"));
  EXPECT_EQ(L"\r\n\r\n", ToWindowsCRLF(L"\n\n"));
  EXPECT_EQ(L"\r\n\r\n", ToWindowsCRLF(L"\r\n\n"));
  EXPECT_EQ(L"\r\n\r\n", ToWindowsCRLF(L"\n\r\n"));
  EXPECT_EQ(L"\r\n\r\n", ToWindowsCRLF(L"\r\n\r\n"));
  EXPECT_EQ(L"multi\r\nline\r\n", ToWindowsCRLF(L"multi\nline\n"));
  EXPECT_EQ(L"multi\r\nline", ToWindowsCRLF(L"multi\nline"));
  EXPECT_EQ(L"\r", ToWindowsCRLF(L"\r"));
}

TEST(StringUtilsTest, JoinStringsIterator) {
  const char* array[] = {"a", "b", "c"};
  const vector<string> vec(array, array + arraysize(array));
  string result;
  JoinStringsIterator(vec.begin(), vec.end(), string(", "), &result);
  EXPECT_EQ("a, b, c", result);
}

TEST(StringUtilsTest, WideToCodePage) {
  EXPECT_STREQ("", WideToCodePage(L"", CP_ACP).c_str());
  EXPECT_STREQ("abc", WideToCodePage(L"abc", CP_ACP).c_str());
  EXPECT_STREQ("\xb9\xc8\xb8\xe8",
               WideToCodePage(L"\u8c37\u6b4c",
                              936 /*simplified Chinese*/).c_str());
  EXPECT_STREQ("\xe8\xb0\xb7\xe6\xad\x8c",
               WideToCodePage(L"\u8c37\u6b4c", 65001 /*utf-8*/).c_str());
}
}  // namespace ime_goopy
