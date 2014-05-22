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
#include <set>
#include <iostream>
#include <clocale>
#include <cstdlib>

#include "ggadget/common.h"
#include "ggadget/dir_file_manager.h"
#include "ggadget/file_manager_interface.h"
#include "ggadget/file_manager_wrapper.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/locales.h"
#include "ggadget/localized_file_manager.h"
#include "ggadget/logger.h"
#include "ggadget/scoped_ptr.h"
#include "ggadget/slot.h"
#include "ggadget/system_file_functions.h"
#include "ggadget/system_utils.h"
#include "ggadget/zip_file_manager.h"
#include "unittest/gtest.h"

#if defined(OS_WIN)
#include <time.h>
#include <sys/stat.h>
#include "third_party/unzip/zip.h"
#endif

using namespace ggadget;

#if defined(OS_WIN)
#define SEP "\\"
#elif defined(OS_POSIX)
#define SEP "/"
#endif

const char *base_dir_path = "file_manager_test_data_dest";
#if defined(OS_WIN)
// Check if non-ASCII file path can be handled correctly.
const char *base_gg_path =
    "file_manager_\xE6\xB5\x8B\xE8\xAF\x95_data_dest.gg";
#elif defined(OS_POSIX)
const char *base_gg_path = "file_manager_test_data_dest.gg";
#endif

const char *base_new_dir_path = "file_manager_test_data_new";
const char *base_new_gg_path  = "file_manager_test_data_new.gg";

const char *filenames[] = {
    "main.xml",
    "strings.xml",
    "global_file",
    "1033"SEP"1033_file",
    "2052"SEP"2052_file",
    "en"SEP"en_file",
    "en"SEP"global_file",
    "zh_CN"SEP"2048_file",
    "zh_CN"SEP"big_file",
    "zh_CN"SEP"global_file",
    "zh_CN"SEP"zh-CN_file",
    "zh_CN"SEP"strings.xml",
};

#if defined(OS_WIN) && defined(GENERATE_ZIP_TEST_DATA)
static bool WriteFileToZipPackege(FileManagerInterface *fm,
                                  const std::string &source_path,
                                  const std::string &target_path,
                                  int prefix_length) {
  WIN32_FIND_DATAA file_data = {0};
  std::string path;
  if (source_path.size() != 0 &&
      source_path[source_path.size() - 1] != kDirSeparator) {
    path = source_path + kDirSeparatorStr + '*';
  } else {
    path = source_path + '*';
  }
  HANDLE handle = FindFirstFileA(path.c_str(), &file_data);
  if (handle != INVALID_HANDLE_VALUE) {
    do {
      if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0) {
        std::string source_file(BuildFilePath(source_path.c_str(),
                                              file_data.cFileName,
                                              NULL));
        std::string target_file(BuildFilePath(target_path.c_str(),
                                              file_data.cFileName,
                                              NULL));
        if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
          std::string data;
          if (!ReadFileContents(source_file.c_str(), &data) ||
              !fm->WriteFile(target_file.substr(prefix_length).c_str(),
                             data,
                             true)) {
            FindClose(handle);
            return false;
          }
        } else {
          if (*file_data.cFileName != '.' &&
              !WriteFileToZipPackege(fm,
                                     source_file,
                                     target_file,
                                     prefix_length)) {
            FindClose(handle);
            return false;
          }
        }
      }
    } while (FindNextFileA(handle, &file_data));
    FindClose(handle);
    return true;
  } else {
    DLOG("Failed to list directory %s: %h.", source_path, GetLastError());
    // Enumeration non-exist directory succeeds with empty result.
    return true;
  }
}

static void CreateZipPackege() {
  FileManagerInterface *written_fm = new ZipFileManager();
  std::string source_base_dir_path =
      BuildFilePath(GetCurrentDirectory().c_str(), base_dir_path, NULL);
  std::string target_base_dir_path =
      BuildFilePath(GetCurrentDirectory().c_str(), base_gg_path, NULL);
  if (written_fm->Init(base_gg_path, true)) {
    WriteFileToZipPackege(written_fm,
                          source_base_dir_path,
                          target_base_dir_path,
                          target_base_dir_path.length() + 1);
  }
  delete written_fm;
  return;
}
#endif  // OS_WIN && GENERATE_ZIP_TEST_DATA

