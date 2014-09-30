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

#include "common/debug.h"

#define APPEND_STRING_IF_BIT_SET(text, flag, test_bit) \
  if ((flag & test_bit) == test_bit) { \
    text.append(L#test_bit); \
    text.append(L" | "); \
  }

#define CASE_RETURN_STRING(test_value) \
  case test_value: return L#test_value;

namespace ime_goopy {
static const wchar_t kUnknownFlag[] = L"UNKNOWN_FLAG: %x!";
static const wchar_t kNullFlag[] = L"NULL";

const char* Debug::FindFunctionName(const char* full_name) {
  const char* name = strrchr(full_name, L':');
  if (name) return name + 1;
  return full_name;
}

}  // namespace ime_goopy
