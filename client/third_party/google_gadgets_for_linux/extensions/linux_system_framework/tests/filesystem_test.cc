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
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <unittest/gtest.h>
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

#define TEST_DIR_NAME "GGL_FileSystem_Test"
#define TEST_DIR "/tmp/" TEST_DIR_NAME

TEST(FileSystem, GetDrives) {
  FileSystem filesystem;
  DrivesInterface *drives = filesystem.GetDrives();
  EXPECT_TRUE(drives != NULL);
  EXPECT_EQ(1, drives->GetCount());
  DriveInterface *drive = drives->GetItem();
  EXPECT_TRUE(drive != NULL);
  drive->Destroy();
  EXPECT_FALSE(drives->AtEnd());
  drives->MoveNext();
  EXPECT_TRUE(drives->AtEnd());
  drives->Destroy();
}

TEST(FileSystem, BuildPath) {
  FileSystem filesystem;
  EXPECT_EQ(TEST_DIR "/file.cc", filesystem.BuildPath(TEST_DIR "/", "file.cc"));
  EXPECT_EQ(TEST_DIR "/file.cc", filesystem.BuildPath(TEST_DIR, "file.cc"));
  EXPECT_EQ("/file.cc", filesystem.BuildPath("/", "file.cc"));
  EXPECT_EQ("/tmp", filesystem.BuildPath("/tmp", ""));
  EXPECT_EQ("/tmp", filesystem.BuildPath("/tmp", NULL));
  EXPECT_EQ("", filesystem.BuildPath("", NULL));
  EXPECT_EQ("", filesystem.BuildPath(NULL, NULL));
}

// test method GetParentFolderName with valid arguments
TEST(FileSystem, GetParentFolderName) {
  FileSystem filesystem;
  EXPECT_EQ("/tmp", filesystem.GetParentFolderName("/tmp/test/"));
  EXPECT_EQ("/tmp", filesystem.GetParentFolderName("/tmp/test"));
  EXPECT_EQ("/", filesystem.GetParentFolderName("/tmp"));
  EXPECT_EQ("", filesystem.GetParentFolderName("/"));
  EXPECT_EQ("", filesystem.GetParentFolderName(""));
  EXPECT_EQ("", filesystem.GetParentFolderName(NULL));
}

// test method GetFileName
TEST(FileSystem, GetFileName) {
  FileSystem filesystem;
  EXPECT_EQ("file.cc", filesystem.GetFileName(TEST_DIR "/file.cc"));
  EXPECT_EQ(TEST_DIR_NAME, filesystem.GetFileName(TEST_DIR));
  EXPECT_EQ(TEST_DIR_NAME, filesystem.GetFileName(TEST_DIR "/"));
  EXPECT_EQ("", filesystem.GetFileName("/"));
  EXPECT_EQ("", filesystem.GetFileName(""));
  EXPECT_EQ("", filesystem.GetFileName(NULL));
}

// test method GetBaseName
TEST(FileSystem, GetBaseName) {
  FileSystem filesystem;
  EXPECT_EQ("file", filesystem.GetBaseName(TEST_DIR "/file.cc"));
  EXPECT_EQ("file", filesystem.GetBaseName(TEST_DIR "/file"));
  EXPECT_EQ("file", filesystem.GetBaseName(TEST_DIR "/file.cc/"));
  EXPECT_EQ("file", filesystem.GetBaseName(TEST_DIR "/file/"));
  EXPECT_EQ("", filesystem.GetFileName("/"));
  EXPECT_EQ("", filesystem.GetFileName(""));
  EXPECT_EQ("", filesystem.GetFileName(NULL));
}

