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

#include "decorated_view_host.h"

#include <vector>
#include "gadget_consts.h"
#include "host_interface.h"
#include "signals.h"
#include "slot.h"
#include "menu_interface.h"
#include "small_object.h"

namespace ggadget {

class DecoratedViewHost::Impl : public SmallObject<> {
 public:
  Impl(ViewDecoratorBase *view_decorator)
    : view_decorator_(view_decorator),
      auto_load_child_view_size_(true),
      child_view_size_loaded_(false) {
    ASSERT(view_decorator);
  }

  ~Impl() {
    delete view_decorator_;
  }

  void SaveChildViewSize() {
    View *child = view_decorator_->GetChildView();
    if (auto_load_child_view_size_ && child_view_size_loaded_ &&
        child && child->GetWidth() > 0 && child->GetHeight() > 0) {
      view_decorator_->SaveChildViewSize();
    }
  }

  ViewDecoratorBase *view_decorator_;
  bool auto_load_child_view_size_ : 1;
  bool child_view_size_loaded_    : 1;
};

DecoratedViewHost::DecoratedViewHost(ViewDecoratorBase *view_decorator)
  : impl_(new Impl(view_decorator)) {
}

DecoratedViewHost::~DecoratedViewHost() {
  delete impl_;
  impl_ = NULL;
}

ViewDecoratorBase *DecoratedViewHost::GetViewDecorator() const {
  return impl_->view_decorator_;
}

void DecoratedViewHost::LoadChildViewSize() {
  impl_->view_decorator_->LoadChildViewSize();
  impl_->child_view_size_loaded_ = true;
}

void DecoratedViewHost::SetAutoLoadChildViewSize(bool auto_load) {
  impl_->auto_load_child_view_size_ = auto_load;
}

bool DecoratedViewHost::IsAutoLoadChildViewSize() const {
  return impl_->auto_load_child_view_size_;
}

ViewHostInterface::Type DecoratedViewHost::GetType() const {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  return view_host ? view_host->GetType() : VIEW_HOST_MAIN;
}

void DecoratedViewHost::Destroy() {
  delete this;
}

void DecoratedViewHost::SetView(ViewInterface *view) {
  impl_->view_decorator_->SetChildView(down_cast<View *>(view));
  if (impl_->auto_load_child_view_size_ && view &&
      view->GetWidth() > 0 && view->GetHeight() > 0) {
    // Only load child view size if the view has been initialized.
    impl_->view_decorator_->LoadChildViewSize();
    impl_->child_view_size_loaded_ = true;
  } else {
    impl_->child_view_size_loaded_ = false;
  }
}

ViewInterface *DecoratedViewHost::GetView() const {
  return impl_->view_decorator_->GetChildView();
}

GraphicsInterface *DecoratedViewHost::NewGraphics() const {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  return view_host ? view_host->NewGraphics() : NULL;
}

void *DecoratedViewHost::GetNativeWidget() const {
  return impl_->view_decorator_->GetNativeWidget();
}

void DecoratedViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  double px, py;
  impl_->view_decorator_->ChildViewCoordToViewCoord(x, y, &px, &py);
  impl_->view_decorator_->ViewCoordToNativeWidgetCoord(px, py,
                                                       widget_x, widget_y);
}

void DecoratedViewHost::NativeWidgetCoordToViewCoord(
    double x, double y, double *view_x, double *view_y) const {
  double px, py;
  impl_->view_decorator_->NativeWidgetCoordToViewCoord(x, y, &px, &py);
  impl_->view_decorator_->ViewCoordToChildViewCoord(px, py, view_x, view_y);
}

void DecoratedViewHost::QueueDraw() {
  impl_->view_decorator_->QueueDrawChildView();
}

void DecoratedViewHost::QueueResize() {
  impl_->SaveChildViewSize();
  impl_->view_decorator_->UpdateViewSize();
}

void DecoratedViewHost::EnableInputShapeMask(bool /* enable */) {
  // Do nothing.
}

void DecoratedViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  impl_->view_decorator_->SetResizable(mode);
}

void DecoratedViewHost::SetCaption(const std::string &caption) {
  impl_->view_decorator_->SetCaption(caption);
}

void DecoratedViewHost::SetShowCaptionAlways(bool always) {
  impl_->view_decorator_->SetShowCaptionAlways(always);
}

void DecoratedViewHost::SetCursor(ViewInterface::CursorType type) {
  impl_->view_decorator_->SetChildViewCursor(type);
}

void DecoratedViewHost::ShowTooltip(const std::string &tooltip) {
  impl_->view_decorator_->ShowChildViewTooltip(tooltip);
}

void DecoratedViewHost::ShowTooltipAtPosition(const std::string &tooltip,
                                              double x, double y) {
  impl_->view_decorator_->ShowChildViewTooltipAtPosition(tooltip, x, y);
}

bool DecoratedViewHost::ShowView(bool modal, int flags,
                                 Slot1<bool, int> *feedback_handler) {
  if (impl_->auto_load_child_view_size_) {
    impl_->view_decorator_->LoadChildViewSize();
    impl_->child_view_size_loaded_ = true;
  }
  return impl_->view_decorator_->ShowDecoratedView(modal, flags,
                                                   feedback_handler);
}

void DecoratedViewHost::CloseView() {
  impl_->view_decorator_->CloseDecoratedView();
}

bool DecoratedViewHost::ShowContextMenu(int button) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  return view_host ? view_host->ShowContextMenu(button) : false;
}

void DecoratedViewHost::Alert(const ViewInterface *view, const char *message) {
  GGL_UNUSED(view);
  impl_->view_decorator_->Alert(message);
}

ViewHostInterface::ConfirmResponse DecoratedViewHost::Confirm(
    const ViewInterface *view, const char *message, bool cancel_button) {
  GGL_UNUSED(view);
  return impl_->view_decorator_->Confirm(message, cancel_button);
}

std::string DecoratedViewHost::Prompt(const ViewInterface *view,
                                      const char *message,
                                      const char *default_value) {
  GGL_UNUSED(view);
  return impl_->view_decorator_->Prompt(message, default_value);
}

int DecoratedViewHost::GetDebugMode() const {
  return impl_->view_decorator_->GetDebugMode();
}

void DecoratedViewHost::BeginResizeDrag(int button,
                                        ViewInterface::HitTest hittest) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  if (view_host)
    view_host->BeginResizeDrag(button, hittest);
}

void DecoratedViewHost::BeginMoveDrag(int button) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  if (view_host)
    view_host->BeginMoveDrag(button);
}

} // namespace ggadget
