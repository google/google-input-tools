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

#ifndef GOOPY_COMMON_CANVAS_H__
#define GOOPY_COMMON_CANVAS_H__

#include <atlbase.h>
#include <vector>
#include "base/basictypes.h"

namespace ime_goopy {
namespace common {

// ScopedSelectObject is a wrapper of Win32 API SelectObject. It will restore
// old object when the variable is out of scope.
class ScopedSelectObject {
 public:
  ScopedSelectObject(HDC hdc, HGDIOBJ object) : hdc_(hdc) {
    old_object_ = SelectObject(hdc_, object);
  }
  ~ScopedSelectObject() {
    SelectObject(hdc_, old_object_);
  }

 private:
  HDC hdc_;
  HGDIOBJ old_object_;
  DISALLOW_EVIL_CONSTRUCTORS(ScopedSelectObject);
};

// Canvas is a bitmap surface, you can draw on it using GDI APIs. This class is
// used to support double buffer to avoid flashing.
// More functions will be added to make drawing easier.
class Canvas {
 public:
  Canvas();
  ~Canvas();

  bool Create(HDC hdc, int width, int height);
  void Destroy();

  // Draw the canvas content to specific DC.
  void BitBlt(HDC hdc, int x, int y);
  void BitBlt(HDC hdc, int dest_x, int dest_y, int width, int height,
      int src_x, int src_y);

  const HDC hdc() { return hdc_; }

 private:
  HDC hdc_;
  HBITMAP bitmap_;
  HBITMAP old_bitmap_;
  RECT rect_;
  DISALLOW_EVIL_CONSTRUCTORS(Canvas);
};
}  // namespace common
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_CANVAS_H__