// test method GetExtensionName
TEST(FileSystem, GetExtensionName) {
  FileSystem filesystem;
  EXPECT_EQ("cc", filesystem.GetExtensionName(TEST_DIR "/file.cc"));
  EXPECT_EQ("", filesystem.GetExtensionName(TEST_DIR "/file"));
  EXPECT_EQ("cc", filesystem.GetExtensionName(TEST_DIR "/file.cc/"));
  EXPECT_EQ("", filesystem.GetExtensionName(TEST_DIR "/file/"));
  EXPECT_EQ("file", filesystem.GetExtensionName(TEST_DIR "/.file"));
  EXPECT_EQ("", filesystem.GetExtensionName(TEST_DIR "/file."));
  EXPECT_EQ("", filesystem.GetExtensionName("/"));
  EXPECT_EQ("", filesystem.GetExtensionName(""));
  EXPECT_EQ("", filesystem.GetExtensionName(NULL));
}

// test method GetAbsolutePathName
TEST(FileSystem, GetAbsolutePathName) {
  FileSystem filesystem;
  // create the expected directory path
  char current_dir[PATH_MAX + 1];
  getcwd(current_dir, PATH_MAX);
  std::string dir(current_dir);
  if (dir[dir.size() - 1] == '/')
    dir = dir + std::string("file.cc");
  else
    dir = dir + "/"+ std::string("file.cc");
  EXPECT_EQ(dir, filesystem.GetAbsolutePathName("file.cc"));
}

// test method GetTempName
TEST(FileSystem, GetTempName) {
  FileSystem filesystem;
  std::string temp = filesystem.GetTempName();
  EXPECT_GT(temp.size(), 0U);
  LOG("Temp file name: %s", temp.c_str());
}

// test method FileExists
TEST(FileSystem, FileFolderExists) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file.cc", "wb");
  fclose(file);
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file.cc"));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists(""));
  EXPECT_FALSE(filesystem.FileExists(NULL));
  EXPECT_FALSE(filesystem.FolderExists(""));
  EXPECT_FALSE(filesystem.FolderExists(NULL));
  unlink(TEST_DIR "/file.cc");
  rmdir(TEST_DIR);
}

// test method GetFile.
TEST(FileSystem, GetFile) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file.cc", "wb");
  fclose(file);
  FileInterface *fi = filesystem.GetFile(TEST_DIR "/file.cc");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  EXPECT_TRUE(filesystem.GetFile(TEST_DIR) == NULL);
  EXPECT_TRUE(filesystem.GetFile(TEST_DIR "/file2.cc") == NULL);
  EXPECT_TRUE(filesystem.GetFile("") == NULL);
  EXPECT_TRUE(filesystem.GetFile(NULL) == NULL);
  unlink(TEST_DIR "/file.cc");
  rmdir(TEST_DIR);
}

// test method GetFolder.
TEST(FileSystem, GetFolder) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file.cc", "wb");
  fclose(file);
  FolderInterface *fi = filesystem.GetFolder(TEST_DIR "/");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  fi = filesystem.GetFolder(TEST_DIR);
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  fi = filesystem.GetFolder("/");
  EXPECT_TRUE(fi != NULL);
  fi->Destroy();
  EXPECT_TRUE(filesystem.GetFolder(TEST_DIR "/file.cc") == NULL);
  EXPECT_TRUE(filesystem.GetFolder(TEST_DIR "2") == NULL);
  EXPECT_TRUE(filesystem.GetFolder("") == NULL);
  EXPECT_TRUE(filesystem.GetFolder(NULL) == NULL);
  unlink(TEST_DIR "/file.cc");
  rmdir(TEST_DIR);
}

// tests method GetSpecialFolder
TEST(FileSystem, GetSpecialFolder) {
  FileSystem filesystem;
  FolderInterface *folder;
  EXPECT_TRUE((folder = filesystem.GetSpecialFolder(SPECIAL_FOLDER_WINDOWS))
              != NULL);
  folder->Destroy();
  EXPECT_TRUE((folder = filesystem.GetSpecialFolder(SPECIAL_FOLDER_SYSTEM))
              != NULL);
  folder->Destroy();
  EXPECT_TRUE((folder = filesystem.GetSpecialFolder(SPECIAL_FOLDER_TEMPORARY))
              != NULL);
  folder->Destroy();
}

