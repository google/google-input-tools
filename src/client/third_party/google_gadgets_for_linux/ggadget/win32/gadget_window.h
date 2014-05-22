/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_WIN32_GADGET_WINDOW_H_
#define GGADGET_WIN32_GADGET_WINDOW_H_

#include <string>
#include <atlbase.h>
#include <atlwin.h>
#include <ggadget/common.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/slot.h>
#include <ggadget/view_host_interface.h>

namespace Gdiplus {
  class Bitmap;
}

namespace ggadget {

class Connection;

namespace win32 {

class MenuBuilder;

// This class implements the window on which the view will be shown.
// It will translate and post the messages to the view.
// If the system and the display support layered window, the window is set to
// layered window style when enable_input_mask is true to support transparent
// window, otherwise we use SetWindowRgn to set the shape of the window, and in
// this case, the window doesn't support perpixel alpha channel.
class GadgetWindow {
 public:
  GadgetWindow(ViewHostInterface* host,
               ViewInterface* view,
               double zoom,
               int window_class_style,
               int window_style,
               int window_exstyle);
  virtual ~GadgetWindow();
  bool Init();
  void QueueDraw();
  void QueueResize();
  void SetResizable(ViewInterface::ResizableMode mode);
  void ShowTooltip(const std::string &tooltip);
  void ShowTooltipAtPosition(const std::string &tooltip,
                             double x, double y);
  bool ShowViewWindow();
  void CloseWindow();
  void SetCaption(const std::string &caption);
  void SetEnableInputMask(bool enable);
  void SetWindowPosition(int x, int y);
  void GetWindowPosition(int* x, int* y);
  void SetMenuBuilder(MenuBuilder* menu_builder);
  void SetCursor(HCURSOR cursor);
  void GetWindowSize(int* width, int* height);
  // Returns a Gdiplus::Bitmap object that contains the image of the view.
  // The caller may not delete the return value because the bitmap is also used
  // in gadget window.
  const Gdiplus::Bitmap* GetViewContent();
  // Sets if the window is enabled.
  void Enable(bool enabled);
  // Sets the opacity of the whole window. the opacity will be multiplied to the
  // alpha value in the view.
  void SetOpacity(double opacity);
  // Connect a slot to OnEndMoveDrag signal.
  Connection* ConnectOnEndMoveDrag(Slot2<void, int, int>* handler);
  bool IsWindow();
  bool IsWindowVisible();
  HWND GetHWND();
  void SetZoom(double zoom);
  double GetZoom();

  LRESULT OnTimer(UINT message, WPARAM timer_id, LPARAM call_back,
                  BOOL& handled);
  LRESULT OnLButtonDoubleClick(UINT message,
                               WPARAM flag,
                               LPARAM position,
                               BOOL& handled);
  LRESULT OnLButtonDown(UINT message,
                        WPARAM flag,
                        LPARAM position,
                        BOOL& handled);
  LRESULT OnLButtonUp(UINT message,
                      WPARAM flag,
                      LPARAM position,
                      BOOL& handled);
  LRESULT OnMButtonDoubleClick(UINT message,
                               WPARAM flag,
                               LPARAM position,
                               BOOL& handled);
  LRESULT OnMButtonDown(UINT message,
                        WPARAM flag,
                        LPARAM position,
                        BOOL& handled);
  LRESULT OnMButtonUp(UINT message,
                      WPARAM flag,
                      LPARAM position,
                      BOOL& handled);
  LRESULT OnRButtonUp(UINT message,
                      WPARAM flag,
                      LPARAM position,
                      BOOL& handled);
  LRESULT OnRButtonDown(UINT message,
                        WPARAM flag,
                        LPARAM position,
                        BOOL& handled);
  LRESULT OnRButtonDoubleClick(UINT message,
                               WPARAM flag,
                               LPARAM position,
                               BOOL& handled);
  LRESULT OnMouseMove(UINT message,
                      WPARAM flag,
                      LPARAM position,
                      BOOL& handled);
  LRESULT OnMouseLeave(UINT message,
                       WPARAM flag,
                       LPARAM position,
                       BOOL& handled);
  LRESULT OnKeyDown(UINT message,
                    WPARAM key_code,
                    LPARAM flag,
                    BOOL& handled);
  LRESULT OnKeyUp(UINT message,
                  WPARAM key_code,
                  LPARAM flag,
                  BOOL& handled);
  LRESULT OnChar(UINT message,
                 WPARAM key_code,
                 LPARAM flag,
                 BOOL& handled);
  LRESULT OnKillFocus(UINT message,
                      WPARAM focus_window,
                      LPARAM lparam,
                      BOOL& handled);
  LRESULT OnSetFocus(UINT message,
                     WPARAM old_window,
                     LPARAM lparam,
                     BOOL& handled);
  LRESULT OnMouseWheel(UINT message,
                       WPARAM flag,
                       LPARAM position,
                       BOOL& handled);
  LRESULT OnDisplayChange(UINT message,
                          WPARAM image_depth,
                          LPARAM resolution,
                          BOOL& handled);
  LRESULT OnPaint(UINT message,
                  WPARAM wparam,
                  LPARAM lparam,
                  BOOL& handled);
  LRESULT OnCommand(UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled);
  LRESULT OnClose(UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled);
  LRESULT OnSetCursor(UINT message,
                      WPARAM wparam,
                      LPARAM lparam,
                      BOOL& handled);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;

  BEGIN_MSG_MAP(GadgetWindow)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDoubleClick)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
    MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnMButtonDoubleClick)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
    MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnRButtonDoubleClick)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
    MESSAGE_HANDLER(WM_CHAR, OnChar)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
    MESSAGE_HANDLER(WM_PAINT, OnPaint);
    MESSAGE_HANDLER(WM_COMMAND, OnCommand);
    MESSAGE_HANDLER(WM_CLOSE, OnClose);
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);
  END_MSG_MAP()
  DISALLOW_EVIL_CONSTRUCTORS(GadgetWindow);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_GADGET_WINDOW_H_
