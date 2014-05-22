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

#include "base/string_utils_win.h"

#include <windows.h>

#include "base/scoped_ptr.h"

namespace i18n_input {
namespace engine {

std::string WideToUtf8(const std::wstring& wide) {
  int wide_length = static_cast<int>(wide.length());
  int length = WideCharToMultiByte(
      CP_UTF8, 0, wide.data(), wide_length, NULL, 0, NULL, NULL);
  scoped_ptr<char[]> utf8(new char[length]);
  int result = WideCharToMultiByte(
      CP_UTF8, 0, wide.data(), wide_length, utf8.get(), length, NULL, NULL);
  if (result == 0) return "";
  std::string utf8_string(utf8.get(), length);
  return utf8_string;
}

std::wstring Utf8ToWide(const std::string& utf8) {
  int utf8_length = static_cast<int>(utf8.length());
  int length = MultiByteToWideChar(
      CP_UTF8, 0, utf8.data(), utf8_length, NULL, 0);
  scoped_ptr<wchar_t[]> wide(new wchar_t[length]);
  int result = MultiByteToWideChar(
      CP_UTF8, 0, utf8.data(), utf8_length, wide.get(), length);
  if (result == 0) return L"";
  std::wstring wide_string(wide.get(), length);
  return wide_string;
}

}  // namespace engine
}  // namespace i18n_input