// Tests DeleteFile.
TEST(FileSystem, DeleteFile) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file1.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file2.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file3.cc", "wb");
  fclose(file);

  EXPECT_EQ(0, chmod(TEST_DIR "/file1.cc", 0400));
  // Deletes a single file.
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file1.cc"));
  EXPECT_FALSE(filesystem.DeleteFile(TEST_DIR "/file1.cc", false));
  EXPECT_TRUE(filesystem.DeleteFile(TEST_DIR "/file1.cc", true));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file3.cc"));

  // Deletes file with wild characters.
  EXPECT_TRUE(filesystem.DeleteFile(TEST_DIR "/file*.cc", true));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file3.cc"));

  // Deletes non-existing file.
  EXPECT_FALSE(filesystem.DeleteFile(TEST_DIR "/file4.cc", true));

  // Deletes folder.
  EXPECT_FALSE(filesystem.DeleteFile(TEST_DIR, true));

  EXPECT_FALSE(filesystem.DeleteFile("", true));
  EXPECT_FALSE(filesystem.DeleteFile(NULL, true));

  rmdir(TEST_DIR);
}

#include <errno.h>
// test method DeleteFolder with existing folder
TEST(FileSystem, DeleteFolder) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  mkdir(TEST_DIR "/dir", 0700);
  FILE *file = fopen(TEST_DIR "/file1.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file2.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file3.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/dir/file4.cc", "wb");
  fclose(file);
  EXPECT_EQ(0, chmod(TEST_DIR "/dir/file4.cc", 0400));

  // Deletes files.
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/file1.cc", true));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/file2.cc", true));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/file3.cc", true));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/file4.cc", true));
  // Deletes folder.
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/dir", false));
  EXPECT_EQ(0, chmod(TEST_DIR, 0500));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/dir", false));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR "/dir", true));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR, false));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR, true));
  // Though the above DeleteFolder fails, file4.cc should be deleted.
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/dir/file4.cc"));
  file = fopen(TEST_DIR "/dir/file4.cc", "wb");
  fclose(file);
  EXPECT_EQ(0, chmod(TEST_DIR "/dir/file4.cc", 0400));
  EXPECT_EQ(0, chmod(TEST_DIR, 0700));
  EXPECT_FALSE(filesystem.DeleteFolder(TEST_DIR, false));
  EXPECT_TRUE(filesystem.DeleteFolder(TEST_DIR, true));

  EXPECT_FALSE(filesystem.FolderExists(TEST_DIR "/"));

  EXPECT_FALSE(filesystem.DeleteFolder("", true));
  EXPECT_FALSE(filesystem.DeleteFolder(NULL, true));
}

// test method MoveFile.
TEST(FileSystem, MoveFile) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  mkdir(TEST_DIR "2", 0700);
  FILE *file = fopen(TEST_DIR "/file1.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file2.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file3.cc", "wb");
  fclose(file);

  EXPECT_TRUE(filesystem.MoveFile(TEST_DIR "/file1.cc", TEST_DIR "/file1.cc"));

  // Moves an existing file to a non-existing file.
  EXPECT_TRUE(filesystem.MoveFile(TEST_DIR "/file1.cc", TEST_DIR "/file4.cc"));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file4.cc"));

  // Moves an existing file to an existing file.
  EXPECT_FALSE(filesystem.MoveFile(TEST_DIR "/file2.cc", TEST_DIR "/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file3.cc"));

  // Moves an existing file to an existing folder.
  EXPECT_TRUE(filesystem.MoveFile(TEST_DIR "/file*.cc", TEST_DIR "2/"));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file3.cc"));
  EXPECT_FALSE(filesystem.FileExists(TEST_DIR "/file4.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/file4.cc"));

  filesystem.DeleteFolder(TEST_DIR, true);
  filesystem.DeleteFolder(TEST_DIR "2", true);

  EXPECT_FALSE(filesystem.MoveFile("", ""));
  EXPECT_FALSE(filesystem.MoveFile(NULL, NULL));
}

