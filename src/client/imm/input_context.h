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

#ifndef GOOPY_IMM_INPUT_CONTEXT_H__
#define GOOPY_IMM_INPUT_CONTEXT_H__

#include "base/basictypes.h"
#include "imm/context_locker.h"
#include "imm/immdev.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace imm {
// A wrapper around INPUTCONTEXT. This is a struct rather than a class,
// so we can safely cast a locked INPUTCONTEXT pointer to InputContext.
struct InputContext : public INPUTCONTEXT {
 public:
  bool Initialize();
  // Gets top-left corner of the candidate window.
  bool GetCandidatePos(POINT* point);
  // Gets top-left corner of the composition window.
  bool GetCompositionPos(POINT* point);
  bool GetCompositionBoundary(RECT *rect);
  bool GetCompositionFont(LOGFONT *font);
  int GetFontHeight();

  // These two functions are for IME that has combined composition and candidate
  // window.
  bool GetCaretRectFromComposition(RECT* caret);
  bool GetCaretRectFromCandidate(RECT* caret);
 private:
  FRIEND_TEST(InputContextTest, Initialize);
  FRIEND_TEST(InputContextTest, GetCaretRectFromComposition);
  FRIEND_TEST(InputContextTest, GetCaretRectFromCandidate);
  FRIEND_TEST(InputContextTest, GetFontHeight);
};
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_INPUT_CONTEXT_H__
