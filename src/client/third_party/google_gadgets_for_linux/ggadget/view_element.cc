/*
  Copyright 2008 Google Inc.

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

#include <vector>

#include "view_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "graphics_interface.h"
#include "logger.h"
#include "signals.h"
#include "view.h"
#include "view_host_interface.h"
#include "math_utils.h"
#include "small_object.h"
#include "clip_region.h"

namespace ggadget {

static const double kMinimumScale = 0.5;
static const double kMaximumScale = 2.0;

class ViewElement::Impl : public SmallObject<> {
 public:
  Impl(ViewElement *owner, bool no_transparent)
    : owner_(owner),
      child_view_(NULL),
      scale_(1),
      no_transparent_(no_transparent),
      onsizing_called_(false),
      onsizing_result_(false),
      onsizing_width_request_(0),
      onsizing_height_request_(0),
      onsizing_width_result_(0),
      onsizing_height_result_(0),
      abs_x0_(0),
      abs_y0_(0),
      abs_x1_(0),
      abs_y1_(0),
      onsize_connection_(NULL),
      onopen_connection_(NULL),
      on_add_clip_rect_connection_(NULL) {
    on_add_clip_rect_connection_ =
        owner_->GetView()->ConnectOnAddRectangleToClipRegion(
            NewSlot(this, &Impl::OnAddClipRect));
  }

  ~Impl() {
    if (onsize_connection_)
      onsize_connection_->Disconnect();
    if (onopen_connection_)
      onopen_connection_->Disconnect();
    if (on_add_clip_rect_connection_)
      on_add_clip_rect_connection_->Disconnect();
  }

  void OnChildViewOpen() {
    UpdateScaleAndSize();
    // Inform parent view to adjust its size.
    // The parent view might be decorated view or sidebar.
    child_view_->GetViewHost()->QueueResize();
  }

  void UpdateScaleAndSize() {
    if (child_view_) {
      scale_ = child_view_->GetGraphics()->GetZoom() /
               owner_->GetView()->GetGraphics()->GetZoom();

      double width = child_view_->GetWidth() * scale_;
      double height = child_view_->GetHeight() * scale_;

      owner_->SetPixelWidth(width);
      owner_->SetPixelHeight(height);
    } else {
      scale_ = 1.0;
    }
  }

  void OnAddClipRect(double x, double y, double w, double h) {
    if (child_view_) {
      double r[8];
      owner_->ViewCoordToChildViewCoord(x, y, &r[0], &r[1]);
      owner_->ViewCoordToChildViewCoord(x, y + h, &r[2], &r[3]);
      owner_->ViewCoordToChildViewCoord(x + w, y + h, &r[4], &r[5]);
      owner_->ViewCoordToChildViewCoord(x + w, y, &r[6], &r[7]);

      Rectangle child_rect = Rectangle::GetPolygonExtents(4, r);
      child_view_->AddRectangleToClipRegion(child_rect);
    }
  }

  ViewElement *owner_;
  View *child_view_; // This view is not owned by the element.
  double scale_;
  bool no_transparent_;

  bool onsizing_called_;
  bool onsizing_result_;
  double onsizing_width_request_;
  double onsizing_height_request_;
  double onsizing_width_result_;
  double onsizing_height_result_;

  double abs_x0_;
  double abs_y0_;
  double abs_x1_;
  double abs_y1_;

  Connection *onsize_connection_;
  Connection *onopen_connection_;

  Connection *on_add_clip_rect_connection_;
};

ViewElement::ViewElement(View *parent_view, View *child_view,
                         bool no_transparent)
  // Only 1 child so no need to involve Elements here.
  : BasicElement(parent_view, "view", NULL, false),
    impl_(new Impl(this, no_transparent)) {
  SetEnabled(true);
  SetChildView(child_view);
}

ViewElement::~ViewElement() {
  delete impl_;
  impl_ = NULL;
}

void ViewElement::SetChildView(View *child_view) {
  if (child_view == impl_->child_view_)
    return;

  if (impl_->onsize_connection_) {
    impl_->onsize_connection_->Disconnect();
    impl_->onsize_connection_ = NULL;
  }

  if (impl_->onopen_connection_) {
    impl_->onopen_connection_->Disconnect();
    impl_->onopen_connection_ = NULL;
  }

  // Hook onopen event to do the first time initialization.
  // Because when View is initialized from XML, the event is disabled, so the
  // onsize event can't be received.
  if (child_view) {
    impl_->onsize_connection_ = child_view->ConnectOnSizeEvent(
        NewSlot(impl_, &Impl::UpdateScaleAndSize));
    impl_->onopen_connection_ = child_view->ConnectOnOpenEvent(
        NewSlot(impl_, &Impl::OnChildViewOpen));
    if (GetView()->GetFocusedElement() != this)
      child_view->SetFocus(NULL);
  }

  impl_->child_view_ = child_view;
  impl_->UpdateScaleAndSize();
  QueueDraw();
}

View *ViewElement::GetChildView() const {
  return impl_->child_view_;
}

bool ViewElement::OnSizing(double *width, double *height) {
  ASSERT(width && height);
  if (*width <= 0 || *height <= 0)
    return false;

  // Any size is allowed if there is no child view.
  if (!impl_->child_view_)
    return true;

  if (impl_->onsizing_called_ &&
      impl_->onsizing_width_request_ == *width &&
      impl_->onsizing_height_request_ == *height) {
    *width = impl_->onsizing_width_result_;
    *height = impl_->onsizing_height_result_;
    return impl_->onsizing_result_;
  }

  impl_->onsizing_called_ = true;
  impl_->onsizing_width_request_ = *width;
  impl_->onsizing_height_request_ = *height;

  double child_width;
  double child_height;
  bool ret = false;
  ViewInterface::ResizableMode mode = impl_->child_view_->GetResizable();

  // If child view is resizable then just delegate OnSizing request to child
  // view.
  // The resizable view might also be zoomed, so count the scale factor in.
  if (mode == ViewInterface::RESIZABLE_TRUE ||
      mode == ViewInterface::RESIZABLE_KEEP_RATIO) {
    child_width = *width / impl_->scale_;
    child_height = *height / impl_->scale_;
    ret = impl_->child_view_->OnSizing(&child_width, &child_height);
    *width = child_width * impl_->scale_;
    *height = child_height * impl_->scale_;
  } else {
    // Otherwise adjust the width or height to maintain the aspect ratio of
    // child view.
    child_width = impl_->child_view_->GetWidth();
    child_height = impl_->child_view_->GetHeight();
    double aspect_ratio = child_width / child_height;

    // Keep the shorter edge unchanged.
    if (*width / *height < aspect_ratio) {
      *height = *width / aspect_ratio;
    } else {
      *width = *height * aspect_ratio;
    }

    // Don't allow scale to too small.
    double new_scale = *width / child_width;
    if (new_scale < kMinimumScale || new_scale > kMaximumScale) {
      new_scale = Clamp(new_scale, kMinimumScale, kMaximumScale);
      *width = child_width * new_scale;
      *height = child_height * new_scale;
    }

    // Always returns true when zooming to get smooth zoom effect.
    ret = true;
  }

  impl_->onsizing_width_result_ = *width;
  impl_->onsizing_height_result_ = *height;
  impl_->onsizing_result_ = ret;
  return ret;
}

void ViewElement::SetSize(double width, double height) {
  double old_width = GetPixelWidth();
  double old_height = GetPixelHeight();

  if (width <= 0 || height <= 0) return;
  if (width == old_width && height == old_height) return;

  // If there is no child view, then just adjust BasicElement's size.
  if (!impl_->child_view_) {
    SetPixelWidth(width);
    SetPixelHeight(height);
    return;
  }

  ViewInterface::ResizableMode mode = impl_->child_view_->GetResizable();
  if (mode == ViewInterface::RESIZABLE_TRUE ||
      mode == ViewInterface::RESIZABLE_KEEP_RATIO) {
    // The resizable view might also be zoomed, so count the scale factor in.
    impl_->child_view_->SetSize(width / impl_->scale_, height / impl_->scale_);
    impl_->UpdateScaleAndSize();
  } else {
    double child_width = impl_->child_view_->GetWidth();
    double child_height = impl_->child_view_->GetHeight();
    double aspect_ratio = child_width / child_height;

    // Calculate the scale factor according to the shorter edge.
    if (width / height < aspect_ratio)
      SetScale(width / child_width);
    else
      SetScale(height / child_height);
  }

  impl_->onsizing_called_ = false;
  QueueDraw();
}

void ViewElement::SetScale(double scale) {
  // Only set scale if child view is available.
  scale = Clamp(scale, kMinimumScale, kMaximumScale);
  if (impl_->child_view_ && scale != impl_->scale_) {
    double new_zoom = GetView()->GetGraphics()->GetZoom() * scale;
    impl_->child_view_->GetGraphics()->SetZoom(new_zoom);
    impl_->child_view_->MarkRedraw();
    impl_->UpdateScaleAndSize();
    // Inform parent view to adjust its size.
    // The parent view might be decorated view or sidebar.
    impl_->child_view_->GetViewHost()->QueueResize();
    QueueDraw();
  }
}

double ViewElement::GetScale() const {
  return impl_->scale_;
}

void ViewElement::ChildViewCoordToViewCoord(
    double child_x, double child_y, double *parent_x, double *parent_y) const {
  child_x *= impl_->scale_;
  child_y *= impl_->scale_;

  SelfCoordToViewCoord(child_x, child_y, parent_x, parent_y);
}

void ViewElement::ViewCoordToChildViewCoord(
    double view_x, double view_y, double *child_x, double *child_y) const {
  ViewCoordToSelfCoord(view_x, view_y, child_x, child_y);
  *child_x /= impl_->scale_;
  *child_y /= impl_->scale_;
}

ViewInterface::HitTest ViewElement::GetHitTest(double x, double y) const {
  // Assume GetHitTest() will be called immediately after calling
  // OnMouseEvent().
  if (impl_->child_view_) {
    // If the ViewElement's parent is a Sidebar, then in most case,
    // the child view is a view decorator, then it's necessary to
    // /return HT_NOWHERE instead of HT_TRANSPARENT to make sure that
    // the child view decorator won't hide the decorator while the mouse
    // pointer is still inside it.
    ViewInterface::HitTest hittest = impl_->child_view_->GetHitTest();
    return (hittest == ViewInterface::HT_TRANSPARENT &&
            impl_->no_transparent_) ? ViewInterface::HT_NOWHERE : hittest;
  }

  return BasicElement::GetHitTest(x, y);
}

void ViewElement::Layout() {
  BasicElement::Layout();
  if (impl_->child_view_) {
    impl_->child_view_->Layout();
    const ClipRegion *region = impl_->child_view_->GetClipRegion();
    if (region && !region->IsEmpty()) {
      if (1.0 == impl_->scale_) {
        QueueDrawRegion(*region);
      } else {
        ClipRegion scaled_region(*region);
        scaled_region.Zoom(impl_->scale_);
        QueueDrawRegion(scaled_region);
      }
    }
  }
}

void ViewElement::QueueDrawChildView() {
  if (impl_->child_view_) {
    GetView()->QueueDraw();
  }
}

void ViewElement::MarkRedraw() {
  BasicElement::MarkRedraw();
  if (impl_->child_view_)
    impl_->child_view_->MarkRedraw();
}

void ViewElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->child_view_) {
    if (impl_->scale_ != 1)
      canvas->ScaleCoordinates(impl_->scale_, impl_->scale_);
    bool clip_enabled = GetView()->IsClipRegionEnabled();
    impl_->child_view_->EnableClipRegion(clip_enabled);
    impl_->child_view_->Draw(canvas);
    impl_->child_view_->EnableClipRegion(clip_enabled);
  }
}

EventResult ViewElement::OnMouseEvent(const MouseEvent &event,
                                      bool direct,
                                      BasicElement **fired_element,
                                      BasicElement **in_element,
                                      ViewInterface::HitTest *hittest) {
  if (!impl_->child_view_)
    return BasicElement::OnMouseEvent(event, direct, fired_element,
                                      in_element, hittest);

  // child view must process the mouse event first, so that the hittest value
  // can be updated correctly.
  EventResult result1 = EVENT_RESULT_UNHANDLED;
  if (impl_->scale_ != 1.) {
    MouseEvent new_event(event);
    new_event.SetX(event.GetX() / impl_->scale_);
    new_event.SetY(event.GetY() / impl_->scale_);
    result1 = impl_->child_view_->OnMouseEvent(new_event);
  } else {
    result1 = impl_->child_view_->OnMouseEvent(event);
  }

  EventResult result2 = BasicElement::OnMouseEvent(event, direct, fired_element,
                                                   in_element, hittest);

  return std::max(result1, result2);
}

EventResult ViewElement::OnDragEvent(const DragEvent &event, bool direct,
                                     BasicElement **fired_element) {
  GGL_UNUSED(direct);
  if (!impl_->child_view_)
    return EVENT_RESULT_UNHANDLED;

  Event::Type type = event.GetType();

  // View doesn't accept DRAG_OVER event, so converts it to DRAG_MOTION.
  DragEvent new_event(type == Event::EVENT_DRAG_OVER ?
                      Event::EVENT_DRAG_MOTION : type,
                      event.GetX() / impl_->scale_,
                      event.GetY() / impl_->scale_);
  new_event.SetDragFiles(event.GetDragFiles());
  new_event.SetDragUrls(event.GetDragUrls());
  new_event.SetDragText(event.GetDragText());

  EventResult result = impl_->child_view_->OnDragEvent(new_event);

  if (result != EVENT_RESULT_UNHANDLED)
    *fired_element = this;

  return result;
}

bool ViewElement::OnAddContextMenuItems(MenuInterface *menu) {
  if (impl_->child_view_)
    return impl_->child_view_->OnAddContextMenuItems(menu);
  return false;
}

EventResult ViewElement::OnKeyEvent(const KeyboardEvent &event) {
  if (impl_->child_view_)
    return impl_->child_view_->OnKeyEvent(event);
  return EVENT_RESULT_UNHANDLED;
}

EventResult ViewElement::OnOtherEvent(const Event &event) {
  if (impl_->child_view_)
    return impl_->child_view_->OnOtherEvent(event);
  return EVENT_RESULT_UNHANDLED;
}

void ViewElement::GetDefaultSize(double *width, double *height) const {
  if (impl_->child_view_) {
    *width  = impl_->child_view_->GetWidth() * impl_->scale_;
    *height = impl_->child_view_->GetHeight() * impl_->scale_;
  } else {
    BasicElement::GetDefaultSize(width, height);
  }
}

} // namespace ggadget