void TestFileManagerReadFunctions(const std::string &prefix,
                                  const std::string &base_path,
                                  FileManagerInterface *fm, bool zip) {
  ASSERT_TRUE(fm->IsValid());
  std::string data;
  std::string full_base_path = BuildFilePath(GetCurrentDirectory().c_str(),
                                             base_path.c_str(), NULL);
  std::string path;
  std::string base_filename;
  EXPECT_EQ(full_base_path, fm->GetFullPath(prefix.c_str()));
  SplitFilePath(full_base_path.c_str(), &path, &base_filename);
  ASSERT_TRUE(base_path.length());
  ASSERT_TRUE(fm->ReadFile((prefix + "global_file").c_str(), &data));
  EXPECT_STREQ("global_file at top\n", data.c_str());

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN"SEP".."SEP"global_file").c_str(),
                            &data));
  EXPECT_STREQ("global_file at top\n", data.c_str());

  EXPECT_FALSE(fm->ReadFile((prefix + "non-exists").c_str(), &data));

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN"SEP"zh-CN_file").c_str(), &data));
  EXPECT_STREQ("zh-CN_file contents\n", data.c_str());

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN"SEP"2048_file").c_str(), &data));
  EXPECT_EQ(2048U, data.size());

  ASSERT_TRUE(fm->ReadFile((prefix + "zh_CN"SEP"big_file").c_str(), &data));
  EXPECT_EQ(32616U, data.size());

  EXPECT_TRUE(fm->FileExists((prefix + "global_file").c_str(), &path));
  EXPECT_STREQ((full_base_path + SEP"global_file").c_str(),
               path.c_str());
  EXPECT_STREQ(fm->GetFullPath((prefix + "global_file").c_str()).c_str(),
               path.c_str());

  EXPECT_FALSE(fm->FileExists((prefix + "non-exists").c_str(), &path));
  EXPECT_STREQ((full_base_path + SEP"non-exists").c_str(),
               path.c_str());
  EXPECT_STREQ(fm->GetFullPath((prefix + "non-exists").c_str()).c_str(),
               path.c_str());

  EXPECT_FALSE(fm->FileExists((".."SEP"" + base_filename).c_str(), &path));
  EXPECT_STREQ(full_base_path.c_str(), path.c_str());

  if (zip) {
    ASSERT_TRUE(fm->ReadFile((prefix + "gLoBaL_FiLe").c_str(), &data));
    EXPECT_STREQ((full_base_path + SEP"gLoBaL_FiLe").c_str(),
                 fm->GetFullPath((prefix + "gLoBaL_FiLe").c_str()).c_str());
    EXPECT_STREQ("global_file at top\n", data.c_str());
    EXPECT_TRUE(fm->FileExists((prefix + "1033"SEP"1033_FiLe").c_str(), &path));
    EXPECT_STREQ((full_base_path + SEP"1033"SEP"1033_FiLe").c_str(),
                 path.c_str());
    EXPECT_FALSE(fm->IsDirectlyAccessible((prefix + "gLoBaL_FiLe").c_str(),
                                          &path));
    EXPECT_STREQ((full_base_path + SEP"gLoBaL_FiLe").c_str(),
                 path.c_str());
  } else {
    // Case sensitive may not be available on some platforms.
    //EXPECT_FALSE(fm->FileExists("1033/1033_FiLe", &path));
    //EXPECT_STREQ((base_path + "/1033/1033_FiLe").c_str(), path.c_str());
    EXPECT_TRUE(fm->IsDirectlyAccessible((prefix + "gLoBaL_FiLe").c_str(),
                                         &path));
    EXPECT_STREQ((full_base_path + SEP"gLoBaL_FiLe").c_str(),
                 path.c_str());
  }
}

