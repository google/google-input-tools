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

#include "common/canvas.h"

namespace ime_goopy {
namespace common {

Canvas::Canvas()
    : hdc_(NULL),
      bitmap_(NULL),
      old_bitmap_(NULL) {
  ZeroMemory(&rect_, sizeof(rect_));
}

Canvas::~Canvas() {
  if (hdc_) Destroy();
}

bool Canvas::Create(HDC hdc, int width, int height) {
  assert(!hdc_ && !bitmap_);
  assert(hdc && width && height);

  rect_.top = rect_.left = 0;
  rect_.right = width;
  rect_.bottom = height;

  hdc_ = CreateCompatibleDC(hdc);
  bitmap_ = CreateCompatibleBitmap(hdc, width, height);
  old_bitmap_ = static_cast<HBITMAP>(SelectObject(hdc_, bitmap_));
  return true;
}

void Canvas::Destroy() {
  SelectObject(hdc_, old_bitmap_);
  DeleteObject(bitmap_);
  DeleteDC(hdc_);
  bitmap_ = NULL;
  hdc_ = NULL;
}

void Canvas::BitBlt(HDC hdc, int x, int y) {
  ::BitBlt(hdc, x, y, rect_.right, rect_.bottom, hdc_, 0, 0, SRCCOPY);
}

void Canvas::BitBlt(HDC hdc, int x_dest, int y_dest, int width, int height,
    int x_src, int y_src) {
  ::BitBlt(hdc, x_dest, y_dest, width, height, hdc_, x_src, y_src, SRCCOPY);
}

}  // namespace common
}  // namespace ime_goopy
