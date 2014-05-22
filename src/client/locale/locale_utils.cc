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

#include <algorithm>
#include <string>
#include <vector>
#include "locale/locales.h"
#include "locale/text_utils.h"

namespace {

static const char* const kDefaultLanguage = "en";

struct LanguageInfo {
  std::string name;
  bool is_rtl;
};

static const LanguageInfo kAcceptLanguageList[] = {
  {"am", false},     // Amharic
  {"ar", true},     // Arabic
  {"bn", false},     // Bengali
  {"el", false},     // Greek
  {"en", false},     // English
  {"fa", true},     // Persian
  {"gu", false},     // Gujarati
  {"he", true},     // Hebrew
  {"hi", false},     // Hindi
  {"kn", false},     // Kannada
  {"ml", false},     // Malayalam
  {"mr", false},     // Marathi
  {"ne", false},     // Nepali
  {"or", false},     // Oriya
  {"pa", false},     // Punjabi
  {"ru", false},     // Russian
  {"sa", false},     // Sanskrit
  {"si", false},     // Sinhala
  {"sr", false},     // Serbian
  {"ta", false},     // Tamil
  {"te", false},     // Telugu
  {"ti", false},     // Tigrinya
  {"ur", false},     // Urdu
};

// Used to convert from alias short name to primary short name.
typedef struct {
  const char* const primary;
  const char* const alias;
} NormalizedLanguageLocaleAlias;

static const NormalizedLanguageLocaleAlias kNormalizedLocaleAliasPair[] = {
  // Convert google style hebrew short name 'iw' to iso style 'he'.
  {"he", "iw"},
};

// From chrome/src/ui/base/l10n/l10n_util.cc
std::string NormalizeLocale(const std::string& locale) {
  std::string normalized_locale(locale);
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');
  for (int i = 0; i < arraysize(kNormalizedLocaleAliasPair); ++i) {
    if (normalized_locale == kNormalizedLocaleAliasPair[i].alias) {
      normalized_locale = kNormalizedLocaleAliasPair[i].primary;
      break;
    }
  }
  return normalized_locale;
}

}

namespace ime_goopy {

TextManipulator* LocaleUtils::CreateTextManipulator(Locale locale) {
  switch (locale) {
    case EN:
      return new TextManipulatorEn();
    case ZH_CN:
      return new TextManipulatorZhCn();
    default:
      NOTREACHED();
      return NULL;
  }
}

std::string LocaleUtils::GetSystemLocaleName() {
  return NormalizeLocale(locale::GetSystemLocaleName());
}

std::string LocaleUtils::GetKeyboardLayoutLocaleName() {
  return NormalizeLocale(locale::GetKeyboardLayoutLocaleName());
}

std::string LocaleUtils::GetUserUILanguage() {
  return NormalizeLocale(locale::GetUserUILanguage());
}

std::string LocaleUtils::GetApplicationLocale(const std::string& pref_locale) {
  std::vector<std::string> locales;
  GetParentLocales(pref_locale, &locales);
  // Since parent locales contain the original locale, we just proceed to
  // each of these locales to find a match.
  for (int i = 0; i < locales.size(); i++) {
    for (int j = 0; j < arraysize(kAcceptLanguageList); ++j) {
      if (kAcceptLanguageList[j].name == locales[i])
        return locales[i];
    }
  }
  // Fallback to default language
  return std::string(kDefaultLanguage);
}

void LocaleUtils::GetParentLocales(const std::string& current_locale,
                                   std::vector<std::string>* parent_locales) {
  std::string locale(NormalizeLocale(current_locale));
  parent_locales->push_back(locale);
  while (true) {
    size_t pos = locale.rfind('_');
    if (pos == std::string::npos)
      break;
    locale.resize(pos);
    parent_locales->push_back(locale);
  }
}

bool LocaleUtils::PrimaryLocaleEquals(const std::string& locale1,
                                      const std::string& locale2) {
  std::vector<std::string> parent_locales_1;
  std::vector<std::string> parent_locales_2;
  GetParentLocales(locale1, &parent_locales_1);
  GetParentLocales(locale2, &parent_locales_2);
  return parent_locales_1.back() == parent_locales_2.back();
}

bool LocaleUtils::IsRTLLanguage(const std::string& language_short_name) {
  std::vector<std::string> locales;
  GetParentLocales(language_short_name, &locales);
  // Since parent locales contain the original locale, we just proceed to
  // each of these locales to find a match.
  for (int i = 0; i < locales.size(); ++i) {
    for (int j = 0; j < arraysize(kAcceptLanguageList); ++j) {
      if (kAcceptLanguageList[j].name == locales[i])
        return kAcceptLanguageList[j].is_rtl;
    }
  }
  return false;
}
}  // namespace ime_goopy