void TestFileManagerWriteFunctions(const std::string &prefix,
                                   const std::string &base_path,
                                   FileManagerInterface *fm, bool zip) {
  ASSERT_TRUE(fm->IsValid());
  std::string data;
  std::string path;
  std::string path2;
  std::string full_base_path = BuildFilePath(GetCurrentDirectory().c_str(),
                                             base_path.c_str(), NULL);
  // Test write file in top dir.
  data = "new_file contents\n";
  uint64_t t = time(NULL) * UINT64_C(1000);
  ASSERT_TRUE(fm->WriteFile((prefix + "new_file").c_str(), data, false));
  ASSERT_TRUE(fm->FileExists((prefix + "new_file").c_str(), &path));
  EXPECT_LE(abs(static_cast<int>(
      fm->GetLastModifiedTime((prefix + "new_file").c_str()) - t)), 1000);
  EXPECT_STREQ((full_base_path + SEP"new_file").c_str(), path.c_str());
  ASSERT_TRUE(fm->ReadFile((prefix + "new_file").c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  path.clear();
  ASSERT_TRUE(fm->ExtractFile((prefix + "new_file").c_str(), &path));
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  ggadget::unlink(path.c_str());
  EXPECT_FALSE(ReadFileContents(path.c_str(), &data));
  path2 = path;
  ASSERT_TRUE(fm->ExtractFile((prefix + "new_file").c_str(), &path));
  EXPECT_STREQ(path2.c_str(), path.c_str());
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("new_file contents\n", data.c_str());
  ASSERT_TRUE(fm->FileExists((prefix + "new_file").c_str(), &path));

  // Test write file in sub dir.
  data = "en_new_file contents\n";
  ASSERT_TRUE(fm->WriteFile((prefix + "en"SEP"new_file").c_str(), data, false));
  ASSERT_TRUE(fm->FileExists((prefix + "en"SEP"new_file").c_str(), &path));
  EXPECT_STREQ((full_base_path + SEP"en"SEP"new_file").c_str(),
               path.c_str());
  ASSERT_TRUE(fm->ReadFile((prefix + "en"SEP"new_file").c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  path.clear();
  ASSERT_TRUE(fm->ExtractFile((prefix + "en"SEP"new_file").c_str(), &path));
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  ggadget::unlink(path.c_str());
  EXPECT_FALSE(ReadFileContents(path.c_str(), &data));
  path2 = path;
  ASSERT_TRUE(fm->ExtractFile((prefix + "en"SEP"new_file").c_str(), &path));
  EXPECT_STREQ(path2.c_str(), path.c_str());
  ASSERT_TRUE(ReadFileContents(path.c_str(), &data));
  EXPECT_STREQ("en_new_file contents\n", data.c_str());
  ASSERT_TRUE(fm->FileExists((prefix + "en"SEP"new_file").c_str(), &path));

  // Test overwrite an existing file.
  EXPECT_FALSE(fm->WriteFile((prefix + "en"SEP"new_file").c_str(),
                             data, false));

  EXPECT_TRUE(fm->WriteFile((prefix + "en"SEP"new_file").c_str(), data, true));
  EXPECT_TRUE(fm->RemoveFile((prefix + "new_file").c_str()));
  EXPECT_TRUE(fm->RemoveFile((prefix + "en"SEP"new_file").c_str()));
  EXPECT_FALSE(fm->FileExists((prefix + "new_file").c_str(), &path));
  EXPECT_FALSE(fm->FileExists((prefix + "en"SEP"new_file").c_str(), &path));

  // Test write a file with non-ASCII filename.
  data = "\xE6\xB5\x8B\xE8\xAF\x95_file contents\n";
  ASSERT_TRUE(fm->WriteFile((prefix + "\xE6\xB5\x8B\xE8\xAF\x95_file").c_str(),
                            data, false));
  ASSERT_TRUE(fm->FileExists((prefix + "\xE6\xB5\x8B\xE8\xAF\x95_file").c_str(),
                             &path));
  EXPECT_STREQ((full_base_path + SEP"\xE6\xB5\x8B\xE8\xAF\x95_file").c_str(),
               path.c_str());
  ASSERT_TRUE(fm->ReadFile((prefix + "\xE6\xB5\x8B\xE8\xAF\x95_file").c_str(),
                           &data));
  EXPECT_STREQ("\xE6\xB5\x8B\xE8\xAF\x95_file contents\n", data.c_str());
  EXPECT_TRUE(
      fm->RemoveFile((prefix + "\xE6\xB5\x8B\xE8\xAF\x95_file").c_str()));
}

std::set<std::string> actual_set;
bool EnumerateCallback(const char *name) {
  LOG("EnumerateCallback: %"PRIuS" %s", actual_set.size(), name);
  EXPECT_TRUE(actual_set.find(name) == actual_set.end());
  actual_set.insert(name);
  return true;
}

void TestFileManagerEnumerate(FileManagerInterface *fm) {
  actual_set.clear();
  std::set<std::string> expected_set;
  for (size_t i = 0; i < arraysize(filenames); i++)
    expected_set.insert(filenames[i]);
  EXPECT_TRUE(fm->EnumerateFiles("", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);

  actual_set.clear();
  expected_set.clear();
  expected_set.insert("2048_file");
  expected_set.insert("big_file");
  expected_set.insert("global_file");
  expected_set.insert("zh-CN_file");
  expected_set.insert("strings.xml");
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN"SEP"", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
}

void TestFileManagerLocalized(FileManagerInterface *fm,
                              const std::string &locale,
                              const std::string &alternative_locale) {
  std::string contents(" contents\n");
  std::string data;
  std::string filename;

  filename = locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  filename = alternative_locale + "_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  filename = "en_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  filename = "1033_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + contents).c_str(), data.c_str());

  // Global files always have higher priority.
  filename = "global_file";
  ASSERT_TRUE(fm->ReadFile(filename.c_str(), &data));
  EXPECT_STREQ((filename + " at top\n").c_str(), data.c_str());
}

TEST(FileManager, DirRead) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_dir_path, false));
  TestFileManagerReadFunctions("", base_dir_path, fm, false);
  delete fm;
}

TEST(FileManager, ZipRead) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_gg_path, false));
  TestFileManagerReadFunctions("", base_gg_path, fm, true);
  delete fm;
}

