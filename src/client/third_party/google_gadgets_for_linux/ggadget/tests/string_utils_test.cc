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

#include <cstdio>
#include "ggadget/string_utils.h"
#include "ggadget/locales.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(StringUtils, AssignIfDiffer) {
  std::string s;
  ASSERT_FALSE(AssignIfDiffer(NULL, &s));
  ASSERT_STREQ("", s.c_str());
  ASSERT_FALSE(AssignIfDiffer("", &s));
  ASSERT_STREQ("", s.c_str());
  ASSERT_TRUE(AssignIfDiffer("abcd", &s));
  ASSERT_STREQ("abcd", s.c_str());
  ASSERT_FALSE(AssignIfDiffer("abcd", &s));
  ASSERT_STREQ("abcd", s.c_str());
  ASSERT_TRUE(AssignIfDiffer("1234", &s));
  ASSERT_STREQ("1234", s.c_str());
  ASSERT_TRUE(AssignIfDiffer("", &s));
  ASSERT_STREQ("", s.c_str());
  s = "qwer";
  ASSERT_TRUE(AssignIfDiffer(NULL, &s));
  ASSERT_STREQ("", s.c_str());
}

TEST(StringUtils, TrimString) {
  EXPECT_STREQ("", TrimString("").c_str());
  EXPECT_STREQ("", TrimString("  \n \r \t ").c_str());
  EXPECT_STREQ("a b\r c", TrimString(" a b\r c \r\t ").c_str());
  EXPECT_STREQ("a b c", TrimString("a b c  ").c_str());
  EXPECT_STREQ("a b c", TrimString("  a b c").c_str());
  EXPECT_STREQ("a b c", TrimString("a b c").c_str());
  EXPECT_STREQ("abc", TrimString("abc").c_str());
}

TEST(StringUtils, ToUpper) {
  EXPECT_STREQ("", ToUpper("").c_str());
  EXPECT_STREQ("ABCABC123", ToUpper("abcABC123").c_str());
}

TEST(StringUtils, ToLower) {
  EXPECT_STREQ("", ToLower("").c_str());
  EXPECT_STREQ("abcabc123", ToLower("abcABC123").c_str());
}

TEST(StringUtils, StringPrintf) {
  const char *locale_message_lists[] = {
    "ar-SA", "en_US", "zh_CN.UTF8", "", NULL};

  for (int list_no = 0; locale_message_lists[list_no]; ++list_no) {
    SetLocaleForUiMessage(locale_message_lists[list_no]);

    EXPECT_STREQ("123", ggadget::StringPrintf("%d", 123).c_str());
    char *buf = new char[100000];
    for (int i = 0; i < 100000; i++)
      buf[i] = static_cast<char>((i % 50) + '0');
    buf[99999] = 0;
    EXPECT_STREQ(buf, ggadget::StringPrintf("%s", buf).c_str());
    EXPECT_STREQ("123 1.23 aBc 03A8",
        ggadget::StringPrintf("%d %.2lf %s %04X",
            123, 1.225, "aBc", 936).c_str());
    delete buf;
  }
}