// test method MoveFolder.
TEST(FileSystem, MoveFolder) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  mkdir(TEST_DIR "2", 0700);
  FILE *file = fopen(TEST_DIR "/file1.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file2.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file3.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "3", "wb");
  fclose(file);

  EXPECT_TRUE(filesystem.MoveFolder(TEST_DIR, TEST_DIR));
  EXPECT_TRUE(filesystem.MoveFolder(TEST_DIR "/", TEST_DIR));
  EXPECT_FALSE(filesystem.MoveFolder(TEST_DIR "/", TEST_DIR "/"));

  // Moves a folder into its sub-folder.
  EXPECT_FALSE(filesystem.MoveFolder(TEST_DIR "/", TEST_DIR "/subfolder"));
  EXPECT_FALSE(filesystem.MoveFolder(TEST_DIR "/", TEST_DIR "/subfolder/"));
  // Moves a folder into another folder.
  EXPECT_FALSE(filesystem.MoveFolder(TEST_DIR "/", TEST_DIR "2"));
  EXPECT_TRUE(filesystem.MoveFolder(TEST_DIR "/", TEST_DIR "2/"));
  EXPECT_FALSE(filesystem.FolderExists(TEST_DIR));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "2/GGL_FileSystem_Test"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/" TEST_DIR_NAME "/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/" TEST_DIR_NAME "/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/" TEST_DIR_NAME "/file3.cc"));

  // Moves a folder into another folder and rename.
  EXPECT_TRUE(filesystem.MoveFolder(TEST_DIR "2/" TEST_DIR_NAME,
                                    TEST_DIR "4"));
  EXPECT_FALSE(filesystem.FolderExists(TEST_DIR "2/" TEST_DIR_NAME));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "4"));

  // Moves a folder into another folder and rename.
  EXPECT_FALSE(filesystem.MoveFolder(TEST_DIR "4", TEST_DIR "3"));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "4"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "3"));

  filesystem.DeleteFolder(TEST_DIR "4", true);
  filesystem.DeleteFolder(TEST_DIR "2", true);
  unlink(TEST_DIR "3");

  EXPECT_FALSE(filesystem.MoveFolder("", ""));
  EXPECT_FALSE(filesystem.MoveFolder(NULL, NULL));
}

// test method CopyFile
TEST(FileSystem, CopyFile) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  mkdir(TEST_DIR "2", 0700);
  FILE *file = fopen(TEST_DIR "/file1.cc", "wb");
  fputs("test", file);
  fclose(file);
  file = fopen(TEST_DIR "/file2.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file3.cc", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.CopyFile(TEST_DIR "/file1.cc", TEST_DIR "/file1.cc",
                                   false));
  EXPECT_TRUE(filesystem.CopyFile(TEST_DIR "/file1.cc", TEST_DIR "/file1.cc",
                                  true));

  // Copies an existing file to a non-existing file.
  EXPECT_TRUE(filesystem.CopyFile(TEST_DIR "/file1.cc", TEST_DIR "/file4.cc",
                                  false));
  file = fopen(TEST_DIR "/file4.cc", "rb");
  char buffer[32];
  EXPECT_EQ(4U, fread(buffer, 1, 32, file));
  fclose(file);
  EXPECT_EQ("test", std::string(buffer, 4));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file4.cc"));

  // Copies an existing file to an existing file.
  EXPECT_FALSE(filesystem.CopyFile(TEST_DIR "/file2.cc", TEST_DIR "/file3.cc",
                                   false));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file3.cc"));
  EXPECT_TRUE(filesystem.CopyFile(TEST_DIR "/file2.cc", TEST_DIR "/file3.cc",
                                  true));

  // Copies an existing file to an existing folder.
  EXPECT_TRUE(filesystem.CopyFile(TEST_DIR "/file*.cc", TEST_DIR "2", false));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "/file4.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/file3.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/file4.cc"));

  EXPECT_FALSE(filesystem.CopyFile(TEST_DIR "/file*.cc", TEST_DIR "2/", false));
  EXPECT_TRUE(filesystem.CopyFile(TEST_DIR "/file*.cc", TEST_DIR "2/", true));

  filesystem.DeleteFolder(TEST_DIR, true);
  filesystem.DeleteFolder(TEST_DIR "2", true);

  EXPECT_FALSE(filesystem.CopyFile("", "", false));
  EXPECT_FALSE(filesystem.CopyFile(NULL, NULL, false));
}

