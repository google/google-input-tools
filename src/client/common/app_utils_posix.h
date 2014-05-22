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

#ifndef GOOPY_COMMON_APP_UTILS_POSIX_H_
#define GOOPY_COMMON_APP_UTILS_POSIX_H_

#include <algorithm>
#include <string>
#include "base/basictypes.h"
#include "common/windows_types.h"

namespace ime_goopy {
class AppUtils {
 public:

  static wstring PrefixedName(const wstring& id_string, const wstring& prefix) {
    wstring name(prefix + id_string);
    replace(name.begin(), name.end(), L'\\', L'/');
    return name.substr(0, MAX_PATH - 1);
  }

  // Returns current system date. e.g. "20120701".
  static int64 GetSystemDate();
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_APP_UTILS_POSIX_H_
