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
#include "ggadget/build_config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <cstdio>
#include <stdlib.h>
#include <string>
#include "ggadget/common.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/logger.h"
#include "ggadget/system_file_functions.h"
#include "ggadget/system_utils.h"
#include "unittest/gtest.h"

#if defined(OS_WIN)
#include <Shlwapi.h>
#endif

#if defined(OS_WIN)
#define SEP "\\"
#elif defined(OS_POSIX)
#define SEP "/"
#endif
#define SEP2 SEP SEP
#define SEP3 SEP SEP2
using namespace ggadget;

TEST(SystemUtils, BuildPath) {
  EXPECT_STREQ(
      SEP "abc" SEP "def" SEP "ghi",
      BuildPath(kDirSeparatorStr, SEP2 "abc", "def" SEP, "ghi", NULL).c_str());
  EXPECT_STREQ(
      "hello/:world",
      BuildPath("/:", "hello", "", "world", NULL).c_str());
  EXPECT_STREQ(
      "hello",
      BuildPath("//", "hello", NULL).c_str());
  EXPECT_STREQ(
      SEP "usr" SEP "sbin" SEP "sudo",
      BuildPath(kDirSeparatorStr,
                SEP2 "usr", "sbin" SEP2, "sudo", NULL).c_str());
  EXPECT_STREQ(
      "//usr//sbin//a//sudo",
      BuildPath("//", "//usr", "//", "sbin", "////a//", "sudo", NULL).c_str());
  EXPECT_STREQ(
      "//usr",
      BuildPath("//", "////", "//////", "usr//", "////", "////", NULL).c_str());
}


