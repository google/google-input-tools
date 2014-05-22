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

#ifndef GOOPY_COMMON_BASE_WINDOW_H__
#define GOOPY_COMMON_BASE_WINDOW_H__

#include <atlbase.h>
#include <atlwin.h>

#include <algorithm>

#include "base/basictypes.h"
#include "common/window_shadow.h"

namespace ime_goopy {
namespace common {
// Other code should use this wrapper class to call Windows API, so that it's
// possible to replace this class with a mock one to write unit tests.
class WindowsAPIWrapper {
 public:
  inline BOOL GetCursorPos(POINT* point) {
    return ::GetCursorPos(point);
  }

  inline HCURSOR SetCursor(HCURSOR cursor) {
    return ::SetCursor(cursor);
  }

  inline HCURSOR LoadCursor(HINSTANCE instance, LPCTSTR cursor_name) {
    return ::LoadCursor(instance, cursor_name);
  }

  inline BOOL ReleaseCapture() {
    return ::ReleaseCapture();
  }

  inline HMONITOR MonitorFromPoint(POINT pt, DWORD flags) {
    return ::MonitorFromPoint(pt, flags);
  }

  inline BOOL GetMonitorInfo(HMONITOR monitor, LPMONITORINFO lpmi) {
    return ::GetMonitorInfo(monitor, lpmi);
  }
};

// IME window should not has input focus, so it must be disabled and can not
// receive any mouse messages. Fortunately, it can receive WM_SETCURSOR message.
// Use this macro, you can handle the mouse events in WM_SETCURSOR.
//
// When user click on a disabled window, the windows system will beep by
// default. To override this behavior, you should always handle WM_SETCURSOR
// yourself and do not pass this message to default windows message handler.
// You should put your WM_SETCURSOR handler after SETCURSOR_HANDLER.

// TODO(zengjian): The "standard" way to implement non-active window is to
// add a WM_MOUSEACTIVE handler, rather than disabling the window.
// See code in Toolbar and Chrome. In that way we can still catch mouse
// events like a normal window.
// Use the "standard" implementation for all the non-active windows.

#define SETCURSOR_HANDLER(msg, func) \
  if ((uMsg == WM_SETCURSOR) && (HIWORD(lParam) == msg)) { \
    bHandled = TRUE; \
    POINT point; \
    GetCursorPos(&point); \
    ScreenToClient(&point); \
    lResult = !func(msg, 0, MAKELONG(point.x, point.y), bHandled); \
    if (bHandled) return TRUE; \
  }

// Same as above but used in class template.
#define TEMPLATE_SETCURSOR_HANDLER(msg, func) \
  if ((uMsg == WM_SETCURSOR) && (HIWORD(lParam) == msg)) { \
    bHandled = TRUE; \
    POINT point; \
    static_cast<T*>(this)->GetCursorPos(&point); \
    static_cast<T*>(this)->ScreenToClient(&point); \
    lResult = !func(msg, 0, MAKELONG(point.x, point.y), bHandled); \
    if (bHandled) return TRUE; \
  }

// Usually, you can call TrackMouseEvent and then get notified if the mouse is
// leaving your window. However, this doesn't work for a disabled window.
// This class emulates this behavior by using a timer. Your inherited class
// will receive a WM_MOUSEHOVER if the mouse is entering the window, and a
// WM_MOUSELEAVE if it's leaving.
template <class T>
class MouseLeavingTracker {
 public:
  BEGIN_MSG_MAP(MouseLeavingTracker)
    TEMPLATE_SETCURSOR_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
  END_MSG_MAP()

  MouseLeavingTracker() : tracking_mouse_(true), mouse_inside_(false) {
  }

