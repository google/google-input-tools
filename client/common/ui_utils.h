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

#ifndef GOOPY_COMMON_UI_UTILS_H__
#define GOOPY_COMMON_UI_UTILS_H__

#include <windows.h>
#include "base/basictypes.h"

namespace ATL {
class CImage;
}

namespace ime_goopy {
class UIUtils {
 public:
  static void PreMultiply(ATL::CImage *image);
  // This is used to acquire a IStream object from a RCDATA resource.
  // You can instantiate CImage or Gdiplus::Image with it.
  // Please call stream->Release() after you finished using the returned stream.
  static IStream *IStreamFromResource(HINSTANCE instance, LPCTSTR res_name);
  // determine if a window, or its parent, its parent's parent, ...,
  // or its owner is a full screen window
  static bool IsInFullScreenWindow(HWND hwnd);
};

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
  DISALLOW_COPY_AND_ASSIGN(ScopedSelectObject);
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_UI_UTILS_H__
