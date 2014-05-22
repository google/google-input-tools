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
#include "common/string_utils.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {

void SplitString(const wstring& raw,
                 const wstring& sep,
                 vector<wstring>* output) {
  output->clear();
  size_t start = 0;
  size_t end = 0;
  while ((end = raw.find(sep, start)) != wstring::npos) {
    output->push_back(raw.substr(start, end - start));
    start = end + sep.size();
  }
  if (start < raw.size()) {
    output->push_back(raw.substr(start));
  }
}

void SplitString(const string& raw,
                 const string& sep,
                 vector<string>* result) {
  result->clear();
  size_t start = 0;
  size_t end = 0;
  if (sep.empty()) {
    result->push_back(raw);
    return;
  }
  while ((end = raw.find(sep, start)) != string::npos) {
    if (end - start > 0) {  // skips consecutive separators.
      result->push_back(raw.substr(start, end - start));
    }
    start = end + sep.size();
  }
  if (start < raw.size()) {
    result->push_back(raw.substr(start));
  }
}

// NOTE(ygwang): The following two functions are required by Google homepage and
// search setting code ported from protector. The implemtations are directly
// ported from protector code, although they can be re-implemented with
// C++-style string manipulations.
// See googleclient/tactical/protector/lib/search_with_google_internal.cc

// Returns true if "text" ends on "ends".
bool StringEndsWith(const WCHAR* text, const WCHAR* ends) {
  if (!text) return false;
  if (!ends) return true;
  int ends_len = lstrlen(ends);
  int text_len = lstrlen(text);
  int offset = text_len - ends_len;
  if (offset < 0)
    return false;
  return wcsncmp(text + offset, ends, ends_len) == 0;
}

// Returns true if "text" ends on "ends".
bool StringEndsWith(const char* text, const char* ends) {
  if (!text) return false;
  if (!ends) return true;
  int ends_len = strlen(ends);
  int text_len = strlen(text);
  int offset = text_len - ends_len;
  if (offset < 0)
    return false;
  return strncmp(text + offset, ends, ends_len) == 0;
}

// Returns true if "text" begins with "begins".
bool StringBeginsWith(const WCHAR* text, const WCHAR* begins) {
  if (!text) return false;
  if (!begins) return true;
  int begins_length = lstrlen(begins);
  return wcsncmp(text, begins, begins_length) == 0;
}

// Returns true if "text" begins with "begins".
bool StringBeginsWith(const char* text, const char* begins) {
  if (!text) return false;
  if (!begins) return true;
  int begins_length = strlen(begins);
  return strncmp(text, begins, begins_length) == 0;
}

string WideToUtf8(const wstring& wide) {
  int wide_length = static_cast<int>(wide.length());
  int length = WideCharToMultiByte(
      CP_UTF8, 0, wide.data(), wide_length, NULL, 0, NULL, NULL);
  scoped_array<char> utf8(new char[length + 1]);
  int result = WideCharToMultiByte(
      CP_UTF8, 0, wide.data(), wide_length, utf8.get(), length, NULL, NULL);
  utf8[result] = '\0';
  string utf8_string(utf8.get(), result);
  return utf8_string;
}

wstring Utf8ToWide(const string& utf8) {
  int utf8_length = static_cast<int>(utf8.length());
  int length = MultiByteToWideChar(
      CP_UTF8, 0, utf8.data(), utf8_length, NULL, 0);
  scoped_array<wchar_t> wide(new wchar_t[length + 1]);
  int result = MultiByteToWideChar(
      CP_UTF8, 0, utf8.data(), utf8_length, wide.get(), length);
  wide[result] = L'\0';
  wstring wide_string(wide.get(), result);
  return wide_string;
}

string WideToACP(const wstring& wide) {
  int wide_length = static_cast<int>(wide.length());
  int length = WideCharToMultiByte(
    CP_ACP, 0, wide.data(), wide_length, NULL, 0, NULL, NULL);
  scoped_array<char> acp(new char[length + 1]);
  int result = WideCharToMultiByte(
      CP_ACP, 0, wide.data(), wide_length, acp.get(), length, NULL, NULL);
  acp[result] = '\0';
  return string(acp.get(), result);
}


string WideToCodePage(const wstring& text, int codepage) {
  int wide_length = static_cast<int>(text.length());
  int length = WideCharToMultiByte(codepage, 0, text.data(),
                                   wide_length, NULL, 0, NULL, NULL);

  scoped_array<char> acp(new char[length + 1]);
  int result = WideCharToMultiByte(codepage, 0, text.data(),
                                   wide_length, acp.get(), length, NULL, NULL);
  acp[result] = '\0';
  return string(acp.get(), result);
}

string CombinePathUtf8(const string& path, const string& filename) {
  char full_path[MAX_PATH] = { 0 };
  if (PathCombineA(full_path, path.c_str(), filename.c_str()))
    return string(full_path);
  return "";
}

wstring ToWindowsCRLF(const wstring& input) {
  wstring window_crlf_text;
  window_crlf_text.reserve(input.length());
  for (int i = 0; i < input.length(); i++) {
    if (input[i] == L'\n' && (i == 0 || input[i - 1] != L'\r')) {
      window_crlf_text.push_back(L'\r');
    }
    window_crlf_text.push_back(input[i]);
  }
  return window_crlf_text;
}

}  // namespace ime_goopy