TEST(FileManager, DirWrite) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_new_dir_path, true));
  TestFileManagerWriteFunctions("", base_new_dir_path, fm, false);
  delete fm;
  RemoveDirectory(base_new_dir_path, true);
}

TEST(FileManager, ZipWrite) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_new_gg_path, true));
  TestFileManagerWriteFunctions("", base_new_gg_path, fm, true);
  delete fm;
  ggadget::unlink(base_new_gg_path);
}

TEST(FileManager, DirEnumerate) {
  FileManagerInterface *fm = new DirFileManager();
  ASSERT_TRUE(fm->Init(base_dir_path, true));
  TestFileManagerEnumerate(fm);
  delete fm;
}

TEST(FileManager, ZipEnumerate) {
  FileManagerInterface *fm = new ZipFileManager();
  ASSERT_TRUE(fm->Init(base_gg_path, true));
  TestFileManagerEnumerate(fm);
  delete fm;
}

TEST(FileManager, LocalizedFile) {
  static const char *locales_full[] = { "en_US", "zh_CN.UTF8", NULL };
  static const char *locales[] = { "en", "zh-CN", NULL };
  static const char *alt_locales[] = { "1033", "2052", NULL };

  for (size_t i = 0; locales[i]; ++i) {
    SetLocaleForUiMessage(locales_full[i]);

    FileManagerInterface *fm = new LocalizedFileManager(new DirFileManager());
    ASSERT_TRUE(fm->Init(base_dir_path, false));
    TestFileManagerLocalized(fm, locales[i], alt_locales[i]);
    delete fm;

    fm = new LocalizedFileManager(new ZipFileManager());
    ASSERT_TRUE(fm->Init(base_gg_path, false));
    TestFileManagerLocalized(fm, locales[i], alt_locales[i]);
    delete fm;
  }
}

