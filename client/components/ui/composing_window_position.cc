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
#include "components/ui/composing_window_position.h"

#include "components/ui/skin_ui_component_utils.h"
#include "ipc/settings_client.h"

namespace ime_goopy {
namespace components {

ComposingWindowPosition::ComposingWindowPosition()
    : last_adjust_strategy_(ADJUST_NONE),
      rtl_(false) {
  Reset();
}

void ComposingWindowPosition::SetCaretRect(
    const ipc::proto::InputCaret& caret) {
  caret_rect_.x = caret.rect().x();
  caret_rect_.y = caret.rect().y();
  caret_rect_.width = caret.rect().width();
  caret_rect_.height = caret.rect().height();
  Point<int> pt(caret_rect_.x, caret_rect_.y + caret_rect_.height);
  screen_rect_ = SkinUIComponentUtils::GetScreenRectAtPoint(pt);
}

void ComposingWindowPosition::SetViewSize(int x, int y) {
  view_size_.width = x;
  view_size_.height = y;
  if (max_view_size_.width < view_size_.width)
    max_view_size_.width = view_size_.width;
  if (max_view_size_.height < view_size_.height)
    max_view_size_.height = view_size_.height;
}

void ComposingWindowPosition::SetMonitorRect(int x, int y, int w, int h) {
  screen_rect_.SetValue(x, y, w, h);
}

Point<int> ComposingWindowPosition::GetPosition() {
  Point<int> pt;
  // Normal case.
  if (rtl_)
    pt.x = caret_rect_.x + caret_rect_.width - view_size_.width;
  else
    pt.x = caret_rect_.x;
  // Defaults to below the caret.
  MakePosition(ADJUST_BELOW, &pt);

  AdjustInScreen(&pt);
  AdjustIfOverlapped(&pt);
  return pt;
}

void ComposingWindowPosition::Reset() {
  last_adjust_strategy_ = ADJUST_NONE;
  view_size_.width = 0;
  view_size_.height = 0;
  max_view_size_.width = 0;
  max_view_size_.height = 0;
  screen_rect_.SetValue(-1, -1, -1, -1);
}

void ComposingWindowPosition::SetRTL(bool rtl) {
  rtl_ = rtl;
}

bool ComposingWindowPosition::IsInScreen(Point<int> origin) {
  if (screen_rect_.x != -1 && screen_rect_.y != -1 &&
      screen_rect_.width != -1 && screen_rect_.height != -1) {
    int screen_right = screen_rect_.x + screen_rect_.width;
    int screen_bottom = screen_rect_.y + screen_rect_.height;
    return origin.x >= screen_rect_.x &&
           origin.y >= screen_rect_.y &&
           origin.x + view_size_.width <= screen_right &&
           origin.y + view_size_.height <= screen_bottom;
  }
  return true;
}

bool ComposingWindowPosition::IsCaretOverlapped(
    Point<int> origin) {
  return caret_rect_.y + caret_rect_.height > origin.y &&
         caret_rect_.y < origin.y + view_size_.height;
}

void ComposingWindowPosition::AdjustInScreen(
    Point<int>* window_point) {
  if (screen_rect_.x != -1 && screen_rect_.y != -1 &&
      screen_rect_.width != -1 && screen_rect_.height != -1) {
    int screen_right = screen_rect_.x + screen_rect_.width;
    int screen_bottom = screen_rect_.y + screen_rect_.height;
    if (window_point->x < screen_rect_.x)
      window_point->x = screen_rect_.x;
    if (window_point->y < screen_rect_.y)
      window_point->y = screen_rect_.y;
    if (window_point->x + max_view_size_.width > screen_right)
      window_point->x = max(screen_right - max_view_size_.width,
                            screen_rect_.x);
    if (window_point->y + max_view_size_.height > screen_bottom)
      window_point->y = max(screen_bottom - max_view_size_.height,
                            screen_rect_.y);
  }
}

void ComposingWindowPosition::AdjustIfOverlapped(
    Point<int>* position) {
  MakePosition(last_adjust_strategy_, position);
  if (IsInScreen(*position) && !IsCaretOverlapped(*position)) return;

  for (int pref = ADJUST_NONE + 1; pref < ADJUST_COUNT; pref++) {
    MakePosition(pref, position);
    if (IsInScreen(*position) && !IsCaretOverlapped(*position)) {
      last_adjust_strategy_ = pref;
      return;
    }
  }
}

void ComposingWindowPosition::MakePosition(
    int preference,
    Point<int>* window_pos) {
#ifndef OS_MACOSX
  switch (preference) {
    case ADJUST_NONE:
      break;
    case ADJUST_BELOW:
      window_pos->y = caret_rect_.y + caret_rect_.height;
      break;
    case ADJUST_ABOVE:
      window_pos->y = caret_rect_.y - max_view_size_.height;
      break;
    default:
      NOTREACHED() << L"Unknown adjustment strategy.";
      break;
  }
#else
  // Mac OSX coordinate system is up-side-down
  switch (preference) {
    case ADJUST_NONE:
      break;
    case ADJUST_BELOW:
      window_pos->y = caret_rect_.y - max_view_size_.height;
      break;
    case ADJUST_ABOVE:
      window_pos->y = caret_rect_.y + caret_rect_.height;
      break;
    default:
      NOTREACHED() << L"Unknown adjustment strategy.";
      break;
  }
#endif  // OS_MACOSX
}

}  // namespace components
}  // namespace ime_goopy

