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

#include <map>
#include "common.h"
#include "logger.h"
#include "messages.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"
#include "file_manager_factory.h"
#include "string_utils.h"
#include "system_utils.h"
#include "locales.h"
#include "gadget_consts.h"
#include "slot.h"
#include "small_object.h"

#if defined(OS_WIN)
#include "win32/thread_local_singleton_holder.h"
#endif

namespace ggadget {

static const char kMessagesCatalog[] = "messages-catalog.xml";
static const char kDefaultLocale[] = "en";
static const char kMessagesTag[] = "messages";

/**
 * Format of message files:
 *
 * All message files are stored in global resource bundle. There is a messages
 * catalog file messages-catalog.xml in the toplevel directory of the resource
 * bundle, containing a list of all supported locales and corresponding message
 * files. The format is:
 * <messages>
 *   <en>en/strings.xml</en>
 *   <zh-CN>zh-CN/strings.xml</zh-CN>
 * </messages>
 *
 * The root element must be <messages>. Inside the root element, each item is a
 * pair of locale and corresponding file name. The locale name is representing
 * as element tag, which shall be short locale name. @see GetLocaleShortName().
 *
 * The inner text of each item is the file name which contains all strings for
 * the locale. The format is same as Gadget's strings.xml.
 *
 * "en" will be treated as the default locale, all messages that don't have
 * localized text will be fallback to the text for locale "en", or the message
 * id itself if the message is not available for all locales.
 *
 * Because Messages loads all localized messages from resource bundle by global
 * FileManager, while the global FileManager is indeed a LocalizedFileManager,
 * so they might interfere with each other. In order to make Messages class
 * work correctly, the messages-catalog.xml file must be in the toplevel
 * directory of resource bundle, and each strings.xml file must be in the sub
 * directory of corresponding locale.
 */

class Messages::Impl : public SmallObject<> {
 public:
  Impl()
      : system_locale_(GetSystemLocaleName()),
        default_locale_(kDefaultLocale) {
    if (!LoadMessages()) {
      LOG("Failed to load messages.");
    }
  }

  std::string GetMessage(const char *id) {
    ASSERT(id);
    std::string str_id(id);
    std::string result;
    if (GetMessageInternal(str_id, system_locale_, &result) ||
        GetMessageInternal(str_id, default_locale_, &result))
      return result;

    return str_id;
  }

  std::string GetMessageForLocale(const char *id, const char *locale) {
    ASSERT(id);
    ASSERT(locale);
    std::string str_id(id);
    std::string str_locale;
    std::string result;

    // Always search short locale name.
    if (!GetLocaleShortName(locale, &str_locale))
      str_locale = locale;

    if (GetMessageInternal(str_id, str_locale, &result) ||
        GetMessageInternal(str_id, default_locale_, &result))
      return result;

    return str_id;
  }

  bool GetMessageInternal(const std::string &id, const std::string &locale,
                          std::string *result) {
    MessagesCatalog::const_iterator catalog = messages_catalog_.find(locale);
    if (catalog != messages_catalog_.end()) {
      StringMap::const_iterator item = catalog->second.find(id);
      if (item != catalog->second.end()) {
        *result = item->second;
        return true;
      }
    }
    return false;
  }

  bool EnumerateSupportedLocales(Slot1<bool, const char *> *slot) {
    ASSERT(slot);
    MessagesCatalog::const_iterator catalog = messages_catalog_.begin();
    for (; catalog != messages_catalog_.end(); ++catalog) {
      if (!(*slot)(catalog->first.c_str())) {
        delete slot;
        return false;
      }
    }
    delete slot;
    return true;
  }

  bool EnumerateAllMessages(Slot1<bool, const char *> *slot) {
    ASSERT(slot);
    MessagesCatalog::const_iterator catalog =
        messages_catalog_.find(default_locale_);;
    if (catalog != messages_catalog_.end()) {
      StringMap::const_iterator item = catalog->second.begin();
      for (; item != catalog->second.end(); ++item) {
        if (!(*slot)(item->first.c_str())) {
          delete slot;
          return false;
        }
      }
    } else {
      DLOG("Messages for default locale %s are missing.", kDefaultLocale);
      delete slot;
      return false;
    }
    delete slot;
    return true;
  }

