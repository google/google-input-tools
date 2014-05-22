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

#include <cmath>
#include "scrolling_element.h"
#include "canvas_interface.h"
#include "common.h"
#include "elements.h"
#include "math_utils.h"
#include "scrollbar_element.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class ScrollingElement::Impl : public SmallObject<> {
 public:
  Impl(ScrollingElement *owner)
      : owner_(owner),
        scroll_pos_x_(0), scroll_pos_y_(0),
        scroll_range_x_(0), scroll_range_y_(0),
        scrollbar_(NULL),
        x_scrollable_(true), y_scrollable_(true) {
  }
  ~Impl() {
    // Inform the view of this scrollbar to let the view handle mouse grabbing
    // and mouse over/out logics of the scrollbar.
    if (scrollbar_) {
      owner_->GetView()->OnElementRemove(scrollbar_);
      delete scrollbar_;
    }
  }

  void CreateScrollBar() {
    if (!scrollbar_) {
      scrollbar_ = new ScrollBarElement(owner_->GetView(), "");
      scrollbar_->SetParentElement(owner_);
      scrollbar_->SetPixelHeight(owner_->GetPixelHeight());
      scrollbar_->SetPixelWidth(12); // width of default images
      scrollbar_->SetEnabled(true);
      scrollbar_->SetPixelX(owner_->GetPixelWidth() -
                            scrollbar_->GetPixelWidth());
      scrollbar_->SetMax(scroll_range_y_);
      scrollbar_->SetValue(scroll_pos_y_);
      // A reasonable default value.
      scrollbar_->SetLineStep(10);
      scrollbar_->ConnectOnChangeEvent(NewSlot(this, &Impl::OnScrollBarChange));
      // When user clicks the scroll bar, let the view give focus to
      // this element.
      scrollbar_->ConnectOnFocusInEvent(
          NewSlot(implicit_cast<BasicElement *>(owner_), &BasicElement::Focus));
      // Inform the view of this scrollbar to let the view handle mouse
      // grabbing and mouse over/out logics of the scrollbar.
      owner_->GetView()->OnElementAdd(scrollbar_);
    }
  }

  // UpdateScrollBar is called in Layout(), no need to call QueueDraw().
  void UpdateScrollBar(int x_range, int y_range) {
    ASSERT(scrollbar_);
    int old_range_y = scroll_range_y_;
    scroll_range_y_ = std::max(0, y_range);
    scroll_pos_y_ = std::min(scroll_pos_y_, scroll_range_y_);
    bool show_scrollbar = scrollbar_ && (scroll_range_y_ > 0);
    if (old_range_y != scroll_range_y_) {
      scrollbar_->SetMax(scroll_range_y_);
      scrollbar_->SetValue(scroll_pos_y_);
    }
    scrollbar_->SetVisible(show_scrollbar);
    if (show_scrollbar)
      scrollbar_->Layout();

    scroll_range_x_ = std::max(0, x_range);
    scroll_pos_x_ = std::min(scroll_pos_x_, scroll_range_x_);
  }

  void ScrollX(int distance) {
    if (x_scrollable_) {
      int old_pos = scroll_pos_x_;
      scroll_pos_x_ += distance;
      scroll_pos_x_ = std::min(scroll_range_x_, std::max(0, scroll_pos_x_));
      if (old_pos != scroll_pos_x_) {
        owner_->QueueDraw();
      }
    }
  }

  void ScrollY(int distance) {
    if (y_scrollable_) {
      int old_pos = scroll_pos_y_;
      scroll_pos_y_ += distance;
      scroll_pos_y_ = std::min(scroll_range_y_, std::max(0, scroll_pos_y_));
      if (old_pos != scroll_pos_y_ && scrollbar_) {
        scrollbar_->SetValue(scroll_pos_y_); // Will trigger OnScrollBarChange()
      }
    }
  }

  void OnScrollBarChange() {
    scroll_pos_y_ = scrollbar_->GetValue();
    on_scrolled_event_();
    owner_->QueueDraw();
  }

  void MarkRedraw() {
    if (scrollbar_)
      scrollbar_->MarkRedraw();
  }

  ScrollingElement *owner_;
  int scroll_pos_x_, scroll_pos_y_;
  int scroll_range_x_, scroll_range_y_;
  ScrollBarElement *scrollbar_; // NULL if and only if autoscroll=false
  EventSignal on_scrolled_event_;
  bool x_scrollable_:1;
  bool y_scrollable_:1;
};

ScrollingElement::ScrollingElement(View *view, const char *tag_name,
                                   const char *name, bool children)
    : BasicElement(view, tag_name, name, children),
      impl_(new Impl(this)) {
}

void ScrollingElement::DoRegister() {
  BasicElement::DoRegister();
}

void ScrollingElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("scrollbar",
                   NewSlot(static_cast<ScrollBarElement *(ScrollingElement::*)
                       ()>(&ScrollingElement::GetScrollBar)),
                   NULL);
}

ScrollingElement::~ScrollingElement() {
  delete impl_;
}

