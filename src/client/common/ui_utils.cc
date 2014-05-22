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

#include "common/ui_utils.h"

#include <atlimage.h>

namespace ime_goopy {
// Pre-multiply the alpha channel to the RGB colors,
// in order to make functions like AlphaBlend work properly.
void UIUtils::PreMultiply(CImage *image) {
  for (int x = 0; x < image->GetWidth(); x++) {
    for (int y = 0; y < image->GetHeight(); y++) {
      unsigned char *ptr =
        static_cast<unsigned char *>(image->GetPixelAddress(x, y));

      for (int c = 0; c < 3; c++) {
        // Add 127 for round.
        // ptr[3] is the alpha channel.
        ptr[c] = ((ptr[c] * ptr[3] + 127) / 255) & 0xFF;
      }
    }
  }
}

IStream *UIUtils::IStreamFromResource(HINSTANCE instance, LPCTSTR res_name) {
  HRSRC resource = FindResource(instance, res_name, RT_RCDATA);
  if (resource == NULL) return NULL;
  size_t size = SizeofResource(instance, resource);

  HGLOBAL data = LoadResource(instance, resource);
  if (data == NULL) return NULL;

  void *buffer = LockResource(data);
  if (buffer == NULL) return NULL;

  HGLOBAL globaldata = GlobalAlloc(GMEM_MOVEABLE, size);
  void *globalbuffer = GlobalLock(globaldata);
  memcpy(globalbuffer, buffer, size);
  GlobalUnlock(globaldata);
  FreeResource(data);

  IStream *stream = NULL;
  HRESULT hr = CreateStreamOnHGlobal(globaldata, TRUE, &stream);
  if (FAILED(hr)) return NULL;

  return stream;
}

bool UIUtils::IsInFullScreenWindow(HWND hwnd) {
  if (!hwnd || !::IsWindow(hwnd))
    return false;
  wchar_t name[MAX_PATH] = { 0 };
  ::GetModuleFileName(NULL, name, MAX_PATH);
  ::PathStripPath(name);
  // Don't consider the "desktop" windows as fullscreen.
  // Notice the "desktop" windows is different from ::GetDesktopWindow(),
  // GetDesktopWindow will return the parent window of all windows, but
  // the "desktop" window is the window that actually shows the desktop.
  if (::_wcsicmp(name, L"explorer.exe") == 0) {
    return false;
  }
  HWND tmp = hwnd;
  HWND desktop = ::GetDesktopWindow();
  // up to 32 levels
  for (int i = 0; i < 32 && tmp != NULL; i++) {
    RECT r1 = { 0 }, r2 = { 0 };
    if (::GetWindowRect(tmp, &r1) &&
        ::GetWindowRect(desktop, &r2) &&
        !memcmp(&r1, &r2, sizeof(RECT)))
      return true;
    tmp = (::GetWindowLong(tmp, GWL_STYLE) & WS_CHILD) ?
            ::GetParent(tmp) :
            ::GetWindow(tmp, GW_OWNER);
  }
  return false;
}

}  // namespace ime_goopy
