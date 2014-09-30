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

#ifndef GGADGET_MESSAGES_H__
#define GGADGET_MESSAGES_H__

#include <string>

namespace ggadget {

template<typename R, typename P> class Slot1;

/**
 * @defgroup Messages Messages
 * @ingroup CoreLibrary
 * Class and utilities to support localizable messages.
 * @{
 */

/**
 * A class to manage localized messages. It's a global singleton class.
 *
 * All strings managed by this class must use UTF-8 encoding.
 *
 * Not like LocalizedFileManager, which manages localizable files, Messages
 * class manages localizable text messages. For other localizable non-text data
 * files, such as icons, images, etc. LocalizedFileManager is a better choice.
 */
class Messages {
 public:
  /**
   * Gets a message localized for current system locale.
   *
   * @param id id of the message.
   * @return the localized message associated to the id, or the default
   *         (english) one, if the message is not localized. If the message is
   *         not available for all locales, the id itself will be returned.
   */
  std::string GetMessage(const char *id) const;

  /**
   * Gets a message localized for a specified locale.
   *
   * @param id id of the message.
   * @param locale the locale name, either full name like "zh-CN" or short name
   *        like "it".
   * @return the localized message associated to the id, or the default
   *         (english) one, if the message is not localized. If the message is
   *         not available for all locales, the id itself will be returned.
   */
  std::string GetMessageForLocale(const char *id, const char *locale) const;

  /**
   * Enumerates all locales supported by the message database.
   *
   * @param slot the slot that will be called with the short name of each
   *        locale. Returns false to stop enumerating.
   * @return true if all supported locales are enumerated.
   */
  bool EnumerateSupportedLocales(Slot1<bool, const char *> *slot) const;

  /**
   * Enumerates all messages in message database.
   *
   * Only messages available for the default locale will be enumerated.
   *
   * @param slot the slot that will be called with the id of each
   *        message. Returns false to stop enumerating.
   * @return true if all supported locales are enumerated.
   */
  bool EnumerateAllMessages(Slot1<bool, const char *> *slot) const;

 public:
  /**
   * Gets the global singleton instance of Messages class.
   */
  static const Messages *get();

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating Messages object directly.
   */
  Messages();

  /**
   * Private destructor to prevent deleting Messages object directly.
   */
  ~Messages();

  DISALLOW_EVIL_CONSTRUCTORS(Messages);
};

#define GMS_(id)  (::ggadget::Messages::get()->GetMessage(id))
#define GMSL_(id,locale)  \
  (::ggadget::Messages::get()->GetMessageForLocale((id),(locale)))
#define GM_(id)  (GMS_(id).c_str())
#define GML_(id,locale)  (GMSL_(id,locale).c_str())

/** @} */

} // namespace ggadget

#endif // GGADGET_MESSAGES_H__
