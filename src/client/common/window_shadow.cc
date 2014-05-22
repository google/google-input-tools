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

#include "common/window_shadow.h"
#include <math.h>

namespace ime_goopy {
namespace common {

static const TCHAR* const kShadowClassName = L"GPY_FADER";
static const int kShadowWidth = 4;
static const int kShadowOffset = 6;
static const int kShadowAlpha = 128;

WindowShadow::WindowShadow() : hwnd_(NULL), alpha_value_(kShadowAlpha) {
}

WindowShadow::~WindowShadow() {
  Destroy();
}

bool WindowShadow::Create(HINSTANCE hinstance, HWND parent_hwnd) {
  WNDCLASSEX wc;

  // NOTE(henryou): We should register a customized class instead of using
  // system's predefined one. Otherwise, IE7 will try to kill our UI window
  // too, even if we have our own shadow implementation.
  if (!GetClassInfoEx(hinstance, kShadowClassName, &wc)) {
    // register class of UI window.
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_IME;
    wc.lpfnWndProc    = DefWindowProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hinstance;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPTSTR)NULL;
    wc.lpszClassName  = kShadowClassName;
    wc.hbrBackground  = NULL;
    wc.hIconSm        = NULL;

    if (!RegisterClassEx(&wc)) {
      return FALSE;
    }
  }
  hwnd_ = ::CreateWindowEx(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
                           kShadowClassName,
                           NULL,   // no window name
                           WS_POPUP | WS_DISABLED,
                           0,      // x
                           0,      // y
                           1,      // width
                           1,      // height
                           parent_hwnd,
                           NULL,   // menu
                           hinstance,
                           NULL);  // param
  return hwnd_ != NULL;
}

void WindowShadow::UpdatePosition(HWND client_window) {
  if (!hwnd_ || !::IsWindow(hwnd_)) return;

  RECT rect;
  ::GetWindowRect(client_window, &rect);

  if (hwnd_) {
    rect.left += kShadowOffset;
    rect.top += kShadowOffset;
    rect.right += kShadowWidth;
    rect.bottom += kShadowWidth;
    Repaint(&rect);
    ::SetWindowPos(hwnd_,
                   client_window,
                   rect.left,
                   rect.top,
                   rect.right - rect.left,
                   rect.bottom - rect.top,
                   SWP_NOACTIVATE);
  }
}

bool WindowShadow::Repaint(RECT *rc) {
  if (rc->right - rc->left < kShadowOffset ||
      rc->bottom - rc->top < kShadowOffset) {
    return FALSE;
  }

  int width = rc->right - rc->left;
  int height = rc->bottom - rc->top;

  void *shadow_buffer = NULL;
  HDC hdc = CreateCompatibleDC(NULL);
  if (hdc == NULL)
    return FALSE;

  HBITMAP bitmap = CreateBitmap(hdc, width, height, &shadow_buffer);
  if (bitmap) {
    HBITMAP old_bitmap = static_cast<HBITMAP>(SelectObject(hdc, bitmap));

    FillShadowBuffer(shadow_buffer, width, height);
    UpdateWindow(hdc, width, height);

    SelectObject(hdc, old_bitmap);
    DeleteObject(bitmap);
  }
  DeleteDC(hdc);

  return TRUE;
}

void WindowShadow::Destroy() {
  if (hwnd_ && ::IsWindow(hwnd_))
    ::DestroyWindow(hwnd_);
  hwnd_ = NULL;
}

void WindowShadow::Show(bool show) {
  if (hwnd_ && ::IsWindow(hwnd_))
    ::ShowWindow(hwnd_, show ? SW_SHOWNOACTIVATE : SW_HIDE);
}

void WindowShadow::FillShadowBuffer(void *shadow_buffer,
                                    int width, int height) {
  ZeroMemory(shadow_buffer, sizeof(uint32) * width * height);

  // Draw shadow of bottom edge.
  for (int y = 0; y < kShadowWidth; y++) {
    int convert_y =  kShadowWidth - y - 1;
    uint8 *buffer = (uint8 *) shadow_buffer + width * 4 * convert_y;
    for (int x = 0; x < width; x++) {
      buffer[3] = 0xff - 0x100 * y / kShadowWidth;
      buffer += 4;
    }
  }

  // Draw shadow of right edge.
  for (int y = 0; y < height; y++) {
    uint8 *buffer = (uint8 *) shadow_buffer + width * 4 * y;
    buffer += 4 * (width - kShadowWidth);
    for (int x = 0; x < kShadowWidth; x++) {
      buffer[3] = 0xff - 0x100 * x / kShadowWidth;
      buffer += 4;
    }
  }

  // Compute the alpha values of corners and cache in an array for future
  // usage.
  if (corner_fader_pixels_.get() == NULL) {
    // Try to calculate the transparency assuming the corners are round.
    corner_fader_pixels_.reset(new uint8[kShadowWidth * kShadowWidth]);
    for (int y = 0; y < kShadowWidth; y++) {
      for (int x = 0; x < kShadowWidth; x++) {
        corner_fader_pixels_[y * 4 + x] =
            max(0xff - 0x100 / kShadowWidth *
                sqrt(static_cast<float>(
                    (kShadowWidth - x - 1) * (kShadowWidth - x - 1) +
                    (kShadowWidth - y - 1) * (kShadowWidth - y - 1))),
                0.0f);
      }
    }
  }

  for (int y = 0; y < kShadowWidth; y++) {
    // Update the lower-left corner and the lower-right corner.
    uint8 *buffer = (uint8 *) shadow_buffer + width * 4 * y;
    for (int x = 0; x < kShadowWidth; x++) {
      buffer[x * 4 + 3] = corner_fader_pixels_[y * 4 + x];
      buffer[(width - x - 1) * 4 + 3] = corner_fader_pixels_[y * 4 + x];
    }
    // Update the upper-right corner.
    buffer = (uint8 *) shadow_buffer + width * 4 *
        (height - y - 1);
    for (int x = 0; x < kShadowWidth; x++) {
      buffer[(width - x - 1) * 4 + 3] = corner_fader_pixels_[y * 4 + x];
    }
  }
}

void WindowShadow::UpdateWindow(HDC hdc, int width, int height) {
  BLENDFUNCTION blend;
  blend.BlendOp = AC_SRC_OVER;
  blend.BlendFlags = 0;
  blend.AlphaFormat = AC_SRC_ALPHA;
  blend.SourceConstantAlpha = alpha_value_;

  POINT source_point;
  source_point.x = 0;
  source_point.y = 0;
  SIZE client_area;
  client_area.cx = width;
  client_area.cy = height;
  UpdateLayeredWindow(hwnd_, NULL, NULL, &client_area, hdc, &source_point,
                      RGB(0, 0, 0), &blend, ULW_ALPHA);
}

HBITMAP WindowShadow::CreateBitmap(HDC hdc, int width, int height,
                                   void **shadow_buffer) {
  BITMAPINFO bitmap_info;
  BITMAPINFOHEADER* const header = &bitmap_info.bmiHeader;
  header->biSize = sizeof(bitmap_info.bmiHeader);
  header->biWidth = width;
  header->biHeight = height;
  header->biPlanes = 1;
  header->biBitCount = 32;
  header->biCompression = BI_RGB;
  header->biSizeImage = sizeof(uint32) * width * height;

  return CreateDIBSection(hdc,
                          &bitmap_info,
                          DIB_RGB_COLORS,
                          shadow_buffer,
                          NULL,    // no file-mapping object
                          0);      // don't use file-mapping offset
}

}  // namespace common
}  // namespace ime_goopy
