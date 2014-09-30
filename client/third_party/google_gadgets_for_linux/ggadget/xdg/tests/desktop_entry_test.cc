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

#include <cstdio>
#include "../desktop_entry.h"
#include "ggadget/string_utils.h"
#include "ggadget/system_utils.h"
#include "ggadget/dir_file_manager.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::xdg;

static std::string g_temp_dir;
static DirFileManager g_file_manager;

static const char kDesktopEntryAppGood1[] =
  "# A comment\n"
  "\n"
  "[Desktop Entry]\n"
  "Type=Application\n"
  "Exec = hello %c %i %k %U\n"
  "Icon = hello\n"
  "Terminal=true\n"
  "MimeType=text/plain;image/png;application/x-zip\n"
  "StartupNotify=1\n"
  "# Another comment\n"
  "StartupWMClass=hello_world\n"
  "Name=Hello\n"
  "\n"
  "Name[zh_CN]=你好\n"
  "Name[fr]=Bonjour\n"
  "GenericName=A simple app\n"
  "Comment=Hello world\n"
  "Path=/tmp\n";

static const char kDesktopEntryAppGood2[] =
  "[Desktop Entry]\n"
  "Type=Application\n"
  "TryExec=yes\n"
  "Exec = hello %f\n"
  "Icon = hello\n"
  "Terminal=true\n"
  "MimeType=text/plain;image/png;application/x-zip\n"
  "StartupNotify=1\n"
  "# Another comment\n"
  "StartupWMClass=hello\n"
  "Name=Hello\n"
  "\n"
  "Name[zh-CN]=你好\n"
  "Name[fr]=Bonjour\n"
  "Comment=Hello world\n"
  "Path=/tmp\n";

static const char kDesktopEntryURLGood[] =
  "# A comment\n"
  "\n"
  "[Desktop Entry]\n"
  "Type=Link\n"
  "URL = http://www.google.com\n"
  "Icon = hello\n"
  "Terminal=true\n"
  "MimeType=text/plain;image/png;application/x-zip\n"
  "StartupNotify=1\n"
  "StartupWMClass=hello\n"
  "Name = \\sHello\n"
  "Name[zh_CN]=你好\n"
  "Name[fr]=Bonjour\n"
  "Comment=Hello world\n"
  "Path=/tmp\n";

static const char kDesktopEntryBad1[] =
  "# A comment\n"
  "[Desktop Entry]\n"
  "Type=Application\n"
  "[Other Group]\n"
  "Exec=hello\n"
  "Name=hello\n";

static const char kDesktopEntryBad2[] =
  "[Desktop Entry]\n"
  "Type=Application\n"
  "Name=hello\n"
  "URL=hello\n";


TEST(DesktopEntry, AppGood1) {
  g_file_manager.WriteFile("app-good1.desktop", kDesktopEntryAppGood1, true);
  std::string path = g_file_manager.GetFullPath("app-good1.desktop");
  DesktopEntry entry(path.c_str());
  ASSERT_TRUE(entry.IsValid());
  EXPECT_TRUE(entry.GetType() == DesktopEntry::APPLICATION);
  EXPECT_TRUE(entry.NeedTerminal());
  EXPECT_TRUE(entry.SupportStartupNotify());
  EXPECT_TRUE(entry.SupportMimeType("text/plain"));
  EXPECT_TRUE(entry.SupportMimeType("image/png"));
  EXPECT_TRUE(entry.SupportMimeType("application/x-zip"));
  EXPECT_FALSE(entry.SupportMimeType("application/x-pdf"));
  EXPECT_TRUE(entry.AcceptURL(false));
  EXPECT_TRUE(entry.AcceptURL(true));
  EXPECT_FALSE(entry.AcceptFile(false));
  EXPECT_FALSE(entry.AcceptFile(true));
  setlocale(LC_ALL, "C");
  EXPECT_STREQ("Hello", entry.GetName().c_str());
  EXPECT_STREQ("Hello world", entry.GetComment().c_str());
  EXPECT_STREQ("A simple app", entry.GetGenericName().c_str());
  setlocale(LC_ALL, "zh_CN.UTF-8");
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (locale && strncmp(locale, "zh_CN", 5) == 0)
    EXPECT_STREQ("你好", entry.GetName().c_str());
  EXPECT_STREQ("Hello world", entry.GetComment().c_str());
  setlocale(LC_ALL, "fr_FR.UTF-8");
  locale = setlocale(LC_MESSAGES, NULL);
  if (locale && strncmp(locale, "fr_FR", 5) == 0)
    EXPECT_STREQ("Bonjour", entry.GetName().c_str());
  EXPECT_STREQ("hello", entry.GetIcon().c_str());
  EXPECT_STREQ("/tmp", entry.GetWorkingDirectory().c_str());
  EXPECT_STREQ("text/plain;image/png;application/x-zip",
               entry.GetMimeTypes().c_str());
  EXPECT_STREQ("hello_world", entry.GetStartupWMClass().c_str());
  std::string cmd = StringPrintf("hello 'Bonjour' --icon 'hello' '%s' '%s' '%s'",
                                 path.c_str(),
                                 "http://www.google.com",
                                 "file:///tmp/abc%20def");
  static const char *argv[] = {
    "http://www.google.com",
    "/tmp/abc def"
  };
  EXPECT_STREQ(cmd.c_str(), entry.GetExecCommand(2, argv).c_str());
}

