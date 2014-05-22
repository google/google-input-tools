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

// This file contains the declaration of CharsetUtils classes, which declares
// static methods for charset operation.

#ifndef GOOPY_COMMON_CHARSETUTILS_H__
#define GOOPY_COMMON_CHARSETUTILS_H__

#include <windows.h>
#include <string>

#include "base/basictypes.h"

namespace ime_goopy {
class CharsetUtils {
 public:
  // convert traditional chinese into simplified chinese
  static std::wstring Traditional2Simplified(const std::wstring& source);

  // convert simplified chinese into traditional chinese
  static std::wstring Simplified2Traditional(const std::wstring& source);

  // convert Unicode into UTF8 "%xx%xx" mode
  static std::wstring Unicode2UTF8Escaped(const std::wstring& source);

  // convert utf8 encoded string into UTF8 "%xx%xx" mode
  static std::wstring UTF8ToWstringEscaped(const char* source, int length);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CharsetUtils);
};
}  // ime_goopy
#endif  // GOOPY_COMMON_CHARSETUTILS_H__