TEST(StringUtils, EncodeDecodeURL) {
  // Valid url chars, no conversion
  char src1[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=;',./~!@#$&*()_+:?";
  char src1_comp[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-'.~!*()_";

  // Invalid url chars, will be converted
  char src2[] = "|^` []{}<>\"%";
  char src2_comp[] = "|^` []{}<>\"%\\#;/?:@#&=+$,";

  // Back slash, will be converted to '/'
  char src3[] = "\\";

  // Valid but invisible chars, will be converted
  char src4[] = "\a\b\f\n\r\t\v\007\013\xb\x7";

  // Non-ascii chars(except \x7f), will be converted
  char src5[] = "\x7f\x80\x81 asd\x8f 3\x9a\xaa\xfe\xff";

  // Invalid encoded URL.
  char src6[] = "%25%3X%5babc%5";

  std::string dest, dest_comp;

  dest = EncodeURL(src1);
  EXPECT_STREQ(src1, dest.c_str());
  dest = DecodeURL(dest);
  EXPECT_STREQ(src1, dest.c_str());

  dest_comp = EncodeURLComponent(src1_comp);
  EXPECT_STREQ(src1_comp, dest_comp.c_str());
  dest = DecodeURL(dest_comp);
  EXPECT_STREQ(src1_comp, dest.c_str());

  dest = EncodeURL(src2);
  EXPECT_STREQ("%7c%5e%60%20%5b%5d%7b%7d%3c%3e%22%25", dest.c_str());
  dest = DecodeURL(dest);
  EXPECT_STREQ(src2, dest.c_str());

  dest_comp = EncodeURLComponent(src2_comp);
  EXPECT_STREQ("%7c%5e%60%20%5b%5d%7b%7d%3c%3e%22%25"
               "%5c%23%3b%2f%3f%3a%40%23%26%3d%2b%24%2c", dest_comp.c_str());
  dest_comp = DecodeURL(dest_comp);
  EXPECT_STREQ(src2_comp, dest_comp.c_str());

  dest = EncodeURL(src3);
  EXPECT_STREQ("/", dest.c_str());
  dest = DecodeURL(dest);
  EXPECT_STREQ("/", dest.c_str());

  dest_comp = EncodeURLComponent(src3);
  EXPECT_STREQ("%5c", dest_comp.c_str());
  dest_comp = DecodeURL(dest_comp);
  EXPECT_STREQ("\\", dest_comp.c_str());

  dest = EncodeURL(src4);
  EXPECT_STREQ("%07%08%0c%0a%0d%09%0b%07%0b%0b%07", dest.c_str());
  dest = DecodeURL(dest);
  EXPECT_STREQ(src4, dest.c_str());

  dest_comp = EncodeURLComponent(src4);
  EXPECT_STREQ("%07%08%0c%0a%0d%09%0b%07%0b%0b%07", dest_comp.c_str());

  dest = EncodeURL(src5);
  EXPECT_STREQ("\x7f%80%81%20asd%8f%203%9a%aa%fe%ff", dest.c_str());
  dest = DecodeURL(dest);
  EXPECT_STREQ(src5, dest.c_str());

  dest_comp = EncodeURLComponent(src5);
  EXPECT_STREQ("\x7f%80%81%20asd%8f%203%9a%aa%fe%ff", dest_comp.c_str());

  dest = DecodeURL(src6);
  EXPECT_STREQ("%%3X[abc%5", dest.c_str());
}

TEST(StringUtils, GetHostFromURL) {
  EXPECT_STREQ("", GetHostFromURL(NULL).c_str());
  EXPECT_STREQ("", GetHostFromURL("").c_str());
  EXPECT_STREQ("", GetHostFromURL("mailto:a@b.com").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com/").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com/path/path?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com:1234").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com:1234/").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com:1234?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://a.com:1234/path/path?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://user:pa?ss@a.com").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://user:@a.com/").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://user:pa?ss@a.com?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://@a.com/path/path?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://user:pa?ss@a.com:1234").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://user:pa?ss@a.com?param=value").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://user:@a.com:1234/").c_str());
  EXPECT_STREQ("a.com", GetHostFromURL("http://@a.com:1234/path/path?param=value").c_str());
}

TEST(StringUtils, GetPathFromFileURL) {
  EXPECT_STREQ("", GetPathFromFileURL(NULL).c_str());
  EXPECT_STREQ("", GetPathFromFileURL("").c_str());
  EXPECT_STREQ("", GetPathFromFileURL("http://abc").c_str());
  EXPECT_STREQ("", GetPathFromFileURL("mailto:a@b.com").c_str());
  EXPECT_STREQ("", GetPathFromFileURL("file://").c_str());
  EXPECT_STREQ("/", GetPathFromFileURL("file:///").c_str());
  EXPECT_STREQ("/abc", GetPathFromFileURL("file:///abc").c_str());
  EXPECT_STREQ("/abc/dev", GetPathFromFileURL("file:///abc/dev").c_str());
  EXPECT_STREQ("/dev", GetPathFromFileURL("file://abc/dev").c_str());
  EXPECT_STREQ("/dev fff", GetPathFromFileURL("file://abc/dev%20fff").c_str());
}

TEST(StringUtils, GetUsernamePasswordFromURL) {
  EXPECT_STREQ("", GetUsernamePasswordFromURL(NULL).c_str());
  EXPECT_STREQ("", GetUsernamePasswordFromURL("").c_str());
  EXPECT_STREQ("", GetUsernamePasswordFromURL("mailto:a@b.com").c_str());
  EXPECT_STREQ("", GetUsernamePasswordFromURL("http://a.com").c_str());
  EXPECT_STREQ("", GetUsernamePasswordFromURL("http://a.com/").c_str());
  EXPECT_STREQ("", GetUsernamePasswordFromURL("http://a.com:1234/path/path?param=value").c_str());
  EXPECT_STREQ("user:pa?ss", GetUsernamePasswordFromURL("http://user:pa?ss@a.com").c_str());
  EXPECT_STREQ("user:", GetUsernamePasswordFromURL("http://user:@a.com/").c_str());
  EXPECT_STREQ("user:pa?ss", GetUsernamePasswordFromURL("http://user:pa?ss@a.com?param=value").c_str());
  EXPECT_STREQ("", GetUsernamePasswordFromURL("http://@a.com/path/path?param=value").c_str());
  EXPECT_STREQ("user:pa?ss", GetUsernamePasswordFromURL("http://user:pa?ss@a.com:1234").c_str());
  EXPECT_STREQ("user:pa?ss", GetUsernamePasswordFromURL("http://user:pa?ss@a.com?param=value").c_str());
  EXPECT_STREQ("user:", GetUsernamePasswordFromURL("http://user:@a.com:1234/").c_str());
}

void TestGetAbsoluteURL(const char *base_extra, const char *url_path) {
  EXPECT_EQ(std::string("http://abc/") + url_path,
      GetAbsoluteURL((std::string("http://abc") + base_extra).c_str(),
                     url_path));
  EXPECT_EQ(std::string("http://abc/") + url_path,
      GetAbsoluteURL((std::string("http://abc/") + base_extra).c_str(),
                     url_path));
  EXPECT_EQ(std::string("http://abc/def/") + url_path,
      GetAbsoluteURL((std::string("http://abc/def/ghi") + base_extra).c_str(),
                     url_path));
  EXPECT_EQ(std::string("http://abc/def/ghi/") + url_path,
      GetAbsoluteURL((std::string("http://abc/def/ghi/") + base_extra).c_str(),
                     url_path));

  std::string p1 = std::string("http://abc") + base_extra;
  std::string p2 = std::string("/") + url_path;
  std::string r = GetAbsoluteURL(p1.c_str(), p2.c_str());
  EXPECT_EQ(std::string("http://abc/") + url_path, r);
  ///EXPECT_EQ(std::string("http://abc/") + url_path,
  //    GetAbsoluteURL((std::string("http://abc") + base_extra).c_str(),
  //                   (std::string("/") + url_path).c_str()));
  EXPECT_EQ(std::string("http://abc/") + url_path,
      GetAbsoluteURL((std::string("http://abc/") + base_extra).c_str(),
                     (std::string("/") + url_path).c_str()));
  EXPECT_EQ(std::string("http://abc/") + url_path,
      GetAbsoluteURL((std::string("http://abc/def/ghi") + base_extra).c_str(),
                     (std::string("/") + url_path).c_str()));
  EXPECT_EQ(std::string("http://abc/") + url_path,
      GetAbsoluteURL((std::string("http://abc/def/ghi/") + base_extra).c_str(),
                     (std::string("/") + url_path).c_str()));

  EXPECT_EQ(std::string("http://") + url_path,
      GetAbsoluteURL((std::string("http://abc") + base_extra).c_str(),
                     (std::string("//") + url_path).c_str()));
  EXPECT_EQ(std::string("http://") + url_path,
      GetAbsoluteURL((std::string("http://abc/") + base_extra).c_str(),
                     (std::string("//") + url_path).c_str()));
  EXPECT_EQ(std::string("http://") + url_path,
      GetAbsoluteURL((std::string("http://abc/def/ghi") + base_extra).c_str(),
                     (std::string("//") + url_path).c_str()));
  EXPECT_EQ(std::string("http://") + url_path,
      GetAbsoluteURL((std::string("http://abc/def/ghi/") + base_extra).c_str(),
                     (std::string("//") + url_path).c_str()));
}

TEST(StringUtils, GetAbsoluteURL) {
  EXPECT_STREQ("", GetAbsoluteURL(NULL, NULL).c_str());
  EXPECT_STREQ("", GetAbsoluteURL(NULL, "").c_str());
  EXPECT_STREQ("", GetAbsoluteURL(NULL, "abc").c_str());
  EXPECT_STREQ("", GetAbsoluteURL(NULL, "/abc").c_str());
  EXPECT_STREQ("", GetAbsoluteURL(NULL, "//abc").c_str());
  EXPECT_STREQ("", GetAbsoluteURL("abc", NULL).c_str());
  EXPECT_STREQ("", GetAbsoluteURL("abc", "").c_str());
  EXPECT_STREQ("", GetAbsoluteURL("abc", "abc").c_str());
  EXPECT_STREQ("", GetAbsoluteURL("abc", "/abc").c_str());
  EXPECT_STREQ("", GetAbsoluteURL("abc", "//abc").c_str());
  EXPECT_STREQ("http://abc", GetAbsoluteURL(NULL, "http://abc").c_str());
  EXPECT_STREQ("http://abc", GetAbsoluteURL("", "http://abc").c_str());
  EXPECT_STREQ("http://abc", GetAbsoluteURL("abc", "http://abc").c_str());
  EXPECT_STREQ("http://abc", GetAbsoluteURL("http://abc", "").c_str());
  EXPECT_STREQ("http://abc", GetAbsoluteURL("http://abc", NULL).c_str());
  EXPECT_STREQ("http://abc/", GetAbsoluteURL("http://abc/", "").c_str());
  EXPECT_STREQ("http://abc/", GetAbsoluteURL("http://abc/", NULL).c_str());

  TestGetAbsoluteURL("", "xyz");
  TestGetAbsoluteURL("", "xyz/");
  TestGetAbsoluteURL("", "x/y/z");
  TestGetAbsoluteURL("", "x/y/z?b=/f");
  TestGetAbsoluteURL("", "x/y/z#bf");
  TestGetAbsoluteURL("#def", "xyz");
  TestGetAbsoluteURL("#def", "xyz/");
  TestGetAbsoluteURL("#def", "x/y/z");
  TestGetAbsoluteURL("#def", "x/y/z?b=/f");
  TestGetAbsoluteURL("#def", "x/y/z#bf");
  TestGetAbsoluteURL("?a=/h", "xyz");
  TestGetAbsoluteURL("?a=/h", "xyz/");
  TestGetAbsoluteURL("?a=/h", "x/y/z");
  TestGetAbsoluteURL("?a=/h", "x/y/z?b=/f");
  TestGetAbsoluteURL("?a=/h", "x/y/z#bf");

  EXPECT_STREQ("", GetAbsoluteURL("http://abc/", "../xyz").c_str());
  EXPECT_STREQ("http://abc/xyz",
      GetAbsoluteURL("http://abc/def/", "../xyz").c_str());
  EXPECT_STREQ("http://abc/xyz",
      GetAbsoluteURL("http://abc/def/ghi", "../xyz").c_str());
  EXPECT_STREQ("http://abc/xyz?b=/f",
      GetAbsoluteURL("http://abc/def/ghi?c=/e", "../xyz?b=/f").c_str());
  EXPECT_STREQ("http://abc/def/xyz/",
      GetAbsoluteURL("http://abc/def/ghi?c=/e", "./xyz/.").c_str());
  EXPECT_STREQ("http://abc/xyz/?b=/f",
      GetAbsoluteURL("http://abc/def/ghi?c=/e", "../xyz/.?b=/f").c_str());
}

TEST(StringUtils, EncodeJavaScriptString) {
  UTF16Char src[] = { '\"', '\'', '\\', 'a', 'b', 1, 0x1f, 0xfff, 0 };
  std::string dest = EncodeJavaScriptString(src, '"');
  EXPECT_STREQ("\"\\\"'\\\\ab\\u0001\\u001F\\u0FFF\"", dest.c_str());
  dest = EncodeJavaScriptString(src, '\'');
  EXPECT_STREQ("'\"\\\'\\\\ab\\u0001\\u001F\\u0FFF'", dest.c_str());
}

TEST(StringUtils, DecodeJavaScriptString) {
  UTF16Char expected[] = { '\"', '\'', '\\', 'a', 'b', '(', 1, 0x1f, 0xfff, 0 };
  const char *src1 = "\"\\\"'\\\\ab\\(\\u0001\\u001F\\u0FFF\"";
  const char *src2 = "'\"\\\'\\\\ab\\(\\u0001\\u001F\\u0FFF'";
  UTF16String result;
  EXPECT_TRUE(DecodeJavaScriptString(src1, &result));
  EXPECT_TRUE(result == expected);
  result.clear();
  EXPECT_TRUE(DecodeJavaScriptString(src2, &result));
  EXPECT_TRUE(result == expected);
  EXPECT_FALSE(DecodeJavaScriptString("'xyz", &result));
  EXPECT_FALSE(DecodeJavaScriptString("'x\\'", &result));
  EXPECT_FALSE(DecodeJavaScriptString("'xyz\"", &result));
  EXPECT_FALSE(DecodeJavaScriptString("'\\u'", &result));
  EXPECT_FALSE(DecodeJavaScriptString("'\\u123'", &result));
}

TEST(StringUtils, SplitString) {
  std::string left, right;
  EXPECT_FALSE(SplitString("", "", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("", right.c_str());
  EXPECT_FALSE(SplitString("abcde", "", &left, &right));
  EXPECT_STREQ("abcde", left.c_str());
  EXPECT_STREQ("", right.c_str());
  EXPECT_TRUE(SplitString("abcde", "c", &left, &right));
  EXPECT_STREQ("ab", left.c_str());
  EXPECT_STREQ("de", right.c_str());
  EXPECT_TRUE(SplitString("abcde", "abcde", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("", right.c_str());
  EXPECT_TRUE(SplitString("abcdeabcde", "a", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("bcdeabcde", right.c_str());
  EXPECT_TRUE(SplitString("abcdeabcde", "d", &left, &right));
  EXPECT_STREQ("abc", left.c_str());
  EXPECT_STREQ("eabcde", right.c_str());
  EXPECT_FALSE(SplitString("abcde", "cb", &left, &right));
  EXPECT_STREQ("abcde", left.c_str());
  EXPECT_STREQ("", right.c_str());
}

TEST(StringUtils, SplitStringList) {
  std::vector<std::string> result;
  EXPECT_FALSE(SplitStringList("", "", &result));
  EXPECT_EQ(static_cast<size_t>(0), result.size());
  EXPECT_FALSE(SplitStringList("abc", "", &result));
  EXPECT_EQ(static_cast<size_t>(1), result.size());
  EXPECT_STREQ("abc", result[0].c_str());
  EXPECT_TRUE(SplitStringList(":ab::cd:ef:", ":", &result));
  EXPECT_EQ(static_cast<size_t>(3), result.size());
  EXPECT_STREQ("ab", result[0].c_str());
  EXPECT_STREQ("cd", result[1].c_str());
  EXPECT_STREQ("ef", result[2].c_str());
  EXPECT_TRUE(SplitStringList("::ab::cde::f::ghi", "::", &result));
  EXPECT_EQ(static_cast<size_t>(4), result.size());
  EXPECT_STREQ("ab", result[0].c_str());
  EXPECT_STREQ("cde", result[1].c_str());
  EXPECT_STREQ("f", result[2].c_str());
  EXPECT_STREQ("ghi", result[3].c_str());
  EXPECT_FALSE(SplitStringList("abcdef", "ce", &result));
  EXPECT_EQ(static_cast<size_t>(1), result.size());
  EXPECT_STREQ("abcdef", result[0].c_str());
}

TEST(StringUtils, CompressWhiteSpaces) {
  EXPECT_STREQ("", CompressWhiteSpaces("").c_str());
  EXPECT_STREQ("", CompressWhiteSpaces(" \n\r\t  ").c_str());
  EXPECT_STREQ("A", CompressWhiteSpaces("A").c_str());
  EXPECT_STREQ("A", CompressWhiteSpaces(" A ").c_str());
  EXPECT_STREQ("A", CompressWhiteSpaces("   A   ").c_str());
  EXPECT_STREQ("AB", CompressWhiteSpaces("AB").c_str());
  EXPECT_STREQ("AB", CompressWhiteSpaces(" AB ").c_str());
  EXPECT_STREQ("AB", CompressWhiteSpaces("  AB  ").c_str());
  EXPECT_STREQ("A AB ABC", CompressWhiteSpaces("  A     AB     ABC ").c_str());
}

TEST(StringUtils, ExtractTextFromHTML) {
  EXPECT_STREQ("", ExtractTextFromHTML("").c_str());
  EXPECT_STREQ("< > &' \" \xc2\xa9 \xc2\xae<< &unknown;0\xf4\x81\x84\x91\xe2\x80\x89 Text",
    ExtractTextFromHTML(
      " <script language=\"javascript\"> some script and may be <tags>\n"
      " </script>\n"
      " <!-- some comments <tags> <script> -->\n"
      " <style>style</style>\n"
      " <input type='button' value='<tag>'>\n"
      " &lt; &gt &amp&apos; &nbsp; &nbsp; &quot;<b>&copy;</b>&reg;&lt&lt\n"
      " &#32;&#x&#&unknown;&#x30;&#x101111;&#x2009;\n\r\t Text ").c_str());
}

TEST(StringUtils, CleanupLineBreaks) {
  EXPECT_STREQ("", CleanupLineBreaks("").c_str());
  EXPECT_STREQ(" ", CleanupLineBreaks("\r\n").c_str());
  EXPECT_STREQ(" ", CleanupLineBreaks("\n").c_str());
  EXPECT_STREQ(" ", CleanupLineBreaks("\r").c_str());
  EXPECT_STREQ("    ", CleanupLineBreaks("\r\n\n\r\r\n").c_str());
  EXPECT_STREQ("one    two three four",
               CleanupLineBreaks("one \r\n  two\rthree\nfour").c_str());
}

TEST(StringUtils, ContainsHTML) {
  EXPECT_FALSE(ContainsHTML(""));
  EXPECT_FALSE(ContainsHTML(NULL));
  EXPECT_FALSE(ContainsHTML("abcde"));
  EXPECT_FALSE(ContainsHTML("<abcde>"));
  EXPECT_TRUE(ContainsHTML("1234<!-- comments -->6789"));
  EXPECT_TRUE(ContainsHTML("1234<a href=abcde>abcde</a>defg"));
  EXPECT_TRUE(ContainsHTML("1234<br>5678"));
  EXPECT_TRUE(ContainsHTML("<b>6789</b>"));
  EXPECT_TRUE(ContainsHTML("6789&quot;1234"));
}

TEST(StringUtils, SimpleMatchXPath) {
  EXPECT_TRUE(SimpleMatchXPath("", ""));
  EXPECT_TRUE(SimpleMatchXPath("a[1]", "a"));
  // Invalid pattern: no '[' or ']' is allowed.
  EXPECT_FALSE(SimpleMatchXPath("a[1]", "a[1]"));
  EXPECT_TRUE(SimpleMatchXPath("a[1]/b[9999]/c[10000]@d", "a/b/c@d"));
  EXPECT_FALSE(SimpleMatchXPath("a[1]/b[9999]/c[10000]@d", "a/b/c@f"));
  // Missing closing ']'.
  EXPECT_FALSE(SimpleMatchXPath("a[1]/b[9999]/c[10000@d", "a/b/c@d"));
}

TEST(StringUtils, CompareVersion) {
  int result = 0;
  ASSERT_FALSE(CompareVersion("1234.", "5678.", &result));
  ASSERT_TRUE(CompareVersion("12.34", "56.78", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(CompareVersion("1.2.3.4", "5678", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(CompareVersion("5678", "1.2.3.4", &result));
  ASSERT_EQ(1, result);
  ASSERT_FALSE(CompareVersion("1.2.3.4", "abcd", &result));
  ASSERT_FALSE(CompareVersion("1.2.3.4", "1.2.3.4.5", &result));
  ASSERT_FALSE(CompareVersion("1.2.3.4", "1.2.3.4.", &result));
  ASSERT_FALSE(CompareVersion("1.2.3.4", "-1.2.3.4", &result));
  ASSERT_TRUE(CompareVersion("1.2.3.4", "5.6.7.8", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(CompareVersion("1.2.3.4", "1.2.3.4", &result));
  ASSERT_EQ(0, result);
  ASSERT_TRUE(CompareVersion("1.2.3.4", "1.2.3.15", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(CompareVersion("1.2.3.4", "14.3.2.1", &result));
  ASSERT_EQ(-1, result);
  ASSERT_TRUE(CompareVersion("1.2.3.15", "1.2.3.4", &result));
  ASSERT_EQ(1, result);
  ASSERT_TRUE(CompareVersion("14.3.2.1", "1.2.3.4", &result));
  ASSERT_EQ(1, result);
  ASSERT_TRUE(CompareVersion("1.2", "1.2.0.0", &result));
  ASSERT_EQ(0, result);
}

TEST(StringUtils, StartEndWith) {
  EXPECT_TRUE(StartWith("", ""));
  EXPECT_TRUE(StartWith("abcdef", ""));
  EXPECT_TRUE(StartWith("abcdef", "ab"));
  EXPECT_TRUE(StartWith("abcdef", "abcdef"));
  EXPECT_FALSE(StartWith("abcdef", "aBc"));
  EXPECT_FALSE(StartWith("abcdef", "abcdefg"));

  EXPECT_TRUE(StartWithNoCase("", ""));
  EXPECT_TRUE(StartWithNoCase("abcdef", ""));
  EXPECT_TRUE(StartWithNoCase("abcdef", "ab"));
  EXPECT_TRUE(StartWithNoCase("abcdef", "aBcDef"));
  EXPECT_FALSE(StartWithNoCase("abcdef", "aBcdeFg"));

  EXPECT_TRUE(EndWith("", ""));
  EXPECT_TRUE(EndWith("abcdef", ""));
  EXPECT_TRUE(EndWith("abcdef", "ef"));
  EXPECT_TRUE(EndWith("abcdef", "abcdef"));
  EXPECT_FALSE(EndWith("abcdef", "dEf"));
  EXPECT_FALSE(EndWith("abcdef", "abcdefg"));

  EXPECT_TRUE(EndWithNoCase("", ""));
  EXPECT_TRUE(EndWithNoCase("abcdef", ""));
  EXPECT_TRUE(EndWithNoCase("abcdef", "ef"));
  EXPECT_TRUE(EndWithNoCase("abcdef", "DeF"));
  EXPECT_FALSE(EndWithNoCase("abcdef", "aBcdeFg"));
}

TEST(StringUtils, ValidURL) {
  EXPECT_TRUE(IsValidURLString("abc%20def"));
  EXPECT_FALSE(IsValidURLString("abc def"));
  EXPECT_FALSE(IsValidURLString("\r"));
  EXPECT_FALSE(IsValidURLComponent("\r"));
  EXPECT_TRUE(IsValidURLString("http://"));
  EXPECT_FALSE(IsValidURLComponent("http://"));
  EXPECT_TRUE(IsValidURLComponent("http%3A%2F%2F"));
  EXPECT_TRUE(HasValidURLPrefix("http://"));
  EXPECT_TRUE(HasValidURLPrefix("http://def"));
  EXPECT_TRUE(HasValidURLPrefix("https://"));
  EXPECT_TRUE(HasValidURLPrefix("feed://"));
  EXPECT_TRUE(HasValidURLPrefix("file://"));
  EXPECT_TRUE(HasValidURLPrefix("mailto:"));
  EXPECT_FALSE(HasValidURLPrefix("mailto"));
  EXPECT_FALSE(HasValidURLPrefix(" http://def"));
  EXPECT_FALSE(HasValidURLPrefix(" http:/"));
  EXPECT_TRUE(IsValidURL("http://www.abcdef.com/abc%20def"));
  EXPECT_TRUE(IsValidURL("http://www.abcdef.com/abc%20def?a=http://a.cn&hl"));
  EXPECT_FALSE(IsValidURL("http://www.abc def.com"));
  EXPECT_FALSE(IsValidURL("ftp://www.abcdef.com"));
  EXPECT_TRUE(IsValidWebURL("http://www.abcdef.com"));
  EXPECT_TRUE(IsValidWebURL("https://www.abcdef.com"));
  EXPECT_TRUE(IsValidFileURL("file:///abcdef"));
  EXPECT_FALSE(IsValidFileURL("http:///abcdef"));
  EXPECT_FALSE(IsValidRSSURL("file:///abcdef"));
  EXPECT_TRUE(IsValidRSSURL("feed:///abcdef"));
  EXPECT_TRUE(IsValidRSSURL("http:///abcdef"));
  EXPECT_TRUE(IsValidRSSURL("https:///abcdef"));
}

TEST(StringUtils, URLScheme) {
  EXPECT_STREQ("http", GetURLScheme("http://abc.com").c_str());
  EXPECT_STREQ("h323", GetURLScheme("h323://abc.com").c_str());
  EXPECT_STREQ("iris.beep", GetURLScheme("iris.beep://abc.com").c_str());
  EXPECT_STREQ("A+B.C-D", GetURLScheme("A+B.C-D://abc.com").c_str());
  EXPECT_STREQ("", GetURLScheme("http//abc.com").c_str());
  EXPECT_STREQ("", GetURLScheme("323://abc.com").c_str());
  EXPECT_STREQ("", GetURLScheme("h*tp://abc.com").c_str());

  EXPECT_TRUE(IsValidURLScheme("http"));
  EXPECT_TRUE(IsValidURLScheme("https"));
  EXPECT_TRUE(IsValidURLScheme("feed"));
  EXPECT_TRUE(IsValidURLScheme("file"));
  EXPECT_TRUE(IsValidURLScheme("mailto"));
  EXPECT_FALSE(IsValidURLScheme("ftp"));
  EXPECT_FALSE(IsValidURLScheme("javascript"));
}

TEST(StringUtils, BorderSize) {
  double left, top, right, bottom;

  EXPECT_TRUE(
      ggadget::StringToBorderSize("1 2 3 4", &left, &top, &right, &bottom));
  EXPECT_EQ(1, left);
  EXPECT_EQ(2, top);
  EXPECT_EQ(3, right);
  EXPECT_EQ(4, bottom);

  EXPECT_TRUE(
      ggadget::StringToBorderSize("1.0 2", &left, &top, &right, &bottom));
  EXPECT_EQ(1.0, left);
  EXPECT_EQ(1.0, right);
  EXPECT_EQ(2.0, top);
  EXPECT_EQ(2.0, bottom);

  EXPECT_TRUE(
      ggadget::StringToBorderSize("0.1 ", &left, &top, &right, &bottom));
  EXPECT_EQ(0.1, left);
  EXPECT_EQ(0.1, right);
  EXPECT_EQ(0.1, top);
  EXPECT_EQ(0.1, bottom);

  EXPECT_TRUE(
      ggadget::StringToBorderSize(
          "1.0,2.0,3.0,4.0", &left, &top, &right, &bottom));
  EXPECT_EQ(1.0, left);
  EXPECT_EQ(2.0, top);
  EXPECT_EQ(3.0, right);
  EXPECT_EQ(4.0, bottom);

  EXPECT_TRUE(
      ggadget::StringToBorderSize("1.0, 2", &left, &top, &right, &bottom));
  EXPECT_EQ(1.0, left);
  EXPECT_EQ(1.0, right);
  EXPECT_EQ(2, top);
  EXPECT_EQ(2, bottom);

  EXPECT_FALSE(
      ggadget::StringToBorderSize("", &left, &top, &right, &bottom));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