TEST(DesktopEntry, AppGood2) {
  g_file_manager.WriteFile("app-good2.desktop", kDesktopEntryAppGood2, true);
  std::string path = g_file_manager.GetFullPath("app-good2.desktop");
  DesktopEntry entry(path.c_str());
  ASSERT_TRUE(entry.IsValid());
  EXPECT_TRUE(entry.GetType() == DesktopEntry::APPLICATION);
  setlocale(LC_ALL, "zh_CN.UTF-8");
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (locale && strncmp(locale, "zh_CN", 5) == 0)
    EXPECT_STREQ("你好", entry.GetName().c_str());
  static const char *argv[] = {
    "http://www.google.com",
    "file:///tmp/abc%20def"
  };
  EXPECT_STREQ("hello '/tmp/abc def'", entry.GetExecCommand(2, argv).c_str());
  EXPECT_STREQ("hello", entry.GetExecCommand(0, NULL).c_str());
}

TEST(DesktopEntry, URLGood) {
  g_file_manager.WriteFile("url-good.desktop", kDesktopEntryURLGood, true);
  std::string path = g_file_manager.GetFullPath("url-good.desktop");
  DesktopEntry entry(path.c_str());
  ASSERT_TRUE(entry.IsValid());
  EXPECT_TRUE(entry.GetType() == DesktopEntry::LINK);
  setlocale(LC_ALL, "en_US.UTF-8");
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (locale && strncmp(locale, "en_US", 5) == 0)
    EXPECT_STREQ(" Hello", entry.GetName().c_str());
  EXPECT_STREQ("http://www.google.com", entry.GetURL().c_str());
}

TEST(DesktopEntry, Bad) {
  g_file_manager.WriteFile("bad1.desktop", kDesktopEntryBad1, true);
  std::string path = g_file_manager.GetFullPath("bad1.desktop");
  DesktopEntry entry(path.c_str());
  ASSERT_FALSE(entry.IsValid());
  EXPECT_TRUE(entry.GetType() == DesktopEntry::UNKNOWN);

  g_file_manager.WriteFile("url-good.desktop", kDesktopEntryURLGood, true);
  path = g_file_manager.GetFullPath("url-good.desktop");
  ASSERT_TRUE(entry.Load(path.c_str()));
  ASSERT_TRUE(entry.IsValid());
  EXPECT_TRUE(entry.GetType() == DesktopEntry::LINK);

  g_file_manager.WriteFile("bad2.desktop", kDesktopEntryBad2, true);
  path = g_file_manager.GetFullPath("bad2.desktop");
  ASSERT_FALSE(entry.Load(path.c_str()));
  ASSERT_FALSE(entry.IsValid());
  EXPECT_TRUE(entry.GetType() == DesktopEntry::UNKNOWN);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  if (!CreateTempDirectory("desktop-entry-test", &g_temp_dir) ||
      !g_file_manager.Init(g_temp_dir.c_str(), false)) {
    fprintf(stderr, "Failed to create temp directory.\n");
    return 1;
  }
  int result = RUN_ALL_TESTS();
  RemoveDirectory(g_temp_dir.c_str(), true);
  return result;
}
