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

#include "single_view_host.h"

#include <cmath>
#include <string>
#include <ggadget/event.h>
#include <ggadget/logger.h>
#include <ggadget/math_utils.h>
#include <ggadget/messages.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/slot.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/view.h>
#include "gadget_window.h"
#include "gdiplus_graphics.h"
#include "menu_builder.h"

namespace ggadget {
namespace win32 {

namespace {

static const int16_t kMenuStartCommandId = 0x1000;
static const char kMenuItemAlwaysOnTop[] = "MENU_ITEM_ALWAYS_ON_TOP";

}  // namespace

class SingleViewHost::Impl {
 public:
  Impl(SingleViewHost* owner,
       ViewHostInterface::Type type,
       double zoom,
       int debug_mode,
       const PrivateFontDatabase* private_font_database,
       int window_class_style,
       int window_style,
       int window_exstyle)
      : owner_(owner),
        view_(NULL),
        window_(NULL),
        type_(type),
        zoom_(zoom),
        debug_mode_(debug_mode),
        private_font_database_(private_font_database),
        context_menu_(NULL),
        enable_always_on_top_menu_(false),
        window_class_style_(window_class_style),
        window_style_(window_style),
        window_exstyle_(window_exstyle),
        font_scale_(1.0) {
    ASSERT(owner);
    ASSERT_M(type_ == ViewHostInterface::VIEW_HOST_MAIN,
             ("Only support VIEW_HOST_MAIN!\n"));
  }

  ~Impl() {
    Detach();
  }

  void Detach() {
    // To make sure that it won't be accessed anymore.
    view_ = NULL;
    if (context_menu_)
      ::DestroyMenu(context_menu_);
    context_menu_ = NULL;
    if (window_.get())
      window_.reset(NULL);
  }

  void SetView(ViewInterface* view) {
    if (view_ != view) {
      Detach();
      view_ = view;
      if (view == NULL)
        return;
      down_cast<View*>(view_)->EnableCanvasCache(false);
      window_.reset(new GadgetWindow(owner_, view_, zoom_,
                                     window_class_style_,
                                     window_style_ | WS_POPUP,
                                     window_exstyle_));
      if (!window_->Init()) {
        LOG("GadgetWindow initialization Failed");
        view_ = NULL;
      }
    }
  }

  void* GetNativeWidget() const {
    return static_cast<void*>(window_->GetHWND());
  }

  void ViewCoordToNativeWindowCoord(double x, double y,
                                    double* window_x, double *window_y) const {
    double zoom = view_->GetGraphics()->GetZoom();
    if (window_x) *window_x = x * zoom;
    if (window_y) *window_y = y * zoom;
  }

  void NativeWidgetCoordToViewCoord(double x, double y,
                                    double *view_x, double *view_y) const {
    double zoom = view_->GetGraphics()->GetZoom();
    if (zoom == 0) return;
    if (view_x) *view_x = x / zoom;
    if (view_y) *view_y = y / zoom;
  }

  void SetCursor(ViewInterface::CursorType type) {
    HCURSOR cursor = 0;
    switch (type) {
      case ViewInterface::CURSOR_DEFAULT:
        cursor = ::LoadCursor(NULL, IDC_ARROW);
        break;
      case ViewInterface::CURSOR_ARROW:
        cursor = ::LoadCursor(NULL, IDC_ARROW);
        break;
      case ViewInterface::CURSOR_IBEAM:
        cursor = ::LoadCursor(NULL, IDC_IBEAM);
        break;
      case ViewInterface::CURSOR_WAIT:
        cursor = ::LoadCursor(NULL, IDC_WAIT);
        break;
      case ViewInterface::CURSOR_CROSS:
        cursor = ::LoadCursor(NULL, IDC_CROSS);
        break;
      case ViewInterface::CURSOR_UPARROW:
        cursor = ::LoadCursor(NULL, IDC_UPARROW);
        break;
      case ViewInterface::CURSOR_SIZE:
        cursor = ::LoadCursor(NULL, IDC_SIZE);
        break;
      case ViewInterface::CURSOR_SIZENWSE:
        cursor = ::LoadCursor(NULL, IDC_SIZENWSE);
        break;
      case ViewInterface::CURSOR_SIZENESW:
        cursor = ::LoadCursor(NULL, IDC_SIZENESW);
        break;
      case ViewInterface::CURSOR_SIZEWE:
        cursor = ::LoadCursor(NULL, IDC_SIZEWE);
        break;
      case ViewInterface::CURSOR_SIZENS:
        cursor = ::LoadCursor(NULL, IDC_SIZENS);
        break;
      case ViewInterface::CURSOR_SIZEALL:
        cursor = ::LoadCursor(NULL, IDC_SIZEALL);
        break;
      case ViewInterface::CURSOR_NO:
        cursor = ::LoadCursor(NULL, IDC_NO);
        break;
      case ViewInterface::CURSOR_HAND:
        cursor = ::LoadCursor(NULL, IDC_HAND);
        break;
      case ViewInterface::CURSOR_BUSY:
        cursor = ::LoadCursor(NULL, IDC_APPSTARTING);
        break;
      case ViewInterface::CURSOR_HELP:
        cursor = ::LoadCursor(NULL, IDC_HELP);
        break;
      default:
        ASSERT_M(0, ("Unsupport cursor type"));
    }
    window_->SetCursor(cursor);
  }

