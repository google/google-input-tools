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

#ifndef GOOPY_IMM_COMPOSITION_STRING_H__
#define GOOPY_IMM_COMPOSITION_STRING_H__

#include "base/basictypes.h"
#include "imm/context_locker.h"
#include "imm/immdev.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace imm {
struct CompositionString {
 public:
  bool Initialize();
  void set_composition(const wstring& value);
  const wchar_t* get_composition() { return composition; }
  void set_caret(int value);
  void set_result(const wstring& value);

 private:
  static const int kMaxCompositionLength = 500;
  static const int kMaxResultLength = kMaxCompositionLength;

  COMPOSITIONSTRING info;

  // Composition.
  wchar_t composition[kMaxCompositionLength];
  DWORD composition_clause[2];
  BYTE composition_attribute[kMaxCompositionLength];

  // Result.
  wchar_t result[kMaxResultLength];
  DWORD result_clause[2];

  FRIEND_TEST(CompositionString, Test);
  FRIEND_TEST(ContextTest, Update);
};

}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_COMPOSITION_STRING_H__
