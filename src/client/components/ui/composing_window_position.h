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
#ifndef GOOPY_COMPONENTS_UI_COMPOSING_WINDOW_POSITION_H_
#define GOOPY_COMPONENTS_UI_COMPOSING_WINDOW_POSITION_H_

#include "base/macros.h"  // For DISALLOW_COPY_AND_ASSIGN
#include "components/ui/ui_types.h"

namespace ipc {
namespace proto {

class InputCaret;

}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace components {

// ComposingWindowPosition is used to keep track of the position of candidate
// view. Usually the view is put right below the caret. However there are
// certain cases which make it not possible.
// 1. Part of the candidate view is off the screen. In such case, we move the
//    window back to the window with smallest move distance.
// 2. The view is over the input text. We cannot actively detect the exact
//    boundary of the input area, so we just avoid putting the window within
//    the area between caret_rect.top and caret_rect.bottom
// If we have to adjust the position because of 2, we try to memorize this
// adjustment, and keep using the same position until it is off the screen
// again. By doing this we avoid moving the candidate view up and down when
// user delete part of the text which was overlapped by the view.
class ComposingWindowPosition {
 public:
  ComposingWindowPosition();
  void SetCaretRect(const ipc::proto::InputCaret& caret);
  void SetViewSize(int width, int height);
  void SetMonitorRect(int x, int y, int width, int height);
  Point<int> GetPosition();
  void Reset();
  void SetRTL(bool rtl);

 private:
  bool IsInScreen(Point<int> origin);
  bool IsCaretOverlapped(Point<int> origin);
  void AdjustInScreen(Point<int> *pt);
  void AdjustIfOverlapped(Point<int> *pt);
  void MakePosition(int preference, Point<int>* window_pos);
  enum AdjustStrategy {
    ADJUST_NONE = 0,
    ADJUST_BELOW,
    ADJUST_ABOVE,
    ADJUST_COUNT
  };
  int last_adjust_strategy_;
  Rect<int> caret_rect_;
  Size<int> view_size_;
  Size<int> max_view_size_;
  Rect<int> screen_rect_;
  bool rtl_;

  DISALLOW_COPY_AND_ASSIGN(ComposingWindowPosition);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_UI_COMPOSING_WINDOW_POSITION_H_
