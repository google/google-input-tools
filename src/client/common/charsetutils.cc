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

// This file contains the implementation of CharsetUtils classes, which declares
// static methods for charset operations.  See comments before each method
// for details.

#include "common/charsetutils.h"

namespace ime_goopy {
// Convert Simplified Chinese string into Traditional Chinese
// source: Simplified Chinese string
// target: buffer to translate result
std::wstring CharsetUtils::Simplified2Traditional(const std::wstring& source) {
  int len = static_cast<int>(source.size());
  WCHAR* target_str = new WCHAR[len + 1];
  // translate simplified chinese into traditional chinese
  LCID lcid = MAKELCID(
      MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
      SORT_CHINESE_PRCP);
  LCMapString(lcid, LCMAP_TRADITIONAL_CHINESE,
              source.c_str(), len, target_str, len);

  target_str[len]= L'\0';
  std::wstring target = target_str;
  delete[] target_str;
  return target;
}

// Convert Traditional Chinese string into Simplified Chinese
// source: Traditional Chinese string
// target: buffer to translate result
std::wstring CharsetUtils::Traditional2Simplified(const std::wstring& source) {
  int len = static_cast<int>(source.size());
  WCHAR* target_str = new WCHAR[len + 1];
  // translate Traditional Chinese into Simplified Chinese
  LCID lcid = MAKELCID(
     MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
     SORT_CHINESE_BIG5);
  LCMapString(lcid, LCMAP_SIMPLIFIED_CHINESE,
              source.c_str(), len, target_str, len);
  target_str[len]= L'\0';
  std::wstring target = target_str;
  delete[] target_str;
  return target;
}

// Convert Unicode string into UTF8
// source: Traditional Chinese string
// target: buffer to translate result
std::wstring CharsetUtils::Unicode2UTF8Escaped(const std::wstring& source) {
  std::wstring output;
  int len = WideCharToMultiByte(CP_UTF8, 0, source.c_str(),
                                static_cast<int>(source.size()),
                                NULL, 0, NULL, NULL);
  if (len == 0) return output;
  char* utf8_string = new char[len];
  if (!utf8_string) return output;
  WideCharToMultiByte(CP_UTF8, 0, source.c_str(),
                      static_cast<int>(source.size()), utf8_string,
                      len, NULL, NULL);
  output = UTF8ToWstringEscaped(utf8_string, len);
  delete[] utf8_string;
  return output;
}

// Convert utf8 encoded string to wstring.
// source: utf8 encoded string buffer pointer
// length: buffer length
std::wstring CharsetUtils::UTF8ToWstringEscaped(
    const char* source, int length) {
  std::wstring output;
  wchar_t ch[10] = {0};
  for (int i = 0; i < length; ++i) {
    wsprintf(ch, L"%%%2X",
             static_cast<UINT>(static_cast<BYTE>(source[i])));
    output += ch;
  }
  return output;
}

}  // namespace ime_goopy
