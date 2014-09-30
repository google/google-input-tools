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
#include <locale.h>
#include "../file_system.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

#define TEST_DIR_NAME "GGL_FileSystem_Test"
#define TEST_DIR "/tmp/" TEST_DIR_NAME

TEST(FileSystem, OpenBinaryFile) {
  FileSystem filesystem;
  filesystem.DeleteFolder(TEST_DIR, true);

  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file.bin", "wb");
  fclose(file);

  // Opens an existing file for reading.
  BinaryStreamInterface *bi = filesystem.OpenBinaryFile(TEST_DIR "/file.bin",
                                                        IO_MODE_READING,
                                                        false);
  ASSERT_TRUE(bi != NULL);
  bi->Close();
  bi->Destroy();

  // Opens a non-existing file for reading.
  bi = filesystem.OpenBinaryFile(TEST_DIR "/file2.bin",
                                 IO_MODE_READING,
                                 false);
  ASSERT_TRUE(bi == NULL);

  // Opens a non-existing file for reading and create it.
  bi = filesystem.OpenBinaryFile(TEST_DIR "/file2.bin",
                                 IO_MODE_READING,
                                 true);
  ASSERT_TRUE(bi != NULL);
  bi->Close();
  bi->Destroy();

  // Opens an existing file for writing.
  bi = filesystem.CreateBinaryFile(TEST_DIR "/file.bin", false);
  ASSERT_TRUE(bi == NULL);

  // Opens an existing file for writing.
  bi = filesystem.CreateBinaryFile(TEST_DIR "/file.bin", true);
  ASSERT_TRUE(bi != NULL);
  bi->Close();
  bi->Destroy();

  filesystem.DeleteFolder(TEST_DIR, true);
}

TEST(FileSystem, Read) {
  FileSystem filesystem;
  filesystem.DeleteFolder(TEST_DIR, true);

  mkdir(TEST_DIR, 0700);
  FILE *file = fopen(TEST_DIR "/file.bin", "wb");
  char data[] = "0123456789";

  fwrite(data, 1, sizeof(data) - 1, file);
  fclose(file);

  // Opens an existing file for reading.
  BinaryStreamInterface *bi = filesystem.OpenBinaryFile(TEST_DIR "/file.bin",
                                                        IO_MODE_READING,
                                                        false);
  ASSERT_TRUE(bi != NULL);

  EXPECT_EQ(0, bi->GetPosition());
  EXPECT_FALSE(bi->IsAtEndOfStream());

  std::string result;
  EXPECT_TRUE(bi->Read(5, &result));
  EXPECT_EQ("01234", result);
  EXPECT_FALSE(bi->IsAtEndOfStream());
  EXPECT_EQ(5, bi->GetPosition());
  EXPECT_TRUE(bi->Read(2, &result));
  EXPECT_EQ("56", result);
  EXPECT_EQ(7, bi->GetPosition());
  EXPECT_FALSE(bi->IsAtEndOfStream());
  EXPECT_TRUE(bi->Skip(1));

  EXPECT_TRUE(bi->Read(1, &result));
  EXPECT_EQ("8", result);
  EXPECT_EQ(9, bi->GetPosition());
  EXPECT_TRUE(bi->ReadAll(&result));
  EXPECT_EQ("9", result);
  EXPECT_EQ(10, bi->GetPosition());
  EXPECT_TRUE(bi->IsAtEndOfStream());

  bi->Close();
  bi->Destroy();

  bi = filesystem.OpenBinaryFile(TEST_DIR "/file.bin",
                                 IO_MODE_READING,
                                 false);
  ASSERT_TRUE(bi != NULL);
  EXPECT_TRUE(bi->ReadAll(&result));
  EXPECT_EQ("0123456789", result);
  EXPECT_EQ(10, bi->GetPosition());
  EXPECT_TRUE(bi->IsAtEndOfStream());

  bi->Close();
  bi->Destroy();

  filesystem.DeleteFolder(TEST_DIR, true);
}

TEST(FileSystem, Write) {
  FileSystem filesystem;
  filesystem.DeleteFolder(TEST_DIR, true);

  mkdir(TEST_DIR, 0700);
  // Opens an existing file for reading.
  BinaryStreamInterface *bi = filesystem.CreateBinaryFile(TEST_DIR "/file.bin",
                                                          true);
  ASSERT_TRUE(bi != NULL);

  EXPECT_EQ(0, bi->GetPosition());

  EXPECT_TRUE(bi->Write("01234"));
  EXPECT_EQ(5, bi->GetPosition());
  EXPECT_TRUE(bi->IsAtEndOfStream());
  EXPECT_TRUE(bi->Write("56789"));
  EXPECT_EQ(10, bi->GetPosition());
  EXPECT_TRUE(bi->IsAtEndOfStream());

  bi->Close();
  bi->Destroy();

  char buffer[1024];
  FILE *file = fopen(TEST_DIR "/file.bin", "rb");
  size_t size = fread(buffer, 1, sizeof(buffer), file);
  EXPECT_EQ("0123456789", std::string(buffer, size));
  fclose(file);

  filesystem.DeleteFolder(TEST_DIR, true);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  setlocale(LC_ALL, "en_US.UTF-8");
  system("rm -rf " TEST_DIR "*");
  int result = RUN_ALL_TESTS();
  system("rm -rf " TEST_DIR "*");
  return result;
}