  bool LoadMessages() {
    FileManagerInterface *file_manager = GetGlobalFileManager();
    XMLParserInterface *xml_parser = GetXMLParser();
    ASSERT(file_manager);
    ASSERT(xml_parser);
    if (!file_manager || !xml_parser)
      return false;

    std::string catalog_xml;
    std::string catalog_file =
        std::string(kGlobalResourcePrefix) + kMessagesCatalog;
    if (!file_manager->ReadFile(catalog_file.c_str(), &catalog_xml)) {
      DLOG("Failed load messages catalog file.");
      return false;
    }

    StringMap catalog_map;
    if (!xml_parser->ParseXMLIntoXPathMap(catalog_xml, NULL,
                                          catalog_xml.c_str(),
                                          kMessagesTag, NULL,
                                          kEncodingFallback,
                                          &catalog_map)) {
      DLOG("Failed to parse messages catalog.");
      return false;
    }

    for (StringMap::iterator it = catalog_map.begin();
         it != catalog_map.end(); ++it) {
      std::string strings_file;
      std::string strings_xml;
      std::string lang;
      // Always use short locale name.
      if (!GetLocaleShortName(it->first.c_str(), &lang))
        lang = it->first;
      if (messages_catalog_.find(lang) != messages_catalog_.end()) {
        DLOG("Messages for locale %s had already been loaded.",
             it->first.c_str());
        continue;
      }
      strings_file = std::string(kGlobalResourcePrefix) + it->second;
      if (!file_manager->ReadFile(strings_file.c_str(), &strings_xml)) {
        DLOG("Failed to load message file %s", it->second.c_str());
        continue;
      }
      if (!xml_parser->ParseXMLIntoXPathMap(strings_xml, NULL,
                                            it->second.c_str(),
                                            kStringsTag, NULL,
                                            kEncodingFallback,
                                            &messages_catalog_[lang])) {
        DLOG("Failed to parse message file %s", it->second.c_str());
        messages_catalog_.erase(lang);
      }
    }

    if (messages_catalog_.find(default_locale_) == messages_catalog_.end())
      LOG("Default messages are not available.");

    return messages_catalog_.size() != 0;
  }

  typedef LightMap<std::string, StringMap> MessagesCatalog;
  MessagesCatalog messages_catalog_;
  std::string system_locale_;
  std::string default_locale_;

#if !defined(OS_WIN)
  static Messages *messages_;
#endif
};

#if !defined(OS_WIN)
Messages *Messages::Impl::messages_ = NULL;
#endif

Messages::Messages()
  : impl_(new Impl()) {
}

Messages::~Messages() {
  delete impl_;
  impl_ = NULL;
}

std::string Messages::GetMessage(const char *id) const {
  return impl_->GetMessage(id);
}

std::string Messages::GetMessageForLocale(const char *id,
                                          const char *locale) const {
  return impl_->GetMessageForLocale(id, locale);
}

bool Messages::EnumerateSupportedLocales(Slot1<bool, const char*> *slot) const {
  return impl_->EnumerateSupportedLocales(slot);
}

bool Messages::EnumerateAllMessages(Slot1<bool, const char *> *slot) const {
  return impl_->EnumerateAllMessages(slot);
}

const Messages *Messages::get() {
#if defined(OS_WIN)
  Messages *messages =
      win32::ThreadLocalSingletonHolder<Messages>::GetValue();
  if (!messages) {
    messages = new Messages();
    const bool result =
        win32::ThreadLocalSingletonHolder<Messages>::SetValue(messages);
    ASSERT(result);
  }

  return messages;
#else
  if (!Impl::messages_)
    Impl::messages_ = new Messages();

  return Impl::messages_;
#endif
}

} // namespace ggadget
