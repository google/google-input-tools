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

#ifndef GOOPY_COMMON_STRING_UTILS_H__
#define GOOPY_COMMON_STRING_UTILS_H__

#include <algorithm>
#include <string>
#include <vector>
#include "base/basictypes.h"

namespace ime_goopy {
// WCHAR
#if !defined(_WINNT_)
#define WCHAR uint16
#endif
// Split a string into parts using the provided separator.
void SplitString(const wstring& raw,
                 const wstring& sep,
                 vector<wstring>* output);
void SplitString(const string& raw,
                 const string& sep,
                 vector<string>* output);

// Split string vector into parts.
template <class T>
inline void SplitStringVector(const vector<T>& raw,
                              const typename vector<T>::value_type& sep,
                              vector<T>* output) {
  vector<T> candidate;
  for (typename vector<T>::const_iterator it = raw.begin();
       it != raw.end(); ++it) {
    candidate.clear();
    SplitString(*it, sep, &candidate);
    copy(candidate.begin(), candidate.end(), back_inserter(*output));
  }
}

// NOTE(ygwang): The following two functions are required by Google homepage and
// search setting code ported from protector. For interface consistency, the
// original C-style interfaces are kept here and later we may need real
// C++-style interfaces for them.

// Returns true if "text" ends on "ends".
bool StringEndsWith(const WCHAR* text, const WCHAR* ends);
bool StringEndsWith(const char* text, const char* ends);

// Returns true if "text" begins with "begins".
bool StringBeginsWith(const WCHAR* text, const WCHAR* begins);
bool StringBeginsWith(const char* text, const char* begins);

// Convienent method to convert between Utf8 and Utf16.
string WideToUtf8(const wstring& wide);
wstring Utf8ToWide(const string& utf8);

// Convienent method to convert Utf16 to current system Windows ANSI code page.
string WideToACP(const wstring& wide);

// Convienent method to convert Utf16 to MultiByte according to codepage
string WideToCodePage(const wstring& text, int codepage);

// Utf8 version of CombinePath.
string CombinePathUtf8(const string& path, const string& filename);

// Convert Unix linefeed to Window format.
wstring ToWindowsCRLF(const wstring& input);

// Convert the characters in "s" to lowercase. Works only with ascii characters.
inline void LowerString(string* s) {
  transform(s->begin(), s->end(), s->begin(), ::tolower);
}

// Convert the characters in "s" to uppercase. Works only with ascii characters.
inline void UpperString(string* s) {
  transform(s->begin(), s->end(), s->begin(), ::toupper);
}

// Return true iff there is upper charactor in "s".
inline bool HasUpperChar(const string& s) {
  return find_if(s.begin(), s.end(), ::isupper) != s.end();
}

// Join strings from start iterator to end iterator with delim string, new
// string will be appended to result parameter.
template <class Iterator, class T>
void JoinStringsIterator(const Iterator& start, const Iterator& end,
                         const T& delim, T* result) {
  // Precompute resulting length so we can reserve() memory in one shot.
  if (start != end) {
    int length = delim.size() * (distance(start, end) - 1);
    for (Iterator iter = start; iter != end; ++iter) {
      length += iter->size();
    }
    result->reserve(length);
  }

  // Now combine everything.
  for (Iterator iter = start; iter != end; ++iter) {
    if (iter != start) {
      result->append(delim.data(), delim.size());
    }
    result->append(iter->data(), iter->size());
  }
}

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_STRING_UTILS_H__
