/*
  Copyright 2008 Google Inc.

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

#include "ggadget/digest_utils.h"
#include "ggadget/string_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

std::string ToHexString(const std::string &str) {
  std::string result;
  for (size_t i = 0; i < str.size(); i++)
    result += StringPrintf("%02x", static_cast<unsigned char>(str[i]));
  return result;
}

// Some cases are from the Original Eric Fredricksen implementation
TEST(DigestUtils, GenerateSHA1) {
  std::string result;

  const std::string blank_digest("da39a3ee5e6b4b0d3255bfef95601890afd80709");
  ASSERT_TRUE(GenerateSHA1(std::string(""), &result));
  ASSERT_EQ(blank_digest, ToHexString(result));

  // FIPS 180-1 Appendix A example
  const std::string digest1("a9993e364706816aba3e25717850c26c9cd0d89d");
  ASSERT_TRUE(GenerateSHA1(std::string("abc"), &result));
  ASSERT_EQ(digest1, ToHexString(result));
  std::string result1;
  ASSERT_TRUE(GenerateSHA1(std::string("abc\0\0", 5), &result1));
  ASSERT_NE(result, result1);

  // FIPS 180-1 Appendix A example
  const std::string digest2("84983e441c3bd26ebaae4aa1f95129e5e54670f1");
  const char *to_hash = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  ASSERT_TRUE(GenerateSHA1(std::string(to_hash), &result));
  ASSERT_EQ(digest2, ToHexString(result));

  // Test the boundary conditions.
  const char *digests117_130[] = {
      "44e2519a529d7261f1bebedc8ed95e1182cae0dc",
      "2a81372da39c1df4251539a9922717b7cf5f0334",
      "41c89d06001bab4ab78736b44efe7ce18ce6ae08",
      "d3dbd653bd8597b7475321b60a36891278e6a04a",
      "3723f8ab857804f89f80970e9fc88cf8f890adc2",
      "d031c9fb7af0a461241e539e10db62ed28f7033b",
      "e0b550438e794b65d89b9ee5c8f836ae737decf0",
      "fb3998281c31d1a8eea2ea737affd0b4d6ab6ac2",
      "7a914d8b86a534581aa71ec61912ba3f5b478698",
      "a271f71547442dea7b2edf65cd5fbd5c751710aa",
      "89d7312a903f65cd2b3e34a975e55dbea9033353",
      "e6434bc401f98603d7eda504790c98c67385d535",
      "3352e41cc30b40ae80108970492b21014049e625",
      "6981ed7d97ffca517d531cd3d1874b43e11f1b46",
  };
  char str[130];
  for (int i = 0; i < 130; i++)
    str[i] = static_cast<char>(i);
  for (int i = 117; i < 130; i++) {
    ASSERT_TRUE(GenerateSHA1(std::string(str, i), &result));
    ASSERT_EQ(std::string(digests117_130[i - 117]), ToHexString(result));
  }
}

TEST(DigestUtils, Base64) {
  const char *standard_outputs[] = {
      "",
      "/w==",
      "//4=",
      "//79",
      "//79/A==",
      "//79/Ps=",
      "//79/Pv6",
      "//79/Pv6+Q==",
      "//79/Pv6+fg=",
      "//79/Pv6+fj3",
      "//79/Pv6+fj39g==",
      "//79/Pv6+fj39gA=",
      "//79/Pv6+fj39gAB",
      "//79/Pv6+fj39gABAg==",
      "//79/Pv6+fj39gABAgM=",
      "//79/Pv6+fj39gABAgME",
      "//79/Pv6+fj39gABAgMEBQ==",
      "//79/Pv6+fj39gABAgMEBQY=",
      "//79/Pv6+fj39gABAgMEBQYH",
      "//79/Pv6+fj39gABAgMEBQYHCA==",
  };

  const char *outputs_no_padding[] = {
      "",
      "/w",
      "//4",
      "//79",
      "//79/A",
      "//79/Ps",
      "//79/Pv6",
      "//79/Pv6+Q",
      "//79/Pv6+fg",
      "//79/Pv6+fj3",
      "//79/Pv6+fj39g",
      "//79/Pv6+fj39gA",
      "//79/Pv6+fj39gAB",
      "//79/Pv6+fj39gABAg",
      "//79/Pv6+fj39gABAgM",
      "//79/Pv6+fj39gABAgME",
      "//79/Pv6+fj39gABAgMEBQ",
      "//79/Pv6+fj39gABAgMEBQY",
      "//79/Pv6+fj39gABAgMEBQYH",
      "//79/Pv6+fj39gABAgMEBQYHCA",
  };

  const char *web_safe_outputs[] = {
      "",
      "_w==",
      "__4=",
      "__79",
      "__79_A==",
      "__79_Ps=",
      "__79_Pv6",
      "__79_Pv6-Q==",
      "__79_Pv6-fg=",
      "__79_Pv6-fj3",
      "__79_Pv6-fj39g==",
      "__79_Pv6-fj39gA=",
      "__79_Pv6-fj39gAB",
      "__79_Pv6-fj39gABAg==",
      "__79_Pv6-fj39gABAgM=",
      "__79_Pv6-fj39gABAgME",
      "__79_Pv6-fj39gABAgMEBQ==",
      "__79_Pv6-fj39gABAgMEBQY=",
      "__79_Pv6-fj39gABAgMEBQYH",
      "__79_Pv6-fj39gABAgMEBQYHCA==",
  };

  const char *web_safe_outputs_no_padding[] = {
      "",
      "_w",
      "__4",
      "__79",
      "__79_A",
      "__79_Ps",
      "__79_Pv6",
      "__79_Pv6-Q",
      "__79_Pv6-fg",
      "__79_Pv6-fj3",
      "__79_Pv6-fj39g",
      "__79_Pv6-fj39gA",
      "__79_Pv6-fj39gAB",
      "__79_Pv6-fj39gABAg",
      "__79_Pv6-fj39gABAgM",
      "__79_Pv6-fj39gABAgME",
      "__79_Pv6-fj39gABAgMEBQ",
      "__79_Pv6-fj39gABAgMEBQY",
      "__79_Pv6-fj39gABAgMEBQYH",
      "__79_Pv6-fj39gABAgMEBQYHCA",
  };

  char str[20];
  for (int i = 0; i < 10; i++)
    str[i] = static_cast<char>(255 - i);
  for (int i = 10; i < 20; i++)
    str[i] = static_cast<char>(i - 10);

  for (int i = 0; i < 20; i++) {
    std::string input(str, i);
    std::string result;
    std::string decoded;

    ASSERT_TRUE(EncodeBase64(input, true, &result));
    ASSERT_EQ(std::string(standard_outputs[i]), result);
    ASSERT_TRUE(DecodeBase64(result.c_str(), &decoded));
    ASSERT_EQ(input, decoded);

    ASSERT_TRUE(EncodeBase64(input, false, &result));
    ASSERT_EQ(std::string(outputs_no_padding[i]), result);
    ASSERT_TRUE(DecodeBase64(result.c_str(), &decoded));
    ASSERT_EQ(input, decoded);

    ASSERT_TRUE(WebSafeEncodeBase64(input, true, &result));
    ASSERT_EQ(std::string(web_safe_outputs[i]), result);
    ASSERT_TRUE(WebSafeDecodeBase64(result.c_str(), &decoded));
    ASSERT_EQ(input, decoded);

    ASSERT_TRUE(WebSafeEncodeBase64(input, false, &result));
    ASSERT_EQ(std::string(web_safe_outputs_no_padding[i]), result);
    ASSERT_TRUE(WebSafeDecodeBase64(result.c_str(), &decoded));
    ASSERT_EQ(input, decoded);
  }
}

TEST(DigestUtils, DecodeBase64Failure) {
  std::string result;
  ASSERT_FALSE(DecodeBase64("!@#$%", &result));
  ASSERT_FALSE(DecodeBase64("_-ab", &result));

  ASSERT_TRUE(DecodeBase64("//79//", &result));
  ASSERT_TRUE(DecodeBase64("//79//==", &result));
  ASSERT_FALSE(DecodeBase64("//79/", &result));
  ASSERT_FALSE(DecodeBase64("//79/==", &result));
  ASSERT_FALSE(DecodeBase64("//79/===", &result));
  ASSERT_FALSE(DecodeBase64("//79//==/", &result));

  ASSERT_FALSE(WebSafeDecodeBase64("!@#$%", &result));
  ASSERT_FALSE(WebSafeDecodeBase64("/+ab", &result));

  ASSERT_TRUE(WebSafeDecodeBase64("__79__", &result));
  ASSERT_TRUE(WebSafeDecodeBase64("__79__==", &result));
  ASSERT_FALSE(WebSafeDecodeBase64("__79/", &result));
  ASSERT_FALSE(WebSafeDecodeBase64("__79/==", &result));
  ASSERT_FALSE(WebSafeDecodeBase64("__79/===", &result));
  ASSERT_FALSE(WebSafeDecodeBase64("__79__==_", &result));

}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