  LRESULT OnMouseMove(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    // Track mouse and send notify if the mouse cursor leaves the window.
    T* self = static_cast<T*>(this);
    if (tracking_mouse_ && !mouse_inside_) {
      mouse_inside_ = true;
      self->SendMessage(WM_MOUSEHOVER, 0, 0);
      self->SetTimer(
          kMouseLeaveCheckingTimerID,
          kMouseLeaveCheckingTimerInteval,
          NULL);
    }

    handled = FALSE;
    // Returning a non-zero value means we don't process this message.
    return 1;
  }

  LRESULT OnTimer(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    T* self = static_cast<T*>(this);
    if (wparam == kMouseLeaveCheckingTimerID) {
      POINT point;
      self->GetCursorPos(&point);
      RECT rect;
      self->GetWindowRect(&rect);
      if (!PtInRect(&rect, point)) {
        mouse_inside_ = false;
        self->KillTimer(kMouseLeaveCheckingTimerID);
        self->SendMessage(WM_MOUSELEAVE, 0, 0);
      }
      return 0;
    } else {
      handled = FALSE;
      // Returning a non-zero value means we don't process this message.
      return 1;
    }
  }

  void set_tracking_mouse(bool value) { tracking_mouse_ = value; }
  bool mouse_inside() { return mouse_inside_; }

  static const int kMouseLeaveCheckingTimerID = 1325;
  static const int kMouseLeaveCheckingTimerInteval = 200;

 private:
  bool tracking_mouse_;
  bool mouse_inside_;
};

// You can make a window dragable by inherit this class. If you want disable
// dragging for some time or some area of the window, don't pass WM_LBUTTONDOWN
// to this class.
template <class T>
class DragableWindow {
 public:
  BEGIN_MSG_MAP(DragableWindow)
    TEMPLATE_SETCURSOR_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    TEMPLATE_SETCURSOR_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    TEMPLATE_SETCURSOR_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    // After SetCapture(), system will send real mouse messages to our window,
    // so handle them as well.
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
  END_MSG_MAP()

  DragableWindow() : is_dragging_(false) {
  }

  LRESULT OnSetCursor(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    POINT cursor;
    T* self = static_cast<T*>(this);
    self->GetCursorPos(&cursor);
    self->ScreenToClient(&cursor);

    if (!self->IsInDraggableRect(cursor)) {
      handled = FALSE;
      return 1;
    }
    if (HIWORD(lparam) != 0) {
      self->SetCursor(
          self->LoadCursor(NULL, IDC_SIZEALL));
    }
    return 0;
  }

  LRESULT OnLButtonDown(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    // Record the coordinate offset from the cursor and window position.
    POINT cursor;
    T* self = static_cast<T*>(this);
    self->GetCursorPos(&cursor);
    POINT test_cursor = cursor;
    self->ScreenToClient(&test_cursor);
    if (!self->IsInDraggableRect(test_cursor)) {
      handled = FALSE;
      return 1;
    }
    RECT rect;
    self->GetWindowRect(&rect);
    offset_cursor_to_window_.cx = cursor.x - rect.left;
    offset_cursor_to_window_.cy = cursor.y - rect.top;
    is_dragging_ = true;
    self->SetCapture();
    self->SetWindowPos(HWND_TOP, 0, 0, 0, 0,
                       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER |
                       SWP_NOSIZE);
    return 0;
  }

  LRESULT OnLButtonUp(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    if (!is_dragging_) {
      handled = FALSE;
      // Returning a non-zero value means we don't process this message.
      return 1;
    }
    is_dragging_ = false;
    T* self = static_cast<T*>(this);
    self->ReleaseCapture();
    self->OnDragComplete();
    return 0;
  }

