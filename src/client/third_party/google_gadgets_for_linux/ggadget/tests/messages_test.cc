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

#include <locale.h>
#include <cstring>
#include "ggadget/common.h"
#include "ggadget/locales.h"
#include "ggadget/logger.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/file_manager_wrapper.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/file_manager_interface.h"
#include "ggadget/dir_file_manager.h"
#include "ggadget/localized_file_manager.h"
#include "ggadget/messages.h"
#include "ggadget/string_utils.h"
#include "ggadget/slot.h"
#include "unittest/gtest.h"

#if defined(OS_WIN)
#include "ggadget/win32/xml_parser.h"
#elif defined(OS_POSIX)
#include "init_extensions.h"
#endif

using namespace ggadget;

struct StringsInfo {
  const char *locale;
  const char *strings;
};

static const char kTestingResourceDir[] = "./testing-messages-resource";

static const StringsInfo kStringsInfo[] = {
  { "en",
    "<strings>\n"
    "  <MSG1>English message 1.</MSG1>\n"
    "  <MSG2>English message 2.</MSG2>\n"
    "  <MSG3>English message 3.</MSG3>\n"
    "</strings>\n" },
  { "it",
    "<strings>\n"
    "  <MSG1>Italian message 1.</MSG1>\n"
    "  <MSG2>Italian message 2.</MSG2>\n"
    "</strings>\n" },
  // following messages for "it-IT" won't be loaded, because locale short name
  // is always used, "it-IT" is duplicated with "it".
  { "it-IT",
    "<strings>\n"
    "  <MSG1>Duplicated Italian message 1.</MSG1>\n"
    "  <MSG2>Duplicated Italian message 2.</MSG2>\n"
    "</strings>\n" },
  { "pt-BR",
    "<strings>\n"
    "  <MSG1>Portuguese message 1.</MSG1>\n"
    "  <MSG2>Portuguese message 2.</MSG2>\n"
    "</strings>\n" },
  { "zh-CN",
    "<strings>\n"
    "  <MSG1>Chinese message 1.</MSG1>\n"
    "  <MSG2>Chinese message 2.</MSG2>\n"
    "</strings>\n" },
  { NULL, NULL }
};

static const char *kMessageIDs[] = {
  "MSG1", "MSG2", "MSG3", NULL
};

TEST(Messages, GetMessage) {
  ASSERT_STREQ("English message 1.", GM_("MSG1"));
  ASSERT_STREQ("English message 2.", GM_("MSG2"));
  ASSERT_STREQ("English message 3.", GM_("MSG3"));
  ASSERT_STREQ("MSG4", GM_("MSG4"));
}

TEST(Messages, GetMessageForLocale) {
  ASSERT_STREQ("English message 1.", GML_("MSG1", "en"));
  ASSERT_STREQ("English message 1.", GML_("MSG1", "en-US"));
  ASSERT_STREQ("Chinese message 1.", GML_("MSG1", "zh-CN"));
  ASSERT_STREQ("Italian message 1.", GML_("MSG1", "it"));
  ASSERT_STREQ("Italian message 1.", GML_("MSG1", "it-IT"));
  ASSERT_STREQ("English message 3.", GML_("MSG3", "it-IT"));
  ASSERT_STREQ("MSG4", GML_("MSG4", "it-IT"));
  ASSERT_STREQ("English message 1.", GML_("MSG1", "pt-PT"));
  ASSERT_STREQ("Portuguese message 1.", GML_("MSG1", "pt-BR"));
  ASSERT_STREQ("English message 1.", GML_("MSG1", "pt"));
}

bool EnumerateLocale(const char *locale) {
  LOG("Enumerate locale: %s", locale);
  bool found = false;
  for (size_t i = 0; kStringsInfo[i].locale; ++i) {
    if (strcmp(locale, kStringsInfo[i].locale) == 0) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
  return found;
}

bool EnumerateMessage(const char *msg) {
  LOG("Enumerate message: %s", msg);
  bool found = false;
  for (size_t i = 0; kMessageIDs[i]; ++i) {
    if (strcmp(msg, kMessageIDs[i]) == 0) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
  return found;
}

TEST(Messages, Enumerates) {
  ASSERT_TRUE(Messages::get()->EnumerateSupportedLocales(
      NewSlot(EnumerateLocale)));
  ASSERT_TRUE(Messages::get()->EnumerateAllMessages(
      NewSlot(EnumerateMessage)));
}

bool PrepareResource() {
  FileManagerWrapper *fm_wrapper = new FileManagerWrapper();
  FileManagerInterface *fm = DirFileManager::Create(kTestingResourceDir, true);
  if (!fm) {
    DLOG("Failed to create FileManager %s", kTestingResourceDir);
    return false;
  }
  fm_wrapper->RegisterFileManager(kGlobalResourcePrefix,
                                  new LocalizedFileManager(fm));
  SetGlobalFileManager(fm_wrapper);

  std::string catalog("<messages>\n");
  for (size_t i = 0; kStringsInfo[i].locale; ++i) {
    std::string filename = std::string(kStringsInfo[i].locale) + "/strings.xml";
    fm->WriteFile(filename.c_str(), kStringsInfo[i].strings, true);
    catalog.append(StringPrintf("  <%s>%s</%s>\n",
                                kStringsInfo[i].locale, filename.c_str(),
                                kStringsInfo[i].locale));
  }
  catalog.append("</messages>");

  fm->WriteFile("messages-catalog.xml", catalog, true);
  return true;
}

int main(int argc, char **argv) {
  ggadget::SetLocaleForUiMessage("en_US.UTF-8");
  testing::ParseGTestFlags(&argc, argv);
#if defined(OS_WIN)
  ggadget::win32::XMLParser xml_parser;
  ggadget::SetXMLParser(&xml_parser);
#elif defined(OS_POSIX)
  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };

  INIT_EXTENSIONS(argc, argv, kExtensions);
#endif

  PrepareResource();

  return RUN_ALL_TESTS();
}
