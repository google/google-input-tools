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

#include "imm/context_locker.h"
#include "imm/input_context.h"
#include "imm/immdev.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
static const POINT kTestPoint = {100, 100};
static const RECT kTestRect = {100, 100, 200, 200};
static const int kDefaultFontHeight = 16;
static const int kTestFontHeight = 24;

class InputContextTest : public testing::Test {
 public:
  void SetUp() {
    hwnd_ = CreateWindowEx(
        0,
        L"EDIT",
        L"DUMMY",
        WS_POPUP | WS_DISABLED,
        100,
        100,
        100,
        30,
        NULL,
        NULL,
        NULL,
        NULL);
    ASSERT_TRUE(hwnd_);
  }

  void TearDown() {
    DestroyWindow(hwnd_);
  }

 protected:
  HWND hwnd_;
};

TEST_F(InputContextTest, Initialize) {
  InputContext context;

  ASSERT_TRUE(context.Initialize());
  ASSERT_TRUE(context.fdwInit & INIT_CONVERSION);
  ASSERT_TRUE(context.fdwInit & INIT_LOGFONT);
  ASSERT_TRUE(context.fOpen);
}

TEST_F(InputContextTest, GetCaretRectFromComposition) {
  InputContext context;

  RECT caret;
  POINT point;

  // Not initialized.
  context.fdwInit = 0;
  ASSERT_FALSE(context.GetCaretRectFromComposition(&caret));

  // No window.
  context.fdwInit = INIT_COMPFORM;
  context.hWnd = NULL;
  ASSERT_FALSE(context.GetCaretRectFromComposition(&caret));

  // CFS_DEFAULT
  context.hWnd = hwnd_;
  context.cfCompForm.dwStyle = CFS_DEFAULT;
  ASSERT_TRUE(context.GetCaretRectFromComposition(&caret));

  // CFS_POINT
  context.hWnd = hwnd_;
  context.cfCompForm.dwStyle = CFS_POINT;
  context.cfCompForm.ptCurrentPos = kTestPoint;
  ASSERT_TRUE(context.GetCaretRectFromComposition(&caret));
  point = kTestPoint;
  ClientToScreen(hwnd_, &point);
  ASSERT_EQ(point.x, caret.left);
  ASSERT_EQ(point.y, caret.top);
  ASSERT_NE(caret.top, caret.bottom);

  // CFS_FORCE_POSITION
  context.hWnd = hwnd_;
  context.cfCompForm.dwStyle = CFS_POINT | CFS_FORCE_POSITION;
  context.cfCompForm.ptCurrentPos = kTestPoint;
  ASSERT_TRUE(context.GetCaretRectFromComposition(&caret));
  point = kTestPoint;
  ClientToScreen(hwnd_, &point);
  ASSERT_EQ(point.x, caret.left);
  ASSERT_EQ(point.y, caret.top);
  ASSERT_EQ(caret.top, caret.bottom);
}

TEST_F(InputContextTest, GetCaretRectFromCandidate) {
  InputContext context;

  RECT caret;
  POINT point;
  CANDIDATEFORM& candform = context.cfCandForm[0];

  // No window.
  context.hWnd = 0;
  ASSERT_FALSE(context.GetCaretRectFromCandidate(&caret));

  // No index.
  context.hWnd = hwnd_;
  candform.dwIndex = 1;
  ASSERT_FALSE(context.GetCaretRectFromCandidate(&caret));

  // CFS_DEFAULT
  candform.dwIndex = 0;
  candform.dwStyle = CFS_DEFAULT;
  ASSERT_TRUE(context.GetCaretRectFromCandidate(&caret));

  // CFS_EXCLUDE
  candform.dwStyle = CFS_EXCLUDE;
  candform.rcArea = kTestRect;
  ASSERT_TRUE(context.GetCaretRectFromCandidate(&caret));
  POINT top_left = {kTestRect.left, kTestRect.top};
  POINT bottom_right = {kTestRect.right, kTestRect.bottom};
  ClientToScreen(hwnd_, &top_left);
  ClientToScreen(hwnd_, &bottom_right);
  ASSERT_EQ(top_left.x, caret.left);
  ASSERT_EQ(top_left.y, caret.top);
  ASSERT_EQ(bottom_right.x, caret.right);
  ASSERT_EQ(bottom_right.y, caret.bottom);

  // CFS_CANDIDATEPOS
  candform.ptCurrentPos = kTestPoint;
  candform.dwStyle = CFS_CANDIDATEPOS;
  candform.ptCurrentPos = kTestPoint;
  ASSERT_TRUE(context.GetCaretRectFromCandidate(&caret));
  point = kTestPoint;
  ClientToScreen(hwnd_, &point);
  ASSERT_EQ(point.x, caret.left);
  ASSERT_EQ(point.y, caret.top);
  ASSERT_NE(caret.top, caret.bottom);

  // CFS_FORCE_POSITION
  candform.ptCurrentPos = kTestPoint;
  candform.dwStyle = CFS_CANDIDATEPOS | CFS_FORCE_POSITION;
  candform.ptCurrentPos = kTestPoint;
  ASSERT_TRUE(context.GetCaretRectFromCandidate(&caret));
  point = kTestPoint;
  ClientToScreen(hwnd_, &point);
  ASSERT_EQ(point.x, caret.left);
  ASSERT_EQ(point.y, caret.top);
  ASSERT_EQ(caret.top, caret.bottom);
}

TEST_F(InputContextTest, GetFontHeight) {
  InputContext context;

  // Not initalized.
  context.fdwInit = 0;
  ASSERT_EQ(kDefaultFontHeight, context.GetFontHeight());

  context.fdwInit = INIT_LOGFONT;
  // Zero.
  context.lfFont.W.lfHeight = 0;
  ASSERT_EQ(kDefaultFontHeight, context.GetFontHeight());

  // Positive.
  context.lfFont.W.lfHeight = kTestFontHeight;
  ASSERT_EQ(kTestFontHeight, context.GetFontHeight());

  // Negative.
  context.lfFont.W.lfHeight = -kTestFontHeight;
  ASSERT_EQ(kTestFontHeight, context.GetFontHeight());
}

}  // namespace imm
}  // namespace ime_goopy