TEST(SystemUtils, SplitFilePath) {
  std::string dir;
  std::string file;
#if defined(OS_WIN)
#define ROOT_PATH "C:\\"
  EXPECT_FALSE(SplitFilePath("C:", &dir, &file));
  EXPECT_STREQ("C:", dir.c_str());
  EXPECT_STREQ("", file.c_str());
  EXPECT_TRUE(SplitFilePath("C:some_file", &dir, &file));
  EXPECT_STREQ("C:", dir.c_str());
  EXPECT_STREQ("some_file", file.c_str());
  EXPECT_TRUE(SplitFilePath("D:\\\\some_file", &dir, &file));
  EXPECT_STREQ("D:\\", dir.c_str());
  EXPECT_STREQ("some_file", file.c_str());
#elif defined(OS_POSIX)
#define ROOT_PATH "/"
#endif
  EXPECT_FALSE(SplitFilePath(ROOT_PATH, &dir, &file));
  EXPECT_STREQ(ROOT_PATH, dir.c_str());
  EXPECT_STREQ("", file.c_str());
  EXPECT_TRUE(SplitFilePath(ROOT_PATH "tmp", &dir, &file));
  EXPECT_STREQ(ROOT_PATH, dir.c_str());
  EXPECT_STREQ("tmp", file.c_str());
  EXPECT_TRUE(SplitFilePath(ROOT_PATH "foo" SEP "bar" SEP "file", &dir, &file));
  EXPECT_STREQ(ROOT_PATH "foo" SEP "bar", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_FALSE(SplitFilePath("file", &dir, &file));
  EXPECT_STREQ("", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_FALSE(SplitFilePath("dir" SEP, &dir, &file));
  EXPECT_STREQ("dir", dir.c_str());
  EXPECT_STREQ("", file.c_str());
  EXPECT_TRUE(SplitFilePath("dir" SEP3 "file", &dir, &file));
  EXPECT_STREQ("dir", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
  EXPECT_TRUE(SplitFilePath(ROOT_PATH SEP2 "dir" SEP3 "file",
                            &dir, &file));
  EXPECT_STREQ(ROOT_PATH SEP2 "dir", dir.c_str());
  EXPECT_STREQ("file", file.c_str());
}

#if defined(OS_WIN)
static void CheckEnvironmentVariablePath(const char *env_variable,
                                         bool expectation) {
  ASSERT_TRUE(env_variable != NULL);
  const char *path = getenv(env_variable);
  if (path == NULL || *path == '\0') {
    LOG("Environment variable \"%s\" doesn't exist.", env_variable);
    return;  // skip the test
  }
  if (expectation) {
    EXPECT_TRUE(EnsureDirectories(path));
  } else {
    EXPECT_FALSE(EnsureDirectories(path));
  }
}
#endif

TEST(SystemUtils, EnsureDirectories) {
#if defined(OS_WIN)
  char temp_path[MAX_PATH];
  std::string test_home;
  ASSERT_NE(0, GetTempPathA(MAX_PATH, temp_path));
  test_home = temp_path;
  test_home += "\\TestEnsureDirectories";
  EXPECT_FALSE(EnsureDirectories(""));
  EXPECT_TRUE(EnsureDirectories("\\"));

  CheckEnvironmentVariablePath("SYSTEMDRIVE", true);
  CheckEnvironmentVariablePath("SYSTEMROOT", true);
  CheckEnvironmentVariablePath("ProgramFiles", true);
  CheckEnvironmentVariablePath("TEMP", true);

  EXPECT_FALSE(EnsureDirectories("AAA:\\"));
  EXPECT_FALSE(EnsureDirectories("1234:\\"));
  system(("rmdir /S/Q " + test_home).c_str());
  EXPECT_TRUE(EnsureDirectories(test_home.c_str()));
  system(("rmdir /S/Q " + test_home).c_str());
  EXPECT_TRUE(EnsureDirectories((test_home + "\\").c_str()));
  EXPECT_TRUE(EnsureDirectories((test_home + "\\a\\b\\c\\d\\e").c_str()));
  system(("echo.>" + test_home + "\\file").c_str());
  EXPECT_FALSE(EnsureDirectories((test_home + "\\file").c_str()));
  EXPECT_FALSE(EnsureDirectories((test_home + "\\file\\").c_str()));
  EXPECT_FALSE(EnsureDirectories((test_home + "\\file\\a\\b\\c").c_str()));

  char cwd[4096];
  ASSERT_TRUE(_getcwd(cwd, sizeof(cwd)));
  _chdir(test_home.c_str());
  EXPECT_TRUE(EnsureDirectories("a\\b\\c\\d\\e"));
  EXPECT_TRUE(EnsureDirectories("d\\e"));
  _chdir(cwd);
#elif defined(OS_POSIX)
#define TEST_HOME "/tmp/TestEnsureDirectories"
  EXPECT_FALSE(EnsureDirectories(""));
  // NOTE: The following tests are Unix/Linux specific.
  EXPECT_TRUE(EnsureDirectories("/etc"));
  EXPECT_FALSE(EnsureDirectories("/etc/hosts"));
  EXPECT_FALSE(EnsureDirectories("/etc/hosts/anything"));
  EXPECT_TRUE(EnsureDirectories("/tmp"));
  EXPECT_TRUE(EnsureDirectories("/tmp/"));
  system("rm -rf " TEST_HOME);
  EXPECT_TRUE(EnsureDirectories(TEST_HOME));
  system("rm -rf " TEST_HOME);
  EXPECT_TRUE(EnsureDirectories(TEST_HOME "/"));
  EXPECT_TRUE(EnsureDirectories(TEST_HOME "/a/b/c/d/e"));
  system("touch " TEST_HOME "/file");
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file"));
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file/"));
  EXPECT_FALSE(EnsureDirectories(TEST_HOME "/file/a/b/c"));

  char cwd[4096];
  ASSERT_TRUE(getcwd(cwd, sizeof(cwd)));
  chdir(TEST_HOME);
  EXPECT_TRUE(EnsureDirectories("a/b/c/d/e"));
  EXPECT_TRUE(EnsureDirectories("d/e"));
  chdir(cwd);
#endif
}

TEST(SystemUtils, GetCurrentDirectory) {
  std::string curdir = GetCurrentDirectory();
  ASSERT_TRUE(curdir.length() > 0);
#if defined(OS_WIN)
  char temp_path[MAX_PATH];
  ASSERT_NE(0, GetTempPathA(MAX_PATH, temp_path));
  ASSERT_EQ(0, _chdir(temp_path));
  EXPECT_EQ(NormalizeFilePath(temp_path),
            NormalizeFilePath(GetCurrentDirectory().c_str()));
  ASSERT_EQ(0, _chdir(curdir.c_str()));
#elif defined(OS_POSIX)
  ASSERT_EQ(0, chdir("/"));
  EXPECT_STREQ("/", GetCurrentDirectory().c_str());
  ASSERT_EQ(0, chdir(curdir.c_str()));
#endif
}

TEST(SystemUtils, CreateTempDirectory) {
  std::string path1;
  std::string path2;
  ASSERT_TRUE(CreateTempDirectory("abc", &path1));
  ASSERT_TRUE(CreateTempDirectory("abc", &path2));
  ASSERT_STRNE(path1.c_str(), path2.c_str());
  ASSERT_EQ(0, ggadget::access(path1.c_str(), R_OK|X_OK|W_OK|F_OK));
  ASSERT_EQ(0, ggadget::access(path2.c_str(), R_OK|X_OK|W_OK|F_OK));

  ggadget::StatStruct stat_value;
  ASSERT_EQ(0, ggadget::stat(path1.c_str(), &stat_value));
  ASSERT_TRUE(S_ISDIR(stat_value.st_mode));
  ASSERT_EQ(0, ggadget::stat(path2.c_str(), &stat_value));
  ASSERT_TRUE(S_ISDIR(stat_value.st_mode));

#if defined(OS_WIN)
  _rmdir(path1.c_str());
  _rmdir(path2.c_str());
#elif defined(OS_POSIX)
  rmdir(path1.c_str());
  rmdir(path2.c_str());
#endif
}

TEST(SystemUtils, RemoveDirectory) {
#if defined(OS_WIN)
#define CREATE_FILE_COMMAND "echo.> "
#elif defined(OS_POSIX)
#define CREATE_FILE_COMMAND "touch "
#endif
  std::string tempdir;
  ASSERT_TRUE(CreateTempDirectory("removeme", &tempdir));
  std::string subdir = BuildFilePath(tempdir.c_str(), "subdir", NULL);
  std::string file = BuildFilePath(tempdir.c_str(), "file", NULL);
  std::string subfile = BuildFilePath(subdir.c_str(), "file", NULL);
  ASSERT_EQ(0, system(("mkdir " + subdir).c_str()));
  ASSERT_EQ(0, system((CREATE_FILE_COMMAND + file).c_str()));
  ASSERT_EQ(0, system((CREATE_FILE_COMMAND + subfile).c_str()));
  ASSERT_TRUE(RemoveDirectory(tempdir.c_str(), true));

  ASSERT_TRUE(CreateTempDirectory("removeme1", &tempdir));
  subdir = BuildFilePath(tempdir.c_str(), "subdir", NULL);
  file = BuildFilePath(tempdir.c_str(), "file", NULL);
  subfile = BuildFilePath(subdir.c_str(), "file", NULL);
  ASSERT_EQ(0, system(("mkdir " + subdir).c_str()));
  ASSERT_EQ(0, system((CREATE_FILE_COMMAND + file).c_str()));
  ASSERT_EQ(0, system((CREATE_FILE_COMMAND + subfile).c_str()));
#if defined(OS_WIN)
  ASSERT_EQ(0, _chmod(subfile.c_str(), _S_IREAD));
#elif defined(OS_POSIX)
  ASSERT_EQ(0, system(("chmod a-w " + subfile).c_str()));
#endif
  ASSERT_FALSE(RemoveDirectory(tempdir.c_str(), false));
  ASSERT_TRUE(RemoveDirectory(tempdir.c_str(), true));
}

TEST(SystemUtils, NormalizeFilePath) {
  ASSERT_STREQ(SEP, NormalizeFilePath("/").c_str());
  ASSERT_STREQ(SEP, NormalizeFilePath("//").c_str());
  ASSERT_STREQ(SEP "abc", NormalizeFilePath("/abc").c_str());
  ASSERT_STREQ(SEP "abc", NormalizeFilePath("/abc/").c_str());
  ASSERT_STREQ(SEP "abc", NormalizeFilePath("/abc/def/..").c_str());
  ASSERT_STREQ(SEP "abc", NormalizeFilePath("//abc/.///def/..").c_str());
  ASSERT_STREQ(SEP "abc", NormalizeFilePath("//abc/./def/../../abc/").c_str());
  ASSERT_STREQ(SEP, NormalizeFilePath("//abc/./def/../../").c_str());

  ASSERT_STREQ(SEP, NormalizeFilePath("\\").c_str());
  ASSERT_STREQ(SEP, NormalizeFilePath("\\\\").c_str());
  ASSERT_STREQ(SEP, NormalizeFilePath("\\\\abc\\.\\def\\..\\..\\").c_str());
}

#if defined(OS_POSIX)
TEST(Locales, GetSystemLocaleInfo) {
  std::string lang, terr;
  setlocale(LC_MESSAGES, "en_US.UTF-8");
  ASSERT_TRUE(GetSystemLocaleInfo(&lang, &terr));
  ASSERT_STREQ("en", lang.c_str());
  ASSERT_STREQ("US", terr.c_str());
  setlocale(LC_MESSAGES, "en_US");
  ASSERT_TRUE(GetSystemLocaleInfo(&lang, &terr));
  ASSERT_STREQ("en", lang.c_str());
  ASSERT_STREQ("US", terr.c_str());
}
#endif

#if defined(OS_WIN)
namespace {
struct SystemCodePageAndSampleFileName {
  const int code_page;
  const char* locale_file_name;
  const char* utf8_file_name;
};
static const SystemCodePageAndSampleFileName locale_test_cases[] = {
  {936, "\xCE\xC4\xBC\xFE\xBC\xD0", "\xE6\x96\x87\xE4\xBB\xB6\xE5\xA4\xB9"},
  {950, "\xC1\x63\xC5\xE9", "\xE7\xB9\x81\xE9\xAB\x94"},
};
}

TEST(SystemUtils, SystemIsNotUTF8Encoding) {
  std::string temp_root;
  ASSERT_TRUE(CreateTempDirectory("removeme", &temp_root));
  std::string temp_file = BuildFilePath(temp_root.c_str(), "temp", NULL);
  ASSERT_TRUE(WriteFileContents(temp_file.c_str(), "test"));
  for (size_t i = 0; i < arraysize(locale_test_cases); ++i) {
    // Test only when current active codepage is matched.
    if (GetACP() == locale_test_cases[i].code_page) {
      // Create a file with multiply character name
      std::string locale_path = temp_root + "\\" +
                                locale_test_cases[i].locale_file_name;
      std::string path = temp_root + "\\" + locale_test_cases[i].utf8_file_name;
      EXPECT_TRUE(::CopyFileA(temp_file.c_str(), locale_path.c_str(), false));

      // Verify ANSI API doesn't work for utf8 input in non-utf8 system
      WIN32_FIND_DATAA find_data = {0};
      HANDLE h_locale = ::FindFirstFileA(locale_path.c_str(), &find_data);
      EXPECT_NE(INVALID_HANDLE_VALUE, h_locale);  // locale path can work.
      ::FindClose(h_locale);
      HANDLE h_utf8 = ::FindFirstFileA(path.c_str(), &find_data);
      EXPECT_EQ(INVALID_HANDLE_VALUE, h_utf8);  // utf-8 path can not work.

      std::string data;
      EXPECT_TRUE(ReadFileContents(path.c_str(), &data));
      EXPECT_STREQ("test", data.c_str());
      EXPECT_TRUE(IsAbsolutePath(path.c_str()));
      std::string new_path = temp_root + "\\\\" +
                             locale_test_cases[i].utf8_file_name;
      EXPECT_STREQ(path.c_str(), NormalizeFilePath(new_path.c_str()).c_str());
      std::string file_name;
      EXPECT_TRUE(SplitFilePath(path.c_str(), NULL, &file_name));
      EXPECT_STREQ(locale_test_cases[i].utf8_file_name, file_name.c_str());

      // Create a folder with multiply character name
      std::string new_folder = temp_root + "\\folder_" +
                               locale_test_cases[i].utf8_file_name;
      EXPECT_TRUE(EnsureDirectories(new_folder.c_str()));
      char old_current_directory[MAX_PATH] = {0};
      ::GetCurrentDirectoryA(MAX_PATH, old_current_directory);
      std::string local_new_folder = temp_root + "\\folder_" +
                                     locale_test_cases[i].locale_file_name;
      EXPECT_TRUE(::SetCurrentDirectoryA(local_new_folder.c_str()));
      EXPECT_STREQ(new_folder.c_str(), GetCurrentDirectory().c_str());
      EXPECT_STREQ(
          (new_folder + "\\" + locale_test_cases[i].utf8_file_name).c_str(),
          GetAbsolutePath(locale_test_cases[i].utf8_file_name).c_str());
      EXPECT_TRUE(::SetCurrentDirectoryA(old_current_directory));
      std::string new_subfolder = new_folder + "\\" +
                                  locale_test_cases[i].utf8_file_name;
      EXPECT_TRUE(EnsureDirectories(new_subfolder.c_str()));
      std::string new_file_path = new_subfolder + "\\" +
                                  locale_test_cases[i].utf8_file_name;
      EXPECT_TRUE(WriteFileContents(new_file_path.c_str(),
                                    locale_test_cases[i].locale_file_name));
      EXPECT_TRUE(ReadFileContents(new_file_path.c_str(), &data));
      EXPECT_STREQ(locale_test_cases[i].locale_file_name, data.c_str());

      EXPECT_TRUE(RemoveDirectory(new_folder.c_str(), true));
    }
  }
  ASSERT_TRUE(RemoveDirectory(temp_root.c_str(), true));
}
#endif

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
