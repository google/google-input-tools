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
//
// This class represents our own implemenetation of window shadow effect.
#ifndef GOOPY_COMMON_WINDOW_SHADOW_H__
#define GOOPY_COMMON_WINDOW_SHADOW_H__

#include <windows.h>
#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {
namespace common {

// TODO(ybo): Migrate to ATL.
class WindowShadow {
 public:
  WindowShadow();
  ~WindowShadow();
  bool Create(HINSTANCE hinstance, HWND parent_hwnd);
  void UpdatePosition(HWND parent_hwnd);
  void Destroy();
  void Show(bool show);
  void set_alpha(int alpha) { alpha_value_ = alpha; }
 private:
  bool Repaint(RECT *rc);
  void FillShadowBuffer(void *shadow_buffer, int width, int height);
  void UpdateWindow(HDC hdc, int width, int height);
  HBITMAP CreateBitmap(HDC hdc, int width, int height, void **shadow_buffer);
  HWND hwnd_;
  scoped_array<uint8> corner_fader_pixels_;
  int alpha_value_;
  DISALLOW_EVIL_CONSTRUCTORS(WindowShadow);
};

}  // namespace common
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_WINDOW_SHADOW_H__