  void SetResizable(ViewInterface::ResizableMode mode) {
    ASSERT(window_.get() && window_->IsWindow());
    window_->SetResizable(mode);
    // Reset the zoom factor to 1 if the child view is changed to resizable.
    if ((mode == ViewInterface::RESIZABLE_TRUE ||
         mode == ViewInterface::RESIZABLE_KEEP_RATIO) &&
        view_->GetGraphics()->GetZoom() != 1.0) {
      view_->GetGraphics()->SetZoom(1.0);
      view_->MarkRedraw();
    }
  }

  bool ShowView(bool modal, int flags, Slot1<bool, int> *feedback_handler) {
    GGL_UNUSED(feedback_handler);
    GGL_UNUSED(flags);
    GGL_UNUSED(modal);
    if (view_ == NULL)
      return false;
    bool result = window_->ShowViewWindow();
    return result;
  }

  void CloseView() {
    if (window_.get() && window_->IsWindow())
      window_->CloseWindow();
  }

  bool ShowContextMenu(int button) {
    if (window_.get() == NULL || !window_->IsWindow())
      return false;
    menu_.reset(new MenuBuilder);
    if (view_->OnAddContextMenuItems(menu_.get())) {
      if (type_ == ViewHostInterface::VIEW_HOST_MAIN &&
          enable_always_on_top_menu_) {
        menu_->AddItem(
            GM_(kMenuItemAlwaysOnTop),
            owner_->GetTopMost() ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
            NewSlot(this, &Impl::KeepAboveMenuCallback, !owner_->GetTopMost()),
            MenuInterface::MENU_ITEM_PRI_HOST);
      }
    }
    if (menu_->IsEmpty())
      return false;
    Rectangle position_hint = menu_->GetPositionHint();
    RECT rect = {position_hint.x, position_hint.y,
                 position_hint.x + position_hint.w,
                 position_hint.y + position_hint.h};
    POINT topleft = {position_hint.x, position_hint.y};
    if (position_hint.x == 0 && position_hint.y == 0 &&
        position_hint.w == 0 && position_hint.h == 0) {
      ::GetCursorPos(&topleft);
    } else {
      ::ClientToScreen(window_->GetHWND(), &topleft);
    }
    position_hint.x = topleft.x;
    position_hint.y = topleft.y;
    menu_->SetPositionHint(position_hint);
    if (!on_show_context_menu_(menu_.get()))
      return true;
    if (context_menu_ != NULL)
      ::DestroyMenu(context_menu_);
    context_menu_ = ::CreatePopupMenu();
    if (context_menu_ == NULL) return false;
    menu_->BuildMenu(kMenuStartCommandId, &context_menu_);
    int menu_flag = TPM_LEFTBUTTON | TPM_TOPALIGN;
    if (button & MouseEvent::BUTTON_RIGHT)
      menu_flag |= TPM_RIGHTBUTTON;
    POINT cursor_pos;
    ::GetCursorPos(&cursor_pos);
    window_->SetMenuBuilder(menu_.get());
    if (position_hint.w == 0 && position_hint.h == 0) {
      return ::TrackPopupMenu(context_menu_, menu_flag, cursor_pos.x,
                              cursor_pos.y, 0, window_->GetHWND(), NULL);
    } else {
      TPMPARAMS menu_params = {0};
      menu_params.cbSize = sizeof(TPMPARAMS);
      menu_params.rcExclude.left = position_hint.x;
      menu_params.rcExclude.top = position_hint.y;
      menu_params.rcExclude.right = position_hint.x + position_hint.w;
      menu_params.rcExclude.bottom = position_hint.y + position_hint.h;
      return ::TrackPopupMenuEx(context_menu_, menu_flag, topleft.x,
                                topleft.y + position_hint.h, window_->GetHWND(),
                                &menu_params);
    }
  }

  void KeepAboveMenuCallback(const char* text, bool keep_above) {
    GGL_UNUSED(text);
    owner_->SetTopMost(keep_above);
  }

