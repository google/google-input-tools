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

#include "imm/composition_string.h"

#include <stddef.h>
#include <windows.h>  // windows.h must be included before strsafe.h
#include <strsafe.h>

namespace ime_goopy {
namespace imm {

void CompositionString::set_composition(const wstring& value) {
  StringCchCopyN(composition,
                 kMaxCompositionLength,
                 value.c_str(),
                 kMaxCompositionLength);
  DWORD size = static_cast<DWORD>(value.size());
  info.dwCompStrLen = size;
  info.dwCompAttrLen = size;
  info.dwCompReadStrLen = size;
  info.dwCompReadAttrLen = size;
  composition_clause[1] = size;
}

void CompositionString::set_caret(int value) {
  info.dwCursorPos = value;
}

void CompositionString::set_result(const wstring& value) {
  StringCchCopyN(result,
                 kMaxResultLength,
                 value.c_str(),
                 kMaxResultLength);
  info.dwResultStrLen = static_cast<DWORD>(value.size());
  result_clause[1] = static_cast<DWORD>(value.size());
}

bool CompositionString::Initialize() {
  // TODO(ybo): Not initialized result_read(str and clause) and result clause
  // comparing to DDK sample. Implement them if there are compatibility issues.
  ZeroMemory(this, sizeof(*this));
  info.dwSize = sizeof(CompositionString);

  // Composition string.
  info.dwCompStrOffset = offsetof(CompositionString, composition);
  info.dwCompAttrOffset = offsetof(CompositionString, composition_attribute);
  info.dwCompClauseOffset = offsetof(CompositionString, composition_clause);

  // Composition reading string.
  // It's the same with composition string for simple IME.
  info.dwCompReadStrOffset = info.dwCompStrOffset;
  info.dwCompReadAttrOffset = info.dwCompAttrOffset;
  info.dwCompReadClauseOffset = info.dwCompClauseOffset;

  // Result.
  info.dwResultStrOffset = offsetof(CompositionString, result);
  return true;
}

}  // namespace imm
}  // namespace ime_goopy