  LRESULT OnMouseMove(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    if (!is_dragging_) {
      handled = FALSE;
      // Returning a non-zero value means we don't process this message.
      return 1;
    }

    POINT cursor;
    T* self = static_cast<T*>(this);
    self->GetCursorPos(&cursor);
    POINT window = {
      cursor.x - offset_cursor_to_window_.cx,
      cursor.y - offset_cursor_to_window_.cy,
    };

    RECT rect;
    self->GetWindowRect(&rect);
    SIZE window_size = {
      rect.right - rect.left,
      rect.bottom - rect.top,
    };
    self->AdjustInDesktop(cursor, window_size, &window);

    self->SetWindowPos(
        NULL,
        window.x,
        window.y,
        0,
        0,
        SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    self->SetCursor(
        self->LoadCursor(NULL, IDC_SIZEALL));

    return 0;
  }

  // Use referrence to get the current desktop, and change the point value to
  // adjust the window inside that desktop.
  void AdjustInDesktop(const POINT& reference,
                       const SIZE& window_size,
                       POINT* window_point) {
    T* self = static_cast<T*>(this);
    HMONITOR monitor = self->MonitorFromPoint(
        reference, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    self->GetMonitorInfo(monitor, &monitor_info);

    if (window_point->x < monitor_info.rcWork.left)
      window_point->x = monitor_info.rcWork.left;
    if (window_point->y < monitor_info.rcWork.top)
      window_point->y = monitor_info.rcWork.top;
    if (window_point->x + window_size.cx > monitor_info.rcWork.right)
      window_point->x = max(monitor_info.rcWork.right - window_size.cx,
                            monitor_info.rcWork.left);
    if (window_point->y + window_size.cy > monitor_info.rcWork.bottom)
      window_point->y = max(monitor_info.rcWork.bottom - window_size.cy,
                            monitor_info.rcWork.top);
  }

  bool IsInDesktop(const POINT& reference,
                   const SIZE& window_size,
                   const POINT& window_point) {
    T* self = static_cast<T*>(this);
    HMONITOR monitor = self->MonitorFromPoint(
        reference, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    self->GetMonitorInfo(monitor, &monitor_info);

    return window_point.x >= monitor_info.rcWork.left &&
           window_point.y >= monitor_info.rcWork.top &&
           window_point.x + window_size.cx <= monitor_info.rcWork.right &&
           window_point.y + window_size.cy <= monitor_info.rcWork.bottom;
  }

  // Nofity.
  void OnDragComplete() { }

  bool IsInDraggableRect(const POINT& cursor) {
    return true;
  }

 private:
  SIZE offset_cursor_to_window_;
  bool is_dragging_;
};

// Enable shadow for the window.
template <class T>
class DropShadowWindow {
 public:
  BEGIN_MSG_MAP(DropShadowWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowsPosChanged)
  END_MSG_MAP()

  void set_alpha(int alpha) {
    DCHECK_GE(alpha, 0);
    DCHECK_LE(alpha, 255);
    shadow_.set_alpha(alpha);
  }

  void ShowShadow() {
    shadow_.UpdatePosition(static_cast<T*>(this)->m_hWnd);
    shadow_.Show(true);
  }

 private:
  LRESULT OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    CREATESTRUCT* info = reinterpret_cast<CREATESTRUCT*>(lparam);
    shadow_.Create(info->hInstance, GetParent(static_cast<T*>(this)->m_hWnd));
    handled = FALSE;
    return 0;
  }

  LRESULT OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    shadow_.Destroy();
    handled = FALSE;
    return 0;
  }

  LRESULT OnShowWindow(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    shadow_.Show(wparam != 0);
    handled = FALSE;
    return 0;
  }

  LRESULT OnWindowsPosChanged(UINT msg,
                              WPARAM wparam,
                              LPARAM lparam,
                              BOOL& handled) {
    WINDOWPOS* window_pos = reinterpret_cast<WINDOWPOS*>(lparam);
    shadow_.UpdatePosition(static_cast<T*>(this)->m_hWnd);
    handled = FALSE;
    return 0;
  }

  WindowShadow shadow_;
};

// This class is for creating layered windows (i.e. translucent windows).
// Need a bitmap with alpha channel for the content of the window.
template <class T>
class LayeredWindow {
 public:
  void EnableLayered() {
    static_cast<T*>(this)->SetWindowLongPtr(GWL_EXSTYLE, WS_EX_LAYERED);
  }