void ScrollingElement::MarkRedraw() {
  BasicElement::MarkRedraw();
  impl_->MarkRedraw();
}

bool ScrollingElement::IsAutoscroll() const {
  return impl_->scrollbar_ != NULL;
}

void ScrollingElement::SetAutoscroll(bool autoscroll) {
  if (impl_->y_scrollable_ && (impl_->scrollbar_ != NULL) != autoscroll) {
    if (autoscroll) {
      impl_->CreateScrollBar();
    } else {
      // Inform the view of this scrollbar to let the view handle mouse grabbing
      // and mouse over/out logics of the scrollbar.
      GetView()->OnElementRemove(impl_->scrollbar_);
      delete impl_->scrollbar_;
      impl_->scrollbar_ = NULL;
    }
    SetChildrenScrollable(autoscroll);
    QueueDraw();
  }
}

bool ScrollingElement::IsXScrollable() {
  return impl_->x_scrollable_;
}

void ScrollingElement::SetXScrollable(bool x_scrollable) {
  impl_->x_scrollable_ = x_scrollable;
}

bool ScrollingElement::IsYScrollable() {
  return impl_->y_scrollable_;
}

void ScrollingElement::SetYScrollable(bool y_scrollable) {
  impl_->y_scrollable_ = y_scrollable;
}

void ScrollingElement::ScrollX(int distance) {
  impl_->ScrollX(distance);
}

void ScrollingElement::ScrollY(int distance) {
  impl_->ScrollY(distance);
}

int ScrollingElement::GetScrollXPosition() const {
  return impl_->scroll_pos_x_;
}

int ScrollingElement::GetScrollYPosition() const {
  return impl_->scroll_pos_y_;
}

void ScrollingElement::SetScrollXPosition(int pos) {
  impl_->ScrollX(pos - impl_->scroll_pos_x_);
}

void ScrollingElement::SetScrollYPosition(int pos) {
  impl_->ScrollY(pos - impl_->scroll_pos_y_);
}

int ScrollingElement::GetXPageStep() const {
  // TODO: X scroll bar is not supported yet.
  return 0;
}

void ScrollingElement::SetXPageStep(int value) {
  GGL_UNUSED(value);
  // TODO: X scroll bar is not supported yet.
}

int ScrollingElement::GetYPageStep() const {
  if (impl_->scrollbar_)
    return impl_->scrollbar_->GetPageStep();
  return 0;
}

void ScrollingElement::SetYPageStep(int value) {
  if (impl_->scrollbar_)
    impl_->scrollbar_->SetPageStep(value);
}

int ScrollingElement::GetXLineStep() const {
  // TODO: X scroll bar is not supported yet.
  return 0;
}

void ScrollingElement::SetXLineStep(int value) {
  GGL_UNUSED(value);
  // TODO: X scroll bar is not supported yet.
}

int ScrollingElement::GetYLineStep() const {
  if (impl_->scrollbar_)
    return impl_->scrollbar_->GetLineStep();
  return 0;
}

void ScrollingElement::SetYLineStep(int value) {
  if (impl_->scrollbar_)
    impl_->scrollbar_->SetLineStep(value);
}

EventResult ScrollingElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                           BasicElement **fired_element,
                                           BasicElement **in_element,
                                           ViewInterface::HitTest *hittest) {
  if (!direct && impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    double new_x = event.GetX() - impl_->scrollbar_->GetPixelX();
    double new_y = event.GetY() - impl_->scrollbar_->GetPixelY();
    if (IsPointInElement(new_x, new_y, impl_->scrollbar_->GetPixelWidth(),
                         impl_->scrollbar_->GetPixelHeight())) {
      MouseEvent new_event(event);
      new_event.SetX(new_x);
      new_event.SetY(new_y);
      return impl_->scrollbar_->OnMouseEvent(new_event, direct, fired_element,
                                             in_element, hittest);
    }
  }

  ElementHolder self_holder(this);
  EventResult result = BasicElement::OnMouseEvent(event, direct, fired_element,
                                                  in_element, hittest);
  if (result == EVENT_RESULT_UNHANDLED &&
      event.GetType() == Event::EVENT_MOUSE_WHEEL &&
      impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    return impl_->scrollbar_->HandleMouseEvent(event);
  }
  return result;
}

void ScrollingElement::SelfCoordToChildCoord(const BasicElement *child,
                                             double x, double y,
                                             double *child_x,
                                             double *child_y) const {
  if (child != impl_->scrollbar_) {
    x += impl_->scroll_pos_x_;
    y += impl_->scroll_pos_y_;
  }
  BasicElement::SelfCoordToChildCoord(child, x, y, child_x, child_y);
}

void ScrollingElement::ChildCoordToSelfCoord(const BasicElement *child,
                                             double x, double y,
                                             double *self_x,
                                             double *self_y) const {
  BasicElement::ChildCoordToSelfCoord(child, x, y, self_x, self_y);
  if (child != impl_->scrollbar_) {
    *self_x -= impl_->scroll_pos_x_;
    *self_y -= impl_->scroll_pos_y_;
  }
}

