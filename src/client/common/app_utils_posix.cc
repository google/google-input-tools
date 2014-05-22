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

#include "common/app_utils.h"

#include <time.h>
#include <libgen.h>
#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/file/file_path.h"
#include "base/file/file_util.h"
#include "base/scoped_handle.h"
#include "common/app_const.h"

namespace ime_goopy {
static const wchar_t kDumpFolderName[] = L"Crash";
static const wchar_t kRegistryDataPathValue[] = L"DataPath";
static const wchar_t kRegistryPathValue[] = L"PATH";

static wstring CombinePath(const wstring& path,
                           const wstring& filename) {
  wchar_t result[MAX_PATH] = {0};
  swprintf(result, MAX_PATH, "%s/%s", path.c_str(), filename.c_str());
  return result;
}

unsigned int64 AppUtils::GetSystemDate() {
  time_t t = time(NULL);
  struct tm *aTime = localtime(&t);

  int day = aTime->tm_mday;
  int month = aTime->tm_mon + 1;
  int year = aTime->tm_year + 1900;

  return year * 10000 + month * 100 + day;
}

}  // namespace ime_goopy
