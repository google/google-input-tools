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

#include "imm/input_context.h"

#define _ATL_NO_HOSTING  // prevents including <dimm.h>
#include <atlbase.h>
#include <atlwin.h>

#include "imm/candidate_info.h"
#include "imm/composition_string.h"

namespace ime_goopy {
namespace imm {
static const int kDefaultFontHeight = 16;

bool InputContext::Initialize() {
  DVLOG(3) << __FUNCTION__;

  // Initialize conversion mode.
  if (!(fdwInit & INIT_CONVERSION)) {
    DLOG(WARNING) << __FUNCTION__
                  << L"Conversion mode not initialized.";
    fdwConversion = IME_CMODE_NATIVE;
    fdwInit |= INIT_CONVERSION;
  }

  // We can't assume the open status is true when the input method is opened in
  // a given context. For example, when you open the IME in the password control
  // of Opera, the open status provided by the applications is false. If we
  // change it to true here, it will be changed to false after the focus is
  // switching away and never changed back to true.
  //  fOpen = TRUE;
  return true;
}

// Returns true if we can use GetCaretPos API to get caret position. In MS
// publisher, GetCaretPos returns incorrent caret position, so we should not
// use it.
// TODO(synch): refactor the logic of getting caret position.
static bool ShouldUseGetCaretPos(HWND hwnd) {
  wchar_t class_name[MAX_PATH] = {0};
  if (::GetClassName(hwnd, class_name, MAX_PATH) == 0) {
    DLOG(ERROR) << "Failed to get the windows class name for HWND" << hwnd;
  } else if (wcscmp(class_name, L"MSWinPub") == 0) {
    return false;
  }
  return true;
}

// Returns true if we should only use GetGuiThreadInfo API to get caret position
// , other than getting caret position using other methods.
// It happens when in chromes omnibox(address bar) with india language, and
// in Firefox. the caret position got from candidate and composition is wrong,
// so we can only use GetGuiThreadInfo API to get caret position.
static bool ShouldGetCaretPositionFromGuiThreadInfo(HWND hwnd) {
  wchar_t class_name[MAX_PATH] = {0};
  if (::GetClassName(hwnd, class_name, MAX_PATH) == 0) {
    DLOG(ERROR) << "Failed to get the windows class name for HWND" << hwnd;
  } else if (wcscmp(class_name, L"Chrome_OmniboxView") == 0 ||
             wcscmp(class_name, L"MozillaWindowClass") == 0) {
    return true;
  }
  return false;
}

static bool GetCaretRectFromGuiThreadInfo(RECT* caret) {
  GUITHREADINFO guithread_info = {0};
  guithread_info.cbSize = sizeof(guithread_info);
  if (!::GetGUIThreadInfo(GetCurrentThreadId(), &guithread_info) ||
      !guithread_info.hwndCaret) {
    return false;
  }
  *caret = guithread_info.rcCaret;
  CWindow window(guithread_info.hwndCaret);
  window.ClientToScreen(caret);
  return true;
}

static bool GetCaretRectFromCaretPosition(InputContext* context, RECT* caret) {
  POINT caret_point = {0};
  if (!ShouldUseGetCaretPos(context->hWnd) || !::GetCaretPos(&caret_point)) {
    DLOG(WARNING) << L"We can't get caret pos here!";
    return false;
  }
  if (caret_point.x == 0 && caret_point.y == 0)
    return false;
  caret->left = caret_point.x;
  caret->right = caret_point.x;
  caret->top = caret_point.y;
  caret->bottom = caret_point.y + context->GetFontHeight();
  CWindow window(context->hWnd);
  window.ClientToScreen(caret);
  return true;
}

bool InputContext::GetCaretRectFromComposition(RECT* caret) {
  if (!caret) return false;

  if (!IsWindow(hWnd)) return false;

  if (ShouldGetCaretPositionFromGuiThreadInfo(hWnd))
    return GetCaretRectFromGuiThreadInfo(caret);

  if (!(fdwInit & INIT_COMPFORM) || cfCompForm.dwStyle == CFS_DEFAULT) {
    // If not initialized or no indication from dwStyle, try to get the caret
    // position with windows api as back off.
    // Some times in word 2010, fdwInit & INIT_COMPFORM will be false.
    if (!GetCaretRectFromGuiThreadInfo(caret))
      return GetCaretRectFromCaretPosition(this, caret);
    return true;
  } else {
    CWindow window;
    POINT caret_point = {0, 0};
    caret_point = cfCompForm.ptCurrentPos;
    caret->left = caret_point.x;
    caret->top = caret_point.y;
    caret->right = caret->left;
    if (cfCompForm.dwStyle & CFS_FORCE_POSITION) {
      caret->bottom = caret->top;
    } else {
      caret->bottom = caret->top + GetFontHeight();
    }
    window.Attach(hWnd);
    window.ClientToScreen(caret);
  }
  return true;
}

bool InputContext::GetCaretRectFromCandidate(RECT* caret) {
  if (!caret) return false;

  CANDIDATEFORM& candform = cfCandForm[0];

  // NOTE(haicsun): the following line is commented since some application
  // don't set/initialize dwIndex value, so the following check will return
  // false in those cases which cause that the composition window can't get
  // right position of input cursor. if any case found in future that proves
  // the following check is neccesary, it should be activated again.
  //
  // if (candform.dwIndex != 0) return false;

  CWindow window(hWnd);
  if (!window.IsWindow()) return false;

  if (ShouldGetCaretPositionFromGuiThreadInfo(hWnd))
    return GetCaretRectFromGuiThreadInfo(caret);

  if (candform.dwStyle & CFS_EXCLUDE) {
    caret->left = candform.ptCurrentPos.x;
    caret->top = candform.ptCurrentPos.y;
    caret->right = candform.ptCurrentPos.x;
    caret->bottom = candform.ptCurrentPos.y + GetFontHeight();
    window.ClientToScreen(caret);
    return true;
  } else {
    // If no indication from dwStyle, just try to get the caret position with
    // Windows API as back off.
    if (!GetCaretRectFromGuiThreadInfo(caret))
      return GetCaretRectFromCaretPosition(this, caret);
    return true;
  }
  return false;
}

bool InputContext::GetCandidatePos(POINT* point) {
  if (!point) {
    DCHECK(false);
    return false;
  }
  if (!IsWindow(hWnd))
    return false;
  if (cfCandForm[0].dwIndex == 0) {
    switch(cfCandForm[0].dwStyle) {
      case CFS_CANDIDATEPOS: {
        *point = cfCandForm[0].ptCurrentPos;
        point->y += GetFontHeight();
        ClientToScreen(hWnd, point);
        return true;
      }
      case CFS_EXCLUDE: {
        point->x = cfCandForm[0].rcArea.left;
        point->y = cfCandForm[0].rcArea.bottom;
        ClientToScreen(hWnd, point);
        return true;
      }
    }
  }
  if (!GetCompositionPos(point))
    return false;
  point->y += GetFontHeight();
  return true;
}

bool InputContext::GetCompositionPos(POINT* point) {
  if (!point) {
    DCHECK(false);
    return false;
  }
  if (!IsWindow(hWnd))
    return false;

  if (fdwInit & INIT_COMPFORM) {
    if (!GetCaretPos(point))
      return false;
  } else {
    switch (cfCompForm.dwStyle) {
      case CFS_POINT:
      case CFS_FORCE_POSITION:
        *point = cfCompForm.ptCurrentPos;
        break;
      case CFS_RECT:
        point->x = cfCompForm.rcArea.left;
        point->y = cfCompForm.rcArea.top;
        break;
      default:
        if (!GetCaretPos(point))
          return false;
        break;
    }
  }
  ClientToScreen(hWnd, point);
  return true;
}

bool InputContext::GetCompositionBoundary(RECT *rect) {
  if (!(fdwInit & INIT_COMPFORM))
    return false;
  if (!IsWindow(hWnd))
    return false;
  if (!rect) {
    DCHECK(false);
    return false;
  }
  if (cfCompForm.dwStyle & CFS_RECT) {
    *rect = cfCompForm.rcArea;
    return true;
  }
  GetWindowRect(hWnd, rect);
  return true;
}

bool InputContext::GetCompositionFont(LOGFONT *font) {
  if (!IsWindow(hWnd))
    return false;
  if (!font) {
    DCHECK(false);
    return false;
  }

  if (fdwInit & INIT_LOGFONT) {
    *font = lfFont.W;
    return true;
  }

  HFONT current_font = reinterpret_cast<HFONT>(
      SendMessage(hWnd, WM_GETFONT, 0, 0));
  if (!current_font) {
    HDC hdc = GetDC(hWnd);
    current_font = reinterpret_cast<HFONT>(GetCurrentObject(hdc, OBJ_FONT));
    ReleaseDC(hWnd, hdc);
  }

  if (GetObject(current_font, sizeof(LOGFONT), font) == 0) {
    DLOG(WARNING) << __FUNCTION__
                  << L"Font info cannot be obtained.";
    return false;
  }
  return true;
}

int InputContext::GetFontHeight() {
  LOGFONT font;
  if (!GetCompositionFont(&font))
    return kDefaultFontHeight;
  LONG height = font.lfHeight;
  if (height == 0) {
    return kDefaultFontHeight;
  } else if (height < 0) {
    return -height;
  } else {
    return height;
  }
}

}  // namespace imm
}  // namespace ime_goopy
