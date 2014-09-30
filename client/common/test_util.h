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
//
// This file contains the utility class to assist writing tests.

#ifndef GOOPY_COMMON_TEST_UTIL_H__
#define GOOPY_COMMON_TEST_UTIL_H__

#include <string>

#include "base/basictypes.h"

namespace ime_goopy {
namespace testing {

class TestUtility {
 public:
  // Returns the absolute path of given test data file.
  //
  // related_path: The path related to the directory containing test binary.
  static wstring TestDataGetPath(const wstring& related_path);

  // Returns the path for a temporary file to be created.
  //
  // filename: The filename to create.
  static wstring TempFile(const wstring& filename);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TestUtility);
};

}  // namespace testing
}  // namespace ime_goopy

#endif  // GOOPY_COMMON_TEST_UTIL_H__
