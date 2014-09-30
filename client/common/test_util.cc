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

#include "common/test_util.h"

namespace ime_goopy {
namespace testing {

wstring TestUtility::TestDataGetPath(const wstring& related_path) {
  wchar_t current_executable[MAX_PATH] = { '\0' };
  wchar_t file_path[MAX_PATH] = { '\0' };
  wchar_t full_path[MAX_PATH] = { '\0' };
  GetModuleFileName(NULL, current_executable, MAX_PATH);
  PathCombine(file_path, current_executable, L"..");
  PathCombine(full_path, file_path, related_path.c_str());
  return full_path;
}

wstring TestUtility::TempFile(const wstring& filename) {
  wchar_t temp_path[MAX_PATH] = { L'\0' };
  wchar_t full_path[MAX_PATH] = { L'\0' };
  GetTempPathW(MAX_PATH, temp_path);
  PathCombineW(full_path, temp_path, filename.c_str());
  return full_path;
}

}  // namespace testing
}  // namespace ime_goopy