// test method CopyFolder.
TEST(FileSystem, CopyFolder) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  mkdir(TEST_DIR "2", 0700);
  FILE *file = fopen(TEST_DIR "/file1.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file2.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "/file3.cc", "wb");
  fclose(file);
  file = fopen(TEST_DIR "3", "wb");
  fclose(file);

  EXPECT_TRUE(filesystem.CopyFolder(TEST_DIR, TEST_DIR, true));
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR, TEST_DIR, false));
  EXPECT_TRUE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR, true));
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR, false));
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR "/", false));
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR "/", true));

  // Copies a folder into its sub-folder.
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR "/subfolder",
                                     false));
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR "/subfolder",
                                     true));
  // Copies a folder into another folder.
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR "2", false));
  EXPECT_TRUE(filesystem.CopyFolder(TEST_DIR "/", TEST_DIR "2/", false));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "2/" TEST_DIR_NAME));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/" TEST_DIR_NAME "/file1.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/" TEST_DIR_NAME "/file2.cc"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "2/" TEST_DIR_NAME "/file3.cc"));

  // Copies a folder into another folder and rename.
  EXPECT_TRUE(filesystem.CopyFolder(TEST_DIR "2/" TEST_DIR_NAME,
                                    TEST_DIR "4", false));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "2/" TEST_DIR_NAME));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "4"));

  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "2/" TEST_DIR_NAME, "/tmp/",
                                     false));
  EXPECT_TRUE(filesystem.CopyFolder(TEST_DIR "2/" TEST_DIR_NAME, "/tmp/",
                                    true));

  // Copies a folder into another folder and rename.
  EXPECT_FALSE(filesystem.CopyFolder(TEST_DIR "4", TEST_DIR "3", false));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "4"));
  EXPECT_TRUE(filesystem.FileExists(TEST_DIR "3"));

  filesystem.DeleteFolder(TEST_DIR "4", true);
  filesystem.DeleteFolder(TEST_DIR "2", true);
  filesystem.DeleteFolder(TEST_DIR, true);
  unlink(TEST_DIR "3");

  EXPECT_FALSE(filesystem.CopyFolder("", "", false));
  EXPECT_FALSE(filesystem.CopyFolder(NULL, NULL, false));
  EXPECT_FALSE(filesystem.CopyFolder("", "", true));
  EXPECT_FALSE(filesystem.CopyFolder(NULL, NULL, true));
}

// test method CreateFolder.
TEST(FileSystem, CreateFolder) {
  FileSystem filesystem;
  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file.cc", "wb");
  fclose(file);

  EXPECT_FALSE(filesystem.CreateFolder(TEST_DIR "/file.cc"));
  EXPECT_TRUE(filesystem.CreateFolder(TEST_DIR "/folder"));
  EXPECT_FALSE(filesystem.CreateFolder(""));
  EXPECT_FALSE(filesystem.CreateFolder(NULL));
  EXPECT_TRUE(filesystem.FolderExists(TEST_DIR "/folder"));

  filesystem.DeleteFolder(TEST_DIR, true);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  system("rm -rf " TEST_DIR "*");
  int result = RUN_ALL_TESTS();
  system("rm -rf " TEST_DIR "*");
  return result;
}