  // Display a bitmap, and use alpha channel from bitmap.
  void SetAlphaBitmap(HDC dest_dc, POINT dest_point, SIZE window_size,
                      HDC src_dc, POINT src_point, int alpha) {
    BLENDFUNCTION bf;

    bf.AlphaFormat = AC_SRC_ALPHA;
    bf.BlendFlags = 0;
    bf.BlendOp = AC_SRC_OVER;
    bf.SourceConstantAlpha = alpha;

    ::UpdateLayeredWindow(static_cast<T*>(this)->m_hWnd,
                          dest_dc,
                          &dest_point,
                          &window_size,
                          src_dc,
                          &src_point,
                          0,
                          &bf,
                          ULW_ALPHA);
  }
};

template <class T>
class ResizableWindow {
 public:
  BEGIN_MSG_MAP(ResizableWindow)
    TEMPLATE_SETCURSOR_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    TEMPLATE_SETCURSOR_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    TEMPLATE_SETCURSOR_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    // After SetCapture(), system will send real mouse messages to our window,
    // so handle them as well.
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
  END_MSG_MAP()

  ResizableWindow()
      : border_width_(0), is_resizing_(false), border_type_(BORDER_NONE) {
  }

  LRESULT OnSetCursor(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    POINT cursor;
    T* self = static_cast<T*>(this);
    self->GetCursorPos(&cursor);
    self->ScreenToClient(&cursor);

    BorderType border_type;
    if (!IsOnWindowBorder(cursor, &border_type)) {
      handled = FALSE;
      return 1;
    }
    if (HIWORD(lparam) != 0) {
      SetCursorByBorderType(border_type);
    }
    return 0;
  }

  LRESULT OnLButtonDown(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    // Record the coordinate offset from the cursor and window position.
    POINT cursor;
    T* self = static_cast<T*>(this);
    self->GetCursorPos(&cursor);
    self->ScreenToClient(&cursor);
    if (!IsOnWindowBorder(cursor, &border_type_)) {
      handled = FALSE;
      return 1;
    }
    is_resizing_ = true;
    self->SetCapture();
    return 0;
  }

  LRESULT OnLButtonUp(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    if (!is_resizing_) {
      handled = FALSE;
      // Returning a non-zero value means we don't process this message.
      return 1;
    }
    is_resizing_ = false;
    T* self = static_cast<T*>(this);
    self->ReleaseCapture();
    return 0;
  }

  LRESULT OnMouseMove(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    if (!is_resizing_) {
      handled = FALSE;
      // Returning a non-zero value means we don't process this message.
      return 1;
    }

    POINT cursor;
    T* self = static_cast<T*>(this);
    self->GetCursorPos(&cursor);
    RECT rect;
    self->GetWindowRect(&rect);

    SIZE window_size = {0};
    switch (border_type_) {
      case BORDER_TOPLEFT:
        window_size.cx = rect.right - cursor.x;
        window_size.cy = rect.bottom - cursor.y;
        break;
      case BORDER_TOP:
        window_size.cx = rect.right - rect.left;
        window_size.cy = rect.bottom - cursor.y;
        break;
      case BORDER_TOPRIGHT:
        window_size.cx = cursor.x - rect.left;
        window_size.cy = rect.bottom - cursor.y;
        break;
      case BORDER_LEFT:
        window_size.cx = rect.right - cursor.x;
        window_size.cy = rect.bottom - rect.top;
        break;
      case BORDER_RIGHT:
        window_size.cx = cursor.x - rect.left;
        window_size.cy = rect.bottom - rect.top;
        break;
      case BORDER_BOTTOMLEFT:
        window_size.cx = rect.right - cursor.x;
        window_size.cy = cursor.y - rect.top;
        break;
      case BORDER_BOTTOM:
        window_size.cx = rect.right - rect.left;
        window_size.cy = cursor.y - rect.top;
        break;
      case BORDER_BOTTOMRIGHT:
        window_size.cx = cursor.x - rect.left;
        window_size.cy = cursor.y - rect.top;
        break;
      default:
        NOTREACHED();
    }
    if (window_size.cx < min_window_size_.cx) {
      window_size.cx = min_window_size_.cx;
    }
    if (window_size.cy < min_window_size_.cy) {
      window_size.cy = min_window_size_.cy;
    }
    POINT window;
    switch (border_type_) {
      case BORDER_TOPLEFT:
      case BORDER_TOP:
      case BORDER_TOPRIGHT:
        window.y = rect.bottom - window_size.cy;
        break;
      default:
        window.y = rect.top;
    }
    switch (border_type_) {
      case BORDER_TOPLEFT:
      case BORDER_LEFT:
      case BORDER_BOTTOMLEFT:
        window.x = rect.right - window_size.cx;
        break;
      default:
        window.x = rect.left;
    }
    self->SetWindowPos(
        NULL,
        window.x,
        window.y,
        window_size.cx,
        window_size.cy,
        SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER);
    self->InvalidateRect(NULL, FALSE);
    SetCursorByBorderType(border_type_);
    return 0;
  }

