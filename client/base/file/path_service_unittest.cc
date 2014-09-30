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

#include "base/file/path_service.h"

#include "base/basictypes.h"
#include "base/file/base_paths.h"
#include "base/file/file_util.h"
#include "base/file/file_path.h"
#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif
#include "testing/base/public/gunit.h"

namespace {

// Returns true if PathService::Get returns true and sets the path parameter
// to non-empty for the given PathService::DirType enumeration value.
bool ReturnsValidPath(int dir_type) {
  FilePath path;
  bool result = PathService::Get(dir_type, &path);
#if defined(OS_POSIX)
  // If chromium has never been started on this account, the cache path may not
  // exist.
  if (dir_type == base::DIR_CACHE)
    return result && !path.value().empty();
#endif
  return result && !path.value().empty() && file_util::PathExists(path);
}

#if defined(OS_WIN)
// Function to test DIR_LOCAL_APP_DATA_LOW on Windows XP. Make sure it fails.
bool ReturnsInvalidPath(int dir_type) {
  FilePath path;
  bool result = PathService::Get(base::DIR_LOCAL_APP_DATA_LOW, &path);
  return !result && path.empty();
}
#endif

}  // namespace

// On the Mac this winds up using some autoreleased objects, so we need to
// be a PlatformTest.
typedef ::testing::Test PathServiceTest;

// Test that all PathService::Get calls return a value and a true result
// in the development environment.  (This test was created because a few
// later changes to Get broke the semantics of the function and yielded the
// correct value while returning false.)
TEST_F(PathServiceTest, Get) {
  for (int key = base::DIR_CURRENT; key < base::PATH_END; ++key) {
#if defined(OS_ANDROID)
    if (key == base::FILE_MODULE)
      continue;  // Android doesn't implement FILE_MODULE;
#endif
    EXPECT_PRED1(ReturnsValidPath, key);
  }
#if defined(OS_WIN)
  for (int key = base::PATH_WIN_START + 1; key < base::PATH_WIN_END; ++key) {
    if (key == base::DIR_LOCAL_APP_DATA_LOW &&
        base::win::GetVersion() < base::win::VERSION_VISTA) {
      // DIR_LOCAL_APP_DATA_LOW is not supported prior Vista and is expected to
      // fail.
      EXPECT_TRUE(ReturnsInvalidPath(key)) << key;
    } else {
      EXPECT_TRUE(ReturnsValidPath(key)) << key;
    }
  }
#elif defined(OS_MACOSX)
  for (int key = base::PATH_MAC_START + 1; key < base::PATH_MAC_END; ++key) {
      EXPECT_PRED1(ReturnsValidPath, key);
  }
#endif
}
