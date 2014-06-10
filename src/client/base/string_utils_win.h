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

#ifndef I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_STRING_UTILS_H_
#define I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_STRING_UTILS_H_

#include <string>

namespace ime_goopy {

std::string WideToUtf8(const std::wstring& wide);
std::wstring Utf8ToWide(const std::string& utf8);
// Return a C++ wstring
extern wstring WideStringPrintf(const wchar_t* format, ...)
// Tell the compiler to do printf format string checking.
PRINTF_ATTRIBUTE(1, 2);

// Store result into a supplied wstring and return it
extern const wstring& WideSStringPrintf(wstring* dst, const wchar_t* format, ...)
// Tell the compiler to do printf format string checking.
PRINTF_ATTRIBUTE(2, 3);

// Append result to a supplied wstring
extern void WideStringAppendF(wstring* dst, const wchar_t* format, ...)
// Tell the compiler to do printf format string checking.
PRINTF_ATTRIBUTE(2, 3);

// Lower-level routine that takes a va_list and appends to a specified
// wstring.  All other routines are just convenience wrappers around it.
extern void WideStringAppendV(wstring* dst, const wchar_t* format, va_list ap);

// The max arguments supported by WideStringPrintfVector
extern const int kWideStringPrintfVectorMaxArgs;

// You can use this version when all your arguments are strings, but
// you don't know how many arguments you'll have at compile time.
// WideStringPrintfVector will LOG(FATAL) if v.size() > kWideStringPrintfVectorMaxArgs
extern wstring WideStringPrintfVector(
    const wchar_t* format, const std::vector<wstring>& v);

// Convert Unix linefeed to Window format.
wstring ToWindowsCRLF(const wstring& input);

}  // namespace ime_goopy
#endif  // I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_STRING_UTILS_H_