  void set_border_width(int pixel) {
    DCHECK_GE(pixel, 0);
    border_width_ = pixel;
  }
  void set_min_window_size(const SIZE& size) {
    min_window_size_ = size;
  }

 private:
  enum BorderType {
    BORDER_NONE = 0,
    BORDER_TOPLEFT,
    BORDER_TOP,
    BORDER_TOPRIGHT,
    BORDER_LEFT,
    BORDER_RIGHT,
    BORDER_BOTTOMLEFT,
    BORDER_BOTTOM,
    BORDER_BOTTOMRIGHT,
  };
  bool IsOnWindowBorder(const POINT& cursor, BorderType* border_type) {
    RECT rect;
    T* self = static_cast<T*>(this);
    self->GetClientRect(&rect);
    if (cursor.y < border_width_) {
      if (border_type)
        *border_type = (cursor.x < border_width_) ? BORDER_TOPLEFT :
                       (cursor.x > rect.right - border_width_) ?
                       BORDER_TOPRIGHT : BORDER_TOP;
      return true;
    } else if (cursor.y > rect.bottom - border_width_) {
      if (border_type)
        *border_type = (cursor.x < border_width_) ? BORDER_BOTTOMLEFT :
                       (cursor.x > rect.right - border_width_) ?
                       BORDER_BOTTOMRIGHT : BORDER_BOTTOM;
      return true;
    } else {
      if (cursor.x < border_width_) {
        if (border_type) *border_type = BORDER_LEFT;
        return true;
      }
      if (cursor.x > rect.right - border_width_) {
        if (border_type) *border_type = BORDER_RIGHT;
        return true;
      }
    }
    return false;
  }
  void SetCursorByBorderType(BorderType border_type) {
    T* self = static_cast<T*>(this);
    switch (border_type) {
      case BORDER_TOPLEFT:
      case BORDER_BOTTOMRIGHT:
        self->SetCursor(
            self->LoadCursor(NULL, IDC_SIZENWSE));
        break;
      case BORDER_TOPRIGHT:
      case BORDER_BOTTOMLEFT:
        self->SetCursor(
            self->LoadCursor(NULL, IDC_SIZENESW));
        break;
      case BORDER_TOP:
      case BORDER_BOTTOM:
        self->SetCursor(
            self->LoadCursor(NULL, IDC_SIZENS));
        break;
      case BORDER_LEFT:
      case BORDER_RIGHT:
        self->SetCursor(
            self->LoadCursor(NULL, IDC_SIZEWE));
        break;
      default:
        NOTREACHED();
    }
  }
  int border_width_;
  bool is_resizing_;
  BorderType border_type_;
  SIZE min_window_size_;
};

}  // namespace common
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_BASE_WINDOW_H__
