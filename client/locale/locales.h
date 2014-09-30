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

#ifndef GOOPY_LOCALE_LOCALES_H__
#define GOOPY_LOCALE_LOCALES_H__

#include <string>

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace ime_goopy {
namespace locale {

/**
 * @defgroup Locales Locale utilities
 * @ingroup Utilities
 * @{
 */

/**
 * Set corresponding system locale.
 *
 * This function receive a standard Posix locale name,
 * then set the corresponding system locale with this info.
 *
 * @param locale_name Posix locale full name.
 * @return true if the locale is successfully set.
 */
bool SetLocaleForUiMessage(const char *locale_name);

/**
 * Set corresponding input locale.
 *
 * This function receive a standard Posix locale name,
 * then set the corresponding input locale with this info.
 *
 * @param locale_name Posix locale full name.
 * @return true if the locale is successfully set.
 */
bool SetLocaleForInput(const char *locale_name);

/**
 * Gets the corresponding Windows locale identifier for a given short or
 * two-segment locale name.
 *
 * This function is only for historical compatibility. Google desktop gadget
 * library for Windows uses Windows locale identifiers as the names of
 * subdirectories containing localized resources.
 *
 * @param name locale name in format of "lang" or "lang-TERRITORY",
 *     such as "en" or "zh-CN".
 * @param[out] windows_id the corresponding Windows locale identifer.
 * @return @c true if @a windows_id found.
 */
bool GetLocaleWindowsIDString(const char *name, std::string *windows_id);

/**
 * Gets the short name equivalent to the given short or two-segment
 * locale name.
 *
 * "Equivalent" means that the short name is widely used and accepted as the
 * alias of the given locale name, for example, "it-IT"'s short name is "it",
 * but "it-CH" has no short name. Not all language names can be short names of
 * some full locale names, for example, "pt" is not the shortname for either
 * "pt-PT" or "pt-BR".
 *
 * If a short name with only lang part is specified, then this function will
 * verify if the given short name is valid. If it's valid then the output
 * parameter short_name will be set to the specified short name and true will
 * be returned.
 *
 * @param name two-segment locale name in format of "lang-TERRITORY", such as
 *        "zh-CN". Or a short name with only lang part, if you want to verify
 *        the short name.
 * @param[out] short_name the corresponding short name for the locale.
 * @return @c true if @a short_name found.
 */
bool GetLocaleShortName(const char *name, std::string *short_name);

/**
 * Returns the system locale name. The returned value is in short form if the
 * locale has short form, otherwise in two-segment form.
 */
std::string GetSystemLocaleName();

/**
 * Returns the keyboard layout locale name. The returned value is in short
 * form if the locale has short form, otherwise in two-segment form.
 */
std::string GetKeyboardLayoutLocaleName();

/**
 * Returns the locale name for the user UI language for the current user.
 */
std::string GetUserUILanguage();

/** @} */

}  // namespace locale
}  // namespace ime_goopy

#endif  // GOOPY_LOCALE_LOCALES_H__