TEST(FileManager, FileManagerWrapper) {
  FileManagerWrapper *fm = new FileManagerWrapper();
  FileManagerInterface *dir_fm = new DirFileManager();
  ASSERT_TRUE(dir_fm->Init(base_dir_path, true));
  FileManagerInterface *zip_fm = new ZipFileManager();
  ASSERT_TRUE(zip_fm->Init(base_gg_path, true));
  FileManagerInterface *dir_write_fm = new DirFileManager();
  ASSERT_TRUE(dir_write_fm->Init(base_new_dir_path, true));
  FileManagerInterface *zip_write_fm = new ZipFileManager();
  ASSERT_TRUE(zip_write_fm->Init(base_new_gg_path, true));
  EXPECT_TRUE(fm->RegisterFileManager("", dir_fm));
  EXPECT_TRUE(fm->RegisterFileManager("zip"SEP"", zip_fm));
  EXPECT_TRUE(fm->RegisterFileManager("dir_write"SEP"", dir_write_fm));
  EXPECT_TRUE(fm->RegisterFileManager("zip_write"SEP"", zip_write_fm));

  TestFileManagerReadFunctions("", base_dir_path, fm, false);
  TestFileManagerWriteFunctions("dir_write"SEP"", base_new_dir_path, fm, false);
  TestFileManagerReadFunctions("zip"SEP"", base_gg_path, fm, true);
  // TestFileManagerWriteFunctions("zip_write/", base_new_gg_path, fm, true);

  actual_set.clear();
  std::set<std::string> expected_set;
  for (size_t i = 0; i < arraysize(filenames); i++) {
    expected_set.insert(filenames[i]);
    expected_set.insert(std::string("zip"SEP"") + filenames[i]);
  }
  EXPECT_TRUE(fm->EnumerateFiles("", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);

  actual_set.clear();
  expected_set.clear();
  expected_set.insert("2048_file");
  expected_set.insert("big_file");
  expected_set.insert("global_file");
  expected_set.insert("zh-CN_file");
  expected_set.insert("strings.xml");
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip"SEP"zh_CN", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zh_CN"SEP"", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip"SEP"zh_CN"SEP"",
                                 NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);

  expected_set.clear();
  for (size_t i = 0; i < arraysize(filenames); i++)
    expected_set.insert(filenames[i]);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  actual_set.clear();
  EXPECT_TRUE(fm->EnumerateFiles("zip"SEP"", NewSlot(EnumerateCallback)));
  EXPECT_TRUE(expected_set == actual_set);
  delete fm;
  RemoveDirectory(base_new_dir_path, true);
  ggadget::unlink(base_new_gg_path);
}

#if defined(OS_WIN)
TEST(FileManager, OpenZipWithFileNameIncludingSlash) {
  const char* kZipPackageName = "zip_with_file_name_including_slash.zip";
  const char* kZipFileNames[] = {
      "folder1/file1",
      "folder3/subfolder1/file2",
      "file3"
  };
  std::string path = BuildFilePath(GetCurrentDirectory().c_str(),
                                   kZipPackageName,
                                   NULL);
  zipFile zip_handle = ::zipOpen(path.c_str(), APPEND_STATUS_CREATE);
  ASSERT_TRUE(zip_handle);
  zip_fileinfo info = {0};
  for (size_t i = 0; i < arraysize(kZipFileNames); ++i) {
    ASSERT_EQ(ZIP_OK, ::zipOpenNewFileInZip(zip_handle, kZipFileNames[i], &info,
                                            NULL, 0, NULL, 0, NULL, Z_DEFLATED,
                                            Z_DEFAULT_COMPRESSION));
    ASSERT_EQ(ZIP_OK, ::zipWriteInFileInZip(zip_handle, kZipFileNames[i],
                                            strlen(kZipFileNames[i])));
    ::zipCloseFileInZip(zip_handle);
  }
  ::zipClose(zip_handle, NULL);

  scoped_ptr<FileManagerInterface> fm(new ZipFileManager);
  ASSERT_TRUE(fm->Init(path.c_str(), false));
  std::string data;
  for (size_t i = 0; i < arraysize(kZipFileNames); ++i) {
    EXPECT_TRUE(fm->ReadFile(kZipFileNames[i], &data));
    EXPECT_STREQ(kZipFileNames[i], data.c_str());
    if (strchr(kZipFileNames[i], '/')) {
      std::string new_file_name(kZipFileNames[i]);
      for (size_t j = 0; j < new_file_name.size(); ++j) {
        if (new_file_name[j] == '/')
          new_file_name[j] = '\\';
      }
      EXPECT_TRUE(fm->ReadFile(new_file_name.c_str(), &data));
      EXPECT_STREQ(kZipFileNames[i], data.c_str());
    }
  }
  fm.reset();
  ggadget::unlink(path.c_str());
}
#endif

int main(int argc, char **argv) {
#ifdef GGADGET_FILE_MANAGER_TEST_WORK_DIRECTORY
  // Sets work directory if have
  std::string work_directory(GGADGET_FILE_MANAGER_TEST_WORK_DIRECTORY);
  SetCurrentDirectoryA(work_directory.c_str());
#endif
#if defined(OS_WIN) && defined(GENERATE_ZIP_TEST_DATA)
  ggadget::unlink(base_gg_path);
  CreateZipPackege();
#endif
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
