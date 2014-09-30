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

namespace ime_goopy {
#ifdef COMPILER_MSVC
  enum {
    IS_COMPILER_MSVC = 1
  };
#else
  enum {
    IS_COMPILER_MSVC = 0
  };
#endif
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

void WideStringAppendV(wstring* dst, const wchar_t* format, va_list ap) {
  // First try with a small fixed size buffer
  static const int kSpaceLength = 1024;
  wchar_t space[kSpaceLength];

  // It's possible for methods that use a va_list to invalidate
  // the data in it upon use.  The fix is to make a copy
  // of the structure before using it and use that copy instead.
  va_list backup_ap;
  va_copy(backup_ap, ap);
  int result = _vsnwprintf(space, kSpaceLength, format, backup_ap);
  va_end(backup_ap);

  if (result < kSpaceLength) {
    if (result >= 0) {
      // Normal case -- everything fit.
      dst->append(space, result);
      return;
    }

    if (IS_COMPILER_MSVC) {
      // Error or MSVC running out of space.  MSVC 8.0 and higher
      // can be asked about space needed with the special idiom below:
      va_copy(backup_ap, ap);
      result = _vsnwprintf(NULL, 0, format, backup_ap);
      va_end(backup_ap);
    }

    if (result < 0) {
      // Just an error.
      return;
    }
  }

  // Increase the buffer size to the size requested by _vsnwprintf,
  // plus one for the closing \0.
  int length = result + 1;
  wchar_t* buf = new wchar_t[length];

  // Restore the va_list before we use it again
  va_copy(backup_ap, ap);
  result = _vsnwprintf(buf, length, format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < length) {
    // It fit
    dst->append(buf, result);
  }
  delete[] buf;
}

wstring WideStringPrintf(const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  wstring result;
  WideStringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

const wstring& WideSStringPrintf(wstring* dst, const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  dst->clear();
  WideStringAppendV(dst, format, ap);
  va_end(ap);
  return *dst;
}

void WideStringAppendF(wstring* dst, const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  WideStringAppendV(dst, format, ap);
  va_end(ap);
}

// Max arguments supported by WideStringPrintVector
const int kWideStringPrintfVectorMaxArgs = 32;

// An empty block of zero for filler arguments.  This is const so that if
// printf tries to write to it (via %n) then the program gets a SIGSEGV
// and we can fix the problem or protect against an attack.
static const wchar_t wide_string_printf_empty_block[256] = {'\0'};

wstring WideStringPrintfVector(const wchar_t* format, const std::vector<wstring>& v) {
  CHECK_LE(v.size(), kWideStringPrintfVectorMaxArgs)
    << "WideStringPrintfVector currently only supports up to "
    << kWideStringPrintfVectorMaxArgs << " arguments. "
    << "Feel free to add support for more if you need it.";

  // Add filler arguments so that bogus format+args have a harder time
  // crashing the program, corrupting the program (%n),
  // or displaying random chunks of memory to users.

  const wchar_t* cstr[kWideStringPrintfVectorMaxArgs];
  for (int i = 0; i < v.size(); ++i) {
    cstr[i] = v[i].c_str();
  }
  for (int i = v.size(); i < arraysize(cstr); ++i) {
    cstr[i] = &wide_string_printf_empty_block[0];
  }

  // I do not know any way to pass kWideStringPrintfVectorMaxArgs arguments,
  // or any way to build a va_list by hand, or any API for printf
  // that accepts an array of arguments.  The best I can do is stick
  // this COMPILE_ASSERT right next to the actual statement.

  COMPILE_ASSERT(kWideStringPrintfVectorMaxArgs == 32, arg_count_mismatch);
  return WideStringPrintf(format,
                          cstr[0], cstr[1], cstr[2], cstr[3], cstr[4],
                          cstr[5], cstr[6], cstr[7], cstr[8], cstr[9],
                          cstr[10], cstr[11], cstr[12], cstr[13], cstr[14],
                          cstr[15], cstr[16], cstr[17], cstr[18], cstr[19],
                          cstr[20], cstr[21], cstr[22], cstr[23], cstr[24],
                          cstr[25], cstr[26], cstr[27], cstr[28], cstr[29],
                          cstr[30], cstr[31]);
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