  SingleViewHost *owner_;
  ViewInterface *view_;
  scoped_ptr<GadgetWindow> window_;
  ViewHostInterface::Type type_;
  int debug_mode_;
  double zoom_;
  ViewInterface::ResizableMode resizable_mode_;
  bool is_keep_above_;
  scoped_ptr<MenuBuilder> menu_;
  const PrivateFontDatabase* private_font_database_;
  HMENU context_menu_;
  bool enable_always_on_top_menu_;
  int window_class_style_;
  int window_style_;
  int window_exstyle_;
  double font_scale_;
  Signal1<bool, MenuInterface*> on_show_context_menu_;
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

SingleViewHost::SingleViewHost(
    ViewHostInterface::Type type,
    double zoom,
    int debug_mode,
    const PrivateFontDatabase* private_font_database,
    int window_class_style,
    int window_style,
    int window_exstyle)
    : impl_(new Impl(this, type, zoom, debug_mode, private_font_database,
                     window_class_style, window_style, window_exstyle)) {
}

ViewHostInterface::Type SingleViewHost::GetType() const {
  return impl_->type_;
}

void SingleViewHost::Destroy() {
  delete this;
}

void SingleViewHost::SetView(ViewInterface *view) {
  impl_->SetView(view);
}

ViewInterface* SingleViewHost::GetView() const {
  return impl_->view_;
}

void* SingleViewHost::GetNativeWidget() const {
  return impl_->GetNativeWidget();
}

GraphicsInterface* SingleViewHost::NewGraphics() const {
  return new GdiplusGraphics(impl_->zoom_,
                             impl_->private_font_database_);
}

void SingleViewHost::ViewCoordToNativeWidgetCoord(
    double x,
    double y,
    double* widget_x,
    double* widget_y) const {
  impl_->ViewCoordToNativeWindowCoord(x, y, widget_x, widget_y);
}

void SingleViewHost::NativeWidgetCoordToViewCoord(
    double x,
    double y,
    double* view_x,
    double* view_y) const {
  impl_->NativeWidgetCoordToViewCoord(x, y, view_x, view_y);
}

void SingleViewHost::QueueDraw() {
  impl_->window_->QueueDraw();
}

void SingleViewHost::QueueResize() {
  impl_->window_->QueueResize();
}

void SingleViewHost::EnableInputShapeMask(bool enable) {
  impl_->window_->SetEnableInputMask(enable);
}

void SingleViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  impl_->SetResizable(mode);
}

void SingleViewHost::SetCaption(const std::string& caption) {
  impl_->window_->SetCaption(caption);
}

void SingleViewHost::SetShowCaptionAlways(bool always) {
  GGL_UNUSED(always);
}

void SingleViewHost::SetCursor(ViewInterface::CursorType type) {
  impl_->SetCursor(type);
}

void SingleViewHost::ShowTooltip(const std::string &tooltip) {
  impl_->window_->ShowTooltip(tooltip);
}

void SingleViewHost::ShowTooltipAtPosition(const std::string &tooltip,
                                           double x, double y) {
  impl_->window_->ShowTooltipAtPosition(tooltip, x, y);
}

bool SingleViewHost::ShowView(bool modal, int flags,
                              Slot1<bool, int>* feedback_handler) {
  return impl_->ShowView(modal, flags, feedback_handler);
}

void SingleViewHost::CloseView() {
  impl_->CloseView();
}

bool SingleViewHost::ShowContextMenu(int button) {
  SetCursor(ViewInterface::CURSOR_DEFAULT);
  return impl_->ShowContextMenu(button);
}

void SingleViewHost::Alert(const ViewInterface* view, const char* message) {
  UTF16String caption_utf16, message_utf16;
  ConvertStringUTF8ToUTF16(view->GetCaption(), &caption_utf16);
  ConvertStringUTF8ToUTF16(message, &message_utf16);
  ::MessageBox(NULL,
               reinterpret_cast<const wchar_t*>(message_utf16.c_str()),
               reinterpret_cast<const wchar_t*>(caption_utf16.c_str()),
               MB_OK);
}

ViewHostInterface::ConfirmResponse SingleViewHost::Confirm(
    const ViewInterface* view, const char* message, bool cancel_button) {
  UTF16String caption_utf16, message_utf16;
  ConvertStringUTF8ToUTF16(view->GetCaption(), &caption_utf16);
  ConvertStringUTF8ToUTF16(message, &message_utf16);
  UINT type = cancel_button ? MB_YESNOCANCEL : MB_YESNO;
  int response = ::MessageBox(
      NULL,
      reinterpret_cast<const wchar_t*>(message_utf16.c_str()),
      reinterpret_cast<const wchar_t*>(caption_utf16.c_str()),
      type);
  switch (response) {
    case IDYES:
      return ViewHostInterface::CONFIRM_YES;
    case IDNO:
      return ViewHostInterface::CONFIRM_NO;
    case IDCANCEL:
      if (cancel_button)
        return ViewHostInterface::CONFIRM_CANCEL;
      else
        return ViewHostInterface::CONFIRM_NO;
    default:
      ASSERT_M(0, ("Not supported response id: %d", response));
      return ViewHostInterface::CONFIRM_NO;  // To avoid warning C4715.
  }
}

std::string SingleViewHost::Prompt(
    const ViewInterface* view, const char* message, const char* default_value) {
  GGL_UNUSED(view);
  GGL_UNUSED(message);
  ASSERT_M(0, ("Prompt is not supported yet!\n"));
  return default_value;
}

void SingleViewHost::BeginMoveDrag(int button) {
  GGL_UNUSED(button);
}

void SingleViewHost::BeginResizeDrag(int button,
                                     ViewInterface::HitTest hittest) {
  GGL_UNUSED(button);
  GGL_UNUSED(hittest);
}

int SingleViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

SingleViewHost::~SingleViewHost() {
}

void SingleViewHost::SetTopMost(bool topmost) {
  if (impl_->window_.get() && impl_->window_->IsWindow()) {
    if (topmost) {
      ::SetWindowPos(impl_->window_->GetHWND(),
          HWND_TOPMOST, 0, 0, 0, 0,
          SWP_NOMOVE | SWP_NOREDRAW | SWP_NOACTIVATE |SWP_NOSIZE);
    } else {
      ::SetWindowPos(impl_->window_->GetHWND(),
          HWND_NOTOPMOST, 0, 0, 0, 0,
          SWP_NOMOVE | SWP_NOREDRAW | SWP_NOACTIVATE |SWP_NOSIZE);
    }
  }
}

bool SingleViewHost::GetTopMost() {
  if (impl_->window_.get() && impl_->window_->IsWindow()) {
    DWORD exstyle = ::GetWindowLong(impl_->window_->GetHWND(), GWL_EXSTYLE);
    return (exstyle & WS_EX_TOPMOST) != 0;
  } else {
    return false;
  }
}

void SingleViewHost::SetWindowPosition(int x, int y) {
  if (impl_->window_.get() && impl_->window_->IsWindow())
    impl_->window_->SetWindowPosition(x, y);
}

void SingleViewHost::GetWindowPosition(int* x, int* y) {
  if (impl_->window_.get() && impl_->window_->IsWindow())
    impl_->window_->GetWindowPosition(x, y);
}

void SingleViewHost::GetWindowSize(int* width, int* height) {
  if (impl_->window_.get() && impl_->window_->IsWindow())
    impl_->window_->GetWindowSize(width, height);
}

const Gdiplus::Bitmap* SingleViewHost::GetViewContent() {
  ASSERT(impl_->window_.get());
  if (impl_->window_.get() && impl_->window_->IsWindow())
    return impl_->window_->GetViewContent();
  return NULL;
}

void SingleViewHost::EnableAlwaysOnTopMenu(bool enable) {
  impl_->enable_always_on_top_menu_ = enable;
}

void SingleViewHost::SetFocusable(bool focusable) {
  ASSERT(impl_->window_.get());
  impl_->window_->Enable(focusable);
}

void SingleViewHost::SetOpacity(double opacity) {
  ASSERT(impl_->window_.get());
  impl_->window_->SetOpacity(opacity);
}

Connection* SingleViewHost::ConnectOnEndMoveDrag(
    Slot2<void, int, int>* handler) {
  ASSERT(impl_->window_.get());
  return impl_->window_->ConnectOnEndMoveDrag(handler);
}

Connection* SingleViewHost::ConnectOnShowContextMenu(
    Slot1<bool, MenuInterface*>* handler) {
  return impl_->on_show_context_menu_.Connect(handler);
}

void SingleViewHost::SetZoom(double zoom) {
  ASSERT(impl_->window_.get());
  if (impl_->zoom_ == zoom)
    return;
  impl_->zoom_ = zoom;
  impl_->window_->SetZoom(zoom);
}

double SingleViewHost::GetZoom() {
  ASSERT(impl_->window_.get());
  ASSERT(impl_->zoom_ == impl_->window_->GetZoom());
  return impl_->zoom_;
}

void SingleViewHost::SetFontScale(double scale) {
  ASSERT(impl_->window_.get());
  if (impl_->font_scale_ == scale)
    return;
  impl_->font_scale_ = scale;
  down_cast<GdiplusGraphics*>(impl_->view_->GetGraphics())->SetFontScale(scale);
  // Fire EVENT_THEME_CHANGED to notice the view that the font scale is changed.
  ggadget::SimpleEvent event(ggadget::Event::EVENT_THEME_CHANGED);
  impl_->view_->OnOtherEvent(event);
  QueueDraw();
}

}  // namespace win32
}  // namespace ggadget
