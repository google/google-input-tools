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

#include "gadget_window.h"

#include <windows.h>
// Gdiplus.h uses the function min and max but doesn't define them, so use these
// two function before include gdiplus.h
using std::min;
using std::max;
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
#include <gdiplus.h>
#include <string>
#include <ggadget/clip_region.h>
#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/view.h>
#include <ggadget/view_host_interface.h>
#include "gdiplus_canvas.h"
#include "gdiplus_graphics.h"
#include "key_convert.h"
#include "menu_builder.h"
#include "single_view_host.h"

namespace ggadget {
namespace win32 {

namespace {

// Used to avoid sensitive dragging.
static const int kDragThreshold = 2;
static const int kBitsPerPixel = 32;
static const int kBytesPerPixel = kBitsPerPixel >> 3;
static const int kToolTipDuration = 2000;
// Minimal interval between queue draws.
static const int kQueueDrawInterval = 40;
static const int kShowWindowDelay = 25;
static const int kMouseLeaveCheckingTimerInteval = 200;
static const wchar_t kWindowClassName[] = L"GadgetWindow";

inline BYTE GetAlpha(uint32_t color) {
  return LOBYTE(color >> 24);
}

// Gets the bitmap info header of the given device independent bitmap.
void GetBitmapSize(HBITMAP bitmap, int* w, int* h) {
  BITMAPINFO bitmap_info = {0};
  HDC dc = ::GetDC(NULL);
  bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  // Set biBitCount to retrieve bitmap information.
  bitmap_info.bmiHeader.biBitCount = 0;
  ::GetDIBits(dc, bitmap, 0, 1, NULL, &bitmap_info,
              DIB_RGB_COLORS);
  ::ReleaseDC(NULL, dc);
  *w = bitmap_info.bmiHeader.biWidth;
  *h = abs(bitmap_info.bmiHeader.biHeight);
}

// Creates a HRGN object from the pixels that is not fully transparent in the
// bitmap. This function will alpha blend the bitmap with white color and save
// to the bitmap.
// Fix bug http://b/issue?id=5344519
// In some version of rdp, gdiplus will use dithering instead of alpha blending
// when dealing non-opaque images to screen, which make the view look terrible.
// So we use bitblt instead of gdi+ drawing method to update the window. And
// before painting, we need to alpha blend the pixel whose alpha is not 0 with
// white background to make the window looks perfect in white background. In
// the case of layered window, the bitmap is directly provided to the system
// so we don't need to pre-blend it.
// @param bitmap: the bitmap handle.
// @param bitmap_data[INOUT]: the raw bytes of the bitmap data, should be 32
//      bits per pixel with alpha channel.
HRGN CreateRegionFromImageAndBlendImage(HBITMAP bitmap,
                                        uint8_t* bitmap_data) {
  HRGN region = NULL;
  uint8_t* data = bitmap_data;
  int width = 0;
  int height = 0;
  GetBitmapSize(bitmap, &width, &height);
  // kBytesPerPixel is 4 so we don't need to align pixels to word.
  int stride = width * kBytesPerPixel;
  static const int kMaxRectCount = 1024;
  int rgn_data_size =  sizeof(RGNDATAHEADER) + sizeof(RECT) * kMaxRectCount;
  HANDLE global_data = GlobalAlloc(GMEM_MOVEABLE, rgn_data_size);
  RGNDATA* rgn_data = reinterpret_cast<RGNDATA*>(GlobalLock(global_data));
  rgn_data->rdh.dwSize = sizeof(RGNDATAHEADER);
  rgn_data->rdh.iType = RDH_RECTANGLES;
  rgn_data->rdh.nCount = 0;
  rgn_data->rdh.nRgnSize = 0;
  SetRect(&rgn_data->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
  for (int y = 0; y < height; y++) {
    // Scan each bitmap pixel from left to right
    for (int x = 0; x < width; x++) {
      // Search for a continuous range of "non transparent pixels"
      int x0 = x;
      uint32_t* color = reinterpret_cast<uint32_t*>(data) + x;
      while (x < width && GetAlpha(*color) != 0) {
        BYTE a = GetAlpha(*color);
        if (a != 255) {
          // Blend with white color and discard the alpha channel.
          // Given the foreground color (r_1, g_1, b_1) and (r_2, g_2, b_2) and
          // the alpha a_1, the result r channel will be:
          // r = r_1*a_1/255 + (255-a_1)/255*r_2
          // Notice that the color is actually pre-multiplied with alpha
          // r_1 = r_1*a_1/255
          // and the background color is white r2 = 255
          // so r = r_1 + 255 - a.
          BYTE neg_alpha = 255 - a;
          *color += ((neg_alpha << 16) | (neg_alpha << 8) | neg_alpha);
          *color |= 255UL << 24;
        }
        ++color;
        ++x;
      }
      if (x > x0) {
        // Add the rectangle (x0, y) to (x, y+1) in the region.
        RECT* rect_buffer = reinterpret_cast<RECT *>(&rgn_data->Buffer);
        SetRect(&rect_buffer[rgn_data->rdh.nCount], x0, y, x, y + 1);
        if (x0 < rgn_data->rdh.rcBound.left)
          rgn_data->rdh.rcBound.left = x0;
        if (y < rgn_data->rdh.rcBound.top)
          rgn_data->rdh.rcBound.top = y;
        if (x > rgn_data->rdh.rcBound.right)
          rgn_data->rdh.rcBound.right = x;
        if (y + 1 > rgn_data->rdh.rcBound.bottom)
          rgn_data->rdh.rcBound.bottom = y + 1;
        ++rgn_data->rdh.nCount;
        // Create or extend the region with the rectangles
        if (rgn_data->rdh.nCount == kMaxRectCount) {
          HRGN new_region = ExtCreateRegion(NULL, rgn_data_size, rgn_data);
          if (region) {
            CombineRgn(region, region, new_region, RGN_OR);
            DeleteObject(new_region);
          } else {
            region = new_region;
          }
          rgn_data->rdh.nCount = 0;
          SetRect(&rgn_data->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
        }
      }
    }
    data += stride;  // Process next row.
  }
  // Create or extend the region with the remaining rectangles
  HRGN new_region = ExtCreateRegion(NULL, rgn_data_size, rgn_data);
  if (region) {
    CombineRgn(region, region, new_region, RGN_OR);
    DeleteObject(new_region);
  } else {
    region = new_region;
  }
  GlobalUnlock(global_data);
  GlobalFree(global_data);
  return region;
}

}  // namespace

class GadgetWindow::Impl {
 public:
  // Timer Identifiers.
  enum TimerID {
    QUEUE_DRAW = 0x10001,  // draw timer
    TOOL_TIP_HIDE,  // tool tip timer that is responsible to hide the tooltip.
    DETECT_MOUSE_LEAVE,  // timer that detects if the mouse leave the window.
    SHOW_WINDOW  // timer to show or hide window.
  };

  // The resize direction of the window.
  struct ResizeDirection {
    ResizeDirection() : left(false), top(false), bottom(false), right(false) {
    }
    bool left;  // True if the user is resizing the window by dragging the left
                // border or one of the left corner.
    bool top;  // True if the user is resizing the window by dragging the top
               // border or one of the left corner.
    bool bottom;  // True if the user is resizing the window by dragging the
                  // bottom border or one of the left corner.
    bool right;  // True if the user is resizing the window by dragging the
                 // right border or one of the left corner.
  };

  Impl(GadgetWindow* gadget_window,
       ViewHostInterface* host,
       ViewInterface* view,
       double zoom,
       int window_class_style,
       int window_style,
       int window_exstyle)
       : enable_input_mask_(true),
         is_cursor_in_window_(false),
         is_focused_(false),
         is_layered_window_(false),
         is_left_button_press_on_nothing_(false),
         is_mouse_dragging_(false),
         is_resizing_(false),
         is_window_rect_changed_(false),
         queue_draw_(false),
         queue_resize_(false),
         zoom_(zoom),
         gadget_window_(gadget_window),
         buffer_bitmap_(NULL),
         cursor_(NULL),
         buffer_dc_(NULL),
         invalidate_region_(NULL),
         window_region_(NULL),
         native_window_(NULL),
         image_depth_(0),
         show_window_command_(0),
         window_class_style_(window_class_style),
         window_exstyle_(window_exstyle),
         window_style_(window_style),
         menu_(NULL),
         canvas_(NULL),
         tool_tip_text_(L""),
         host_(host),
         view_(view),
         mouse_down_hittest_(ViewInterface::HT_TRANSPARENT),
         resizable_(ViewInterface::RESIZABLE_FALSE),
         buffer_bits_(NULL) {
    ASSERT(gadget_window_);
    ASSERT(host_);
    ASSERT(view_);
    ASSERT(zoom_);
  }

  ~Impl() {
    if (buffer_bitmap_ != NULL) DeleteObject(buffer_bitmap_);
    if (buffer_dc_ != NULL) DeleteDC(buffer_dc_);
    if (tool_tip_.IsWindow()) ::DestroyWindow(tool_tip_);
    if (window_region_) ::DeleteObject(window_region_);
    if (invalidate_region_) ::DeleteObject(invalidate_region_);
    if (IsWindow()) {
      // The data may be invalid when destroy the GadgetWindow, so set
      // GWLP_USERDATA to prevent the window from processing messages.
      ::SetWindowLongPtr(native_window_, GWLP_USERDATA, NULL);
      ::DestroyWindow(native_window_);
    }
    native_window_ = NULL;
  }

  bool IsWindow() {
    return native_window_ && ::IsWindow(native_window_);
  }

  // Updates the window region according to the buffer bitmap.
  void UpdateWindowRegion() {
    int w = window_rect_.right - window_rect_.left;
    int h = window_rect_.bottom - window_rect_.top;
    int bitmap_width = 0;
    int bitmap_height = 0;
    GetBitmapSize(buffer_bitmap_, &bitmap_width, &bitmap_height);
    if (window_region_) ::DeleteObject(window_region_);
    // If this window is not a layered window, it will use WM_PAINT to update
    // its content, as Gdi+ don't work well when drawing on windows in RDP,
    // and BitBlt will direct discard the alpha channel, we need to blend the
    // bitmap with white background to make it looks good in parent window.
    window_region_ = CreateRegionFromImageAndBlendImage(
        buffer_bitmap_, reinterpret_cast<uint8_t*>(buffer_bits_));
  }

  static LRESULT __stdcall WindowProc(HWND hwnd,
                                      UINT message,
                                      WPARAM wparam,
                                      LPARAM lparam) {
    GadgetWindow* gadget_window = reinterpret_cast<GadgetWindow*>(
        ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    LRESULT result = 0;
    if (!gadget_window ||
        !gadget_window->IsWindow() ||
        !gadget_window->ProcessWindowMessage(hwnd, message, wparam, lparam,
                                             result)) {
      return ::DefWindowProc(hwnd, message, wparam, lparam);
    }
    return result;
  }

  bool Init() {
    wchar_t class_name[MAX_PATH] = {0};
    wsprintf(class_name, L"%s_%X", kWindowClassName, window_class_style_);
    WNDCLASSEX wnd_class = { 0 };
    wnd_class.cbSize = sizeof(WNDCLASSEX);
    if (!::GetClassInfoEx(_AtlBaseModule.GetModuleInstance(), class_name,
                          &wnd_class)) {
      wnd_class.lpfnWndProc = &GadgetWindow::Impl::WindowProc;
      wnd_class.style = window_class_style_;
      wnd_class.hInstance = _AtlBaseModule.GetModuleInstance();
      wnd_class.lpszClassName = class_name;
      wnd_class.cbWndExtra = sizeof(LONG);
      ::RegisterClassEx(&wnd_class);
    }
    native_window_ = ::CreateWindowEx(window_exstyle_ | WS_EX_TOOLWINDOW,
                                      class_name,
                                      NULL,
                                      window_style_,
                                      0, 0, 0, 0,
                                      NULL,
                                      NULL,
                                      _AtlBaseModule.GetModuleInstance(),
                                      NULL);
    if (!::IsWindow(native_window_))
      return false;
    ::SetWindowLongPtr(native_window_, GWLP_USERDATA,
                       reinterpret_cast<LONG_PTR>(gadget_window_));
    ::SetWindowLong(native_window_, GWL_STYLE, window_style_);
    // Creates tool tip control.
    tool_tip_.Create(TOOLTIPS_CLASS, NULL, NULL, NULL,
                     WS_POPUP | TTS_NOPREFIX,
                     WS_EX_TOPMOST);
    cursor_ = ::LoadCursor(NULL, IDC_ARROW);
    if (!tool_tip_.IsWindow()) return false;
    tool_info_.cbSize = sizeof(TOOLINFO);
    tool_info_.hinst = ATL::_AtlBaseModule.GetResourceInstance();
    tool_info_.hwnd = native_window_;
    tool_info_.uFlags = TTF_IDISHWND;
    tool_info_.uId = reinterpret_cast<UINT_PTR>(native_window_);
    // Initializes the blend function for layered window.
    blend_.BlendOp = AC_SRC_OVER;
    blend_.BlendFlags = 0;
    blend_.AlphaFormat = AC_SRC_ALPHA;
    blend_.SourceConstantAlpha = 255;  // set alpha to 1

    // Get system resolution and image depth.
    HDC hdcScreen = ::CreateDC(L"DISPLAY", NULL, NULL, NULL);
    screen_size_.cx = ::GetDeviceCaps(hdcScreen, HORZRES);
    screen_size_.cy = ::GetDeviceCaps(hdcScreen, VERTRES);
    image_depth_ = ::GetDeviceCaps(hdcScreen, BITSPIXEL);
    ::DeleteDC(hdcScreen);

    if (view_ != NULL) {
      window_rect_.left = 0;
      window_rect_.right = static_cast<int>(round(view_->GetWidth()));
      window_rect_.top = 0;
      window_rect_.bottom = static_cast<int>(round(view_->GetHeight()));
      is_window_rect_changed_ = true;
    }
    ::SetTimer(native_window_, QUEUE_DRAW, kQueueDrawInterval, NULL);
    return true;
  }

  void ShowTooltip(const std::string &tooltip, int x, int y) {
    if (!IsWindow()) return;
    if (tool_tip_.IsWindow()) tool_tip_.DestroyWindow();
    tool_tip_.Create(TOOLTIPS_CLASS, NULL, NULL, NULL,
                     WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                     WS_EX_TOPMOST);
    ConvertStringUTF8ToUTF16(tooltip, &tool_tip_text_);
    tool_info_.lpszText = const_cast<LPWSTR>(tool_tip_text_.c_str());
    RECT window_rect;
    ::GetWindowRect(native_window_, &window_rect);
    tool_info_.rect = window_rect;
    ::SendMessage(tool_tip_.m_hWnd, TTM_ACTIVATE, true,
                  reinterpret_cast<LPARAM>(&tool_info_));
    ::SendMessage(tool_tip_.m_hWnd, TTM_ADDTOOL, 0,
                  reinterpret_cast<LPARAM>(&tool_info_));
    tool_tip_.SetWindowPos(HWND_TOPMOST, x, y, 0, 0,
                           SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREPOSITION);
  }

  bool ShowViewWindow() {
    DrawView();
    show_window_command_ = SW_SHOW;
    if (!::IsWindowEnabled(native_window_))
      show_window_command_ = SW_SHOWNA;
    ::SetTimer(native_window_, SHOW_WINDOW, kShowWindowDelay, NULL);
    return true;
  }

  void CloseWindow() {
    show_window_command_ = 0;
    ::KillTimer(native_window_, SHOW_WINDOW);
    ::ShowWindow(native_window_, SW_HIDE);
  }

  void HideTooltip() {
    ::SendMessage(tool_tip_.m_hWnd, TTM_ACTIVATE, false,
                  reinterpret_cast<LPARAM>(&tool_info_));
  }

  // TODO(synch): Integrate the mouse event processing functions to reduce code
  // redundancy.
  LRESULT OnLButtonDoubleClick(WPARAM flag, LPARAM position) {
    EventResult result = EVENT_RESULT_UNHANDLED;
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_LEFT;
    Event::Type type = Event::EVENT_MOUSE_DBLCLICK;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    result = view_->OnMouseEvent(event);
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnLButtonDown(WPARAM flag, LPARAM position) {
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_LEFT;
    Event::Type type = Event::EVENT_MOUSE_DOWN;
    cursor_pos_.x = x;
    cursor_pos_.y = y;
    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    result = view_->OnMouseEvent(event);

    mouse_down_hittest_ = view_->GetHitTest();
    // If the View's hittest represents a special button, then handle it
    // here.
    if (result == EVENT_RESULT_UNHANDLED) {
      if (mouse_down_hittest_ == ViewInterface::HT_MENU)
        host_->ShowContextMenu(button);
      else if (mouse_down_hittest_ == ViewInterface::HT_CLOSE)
        host_->CloseView();
      result = EVENT_RESULT_HANDLED;
      is_left_button_press_on_nothing_ = true;
    }
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnLButtonUp(WPARAM flag, LPARAM position) {
    if (IsWindow())
      ::SetFocus(native_window_);

    is_left_button_press_on_nothing_ = false;

    if (is_mouse_dragging_) {
      is_mouse_dragging_ = false;
      if (is_resizing_) {
        is_resizing_ = false;
        gadget_window_->QueueResize();
      } else {
        on_end_move_drag_signal_(window_rect_.left, window_rect_.top);
      }
      return 0;
    } else {
      int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
      int button = MouseEvent::BUTTON_LEFT;
      int x = GET_X_LPARAM(position);
      int y = GET_Y_LPARAM(position);
      Event::Type type = Event::EVENT_MOUSE_UP;
      ASSERT(zoom_ != 0);
      MouseEvent event_up(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
      EventResult result_up = view_->OnMouseEvent(event_up);
      type = Event::EVENT_MOUSE_CLICK;
      MouseEvent event_click(
          type, x / zoom_, y / zoom_, 0, 0, button, modifier);
      EventResult result_click = view_->OnMouseEvent(event_click);
      return (result_click != EVENT_RESULT_UNHANDLED) ? 0 : -1;
    }
  }

  LRESULT OnRButtonUp(WPARAM flag, LPARAM position) {
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_RIGHT;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    Event::Type type = Event::EVENT_MOUSE_UP;
    ASSERT(zoom_ != 0);
    MouseEvent event_up(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    EventResult result_up = view_->OnMouseEvent(event_up);
    type = Event::EVENT_MOUSE_RCLICK;
    MouseEvent event_click(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    EventResult result_click = view_->OnMouseEvent(event_click);
    if (result_up == EVENT_RESULT_UNHANDLED &&
        result_click == EVENT_RESULT_UNHANDLED) {
      return host_->ShowContextMenu(button) ? 0 : -1;
    }
    return (result_up != EVENT_RESULT_UNHANDLED ||
            result_click != EVENT_RESULT_UNHANDLED) ? 0 : -1;
  }

  LRESULT OnRButtonDown(WPARAM flag, LPARAM position) {
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_RIGHT;
    Event::Type type = Event::EVENT_MOUSE_DOWN;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    result = view_->OnMouseEvent(event);
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnRButtonDoubleClick(WPARAM flag, LPARAM position) {
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = ConvertWinButtonFlagToGGadgetButtonFlag(flag);
    Event::Type type = Event::EVENT_MOUSE_RDBLCLICK;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    EventResult result = view_->OnMouseEvent(event);
    return result != EVENT_RESULT_UNHANDLED;
  }

  LRESULT OnMButtonUp(WPARAM flag, LPARAM position) {
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_MIDDLE;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    Event::Type type = Event::EVENT_MOUSE_UP;
    ASSERT(zoom_ != 0);
    MouseEvent event_up(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    EventResult result_up = view_->OnMouseEvent(event_up);
    return result_up != EVENT_RESULT_UNHANDLED ? 0 : -1;
  }

  LRESULT OnMButtonDown(WPARAM flag, LPARAM position) {
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_MIDDLE;
    Event::Type type = Event::EVENT_MOUSE_DOWN;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    result = view_->OnMouseEvent(event);
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnMButtonDoubleClick(WPARAM flag, LPARAM position) {
    if (IsWindow())
      ::SetFocus(native_window_);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int button = MouseEvent::BUTTON_MIDDLE;
    Event::Type type = Event::EVENT_MOUSE_RDBLCLICK;
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, button, modifier);
    EventResult result = view_->OnMouseEvent(event);
    return result != EVENT_RESULT_UNHANDLED;
  }

  bool InWindow(const POINT& point) {
    // We don't call canvas_->GetPointValue() because it's sometimes it may
    // cause access violation when calling GdipBitmapGetPixel. So we read color
    // value directly from the buffer bitmap.
    int w = window_rect_.right - window_rect_.left;
    int h = window_rect_.bottom - window_rect_.top;
    if (point.x < 0 || point.x >= w ||
        point.y < 0 || point.y >= h)
      return false;
    int index = point.y * w + point.x;
    int32_t color = reinterpret_cast<int32_t*>(buffer_bits_)[index];
    return GetAlpha(color);
  }

  LRESULT OnMouseMove(WPARAM flag, LPARAM position) {
    if (tool_tip_.IsWindow()) {
      MSG message = {0};
      message.hwnd = native_window_;
      message.message = WM_MOUSEMOVE;
      message.wParam = flag;
      message.lParam = position;
      SendMessage(tool_tip_.m_hWnd,
                  TTM_RELAYEVENT,
                  0,
                  reinterpret_cast<LPARAM>(&message));
    }

    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int buttons = ConvertWinButtonFlagToGGadgetButtonFlag(flag);
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    Event::Type type = Event::EVENT_MOUSE_MOVE;

    // Send EVENT_MOUSE_OVER first if nessesary.
    EventResult result_enter = EVENT_RESULT_HANDLED;
    if (!is_cursor_in_window_) {
      POINT point = {0};
      ::GetCursorPos(&point);
      ::ScreenToClient(native_window_, &point);
      ggadget::Color color;
      double opacity = 0;
      if (InWindow(point)) {
        // Passes mouse over event to the view.
        is_cursor_in_window_ = true;
        type = Event::EVENT_MOUSE_OVER;
        MouseEvent e(type, x / zoom_, y / zoom_, 0, 0, buttons, modifier);
        result_enter = view_->OnMouseEvent(e);
        if (!::IsWindowEnabled(native_window_))
          ::SetTimer(native_window_, Impl::DETECT_MOUSE_LEAVE,
                     kMouseLeaveCheckingTimerInteval, NULL);
      }
    }

    ASSERT(zoom_ != 0);
    MouseEvent event(type, x / zoom_, y / zoom_, 0, 0, buttons, modifier);
    EventResult result = view_->OnMouseEvent(event);
    if (result == ggadget::EVENT_RESULT_UNHANDLED &&
        (buttons & MouseEvent::BUTTON_LEFT) != 0 &&
        is_left_button_press_on_nothing_) {
      int delta_x = x - cursor_pos_.x;
      int delta_y = y - cursor_pos_.y;
      // Send fake mouse up event to the view so that we can start to drag
      // the window. Note: no mouse click event is sent in this case, to prevent
      // unwanted action after window move.
      if (!is_mouse_dragging_ &&
          (delta_x > kDragThreshold || delta_y > kDragThreshold ||
           delta_x < -kDragThreshold || delta_y < -kDragThreshold)) {
        is_mouse_dragging_ = true;
        type = Event::EVENT_MOUSE_UP;
        MouseEvent e(type, x / zoom_, y / zoom_, 0, 0, buttons, modifier);
        // Ignore the result of this fake event.
        view_->OnMouseEvent(e);
        SetResizeDirection();
        if (is_resizing_)
          host_->BeginResizeDrag(buttons, mouse_down_hittest_);
        else
          host_->BeginMoveDrag(buttons);
      }

      if (is_resizing_) {  // Resizing window.
        RECT rect = window_rect_;
        rect.top = rect.top + resize_direction_.top * delta_y;
        rect.bottom = rect.bottom + resize_direction_.bottom * delta_y;
        rect.left = rect.left + resize_direction_.left * delta_x;
        rect.right = rect.right + resize_direction_.right * delta_x;
        double w = rect.right - rect.left;
        double h = rect.bottom - rect.top;
        if ((w != view_->GetWidth() || h != view_->GetHeight()) &&
            view_->OnSizing(&w, &h))
          view_->SetSize(w, h);
        window_rect_ = rect;
        host_->QueueDraw();
      } else {  // Moving window.
        window_rect_.left += delta_x;
        window_rect_.right += delta_x;
        window_rect_.top += delta_y;
        window_rect_.bottom += delta_y;
        gadget_window_->SetWindowPosition(window_rect_.left, window_rect_.top);
      }
    }
    // Calls TrackMouseEvent to track the WM_MOUSE_LEAVE event if window is
    // enabled. Disable window will always receive WM_MOUSE_LEAVE immediately
    // after calling TrackMouseEvent, so we use a timer to track the mouse leave
    // event.
    if (::IsWindowEnabled(native_window_)) {
      TRACKMOUSEEVENT trmouse = { 0 };
      trmouse.cbSize = sizeof(TRACKMOUSEEVENT);
      trmouse.dwFlags = TME_LEAVE;
      trmouse.dwHoverTime = HOVER_DEFAULT;
      trmouse.hwndTrack = native_window_;
      ::TrackMouseEvent(&trmouse);
    }
    if (result != EVENT_RESULT_UNHANDLED ||
        result_enter != EVENT_RESULT_UNHANDLED)
      return -1;
    else
      return 0;
  }

  LRESULT OnMouseLeave(WPARAM flag, LPARAM position) {
    GGL_UNUSED(position);
    GGL_UNUSED(flag);
    is_cursor_in_window_ = false;
    int modifier = ggadget::win32::GetCurrentKeyModifier();
    int buttons = MouseEvent::BUTTON_NONE;
    if (::GetKeyState(VK_LBUTTON) & 0xFE) buttons |= MouseEvent::BUTTON_LEFT;
    if (::GetKeyState(VK_RBUTTON) & 0xFE) buttons |= MouseEvent::BUTTON_RIGHT;
    if (::GetKeyState(VK_MBUTTON) & 0xFE) buttons |= MouseEvent::BUTTON_MIDDLE;
    MouseEvent event(Event::EVENT_MOUSE_OUT, 0, 0, 0, 0, buttons, modifier);
    EventResult result = view_->OnMouseEvent(event);
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnMouseWheel(WPARAM flag, LPARAM position) {
    int delta_x = HIWORD(flag);
    int modifier = ConvertWinKeyModiferToGGadgetKeyModifer(flag);
    int buttons = ConvertWinButtonFlagToGGadgetButtonFlag(flag);
    int x = GET_X_LPARAM(position);
    int y = GET_Y_LPARAM(position);
    ASSERT(zoom_ != 0);
    MouseEvent event(Event::EVENT_MOUSE_WHEEL, x / zoom_, y / zoom_,
                     delta_x, 0, buttons, modifier);
    EventResult result = view_->OnMouseEvent(event);
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnKeyDown(WPARAM virtual_key, LPARAM flag) {
    GGL_UNUSED(flag);
    int modifier = GetCurrentKeyModifier();
    unsigned int key_code = ConvertVirtualKeyCodeToKeyCode(virtual_key);
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (key_code) {
      KeyboardEvent event(Event::EVENT_KEY_DOWN, key_code, modifier, NULL);
      result = view_->OnKeyEvent(event);
    } else {
      LOG("Unknown key: 0x%x", virtual_key);
    }
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnKeyUp(WPARAM virtual_key, LPARAM flag) {
    GGL_UNUSED(flag);
    int modifier = GetCurrentKeyModifier();
    unsigned int key_code = ConvertVirtualKeyCodeToKeyCode(virtual_key);
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (key_code) {
      KeyboardEvent event(Event::EVENT_KEY_UP, key_code, modifier, NULL);
      result = view_->OnKeyEvent(event);
    } else {
      LOG("Unknown key: 0x%x", virtual_key);
    }
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnChar(WPARAM key_char, LPARAM flag) {
    GGL_UNUSED(flag);
    int modifier = GetCurrentKeyModifier();
    KeyboardEvent event(Event::EVENT_KEY_PRESS, key_char, modifier, NULL);
    EventResult result = view_->OnKeyEvent(event);
    return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
  }

  LRESULT OnKillFocus(WPARAM focus_window, LPARAM lparam) {
    GGL_UNUSED(focus_window);
    GGL_UNUSED(lparam);
    if (is_focused_) {
      is_focused_ = false;
      if (is_mouse_dragging_) {
        is_mouse_dragging_ = false;
        if (is_resizing_) {
          is_resizing_ = false;
          ::ReleaseCapture();
          gadget_window_->QueueResize();
        } else {
          ::ReleaseCapture();
        }
      }
      SimpleEvent event(Event::EVENT_FOCUS_IN);
      EventResult result = view_->OnOtherEvent(event);
      return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
    }
    return -1;
  }

  LRESULT OnSetFocus(WPARAM old_window, LPARAM lparam) {
    GGL_UNUSED(lparam);
    GGL_UNUSED(old_window);
    if (!is_focused_) {
      is_focused_ = true;
      SimpleEvent event(Event::EVENT_FOCUS_OUT);
      EventResult result = view_->OnOtherEvent(event);
      return result == EVENT_RESULT_UNHANDLED ? -1 : 0;
    }
    return -1;
  }

  LRESULT OnTimer(WPARAM timer_id, LPARAM call_back) {
    GGL_UNUSED(call_back);
    switch (timer_id) {
      case Impl::QUEUE_DRAW:
        if (queue_draw_)
          DrawView();
        break;
      case Impl::TOOL_TIP_HIDE:
        HideTooltip();
        break;
      case Impl::DETECT_MOUSE_LEAVE:
        if (is_cursor_in_window_) {
          // Do not send mouse leave if mouse button is down.
          if ((::GetKeyState(VK_LBUTTON) & 0xFE) ||
              (::GetKeyState(VK_MBUTTON) & 0xFE) ||
              (::GetKeyState(VK_RBUTTON) & 0xFE))
            return 0;
          POINT point = {0};
          ::GetCursorPos(&point);
          ::ScreenToClient(native_window_, &point);
          ggadget::Color color;
          double opacity = 0;
          if (!InWindow(point)) {
            OnMouseLeave(0, 0);
            ::KillTimer(native_window_, Impl::DETECT_MOUSE_LEAVE);
          }
        }
        break;
      case Impl::SHOW_WINDOW:
        ::ShowWindow(native_window_, show_window_command_);
        ::KillTimer(native_window_, SHOW_WINDOW);
        show_window_command_ = 0;
        break;
    }
    return 0;
  }

  LRESULT OnDisplayChange(WPARAM image_depth, LPARAM resolution) {
    if (static_cast<int>(image_depth) != image_depth_) {
      ::DeleteDC(buffer_dc_);
      HDC window_dc = ::GetDC(native_window_);
      buffer_dc_ = ::CreateCompatibleDC(window_dc);
      ::SelectObject(buffer_dc_, buffer_bitmap_);
      ::ReleaseDC(native_window_, window_dc);
      image_depth_ = image_depth;
      UpdateLayered();
    }
    int new_width = LOWORD(resolution);
    int new_height = HIWORD(resolution);
    if (new_width != screen_size_.cx || new_height != screen_size_.cy) {
      screen_size_.cx = new_width;
      screen_size_.cy = new_height;
      MakeSureInScreen();
    }
    return 0;
  }

  LRESULT OnPaint(WPARAM wparam, LPARAM lparam) {
    GGL_UNUSED(wparam);
    GGL_UNUSED(lparam);
    PAINTSTRUCT paint_struct = {0};
    HDC window_dc = ::BeginPaint(native_window_, &paint_struct);
    if (!is_layered_window_) {
      int x = paint_struct.rcPaint.left;
      int y = paint_struct.rcPaint.top;
      int w = paint_struct.rcPaint.right - paint_struct.rcPaint.left;
      int h = paint_struct.rcPaint.bottom - paint_struct.rcPaint.top;
      ::BitBlt(window_dc, x, y, w, h, buffer_dc_, x, y, SRCCOPY);
    }
    ::EndPaint(native_window_, &paint_struct);
    return 0;
  }

  // Sets resize direction according to the mouse_down_hittest returned by the
  // view. If none of the border's is hit, then set resize_drag to false.
  void SetResizeDirection() {
    if ((resizable_ & ViewInterface::RESIZABLE_TRUE) != 0 ||
        (resizable_ & ViewInterface::RESIZABLE_KEEP_RATIO) != 0) {
      is_resizing_ = true;
      resize_direction_.left = 0;
      resize_direction_.top = 0;
      resize_direction_.right = 0;
      resize_direction_.bottom = 0;
      switch (mouse_down_hittest_) {
        case ViewInterface::HT_LEFT:
          resize_direction_.left = 1;
          break;
        case ViewInterface::HT_RIGHT:
          resize_direction_.right = 1;
          break;
        case ViewInterface::HT_TOP:
          resize_direction_.top = 1;
          break;
        case ViewInterface::HT_BOTTOM:
          resize_direction_.bottom = 1;
          break;
        case ViewInterface::HT_TOPLEFT:
          resize_direction_.top = 1;
          resize_direction_.left = 1;
          break;
        case ViewInterface::HT_TOPRIGHT:
          resize_direction_.top = 1;
          resize_direction_.right = 1;
          break;
        case ViewInterface::HT_BOTTOMLEFT:
          resize_direction_.bottom = 1;
          resize_direction_.left = 1;
          break;
        case ViewInterface::HT_BOTTOMRIGHT:
          resize_direction_.bottom = 1;
          resize_direction_.right = 1;
          break;
        default:
          is_resizing_ = false;
      }
    }
  }

  // Tries to set window as layered window, if it fails use normal window.
  // Layered window is supported since windows 2000, but it is not supported
  // by some early versions of remote desktop clients with low color depth.
  void UpdateLayered() {
    if (enable_input_mask_ && !is_layered_window_) {
      LONG window_long = ::GetWindowLong(native_window_, GWL_EXSTYLE);
      window_long |= WS_EX_LAYERED;
      ::SetWindowLong(native_window_, GWL_EXSTYLE, window_long);
      is_layered_window_ = true;
      // Set window region to NULL to make layered window take change of it's
      // own shape.
      if (window_region_)
        ::SetWindowRgn(native_window_, NULL, FALSE);
    }
    if (!UpdateLayeredWindowContent()) {
      is_layered_window_ = false;
      LONG window_long = ::GetWindowLong(native_window_, GWL_EXSTYLE);
      window_long &= ~WS_EX_LAYERED;
      if (invalidate_region_)
        ::DeleteObject(invalidate_region_);
      invalidate_region_ = CreateInvalidateHRGN();
      if (enable_input_mask_)
        UpdateWindowRegion();
      ::SetWindowLong(native_window_, GWL_EXSTYLE, window_long);
      ::InvalidateRect(native_window_, &window_rect_, true);
    }
  }

  // Updates the layered window using buffer_bitmap_ and window_rect_.
  bool UpdateLayeredWindowContent() {
    POINT window_origin = {window_rect_.left, window_rect_.top};
    SIZE window_size = {
        window_rect_.right - window_rect_.left,
        window_rect_.bottom - window_rect_.top
    };
    POINT source_point = {0, 0};
    return ::UpdateLayeredWindow(native_window_,
                                 NULL,
                                 &window_origin, &window_size,
                                 buffer_dc_, &source_point,
                                 static_cast<COLORREF>(0),
                                 &blend_, ULW_ALPHA) != 0;
  }

  // Draws the view to the buffer bitmap, calculate the invalidate region and
  // the window shape.
  void UpdateViewAppearance() {
    if (view_->GetWidth() == 0 ||
        view_->GetHeight() == 0 ||
        canvas_.get() == NULL) {
      return;
    }
    canvas_->PushState();
    const ClipRegion* clip_region = view_->GetClipRegion();
    canvas_->IntersectGeneralClipRegion(*clip_region);
    canvas_->ClearRect(0, 0, canvas_->GetWidth(), canvas_->GetHeight());
    view_->Draw(canvas_.get());
    FlushCanvasToBitmap();
    canvas_->PopState();
  }

  // Draws the buffer bitmap to the window.
  void Draw() {
    queue_draw_ = false;
    UpdateLayered();
    if (!is_layered_window_) {
      if (enable_input_mask_)
        ::SetWindowRgn(native_window_, window_region_, true);
      ::InvalidateRgn(native_window_, invalidate_region_, true);
      if (is_window_rect_changed_) {
        ::MoveWindow(native_window_, window_rect_.left, window_rect_.top,
                     window_rect_.right - window_rect_.left,
                     window_rect_.bottom - window_rect_.top, FALSE);
        is_window_rect_changed_ = false;
      }
    }
  }

  // Creates the buffer_bitmap_ with size w*h, returns true if buffer_bitmap_ is
  // created successfully.
  bool CreateBufferBitmap(int w, int h) {
    if (buffer_bitmap_ != NULL)
      ::DeleteObject(buffer_bitmap_);
    int stride = w * kBytesPerPixel;
    int size = h * stride;
    BITMAPINFO info = {0};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = w;
    // Negative height means the bitmap's origin is the upper-left corner.
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = kBitsPerPixel;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = size;
    HDC hdc = ::CreateCompatibleDC(NULL);
    buffer_bitmap_ = ::CreateDIBSection(hdc, &info, DIB_RGB_COLORS,
                                        &buffer_bits_, NULL, 0);
    ::DeleteDC(hdc);
    return buffer_bitmap_ != NULL;
  }

  // Flushs the canvas content to buffer_bitmap_.
  void FlushCanvasToBitmap() {
    if (buffer_dc_ == NULL) {
      HDC window_dc = ::GetDC(native_window_);
      buffer_dc_ = ::CreateCompatibleDC(window_dc);
      ::ReleaseDC(native_window_, window_dc);
      ::SelectObject(buffer_dc_, buffer_bitmap_);
    }
    Gdiplus::Graphics graphics(buffer_dc_);
    graphics.SetClip(canvas_->GetGdiplusGraphics());
    graphics.Clear(Gdiplus::Color::Transparent);
    graphics.DrawImage(canvas_->GetImage(), 0, 0);
  }

  void AdjustToViewSize() {
    ASSERT(view_);
    view_->Layout();
    int w = static_cast<int>(round(view_->GetWidth() * zoom_));
    int h = static_cast<int>(round(view_->GetHeight() * zoom_));
    int dw = w - window_rect_.right + window_rect_.left;
    int dh = h - window_rect_.bottom + window_rect_.top;
    if (dw == 0 && dh == 0)
      return;
    // If the view size changed, re-layout view to make sure view elements are
    // layouted correctly.
    view_->Layout();
    // Check if the width and height is changed.
    ASSERT(w == round(view_->GetWidth() * zoom_));
    ASSERT(h == round(view_->GetHeight() * zoom_));
    // Get the width and height again in case that view size changes.
    w = static_cast<int>(round(view_->GetWidth() * zoom_));
    h = static_cast<int>(round(view_->GetHeight() * zoom_));
    dw = w - window_rect_.right + window_rect_.left;
    dh = h - window_rect_.bottom + window_rect_.top;
    is_window_rect_changed_ = true;
    window_rect_.bottom += dh;
    window_rect_.right += dw;
    CreateBufferBitmap(w, h);
    if (buffer_dc_)
      ::SelectObject(buffer_dc_, buffer_bitmap_);
    canvas_.reset(down_cast<GdiplusCanvas*>(
        view_->GetGraphics()->NewCanvas(view_->GetWidth(),
                                        view_->GetHeight())));
    canvas_->ClearCanvas();
    view_->AddRectangleToClipRegion(
        Rectangle(0, 0, view_->GetWidth(), view_->GetHeight()));
    MakeSureInScreen();
  }

  void MakeSureInScreen() {
    if (::GetWindowLong(native_window_, GWL_STYLE) & WS_CHILD)
      return;
    POINT reference_point = {0};
    reference_point.x = (window_rect_.left + window_rect_.right) / 2;
    reference_point.y = (window_rect_.top + window_rect_.bottom) / 2;
    HMONITOR monitor = ::MonitorFromPoint(
        reference_point, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    ::GetMonitorInfo(monitor, &monitor_info);
    int offset_x = 0;
    int offset_y = 0;
    if (window_rect_.left < monitor_info.rcWork.left)
      offset_x = monitor_info.rcWork.left - window_rect_.left;
    else if (window_rect_.right > monitor_info.rcWork.right)
      offset_x = monitor_info.rcWork.right - window_rect_.right;
    if (window_rect_.top < monitor_info.rcWork.top)
      offset_y = monitor_info.rcWork.top - window_rect_.top;
    else if (window_rect_.bottom > monitor_info.rcWork.bottom)
      offset_y = monitor_info.rcWork.bottom - window_rect_.bottom;
    OffsetRect(&window_rect_, offset_x, offset_y);
  }

  // Creates a HRGN object from the view's clip region. The HRGN will be used
  // by the caller to invalidate the window and it should be deleted by the
  // caller.
  HRGN CreateInvalidateHRGN() {
    const ClipRegion* view_region = view_->GetClipRegion();
    size_t count = view_region->GetRectangleCount();
    if (count > 0) {
      Rectangle rect = view_region->GetRectangle(0);
      HRGN region = ::CreateRectRgn(
          static_cast<int>(round(rect.x)), static_cast<int>(round(rect.y)),
          static_cast<int>(round(rect.x + rect.h)),
          static_cast<int>(round(rect.y + rect.h)));
      for (int i = 1; i < static_cast<int>(count); ++i) {
        rect = view_region->GetRectangle(0);
        HRGN new_region = ::CreateRectRgn(
            static_cast<int>(round(rect.x)), static_cast<int>(round(rect.y)),
            static_cast<int>(round(rect.x + rect.h)),
            static_cast<int>(round(rect.y + rect.h)));
        ::CombineRgn(region, region, new_region, RGN_OR);
        ::DeleteObject(new_region);
      }
      return region;
    }
    return NULL;
  }

  const Gdiplus::Bitmap* GetViewContent() {
    if (!gadget_window_->IsWindowVisible()) {
      AdjustToViewSize();
      UpdateViewAppearance();
    }
    return canvas_->GetImage();
  }

  HRESULT OnMenuCommand(WPARAM wparam) {
    ASSERT(menu_);
    if (menu_->OnCommand(LOWORD(wparam))) {
      menu_ = NULL;
      return 0;
    }
    return -1;
  }

  // Layouts the view, resize the window, and draw.
  void DrawView() {
    AdjustToViewSize();
    UpdateViewAppearance();
    Draw();
  }

 private:
  BLENDFUNCTION blend_;
  bool enable_input_mask_;
  bool is_cursor_in_window_;
  bool is_focused_;
  bool is_layered_window_;
  // True if the previous left button down event is not processed and left
  // button up event is not received.
  bool is_left_button_press_on_nothing_;
  bool is_mouse_dragging_;
  bool is_resizing_;
  bool is_window_rect_changed_;
  bool queue_draw_;
  bool queue_resize_;
  CWindow tool_tip_;
  double zoom_;
  GadgetWindow* gadget_window_;
  HBITMAP buffer_bitmap_;
  HCURSOR cursor_;
  HDC buffer_dc_;
  HRGN invalidate_region_;
  HRGN window_region_;
  HWND native_window_;
  int image_depth_;
  int show_window_command_;
  int window_class_style_;
  int window_exstyle_;
  int window_style_;
  MenuBuilder* menu_;
  POINT cursor_pos_;
  RECT window_rect_;
  ResizeDirection resize_direction_;
  scoped_ptr<GdiplusCanvas> canvas_;
  Signal2<void, int, int> on_end_move_drag_signal_;
  SIZE screen_size_;
  TOOLINFO tool_info_;
  UTF16String tool_tip_text_;
  ViewHostInterface* host_;
  ViewInterface* view_;
  ViewInterface::HitTest mouse_down_hittest_;
  ViewInterface::ResizableMode resizable_;
  void* buffer_bits_;
  friend class GadgetWindow;
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

GadgetWindow::GadgetWindow(ViewHostInterface* host,
                           ViewInterface* view,
                           double zoom,
                           int window_class_style,
                           int window_style,
                           int window_exstyle) {
  impl_.reset(new Impl(this, host, view, zoom, window_class_style, window_style,
                       window_exstyle));
}

GadgetWindow::~GadgetWindow() {
}

void GadgetWindow::QueueDraw() {
  impl_->queue_draw_ = true;
}

void GadgetWindow::QueueResize() {
  // Treat QueueResize as QueueDraw because draw will also change the window
  // size.
  QueueDraw();
}

void GadgetWindow::ShowTooltip(const std::string &tooltip) {
  POINT cursor_position;
  ::GetCursorPos(&cursor_position);
  impl_->ShowTooltip(tooltip, cursor_position.x, cursor_position.y);
}

LRESULT GadgetWindow::OnTimer(UINT message,
                              WPARAM timer_id,
                              LPARAM call_back,
                              BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnTimer(timer_id, call_back);
}

LRESULT GadgetWindow::OnLButtonDoubleClick(UINT message,
                                           WPARAM flag,
                                           LPARAM position,
                                           BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnLButtonDoubleClick(flag, position);
}

LRESULT GadgetWindow::OnLButtonDown(UINT message,
                                    WPARAM flag,
                                    LPARAM position,
                                    BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  // Capture the mouse to receive button up messages even if the cursor leave
  // the window.
  ::SetCapture(impl_->native_window_);
  return impl_->OnLButtonDown(flag, position);
}

LRESULT GadgetWindow::OnLButtonUp(UINT message,
                                  WPARAM flag,
                                  LPARAM position,
                                  BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  ::ReleaseCapture();
  return impl_->OnLButtonUp(flag, position);
}

LRESULT GadgetWindow::OnMButtonDoubleClick(UINT message,
                                           WPARAM flag,
                                           LPARAM position,
                                           BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnMButtonDoubleClick(flag, position);
}

LRESULT GadgetWindow::OnMButtonDown(UINT message,
                                    WPARAM flag,
                                    LPARAM position,
                                    BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  // Capture the mouse to receive button up messages even if the cursor leave
  // the window.
  ::SetCapture(impl_->native_window_);
  return impl_->OnMButtonDown(flag, position);
}

LRESULT GadgetWindow::OnMButtonUp(UINT message,
                                  WPARAM flag,
                                  LPARAM position,
                                  BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  ::ReleaseCapture();
  return impl_->OnMButtonUp(flag, position);
}

LRESULT GadgetWindow::OnRButtonUp(UINT message,
                                  WPARAM flag,
                                  LPARAM position,
                                  BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  ::ReleaseCapture();
  return impl_->OnRButtonUp(flag, position);
}

LRESULT GadgetWindow::OnRButtonDown(UINT message,
                                    WPARAM flag,
                                    LPARAM position,
                                    BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  // Capture the mouse to receive button up messages even if the cursor leave
  // the window.
  ::SetCapture(impl_->native_window_);
  return impl_->OnRButtonDown(flag, position);
}

LRESULT GadgetWindow::OnRButtonDoubleClick(UINT message,
                                           WPARAM flag,
                                           LPARAM position,
                                           BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnRButtonDoubleClick(flag, position);
}

LRESULT GadgetWindow::OnMouseMove(UINT message,
                                  WPARAM flag,
                                  LPARAM position,
                                  BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnMouseMove(flag, position);
}

LRESULT GadgetWindow::OnMouseLeave(UINT message,
                                   WPARAM flag,
                                   LPARAM position,
                                   BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnMouseLeave(flag, position);
}

LRESULT GadgetWindow::OnKeyDown(UINT message,
                                WPARAM key_code,
                                LPARAM flag,
                                BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnKeyDown(key_code, flag);
}

LRESULT GadgetWindow::OnKeyUp(UINT message,
                              WPARAM key_code,
                              LPARAM flag,
                              BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnKeyUp(key_code, flag);
}

LRESULT GadgetWindow::OnChar(UINT message,
                             WPARAM key_code,
                             LPARAM flag,
                             BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnChar(key_code, flag);
}

LRESULT GadgetWindow::OnKillFocus(UINT message,
                                  WPARAM focus_window,
                                  LPARAM lparam,
                                  BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnKillFocus(focus_window, lparam);
}

LRESULT GadgetWindow::OnSetFocus(UINT message,
                                 WPARAM old_window,
                                 LPARAM lparam,
                                 BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnSetFocus(old_window, lparam);
}

LRESULT GadgetWindow::OnMouseWheel(UINT message,
                                   WPARAM flag,
                                   LPARAM position,
                                   BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnMouseWheel(flag, position);
}

LRESULT GadgetWindow::OnDisplayChange(UINT message,
                                      WPARAM image_depth,
                                      LPARAM resolution,
                                      BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnDisplayChange(image_depth, resolution);
}

LRESULT GadgetWindow::OnPaint(UINT message,
                              WPARAM wparam,
                              LPARAM lparam,
                              BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  return impl_->OnPaint(wparam, lparam);
}

LRESULT GadgetWindow::OnCommand(UINT message,
                                WPARAM wparam,
                                LPARAM lparam,
                                BOOL& handled) {
  GGL_UNUSED(message);
  GGL_UNUSED(lparam);
  if (HIWORD(wparam) == 0) {
    return impl_->OnMenuCommand(wparam);
  }
  handled = false;
  return -1;
}

LRESULT GadgetWindow::OnClose(UINT message,
                              WPARAM wparam,
                              LPARAM lparam,
                              BOOL& handled) {
  GGL_UNUSED(handled);
  GGL_UNUSED(message);
  GGL_UNUSED(lparam);
  GGL_UNUSED(wparam);
  SimpleEvent event(Event::EVENT_CLOSE);
  impl_->view_->OnOtherEvent(event);
  impl_->host_->CloseView();
  return 0;
}

LRESULT GadgetWindow::OnSetCursor(UINT message,
                                  WPARAM wparam,
                                  LPARAM lparam,
                                  BOOL& handled) {
  GGL_UNUSED(message);
  GGL_UNUSED(wparam);
  GGL_UNUSED(lparam);
  GGL_UNUSED(handled);
  if (!impl_->cursor_)
    impl_->cursor_ = ::LoadCursor(NULL, IDC_ARROW);
  ::SetCursor(impl_->cursor_);
  // Disabled windows cannot receive mouse messages but them will receive
  // WM_SETCURSOR with the mouse message in the high order word in lparam.
  if (!::IsWindowEnabled(impl_->native_window_)) {
    POINT cursor_position = {0};
    ::GetCursorPos(&cursor_position);
    ::ScreenToClient(impl_->native_window_, &cursor_position);
    int flag = ((::GetKeyState(VK_CONTROL) & 0xFE) ? MK_CONTROL : 0) |
               ((::GetKeyState(VK_SHIFT) & 0xFE) ? MK_SHIFT : 0) |
               ((::GetKeyState(VK_LBUTTON) & 0xFE) ? MK_LBUTTON : 0) |
               ((::GetKeyState(VK_RBUTTON) & 0xFE) ? MK_RBUTTON : 0) |
               ((::GetKeyState(VK_MBUTTON) & 0xFE) ? MK_MBUTTON : 0);
    switch (HIWORD(lparam)) {
      case WM_MOUSEMOVE:
        OnMouseMove(WM_MOUSEMOVE, flag,
                    MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_LBUTTONUP:
        OnLButtonUp(WM_LBUTTONUP, flag,
                    MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_LBUTTONDOWN:
        OnLButtonDown(WM_LBUTTONDOWN, flag,
                      MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_LBUTTONDBLCLK:
        OnLButtonDoubleClick(
            WM_LBUTTONDBLCLK, flag,
            MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_RBUTTONUP:
        OnRButtonUp(WM_RBUTTONUP, flag,
                    MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_RBUTTONDOWN:
        OnRButtonDown(WM_RBUTTONDOWN, 0,
                      MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_RBUTTONDBLCLK:
        OnRButtonDoubleClick(
            WM_RBUTTONDBLCLK, flag,
            MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_MBUTTONUP:
        OnMButtonUp(WM_MBUTTONUP, flag,
                    MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_MBUTTONDOWN:
        OnMButtonDown(WM_MBUTTONDOWN, flag,
                      MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      case WM_MBUTTONDBLCLK:
        OnMButtonDoubleClick(
            WM_MBUTTONDBLCLK, flag,
            MAKELONG(cursor_position.x, cursor_position.y), handled);
        break;
      default:
        break;
    }
  }
  return 0;
}

void GadgetWindow::SetCaption(const std::string &caption) {
  if (::GetWindowLong(impl_->native_window_, GWL_STYLE) & WS_CAPTION) {
    UTF16String caption_utf16;
    ConvertStringUTF8ToUTF16(caption, &caption_utf16);
    ::SetWindowText(impl_->native_window_, caption_utf16.c_str());
  }
}

void GadgetWindow::SetEnableInputMask(bool enable) {
  if (impl_->enable_input_mask_ != enable) {
    impl_->enable_input_mask_ = enable;
    if (IsWindow() && IsWindowVisible())
      QueueDraw();
  }
}

bool GadgetWindow::Init() {
  return impl_->Init();
}

bool GadgetWindow::ShowViewWindow() {
  return impl_->ShowViewWindow();
}

void GadgetWindow::ShowTooltipAtPosition(const std::string &tooltip, double x,
                                         double y) {
  impl_->ShowTooltip(tooltip, static_cast<int>(round(x)),
                     static_cast<int>(round(y)));
}

void GadgetWindow::SetWindowPosition(int x, int y) {
  // Limits the range of x and y within INT16_MAX to prevent the right and
  // bottom of the window_rect_ from overflowing.
  if (x > INT16_MAX) x = INT16_MAX;
  if (x < INT16_MIN) x = INT16_MIN;
  if (y > INT16_MAX) y = INT16_MAX;
  if (y < INT16_MIN) y = INT16_MIN;
  int offset_x = x - impl_->window_rect_.left;
  int offset_y = y - impl_->window_rect_.top;
  OffsetRect(&impl_->window_rect_, offset_x, offset_y);
  impl_->MakeSureInScreen();
  ::SetWindowPos(impl_->native_window_, NULL,
                 impl_->window_rect_.left, impl_->window_rect_.top,
                 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void GadgetWindow::GetWindowPosition(int* x, int* y) {
  if (x)
    *x = impl_->window_rect_.left;
  if (y)
    *y = impl_->window_rect_.top;
}

void GadgetWindow::GetWindowSize(int* width, int* height) {
  // If the window is not shown, then AdjustToViewSize may not have been called,
  // so we need to call AdjustToViewSize to determine the size.
  if (!IsWindowVisible())
    impl_->AdjustToViewSize();
  if (impl_->queue_draw_)
    impl_->DrawView();

  int w = impl_->window_rect_.right - impl_->window_rect_.left;
  int h = impl_->window_rect_.bottom - impl_->window_rect_.top;
  if (width)
    *width = w;
  if (height)
    *height = h;
}

const Gdiplus::Bitmap* GadgetWindow::GetViewContent() {
  return impl_->GetViewContent();
}

void GadgetWindow::SetResizable(ViewInterface::ResizableMode mode) {
  impl_->resizable_ = mode;
}

void GadgetWindow::SetMenuBuilder(MenuBuilder* menu_builder) {
  impl_->menu_ = menu_builder;
}

void GadgetWindow::SetCursor(HCURSOR cursor) {
  impl_->cursor_ = cursor;
  ::SendMessage(impl_->native_window_, WM_SETCURSOR, 0, 0);
}

void GadgetWindow::Enable(bool enabled) {
  if (enabled) {
    ::SetWindowLongPtr(
        impl_->native_window_, GWL_STYLE,
        ::GetWindowLongPtr(impl_->native_window_, GWL_STYLE) & (~WS_DISABLED));
  } else {
    ::SetWindowLongPtr(
        impl_->native_window_, GWL_STYLE,
        ::GetWindowLongPtr(impl_->native_window_, GWL_STYLE) | WS_DISABLED);
  }
}

void GadgetWindow::SetOpacity(double opacity) {
  impl_->blend_.SourceConstantAlpha = static_cast<BYTE>(round(opacity * 255));
  if (IsWindow() && IsWindowVisible())
    impl_->UpdateLayeredWindowContent();
}

Connection* GadgetWindow::ConnectOnEndMoveDrag(Slot2<void, int, int>* handler) {
  return impl_->on_end_move_drag_signal_.Connect(handler);
}

void GadgetWindow::CloseWindow() {
  impl_->CloseWindow();
}

bool GadgetWindow::IsWindow() {
  return impl_->IsWindow();
}

bool GadgetWindow::IsWindowVisible() {
  return IsWindow() && ::IsWindowVisible(impl_->native_window_);
}

HWND GadgetWindow::GetHWND() {
  return impl_->native_window_;
}

void GadgetWindow::SetZoom(double zoom) {
  impl_->zoom_ = zoom;
  impl_->view_->GetGraphics()->SetZoom(zoom);
  impl_->AdjustToViewSize();
  impl_->DrawView();
}

double GadgetWindow::GetZoom() {
  return impl_->zoom_;
}

}  // namespace win32
}  // namespace ggadget