void ScrollingElement::DrawScrollbar(CanvasInterface *canvas) {
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    canvas->TranslateCoordinates(impl_->scrollbar_->GetPixelX(),
                                 impl_->scrollbar_->GetPixelY());
    impl_->scrollbar_->Draw(canvas);
  }
}

bool ScrollingElement::UpdateScrollBar(int x_range, int y_range) {
  if (impl_->scrollbar_) {
    bool old_visible = impl_->scrollbar_->IsVisible();
    impl_->scrollbar_->SetPixelX(GetPixelWidth() -
                                 impl_->scrollbar_->GetPixelWidth());
    impl_->scrollbar_->SetPixelHeight(GetPixelHeight());

    impl_->UpdateScrollBar(x_range, y_range);
    return old_visible != impl_->scrollbar_->IsVisible();
  }
  return false;
}

double ScrollingElement::GetClientWidth() const {
  return impl_->scrollbar_ && impl_->scrollbar_->IsVisible() ?
         std::max(GetPixelWidth() - impl_->scrollbar_->GetPixelWidth(), 0.0) :
         GetPixelWidth();
}

double ScrollingElement::GetClientHeight() const {
  // Horizontal scrollbar is not supported for now.
  return GetPixelHeight();
}

Connection *ScrollingElement::ConnectOnScrolledEvent(Slot0<void> *slot) {
  return impl_->on_scrolled_event_.Connect(slot);
}

ScrollBarElement *ScrollingElement::GetScrollBar() {
  return impl_->scrollbar_;
}

const ScrollBarElement *ScrollingElement::GetScrollBar() const {
  return impl_->scrollbar_;
}

void ScrollingElement::AggregateMoreClipRegion(const Rectangle &boundary,
                                               ClipRegion *region) {
  if (impl_->scrollbar_)
    impl_->scrollbar_->AggregateClipRegion(boundary, region);
}

bool ScrollingElement::IsChildInVisibleArea(const BasicElement *child) const {
  double min_x, min_y, max_x, max_y;
  GetChildRectExtentInParent(child->GetPixelX(), child->GetPixelY(),
                             child->GetPixelPinX(), child->GetPixelPinY(),
                             DegreesToRadians(child->GetRotation()),
                             0, 0,
                             child->GetPixelWidth(), child->GetPixelHeight(),
                             &min_x, &min_y, &max_x, &max_y);
  min_x -= impl_->scroll_pos_x_;
  max_x -= impl_->scroll_pos_x_;
  min_y -= impl_->scroll_pos_y_;
  max_y -= impl_->scroll_pos_y_;
  return max_x > 0 && max_y > 0 &&
         min_x < GetPixelWidth() && min_y < GetPixelHeight();
}

void ScrollingElement::EnsureAreaVisible(const Rectangle &rect,
                                         const BasicElement *source) {
  if (source == impl_->scrollbar_) {
    // The coordinates of the scroll bar have no offset.
    BasicElement::EnsureAreaVisible(rect, source);
  } else {
    double left = rect.x - impl_->scroll_pos_x_;
    double top = rect.y - impl_->scroll_pos_y_;
    if (!IsAutoscroll()) {
      BasicElement::EnsureAreaVisible(
          Rectangle(left, top, rect.w, rect.h), source);
    } else {
      int min_x = static_cast<int>(floor(left));
      int min_y = static_cast<int>(floor(top));
      int max_x = static_cast<int>(ceil(left + rect.w));
      int max_y = static_cast<int>(ceil(top + rect.h));

      if (min_x < 0) {
        int old_scroll_x = impl_->scroll_pos_x_;
        impl_->ScrollX(min_x);
        int diff = old_scroll_x - impl_->scroll_pos_x_;
        min_x += diff;
        max_x += diff;
      }
      if (min_y < 0) {
        int old_scroll_y = impl_->scroll_pos_y_;
        impl_->ScrollY(min_y);
        int diff = old_scroll_y - impl_->scroll_pos_y_;
        min_y += diff;
        max_y += diff;
      }
      int width = static_cast<int>(ceil(GetPixelWidth()));
      if (min_x > 0 && max_x > width) {
        int old_scroll_x = impl_->scroll_pos_x_;
        impl_->ScrollX(max_x - width);
        int diff = old_scroll_x - impl_->scroll_pos_x_;
        min_x += diff;
        max_x += diff;
      }
      int height = static_cast<int>(ceil(GetPixelHeight()));
      if (min_y > 0 && max_y > height) {
        int old_scroll_y = impl_->scroll_pos_y_;
        impl_->ScrollY(max_y - height);
        int diff = old_scroll_y - impl_->scroll_pos_y_;
        min_y += diff;
        max_y += diff;
      }

      BasicElement::EnsureAreaVisible(
          Rectangle(min_x, min_y, max_x - min_x, max_y - min_y), source);
    }
  }
}

} // namespace ggadget
