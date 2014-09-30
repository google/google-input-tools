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
#include <shlwapi.h>
#include <tchar.h>
#include <gtest/gunit.h>
#include "appsensorapi/versionreader.h"
#include "appsensorapi/common.h"

using ime_goopy::VersionReader;
using ime_goopy::VersionInfo;
using ime_goopy::FileInfoKey;

// Define all version info strings pairs. The former is the key to query in
// the version info. The latter is the value of the version info.
static const TCHAR *kInfoStringPairs[] = {
  FileInfoKey::kCompanyName, TEXT("Google Inc."),
  FileInfoKey::kComments, TEXT("This file is used for testing only."),
  FileInfoKey::kFileDescription, TEXT("This is a stub application."),
  FileInfoKey::kFileVersion, TEXT("1.0.0.41"),
  FileInfoKey::kInternalName, TEXT("StubApp"),
  FileInfoKey::kLegalCopyright, TEXT("Copyright 2007"),
  FileInfoKey::kOriginalFilename, TEXT("StubApp.exe"),
  FileInfoKey::kProductName, TEXT("Goopy"),
  FileInfoKey::kProductVersion, TEXT("2.0.0.0"),
  FileInfoKey::kPrivateBuild, TEXT("1.0.1.1"),
  FileInfoKey::kSpecialBuild, TEXT("1.0.1.2"),
  TEXT(""),     // Data ends with an empty string.
};

// Define the application name to retrieve version information from.
static const TCHAR* kCommandPath = TEXT("stubapp.exe");

TEST(VersionReaderTest, GetVersionInfoTest) {
  WCHAR real_path[MAX_PATH], real_path_combined[MAX_PATH];

  // Construct the absolute path of the application.
  _wgetcwd(real_path, MAX_PATH);
  PathCombine(real_path_combined, real_path, kCommandPath);
  PathCanonicalize(real_path, real_path_combined);

  // Retrieve the version information.
  VersionInfo version_info;
  version_info.file_size = 0;
  version_info.modified_time = 0;
  EXPECT_TRUE(VersionReader::GetVersionInfo(real_path, &version_info));

  // Validate the file size and modified time.
  EXPECT_GT(version_info.file_size, (size_t) 0);
  EXPECT_GT(version_info.modified_time, 0);

  // Validate all version info strings to be as expected.
  for (int i = 0; kInfoStringPairs[i][0] != TCHAR('\0'); i += 2) {
    EXPECT_EQ(wstring(kInfoStringPairs[i+1]),
              version_info.file_info[kInfoStringPairs[i]]);
  }
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
