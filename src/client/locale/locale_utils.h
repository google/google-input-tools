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
//
// This utility class defines locale-dependent functions, like checking if the
// given char is treated as "upper letter" in given locale, and create helper
// classes which is specific for different locale.

#ifndef GOOPY_LOCALE_LOCALE_UTILS_H__
#define GOOPY_LOCALE_LOCALE_UTILS_H__

#include <string>
#include "base/basictypes.h"

namespace ime_goopy {

class TextManipulator;

class LocaleUtils {
 public:
  enum Locale {
    UNKNOWN = -1,
    EN = 0,
    ZH_CN,
  };
  // Create a new instance of TextManipulator, given the locale name.
  // Returns NULL if the given locale_name does not match any of known locales.
  // The caller is responsible for releasing the created object when unused.
  //
  // DEPRECATED: Currently it is only used in English IME, we need to
  // replace that with a general library for manipulating sentence begins
  // and word boundaries.
  static TextManipulator* CreateTextManipulator(Locale locale);

  // Retrieves the current UI language.
  static std::string GetSystemLocaleName();

  // Retrieves the locale reflecting to the current keyboard layout
  static std::string GetKeyboardLayoutLocaleName();

  // Retrieves the locale name for the user's current UI language.
  static std::string GetUserUILanguage();

  // Retrives the supported locale language. It will fallback to parent locale
  // when the current locale is not available. If none of the locales are
  // supported, it will fallback to 'en'.
  static std::string GetApplicationLocale(const std::string& pref_locale);

  // Retrieves the parent locales for current locale and put them in
  // |parent_locales|. The current locale itself is also considered as its
  // parent locale. For example, the parent locales for "bs-Cyrl-BA" will be
  // {"bs_Cyrl_BA", "bs_Cyrl", "bs"}.
  static void GetParentLocales(const std::string& current_locale,
                               std::vector<std::string>* parent_locales);

  // Returns true if the two locales |locale1| and |locale2| shares a same
  // primary locale. The primary locale is the first segment of a locale name.
  // For example, primary locale of local "en_XA-xb" is "en".
  static bool PrimaryLocaleEquals(const std::string& locale1,
                                  const std::string& locale2);

  // Returns true if the |language_short_name| represents a right to left
  // language we support.
  // Notice that this function will returns false if we don't support the
  // language.
  static bool IsRTLLanguage(const std::string& language_short_name);

 private:
  LocaleUtils();
  DISALLOW_COPY_AND_ASSIGN(LocaleUtils);
};

}  // namespace ime_goopy

#endif  // GOOPY_LOCALE_LOCALE_UTILS_H__
