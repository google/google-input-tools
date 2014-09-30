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

#include "locale/locale_utils.h"

#include <windows.h>
#include <map>
#include <string>
#include <vector>

#include "locale/locales.h"
#include <gtest/gunit.h>

namespace ime_goopy {

struct LocalePair {
  const char* const input;
  const char* const expect;
};

TEST(LocaleUtils, GetSystemLocaleName) {
  static const LocalePair kTestLocaleList[] = {
    { "ar_SA.UTF-8", "ar_SA" },
    { "en_GB.UTF-8", "en_GB" },
    { "en_US", "en" },
    { "he-IL", "he" },
    { "iw", "he" },
    { "zh_CN.UTF-8", "zh_CN" },
  };
  for (int i = 0; i < arraysize(kTestLocaleList); i++) {
    locale::SetLocaleForUiMessage(kTestLocaleList[i].input);
    ASSERT_EQ(std::string(kTestLocaleList[i].expect),
              LocaleUtils::GetSystemLocaleName());
  }
}

TEST(LocaleUtils, GetKeyboardLayoutLocaleName) {
  static const LocalePair kTestLocaleList[] = {
    { "ar-SA", "ar_SA" },
    { "en-GB", "en_GB" },
    { "en-US", "en" },
    { "he-IL", "he" },
    { "iw", "he" },
    { "zh-CN", "zh_CN" },
  };
  for (int i = 0; i < arraysize(kTestLocaleList); i++) {
    // Some of the locales to test might be unavailable on test machines.
    // In such case, we just ignore those locales and keep trying the next one.
    if (locale::SetLocaleForInput(kTestLocaleList[i].input)) {
      ASSERT_EQ(std::string(kTestLocaleList[i].expect),
                LocaleUtils::GetKeyboardLayoutLocaleName());
    }
  }
}

TEST(LocaleUtils, GetApplicationLocale) {
  static const LocalePair kTestLocaleList[] = {
    // The set of all supported languages
    { "am", "am" },
    { "ar", "ar" },
    { "bn", "bn" },
    { "el", "el" },
    { "en", "en" },
    { "fa", "fa" },
    { "gu", "gu" },
    { "he", "he" },
    { "hi", "hi" },
    { "kn", "kn" },
    { "ml", "ml" },
    { "mr", "mr" },
    { "ne", "ne" },
    { "or", "or" },
    { "pa", "pa" },
    { "ru", "ru" },
    { "sa", "sa" },
    { "si", "si" },
    { "sr", "sr" },
    { "ta", "ta" },
    { "te", "te" },
    { "ti", "ti" },
    { "ur", "ur" },
    // Not localized to these languages, fallback to default English
    { "zh", "en" },
    { "ja", "en" },
    { "kr", "en" },
    { "vi", "en" },
    // Sub languages fallback to parent languages
    { "ar-DZ", "ar" },
    { "en-AU", "en" },
    { "zh-CN", "en" },
    // "iw" -> "he"
    { "iw", "he" },
  };
  for (int i = 0; i < arraysize(kTestLocaleList); i++) {
    EXPECT_EQ(LocaleUtils::GetApplicationLocale(kTestLocaleList[i].input),
              kTestLocaleList[i].expect);
  }
}

TEST(LocaleUtils, GetParentLocales) {
  static const char test_locale[] = "en-XA_test-mocked";
  static const char expected_parent_locales[][20] = {
      "en_XA_test_mocked",
      "en_XA_test",
      "en_XA",
      "en"};
  std::vector<std::string> parent_locales;
  LocaleUtils::GetParentLocales(test_locale, &parent_locales);
  ASSERT_EQ(arraysize(expected_parent_locales), parent_locales.size());
  for (size_t i = 0; i < parent_locales.size(); ++i)
    EXPECT_EQ(expected_parent_locales[i], parent_locales[i]);
}

TEST(LocaleUtils, HasSamePrimaryLocale) {
  EXPECT_TRUE(LocaleUtils::PrimaryLocaleEquals("en", "en-XA"));
  EXPECT_TRUE(LocaleUtils::PrimaryLocaleEquals("en-XA", "en"));
  EXPECT_TRUE(LocaleUtils::PrimaryLocaleEquals("zh-CN", "zh-TW"));
  EXPECT_TRUE(LocaleUtils::PrimaryLocaleEquals("zh_CN", "zh-TW"));
  EXPECT_TRUE(LocaleUtils::PrimaryLocaleEquals("zh_CN", "zh-Hant_TW"));
  EXPECT_FALSE(LocaleUtils::PrimaryLocaleEquals("en", "ar-SA"));
  EXPECT_FALSE(LocaleUtils::PrimaryLocaleEquals("za-CN", "zh-CN"));
  EXPECT_TRUE(LocaleUtils::PrimaryLocaleEquals("iw", "he"));
}

TEST(LocaleUtils, GetUserUILanguage) {
  // We can't set user's ui language so we only can check it doesn't change with
  // locale.
  std::string lang = LocaleUtils::GetUserUILanguage();
  EXPECT_FALSE(lang.empty());
  static const LocalePair kTestLocaleList[] = {
    { "ar_SA.UTF-8", "ar_SA" },
    { "en_GB.UTF-8", "en_GB" },
    { "en_US", "en" },
    { "he-IL", "he" },
    { "zh_CN.UTF-8", "zh_CN" },
  };
  for (int i = 0; i < arraysize(kTestLocaleList); i++) {
    locale::SetLocaleForUiMessage(kTestLocaleList[i].input);
    ASSERT_EQ(lang, LocaleUtils::GetUserUILanguage());
  }
}

TEST(LocaleUtils, IsRTLLanguage) {
  static const char* kRTLLanguages[] = {
    "ar", "fa", "he",
  };
  for (int i = 0; i < arraysize(kRTLLanguages); ++i) {
    EXPECT_TRUE(LocaleUtils::IsRTLLanguage(kRTLLanguages[i]));
  }
  EXPECT_FALSE(LocaleUtils::IsRTLLanguage("ma"));
  EXPECT_FALSE(LocaleUtils::IsRTLLanguage("unsupported_language"));
};

}  // namespace ime_goopy

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
