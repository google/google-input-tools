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

// Enable it to print verbose debug info
// #define EVENT_VERBOSE_DEBUG

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "basic_element.h"
#include "canvas_utils.h"
#include "clip_region.h"
#include "common.h"
#include "element_factory.h"
#include "elements.h"
#include "event.h"
#include "gadget_consts.h"
#include "gadget_interface.h"
#include "graphics_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "menu_interface.h"
#include "permissions.h"
#include "scriptable_event.h"
#include "small_object.h"
#include "string_utils.h"
#include "view.h"

// Prevents windows max/min macros conflicting with std::max/min.
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace ggadget {

class BasicElement::Impl : public SmallObject<> {
 public:
  Impl(View *view, const char *tag_name, const char *name,
       bool allow_children, BasicElement *owner)
      : parent_(NULL),
        owner_(owner),
        children_(allow_children ?
                  new Elements(view->GetElementFactory(), owner, view) :
                  NULL),
        view_(view),
        mask_image_(NULL),
        focus_overlay_(NULL),
        cache_(NULL),
        tag_name_(tag_name),
        index_(kInvalidIndex), // Invalid until set by Elements.
        width_(0.0), height_(0.0), pwidth_(0.0), pheight_(0.0),
        min_width_(0.0), min_height_(0.0),
        x_(0.0), y_(0.0), px_(0.0), py_(0.0),
        pin_x_(0.0), pin_y_(0.0), ppin_x_(0.0), ppin_y_(0.0),
        rotation_(0.0),
        opacity_(1.0),
        name_(name ? name : ""),
#ifdef _DEBUG
        debug_color_index_(++total_debug_color_index_),
        debug_mode_(view->GetDebugMode()),
#endif
        hittest_(ViewInterface::HT_CLIENT),
        cursor_(ViewInterface::CURSOR_DEFAULT),
        flip_(FLIP_NONE),
        text_direction_(TEXT_DIRECTION_INHERIT_FROM_VIEW),
        drop_target_(false),
        enabled_(false),
        width_relative_(false), height_relative_(false),
        width_specified_(false), height_specified_(false),
        x_relative_(false), y_relative_(false),
        x_specified_(false), y_specified_(false),
        pin_x_relative_(false), pin_y_relative_(false),
        visible_(true),
        visibility_changed_(true),
        position_changed_(true),
        size_changed_(true),
        cache_enabled_(false),
        content_changed_(false),
        draw_queued_(false),
        designer_mode_(false),
        show_focus_overlay_(false),
        show_focus_overlay_set_(false),
        tab_stop_(false),
        tab_stop_set_(false) {
  }

  ~Impl() {
    DestroyImage(mask_image_);
    delete children_;
    DestroyCanvas(cache_);
  }

  void SetMask(const Variant &mask) {
    DestroyImage(mask_image_);
    mask_image_ = view_->LoadImage(mask, true);
    QueueDraw();
  }

  const CanvasInterface *GetMaskCanvas() {
    return mask_image_ ? mask_image_->GetCanvas() : NULL;
  }

  void SetFocusOverlay(const Variant &image) {
    DestroyImage(focus_overlay_);
    focus_overlay_ = view_->LoadImage(image, false);
    if (view_->IsFocused() && view_->GetFocusedElement() == owner_) {
      QueueDraw();
    }
  }

  void SetPixelWidth(double width) {
    if (width >= 0.0 && (width != width_ || width_relative_)) {
      AddToClipRegion(NULL);
      width_ = width;
      width_relative_ = false;
      WidthChanged();
    }
  }

  void SetPixelHeight(double height) {
    if (height >= 0.0 && (height != height_ || height_relative_)) {
      AddToClipRegion(NULL);
      height_ = height;
      height_relative_ = false;
      HeightChanged();
    }
  }

  void SetRelativeWidth(double width) {
    if (width >= 0.0 && (width != pwidth_ || !width_relative_)) {
      AddToClipRegion(NULL);
      pwidth_ = width;
      width_relative_ = true;
      WidthChanged();
    }
  }

  void SetRelativeHeight(double height) {
    if (height >= 0.0 && (height != pheight_ || !height_relative_)) {
      AddToClipRegion(NULL);
      pheight_ = height;
      height_relative_ = true;
      HeightChanged();
    }
  }

  void SetPixelX(double x) {
    if (x != x_ || x_relative_) {
      AddToClipRegion(NULL);
      x_ = x;
      x_relative_ = false;
      PositionChanged();
    }
  }

  void SetPixelY(double y) {
    if (y != y_ || y_relative_) {
      AddToClipRegion(NULL);
      y_ = y;
      y_relative_ = false;
      PositionChanged();
    }
  }

  void SetRelativeX(double x) {
    if (x != px_ || !x_relative_) {
      AddToClipRegion(NULL);
      px_ = x;
      x_relative_ = true;
      PositionChanged();
    }
  }

  void SetRelativeY(double y) {
    if (y != py_ || !y_relative_) {
      AddToClipRegion(NULL);
      py_ = y;
      y_relative_ = true;
      PositionChanged();
    }
  }

  void SetPixelPinX(double pin_x) {
    if (pin_x != pin_x_ || pin_x_relative_) {
      AddToClipRegion(NULL);
      pin_x_ = pin_x;
      pin_x_relative_ = false;
      PositionChanged();
    }
  }

  void SetPixelPinY(double pin_y) {
    if (pin_y != pin_y_ || pin_y_relative_) {
      AddToClipRegion(NULL);
      pin_y_ = pin_y;
      pin_y_relative_ = false;
      PositionChanged();
    }
  }

  void SetRelativePinX(double pin_x) {
    if (pin_x != ppin_x_ || !pin_x_relative_) {
      AddToClipRegion(NULL);
      ppin_x_ = pin_x;
      pin_x_relative_ = true;
      PositionChanged();
    }
  }

  void SetRelativePinY(double pin_y) {
    if (pin_y != ppin_y_ || !pin_y_relative_) {
      AddToClipRegion(NULL);
      ppin_y_ = pin_y;
      pin_y_relative_ = true;
      PositionChanged();
    }
  }

  void SetRotation(double rotation) {
    if (rotation != rotation_) {
      AddToClipRegion(NULL);
      rotation_ = rotation;
      PositionChanged();
    }
  }

  void SetOpacity(double opacity) {
    if (opacity != opacity_) {
      // Assumes the visibility is changed if opacity is changed from/to 0.
      if (opacity == 0 || opacity_ == 0)
        visibility_changed_ = true;
      opacity_ = opacity;
      QueueDraw();
    }
  }

  void SetVisible(bool visible) {
    if (visible != visible_) {
      visible_ = visible;
      visibility_changed_ = true;
      QueueDraw();

      // Frees canvas cache when the element becomes invisible to save memory.
      if (!visible && cache_) {
        DestroyCanvas(cache_);
        cache_ = NULL;
      }
    }
  }

  // Retrieves the opacity of the element.
  int GetIntOpacity() {
    return static_cast<int>(round(opacity_ * 255));
  }

  // Sets the opacity of the element.
  void SetIntOpacity(int opacity) {
    opacity = Clamp(opacity, 0, 255);
    SetOpacity(opacity / 255.0);
  }

  double GetParentWidth() {
    return parent_ ? parent_->GetClientWidth() : view_->GetWidth();
  }

  double GetParentHeight() {
    return parent_ ? parent_->GetClientHeight() : view_->GetHeight();
  }

  Variant GetWidth() {
    return BasicElement::GetPixelOrRelative(width_relative_, width_specified_,
                                            width_, pwidth_);
  }

  void SetWidth(const Variant &width) {
    double v;
    switch (ParsePixelOrRelative(width, &v)) {
      case BasicElement::PR_PIXEL:
        width_specified_ = true;
        SetPixelWidth(v);
        break;
      case BasicElement::PR_RELATIVE:
        width_specified_ = true;
        SetRelativeWidth(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetWidthToDefault();
        break;
      default:
        break;
    }
  }

  void ResetWidthToDefault() {
    if (width_specified_) {
      width_relative_ = width_specified_ = false;
      WidthChanged();
    }
  }

  Variant GetHeight() {
    return BasicElement::GetPixelOrRelative(height_relative_, height_specified_,
                                            height_, pheight_);
  }

  void SetHeight(const Variant &height) {
    double v;
    switch (ParsePixelOrRelative(height, &v)) {
      case BasicElement::PR_PIXEL:
        height_specified_ = true;
        SetPixelHeight(v);
        break;
      case BasicElement::PR_RELATIVE:
        height_specified_ = true;
        SetRelativeHeight(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetHeightToDefault();
        break;
      default:
        break;
    }
  }

  void ResetHeightToDefault() {
    if (height_specified_) {
      height_relative_ = height_specified_ = false;
      HeightChanged();
    }
  }

  Variant GetX() {
    return BasicElement::GetPixelOrRelative(x_relative_, x_specified_, x_, px_);
  }

  void SetX(const Variant &x) {
    double v;
    switch (ParsePixelOrRelative(x, &v)) {
      case BasicElement::PR_PIXEL:
        x_specified_ = true;
        SetPixelX(v);
        break;
      case BasicElement::PR_RELATIVE:
        x_specified_ = true;
        SetRelativeX(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetXToDefault();
        break;
      default:
        break;
    }
  }

  void ResetXToDefault() {
    if (x_specified_) {
      x_relative_ = x_specified_ = false;
      PositionChanged();
    }
  }

  Variant GetY() {
    return BasicElement::GetPixelOrRelative(y_relative_, y_specified_, y_, py_);
  }

  void SetY(const Variant &y) {
    double v;
    switch (ParsePixelOrRelative(y, &v)) {
      case BasicElement::PR_PIXEL:
        y_specified_ = true;
        SetPixelY(v);
        break;
      case BasicElement::PR_RELATIVE:
        y_specified_ = true;
        SetRelativeY(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetYToDefault();
        break;
      default:
        break;
    }
  }

  void ResetYToDefault() {
    if (y_specified_) {
      y_relative_ = y_specified_ = false;
      PositionChanged();
    }
  }

  Variant GetPinX() {
    return BasicElement::GetPixelOrRelative(pin_x_relative_, true,
                                            pin_x_, ppin_x_);
  }

  void SetPinX(const Variant &pin_x) {
    double v;
    switch (ParsePixelOrRelative(pin_x, &v)) {
      case BasicElement::PR_PIXEL:
        SetPixelPinX(v);
        break;
      case BasicElement::PR_RELATIVE:
        SetRelativePinX(v);
        break;
      default:
        break;
    }
  }

  Variant GetPinY() {
    return BasicElement::GetPixelOrRelative(pin_y_relative_, true,
                                            pin_y_, ppin_y_);
  }

  void SetPinY(const Variant &pin_y) {
    double v;
    switch (ParsePixelOrRelative(pin_y, &v)) {
      case BasicElement::PR_PIXEL:
        SetPixelPinY(v);
        break;
      case BasicElement::PR_RELATIVE:
        SetRelativePinY(v);
        break;
      default:
        break;
    }
  }

  void CalculateRelativeAttributes() {
    if (!x_specified_ || !y_specified_) {
      double x, y;
      owner_->GetDefaultPosition(&x, &y);
      if (!x_specified_)
        SetPixelX(x);
      if (!y_specified_)
        SetPixelY(y);
    }

    // Adjust relative size and positions.
    // Only values computed with parent width and height need change-checking
    // to react to parent size changes.
    // Other values have already triggered position_changed_ when they were set.
    double parent_width = GetParentWidth();
    if (x_relative_) {
      // 'volatile' prevents the value of new_x from being used from
      // registers, ensures the correct new_x != x_ comparison.
      volatile double new_x = px_ * parent_width;
      if (new_x != x_) {
        position_changed_ = true;
        x_ = new_x;
      }
    } else {
      px_ = parent_width > 0.0 ? x_ / parent_width : 0.0;
    }
    if (width_relative_) {
      volatile double new_width = pwidth_ * parent_width;
      if (new_width != width_) {
        size_changed_ = true;
        width_ = new_width;
      }
    } else {
      pwidth_ = parent_width > 0.0 ? width_ / parent_width : 0.0;
    }

    double parent_height = GetParentHeight();
    if (y_relative_) {
      volatile double new_y = py_ * parent_height;
      if (new_y != y_) {
        position_changed_ = true;
        y_ = new_y;
      }
    } else {
      py_ = parent_height > 0.0 ? y_ / parent_height : 0.0;
    }
    if (height_relative_) {
      volatile double new_height = pheight_ * parent_height;
      if (new_height != height_) {
        size_changed_ = true;
        height_ = new_height;
      }
    } else {
      pheight_ = parent_height > 0.0 ? height_ / parent_height : 0.0;
    }

    if (pin_x_relative_) {
      pin_x_ = ppin_x_ * width_;
    } else {
      ppin_x_ = width_ > 0.0 ? pin_x_ / width_ : 0.0;
    }
    if (pin_y_relative_) {
      pin_y_ = ppin_y_ * height_;
    } else {
      ppin_y_ = height_ > 0.0 ? pin_y_ / height_ : 0.0;
    }

    // Adjusts pixel size according to minimum width/height limitation.
    const double min_width = owner_->GetMinWidth();
    if (width_ < min_width) {
      size_changed_ = true;
      width_ = min_width;
    }
    const double min_height = owner_->GetMinHeight();
    if (height_ < min_height) {
      size_changed_ = true;
      height_ = min_height;
    }
  }

  void Layout() {
    CalculateRelativeAttributes();
    if (position_changed_ || size_changed_ || visibility_changed_) {
      AddToClipRegion(NULL);
    }

    owner_->BeforeChildrenLayout();

    if (size_changed_)
      PostSizeEvent();
    if (children_)
      children_->Layout();

    // Call element's layout to let it layout itself.
    // It should be called after children_->Layout() because some elements needs
    // the position and size of children elements to do its own layout.
    owner_->Layout();

    if (content_changed_) {
      // To let all associated copy elements to update their content.
      FireOnContentChangedSignal();
    }

    // No other code will use it before calling Draw(), so it's safe to reset.
    visibility_changed_ = false;
  }

  void Draw(CanvasInterface *canvas) {
    // GetPixelWidth() and GetPixelHeight might be overrode.
    double width = owner_->GetPixelWidth();
    double height = owner_->GetPixelHeight();
    // Only do draw if visible
    // Check for width, height == 0 since IntersectRectClipRegion fails for
    // those cases.
    if (visible_ && opacity_ != 0 && width > 0 && height > 0) {
      bool force_draw = false;

      // Invalidates the canvas cache if the element size has changed.
      if (cache_ &&
          (cache_->GetWidth() != width || cache_->GetHeight() != height)) {
        DestroyCanvas(cache_);
        cache_ = NULL;
        force_draw = true;
      }

      // Creates the canvas cache only when necessary.
      if (cache_enabled_) {
        if (!cache_) {
          cache_ = view_->GetGraphics()->NewCanvas(width, height);
          force_draw = true;
        } else if (content_changed_) {
          cache_->ClearCanvas();
        }
      }

      const CanvasInterface *mask = GetMaskCanvas();
      CanvasInterface *target = canvas;

      // In order to get correct result, indirect draw is required for
      // following situations:
      // - The element has mask;
      // - The element's flips in x or y;
      // - Opacity of the element is not 1.0 and it has children.
      // - Canvas cache is enabled.
      bool indirect_draw = cache_enabled_ || mask || flip_ != FLIP_NONE ||
          (opacity_ != 1.0 && children_ && children_->GetCount());
      if (indirect_draw) {
        if (cache_) {
          target = cache_;
          target->PushState();
        } else {
          target = view_->GetGraphics()->NewCanvas(width, height);
          force_draw = true;
        }
      }

      canvas->PushState();
      if (!indirect_draw)
        canvas->IntersectRectClipRegion(0, 0, width, height);
      canvas->MultiplyOpacity(opacity_);

      // Only do draw when it's direct draw or the content has been changed.
      if (!indirect_draw || content_changed_ || force_draw) {
        // Disable clip region, so that all children can be drawn correctly.
        if (cache_enabled_)
          view_->EnableClipRegion(false);

        owner_->DoDraw(target);

        if (view_->IsFocused() && view_->GetFocusedElement() == owner_ &&
            IsShowFocusOverlay()) {
          if (!focus_overlay_) {
            focus_overlay_ = view_->LoadImage(Variant(kDefaultFocusOverlay),
                                              false);
          }
          ASSERT(focus_overlay_);
          StretchMiddleDrawImage(focus_overlay_, target, 0, 0, width, height,
                                 -1, -1, -1, -1);
        }

        if (cache_enabled_)
          view_->EnableClipRegion(true);
      }

      if (indirect_draw) {
        double offset_x = 0, offset_y = 0;
        if (flip_ & FLIP_HORIZONTAL) {
          offset_x = -width;
          canvas->ScaleCoordinates(-1, 1);
        }
        if (flip_ & FLIP_VERTICAL) {
          offset_y = -height;
          canvas->ScaleCoordinates(1, -1);
        }
        if (mask)
          canvas->DrawCanvasWithMask(offset_x, offset_y, target,
                                     offset_x, offset_y, mask);
        else
          canvas->DrawCanvas(offset_x, offset_y, target);

        // Don't destroy canvas cache.
        if (cache_ != target)
          target->Destroy();
        else
          target->PopState();
      }

      canvas->PopState();

#ifdef _DEBUG
      if (debug_mode_ & ViewInterface::DEBUG_ALL) {
        DrawBoundingBox(canvas, width, height, debug_color_index_);
      }

      ++total_draw_count_;
      view_->IncreaseDrawCount();
      if ((total_draw_count_ % 5000) == 0)
        DLOG("BasicElement: %d draws, %d queues, %d%% q/d",
             total_draw_count_, total_queue_draw_count_,
             total_queue_draw_count_ * 100 / total_draw_count_);
#endif
    }

    visibility_changed_ = false;
    size_changed_ = false;
    position_changed_ = false;
    content_changed_ = false;
    draw_queued_ = false;
  }

  void DrawChildren(CanvasInterface *canvas) {
    if (children_)
      children_->Draw(canvas);
  }

#ifdef _DEBUG
  static void DrawBoundingBox(CanvasInterface *canvas,
                              double w, double h,
                              int color_index) {
    Color color(((color_index >> 4) & 3) / 3.5,
                ((color_index >> 2) & 3) / 3.5,
                (color_index & 3) / 3.5);
    canvas->DrawLine(0, 0, 0, h, 1, color);
    canvas->DrawLine(0, 0, w, 0, 1, color);
    canvas->DrawLine(w, h, 0, h, 1, color);
    canvas->DrawLine(w, h, w, 0, 1, color);
    canvas->DrawLine(0, 0, w, h, 1, color);
    canvas->DrawLine(w, 0, 0, h, 1, color);
  }
#endif

 public:
  void AddToClipRegion(const Rectangle *rect) {
    if ((visible_ && opacity_ != 0.0) || visibility_changed_) {
      if (rect) {
        clip_region_.AddRectangle(owner_->GetRectExtentsInView(*rect));
      } else {
        clip_region_.AddRectangle(owner_->GetExtentsInView());
      }
    }
  }

  void AggregateClipRegion(const Rectangle &boundary, ClipRegion *region) {
    if (region != NULL && !boundary.IsEmpty()) {
      size_t count = clip_region_.GetRectangleCount();
      for (size_t i = 0; i < count; ++i) {
        Rectangle rect = clip_region_.GetRectangle(i);
        if (rect.Intersect(boundary)) {
          rect.Integerize(true);
          region->AddRectangle(rect);
        }
      }

      if (visible_ && opacity_ != 0.0) {
        Rectangle extents = owner_->GetExtentsInView();
        if (extents.Intersect(boundary)) {
          if (children_ && children_->GetCount()) {
            children_->AggregateClipRegion(extents, region);
          }
          owner_->AggregateMoreClipRegion(extents, region);
          // Short circuit.
          clip_region_.Clear();
          return;
        }
      }
    }

    clip_region_.Clear();
    // children's AggregateClipRegion() and AggregateMoreClipRegion() must be
    // called in order to let children clear their clip region cache.
    if (children_ && children_->GetCount()) {
      children_->AggregateClipRegion(Rectangle(), NULL);
    }
    owner_->AggregateMoreClipRegion(Rectangle(), NULL);
  }

  void QueueDraw() {
    if ((visible_ || visibility_changed_) && !draw_queued_) {
      draw_queued_ = true;
      AddToClipRegion(NULL);
      view_->QueueDraw();
      if (!content_changed_)
        MarkContentChanged();
    }
#ifdef _DEBUG
    ++total_queue_draw_count_;
#endif
  }

  void QueueDrawRect(const Rectangle &rect) {
    if ((visible_ || visibility_changed_) && !draw_queued_) {
      // Don't set draw_queued_, because it's only queued part of the element.
      // Other part might be queued later.
      AddToClipRegion(&rect);
      view_->QueueDraw();
      if (!content_changed_)
        MarkContentChanged();
    }
#ifdef _DEBUG
    ++total_queue_draw_count_;
#endif
  }

  void QueueDrawRegion(const ClipRegion &region) {
    if ((visible_ || visibility_changed_) && !draw_queued_) {
      // Don't set draw_queued_, because it's only queued part of the element.
      // Other part might be queued later.
      size_t count = region.GetRectangleCount();
      for (size_t i = 0; i < count; ++i) {
        Rectangle rect = region.GetRectangle(i);
        AddToClipRegion(&rect);
      }
      view_->QueueDraw();
      if (!content_changed_)
        MarkContentChanged();
    }
#ifdef _DEBUG
    ++total_queue_draw_count_;
#endif
  }

  void MarkContentChanged() {
    content_changed_ = true;
    BasicElement *elm = owner_->GetParentElement();
    for (; elm; elm = elm->GetParentElement())
      elm->impl_->content_changed_ = true;
  }

  void FireOnContentChangedSignal() {
    if (on_content_changed_signal_.HasActiveConnections())
      on_content_changed_signal_();
  }

  void PostSizeEvent() {
    if (onsize_event_.HasActiveConnections())
      view_->PostElementSizeEvent(owner_, onsize_event_);
  }

  void PositionChanged() {
    position_changed_ = true;
    draw_queued_ = false;
    QueueDraw();
  }

  void WidthChanged() {
    size_changed_ = true;
    draw_queued_ = false;
    QueueDraw();
  }

  void HeightChanged() {
    size_changed_ = true;
    draw_queued_ = false;
    QueueDraw();
  }

  void MarkRedraw() {
    if (children_)
      children_->MarkRedraw();

    // Invalidates canvas cache, since the zoom factory might be changed.
    if (cache_) {
      DestroyCanvas(cache_);
      cache_ = NULL;
    }

    // To make sure that this element can be redrew correctly.
    QueueDraw();
  }

  // clip: whether the visible status effected by clipping. If clip is true
  // and the element is out of parent's visible area, this method returns
  // false.
  bool IsReallyVisible(bool clip) {
    return visible_ && opacity_ != 0.0 && width_ > 0 && height_ > 0 &&
        (!parent_ ||
         (parent_->impl_->IsReallyVisible(clip) &&
          (!clip || parent_->IsChildInVisibleArea(owner_))));
  }

  ViewInterface::HitTest GetHitTest() {
    return hittest_;
  }

  ScriptableInterface *GetScriptableParent() {
    return parent_ ? parent_ : view_->GetScriptable();
  }

 public:
  EventResult OnMouseEvent(const MouseEvent &event, bool direct,
                           BasicElement **fired_element,
                           BasicElement **in_element,
                           ViewInterface::HitTest *hittest) {
    Event::Type type = event.GetType();
    ElementHolder this_element_holder(owner_);

    *fired_element = *in_element = NULL;

    // Always process direct messages because the sender wants this element
    // to process.
    // Test visible_ and opacity_ first, to prevent from calling GetHitTest()
    // every time.
    if (!direct && (!visible_ || opacity_ == 0))
      return EVENT_RESULT_UNHANDLED;

    ViewInterface::HitTest this_hittest =
        owner_->GetHitTest(event.GetX(), event.GetY());

    // GetHitTest() might be overrode.
    // FIXME: Verify if the hittest logic is correct.
    if (!direct && this_hittest == ViewInterface::HT_TRANSPARENT)
      return EVENT_RESULT_UNHANDLED;

    if (!direct && children_) {
      // Send to the children first.
      EventResult result = children_->OnMouseEvent(event, fired_element,
                                                   in_element, hittest);
      if (!this_element_holder.Get() || *fired_element)
        return result;
    }

    if (!*in_element) {
      *in_element = owner_;
      *hittest = this_hittest;
    }

    // Direct EVENT_MOUSE_UP and EVENT_MOUSE_OUT must be processed even if the
    // element is disabled.
    if (!enabled_ && !(direct && (type == Event::EVENT_MOUSE_UP ||
                                  type == Event::EVENT_MOUSE_OUT))) {
      return EVENT_RESULT_UNHANDLED;
    }

    // Don't check mouse position, because the event may be out of this
    // element when this element is grabbing mouse.

    // Take this event, since no children took it, and we're enabled.
    ScriptableEvent scriptable_event(&event, owner_, NULL);
#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    if (type != Event::EVENT_MOUSE_MOVE) {
      DLOG("%s(%s|%s): x:%g y:%g dx:%d dy:%d b:%d m:%d",
           scriptable_event.GetName(),
           name_.c_str(), tag_name_,
           event.GetX(), event.GetY(),
           event.GetWheelDeltaX(), event.GetWheelDeltaY(),
           event.GetButton(), event.GetModifier());
    }
#endif

    ElementHolder in_element_holder(*in_element);
    switch (type) {
      case Event::EVENT_MOUSE_MOVE: // put the high volume events near top
        view_->FireEvent(&scriptable_event, onmousemove_event_);
        break;
      case Event::EVENT_MOUSE_DOWN: {
        ElementHolder self_holder(owner_);
        view_->SetFocus(owner_);
        if (!self_holder.Get()) {
          // In case that the element is deleted in focus event handlers.
          *fired_element = *in_element = NULL;
          return EVENT_RESULT_UNHANDLED;
        }
        view_->FireEvent(&scriptable_event, onmousedown_event_);
        break;
      }
      case Event::EVENT_MOUSE_UP:
        view_->FireEvent(&scriptable_event, onmouseup_event_);
        break;
      case Event::EVENT_MOUSE_CLICK:
        view_->FireEvent(&scriptable_event, onclick_event_);
        break;
      case Event::EVENT_MOUSE_DBLCLICK:
        view_->FireEvent(&scriptable_event, ondblclick_event_);
        break;
      case Event::EVENT_MOUSE_RCLICK:
        view_->FireEvent(&scriptable_event, onrclick_event_);
        break;
      case Event::EVENT_MOUSE_RDBLCLICK:
        view_->FireEvent(&scriptable_event, onrdblclick_event_);
        break;
      case Event::EVENT_MOUSE_OUT:
        view_->FireEvent(&scriptable_event, onmouseout_event_);
        break;
      case Event::EVENT_MOUSE_OVER:
        view_->FireEvent(&scriptable_event, onmouseover_event_);
        break;
      case Event::EVENT_MOUSE_WHEEL:
        view_->FireEvent(&scriptable_event, onmousewheel_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleMouseEvent(event));
    *fired_element = this_element_holder.Get();
    *in_element = in_element_holder.Get();
    // No need to reset hittest even if in_element get destroyed. In this case,
    // hittest value is undefined.
    return result;
  }

  EventResult OnDragEvent(const DragEvent &event, bool direct,
                          BasicElement **fired_element) {
    ElementHolder this_element_holder(owner_);

    *fired_element = NULL;

    // Always process direct messages because the sender wants this element
    // to process.
    // GetHitTest() might be overrode.
    // FIXME: Verify if the hittest logic is correct.
    if (!direct && (!visible_ || opacity_ == 0 ||
         owner_->GetHitTest(event.GetX(), event.GetY()) ==
           ViewInterface::HT_TRANSPARENT)) {
      return EVENT_RESULT_UNHANDLED;
    }

    if (!direct && children_) {
      // Send to the children first.
      EventResult result = children_->OnDragEvent(event, fired_element);
      if (!this_element_holder.Get() || *fired_element)
        return result;
    }

    if (!owner_->IsDropTarget())
      return EVENT_RESULT_UNHANDLED;

    // Take this event, since no children took it, and we're enabled.
    ScriptableEvent scriptable_event(&event, owner_, NULL);
#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    if (event.GetType() != Event::EVENT_DRAG_MOTION)
      DLOG("%s(%s|%s): %g %g text=%s", scriptable_event.GetName(),
           name_.c_str(), tag_name_,
           event.GetX(), event.GetY(), event.GetDragText());
#endif

    switch (event.GetType()) {
      case Event::EVENT_DRAG_MOTION: // put the high volume events near top
        // This event is only for drop target testing.
        break;
      case Event::EVENT_DRAG_OUT:
        view_->FireEvent(&scriptable_event, ondragout_event_);
        break;
      case Event::EVENT_DRAG_OVER:
        view_->FireEvent(&scriptable_event, ondragover_event_);
        break;
      case Event::EVENT_DRAG_DROP:
        view_->FireEvent(&scriptable_event, ondragdrop_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    // For Drag events, we only return EVENT_RESULT_UNHANDLED if this element
    // is invisible or not a drag target. Some gadgets rely on this behavior.
    if (result == EVENT_RESULT_UNHANDLED)
      result = EVENT_RESULT_HANDLED;
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleDragEvent(event));
    *fired_element = this_element_holder.Get();
    return result;
  }

  EventResult OnKeyEvent(const KeyboardEvent &event) {
    if (!enabled_)
      return EVENT_RESULT_UNHANDLED;

    ElementHolder this_element_holder(owner_);
    ScriptableEvent scriptable_event(&event, owner_, NULL);
#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    DLOG("%s(%s|%s): %d", scriptable_event.GetName(),
         name_.c_str(), tag_name_, event.GetKeyCode());
#endif

    switch (event.GetType()) {
      case Event::EVENT_KEY_DOWN:
        view_->FireEvent(&scriptable_event, onkeydown_event_);
        break;
      case Event::EVENT_KEY_UP:
        view_->FireEvent(&scriptable_event, onkeyup_event_);
        break;
      case Event::EVENT_KEY_PRESS:
        view_->FireEvent(&scriptable_event, onkeypress_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleKeyEvent(event));
    return result;
  }

  EventResult OnOtherEvent(const Event &event) {
    ElementHolder this_element_holder(owner_);
    ScriptableEvent scriptable_event(&event, owner_, NULL);
#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    DLOG("%s(%s|%s)", scriptable_event.GetName(),
         name_.c_str(), tag_name_);
#endif

    switch (event.GetType()) {
      case Event::EVENT_FOCUS_IN:
        if (!enabled_) return EVENT_RESULT_UNHANDLED;
        if (parent_) {
          double left, top, right, bottom;
          GetChildRectExtentInParent(x_, y_, pin_x_, pin_y_,
                                     DegreesToRadians(rotation_),
                                     0, 0, width_, height_,
                                     &left, &top, &right, &bottom);
          parent_->EnsureAreaVisible(
              Rectangle(left, top, right - left, bottom - top), owner_);
        }
        view_->FireEvent(&scriptable_event, onfocusin_event_);
        if (owner_->IsShowFocusOverlay()) {
          QueueDraw();
        }
        break;
      case Event::EVENT_FOCUS_OUT:
        view_->FireEvent(&scriptable_event, onfocusout_event_);
        if (IsShowFocusOverlay()) {
          QueueDraw();
        }
        break;
      default:
        ASSERT(false);
    }
    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleOtherEvent(event));
    return result;
  }

  bool IsShowFocusOverlay() {
    return (!show_focus_overlay_set_ && focus_overlay_) || show_focus_overlay_;
  }

  void SetShowFocusOverlay(bool show_focus_overlay) {
    if (!show_focus_overlay_set_ ||
        show_focus_overlay_ != show_focus_overlay) {
      show_focus_overlay_set_ = true;
      show_focus_overlay_ = show_focus_overlay;
      if (view_->IsFocused() && view_->GetFocusedElement() == owner_) {
        QueueDraw();
      }
    }
  }

 public:
  BasicElement *parent_;
  BasicElement *owner_;
  Elements *children_;
  View *view_;
  ImageInterface *mask_image_;
  ImageInterface *focus_overlay_;
  CanvasInterface *cache_;
  const char *tag_name_;
  size_t index_;

  double width_, height_, pwidth_, pheight_;
  double min_width_, min_height_;
  double x_, y_, px_, py_;
  double pin_x_, pin_y_, ppin_x_, ppin_y_;
  double rotation_;
  double opacity_;

  std::string name_;
  std::string tooltip_;

  ClipRegion clip_region_;

  EventSignal onclick_event_;
  EventSignal ondblclick_event_;
  EventSignal onrclick_event_;
  EventSignal onrdblclick_event_;
  EventSignal ondragdrop_event_;
  EventSignal ondragout_event_;
  EventSignal ondragover_event_;
  EventSignal onfocusin_event_;
  EventSignal onfocusout_event_;
  EventSignal onkeydown_event_;
  EventSignal onkeypress_event_;
  EventSignal onkeyup_event_;
  EventSignal onmousedown_event_;
  EventSignal onmousemove_event_;
  EventSignal onmouseout_event_;
  EventSignal onmouseover_event_;
  EventSignal onmouseup_event_;
  EventSignal onmousewheel_event_;
  EventSignal onsize_event_;
  EventSignal oncontextmenu_event_;
  EventSignal on_content_changed_signal_;

#ifdef _DEBUG
  int debug_color_index_;
  static int total_debug_color_index_;
  int debug_mode_;

  static int total_draw_count_;
  static int total_queue_draw_count_;

  static LightMap<uint64_t, bool> class_has_children_;
#endif
#if defined(OS_WIN)
  // MSVC treats enum bit fields as signed! That means if a 2-bit enum bit field
  // is assigned an enum value with integer values of 3, when the field is read
  // later, the result will be -1 instead of 3, because the first bit is
  // considered as sign bit. So surprisingly the result isn't equal to the
  // original enum value.
  ViewInterface::HitTest hittest_;
  ViewInterface::CursorType cursor_;
  FlipMode flip_;
  TextDirection text_direction_;
#elif defined(OS_POSIX)
  ViewInterface::HitTest hittest_   : 6;
  ViewInterface::CursorType cursor_ : 4;
  FlipMode flip_                    : 2;
  TextDirection text_direction_     : 2;
#endif
  bool drop_target_             : 1;
  bool enabled_                 : 1;
  bool width_relative_          : 1;
  bool height_relative_         : 1;
  bool width_specified_         : 1;
  bool height_specified_        : 1;
  bool x_relative_              : 1;
  bool y_relative_              : 1;
  bool x_specified_             : 1;
  bool y_specified_             : 1;
  bool pin_x_relative_          : 1;
  bool pin_y_relative_          : 1;
  bool visible_                 : 1;
  bool visibility_changed_      : 1;
  bool position_changed_        : 1;
  bool size_changed_            : 1;
  bool cache_enabled_           : 1;
  bool content_changed_         : 1;
  bool draw_queued_             : 1;
  bool designer_mode_           : 1;
  bool show_focus_overlay_      : 1;
  bool show_focus_overlay_set_  : 1;
  bool tab_stop_                : 1;
  bool tab_stop_set_            : 1;
};

#ifdef _DEBUG
int BasicElement::Impl::total_debug_color_index_ = 0;
int BasicElement::Impl::total_draw_count_ = 0;
int BasicElement::Impl::total_queue_draw_count_ = 0;
LightMap<uint64_t, bool> BasicElement::Impl::class_has_children_;
#endif

// Must sync with ViewInterface::CursorType enumerates
// defined in view_interface.h
static const char *kCursorTypeNames[] = {
  "default", "arrow", "ibeam", "wait", "cross", "uparrow",
  "size", "sizenwse", "sizenesw", "sizewe", "sizens", "sizeall",
  "no", "hand", "busy", "help",
};

// Must sync with ViewInterface::HitTest enumerates
// defined in view_interface.h
static const char *kHitTestNames[] = {
  "httransparent", "htnowhere", "htclient", "htcaption", "htsysmenu",
  "htsize", "htmenu", "hthscroll", "htvscroll", "htminbutton", "htmaxbutton",
  "htleft", "htright", "httop", "httopleft", "httopright",
  "htbottom", "htbottomleft", "htbottomright", "htborder",
  "htobject", "htclose", "hthelp",
};

static const char *kFlipNames[] = {
  "none", "horizontal", "vertical", "both",
};

static const char *kTextDirectionNames[] = {
  "inheritfromview", "inheritfromparent", "ltr", "rtl",
};

BasicElement::BasicElement(View *view, const char *tag_name, const char *name,
                           bool allow_children)
    : impl_(new Impl(view, tag_name, name, allow_children, this)) {
}

void BasicElement::DoRegister() {
#ifdef _DEBUG
  // Objects of a class must be all or none allowing children.
  bool has_children = GetChildren() != NULL;
  ASSERT_M(has_children == Impl::class_has_children_[GetClassId()],
           ("Objects of class %jx aren't consistent about children",
            GetClassId()));
#endif
}

static Elements *GetElementChildren(BasicElement *element) {
  return element->GetChildren();
}

void BasicElement::DoClassRegister() {
  bool has_children = GetChildren() != NULL;
#ifdef _DEBUG
  Impl::class_has_children_[GetClassId()] = has_children;
#endif
  if (has_children) {
    RegisterProperty("children",
                     // Select the non-const version.
                     NewSlot(static_cast<Elements *(BasicElement::*)()>(
                         &BasicElement::GetChildren)), NULL);
    RegisterMethod("appendElement", NewSlot(&Elements::AppendElementVariant,
                                            GetElementChildren));
    // insertElement was deprecated by insertElementBehind.
    RegisterMethod("insertElement", NewSlot(&Elements::InsertElementVariant,
                                            GetElementChildren));
    RegisterMethod("insertElementBehind",
                   NewSlot(&Elements::InsertElementVariant,
                           GetElementChildren));
    // Added in 5.8 API.
    RegisterMethod("insertElementInFrontOf",
                   NewSlot(&Elements::InsertElementVariantAfter,
                           GetElementChildren));
    RegisterMethod("removeElement", NewSlot(&Elements::RemoveElement,
                                            GetElementChildren));
    RegisterMethod("removeAllElements", NewSlot(&Elements::RemoveAllElements,
                                                GetElementChildren));
  }

  RegisterProperty("x",
                   NewSlot(&Impl::GetX, &BasicElement::impl_),
                   NewSlot(&Impl::SetX, &BasicElement::impl_));
  RegisterProperty("y",
                   NewSlot(&Impl::GetY, &BasicElement::impl_),
                   NewSlot(&Impl::SetY, &BasicElement::impl_));
  RegisterProperty("width",
                   NewSlot(&Impl::GetWidth, &BasicElement::impl_),
                   NewSlot(&Impl::SetWidth, &BasicElement::impl_));
  RegisterProperty("height",
                   NewSlot(&Impl::GetHeight, &BasicElement::impl_),
                   NewSlot(&Impl::SetHeight, &BasicElement::impl_));
  RegisterProperty("minWidth",
                   NewSlot(&BasicElement::GetMinWidth),
                   NewSlot(&BasicElement::SetMinWidth));
  RegisterProperty("minHeight",
                   NewSlot(&BasicElement::GetMinHeight),
                   NewSlot(&BasicElement::SetMinHeight));
  RegisterProperty("name", NewSlot(&BasicElement::GetName), NULL);
  RegisterProperty("tagName", NewSlot(&BasicElement::GetTagName), NULL);

  // TODO: Contentarea on Windows seems to support some of the following
  // properties.
  // if (GadgetStrCmp(tag_name, "contentarea") == 0)
    // The following properties and methods are not supported by contentarea.
  //  return;

  RegisterStringEnumProperty("cursor",
                             NewSlot(&BasicElement::GetCursor),
                             NewSlot(&BasicElement::SetCursor),
                             kCursorTypeNames, arraysize(kCursorTypeNames));
  RegisterProperty("dropTarget",
                   NewSlot(&BasicElement::IsDropTarget),
                   NewSlot(&BasicElement::SetDropTarget));
  RegisterProperty("enabled",
                   NewSlot(&BasicElement::IsEnabled),
                   NewSlot(&BasicElement::SetEnabled));
  RegisterStringEnumProperty("hitTest",
                             NewSlot(&Impl::GetHitTest, &BasicElement::impl_),
                             NewSlot(&BasicElement::SetHitTest),
                             kHitTestNames, arraysize(kHitTestNames));
  RegisterProperty("mask",
                   NewSlot(&BasicElement::GetMask),
                   NewSlot(&BasicElement::SetMask));
  RegisterProperty("offsetHeight",
                   NewSlot(&BasicElement::GetPixelHeight), NULL);
  RegisterProperty("offsetWidth",
                   NewSlot(&BasicElement::GetPixelWidth), NULL);
  RegisterProperty("offsetX", NewSlot(&BasicElement::GetPixelX), NULL);
  RegisterProperty("offsetY", NewSlot(&BasicElement::GetPixelY), NULL);
  RegisterProperty("opacity",
                   NewSlot(&Impl::GetIntOpacity, &BasicElement::impl_),
                   NewSlot(&Impl::SetIntOpacity, &BasicElement::impl_));
  RegisterProperty("parentElement",
                   NewSlot(&Impl::GetScriptableParent, &BasicElement::impl_),
                   NULL);

  // Note: don't use relative pinX/pinY until they are in the public API.
  RegisterProperty("pinX",
                   NewSlot(&Impl::GetPinX, &BasicElement::impl_),
                   NewSlot(&Impl::SetPinX, &BasicElement::impl_));
  RegisterProperty("pinY",
                   NewSlot(&Impl::GetPinY, &BasicElement::impl_),
                   NewSlot(&Impl::SetPinY, &BasicElement::impl_));
  RegisterProperty("rotation",
                   NewSlot(&BasicElement::GetRotation),
                   NewSlot(&BasicElement::SetRotation));
  RegisterProperty("tooltip",
                   NewSlot(&BasicElement::GetTooltip),
                   NewSlot(&BasicElement::SetTooltip));
  RegisterProperty("visible",
                   NewSlot(&BasicElement::IsVisible),
                   NewSlot(&BasicElement::SetVisible));

  RegisterStringEnumProperty("textDirection",
                             NewSlot(&BasicElement::GetTextDirection),
                             NewSlot(&BasicElement::SetTextDirection),
                             kTextDirectionNames,
                             arraysize(kTextDirectionNames));
  // Note: don't use 'flip' property until it is in the public API.
  RegisterStringEnumProperty("flip",
                             NewSlot(&BasicElement::GetFlip),
                             NewSlot(&BasicElement::SetFlip),
                             kFlipNames, arraysize(kFlipNames));
  // Note: don't use 'index' property until it is in the public API.
  RegisterProperty("index", NewSlot(&BasicElement::GetIndex), NULL);
  // Note: don't use 'showFocusOverlay' property until it is in the public API.
  RegisterProperty("showFocusOverlay",
                   NewSlot(&BasicElement::IsShowFocusOverlay),
                   NewSlot(&BasicElement::SetShowFocusOverlay));
  // Note: don't use 'focusOverlay' property until it is in the public API.
  RegisterProperty("focusOverlay",
                   NewSlot(&BasicElement::GetFocusOverlay),
                   NewSlot(&BasicElement::SetFocusOverlay));
  // Note: don't use 'tabStop' property until it is in the public API.
  RegisterProperty("tabStop",
                   NewSlot(&BasicElement::IsTabStop),
                   NewSlot(&BasicElement::SetTabStop));

  RegisterMethod("focus", NewSlot(&BasicElement::Focus));
  RegisterMethod("killFocus", NewSlot(&BasicElement::KillFocus));
  RegisterMethod("showTooltip", NewSlot(&BasicElement::ShowTooltip));

  RegisterClassSignal(kOnClickEvent, &Impl::onclick_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnDblClickEvent, &Impl::ondblclick_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnRClickEvent, &Impl::onrclick_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnRDblClickEvent, &Impl::onrdblclick_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnDragDropEvent, &Impl::ondragdrop_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnDragOutEvent, &Impl::ondragout_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnDragOverEvent, &Impl::ondragover_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnFocusInEvent, &Impl::onfocusin_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnFocusOutEvent, &Impl::onfocusout_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnKeyDownEvent, &Impl::onkeydown_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnKeyPressEvent, &Impl::onkeypress_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnKeyUpEvent, &Impl::onkeyup_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnMouseDownEvent, &Impl::onmousedown_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnMouseMoveEvent, &Impl::onmousemove_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnMouseOutEvent, &Impl::onmouseout_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnMouseOverEvent, &Impl::onmouseover_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnMouseUpEvent, &Impl::onmouseup_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnMouseWheelEvent, &Impl::onmousewheel_event_,
                      &BasicElement::impl_);
  RegisterClassSignal(kOnSizeEvent, &Impl::onsize_event_,
                      &BasicElement::impl_);
  // Not a standard signal yet.
  RegisterClassSignal(kOnContextMenuEvent, &Impl::oncontextmenu_event_,
                      &BasicElement::impl_);
}

BasicElement::~BasicElement() {
  delete impl_;
}

const char *BasicElement::GetTagName() const {
  return impl_->tag_name_;
}

View *BasicElement::GetView() {
  return impl_->view_;
}

const View *BasicElement::GetView() const {
  return impl_->view_;
}

ViewInterface::HitTest BasicElement::GetHitTest(double x, double y) const {
  if (IsPointIn(x, y))
    return impl_->hittest_;
  else
    return ViewInterface::HT_TRANSPARENT;
}

void BasicElement::SetHitTest(ViewInterface::HitTest value) {
  impl_->hittest_ = value;
  // FIXME: Verify if the hittest logic is correct.
  if (value != ViewInterface::HT_CLIENT)
    impl_->enabled_ = false;
}

const Elements *BasicElement::GetChildren() const {
  return impl_->children_;
}

Elements *BasicElement::GetChildren() {
  return impl_->children_;
}

ViewInterface::CursorType BasicElement::GetCursor() const {
  return impl_->cursor_;
}

void BasicElement::SetCursor(ViewInterface::CursorType cursor) {
  impl_->cursor_ = cursor;
}

bool BasicElement::IsDropTarget() const {
  GadgetInterface *gadget = GetView()->GetGadget();
  // If gadget is NULL then means that the view is not used in a gadget
  // (eg. in unittest), so it's ok to grant the permission.
  const Permissions *permissions = gadget ? gadget->GetPermissions() : NULL;
  if (permissions && permissions->IsRequiredAndGranted(Permissions::FILE_READ))
    return impl_->drop_target_;
  else
    LOG("No permission to use basicElement.dropTarget.");
  return false;
}

void BasicElement::SetDropTarget(bool drop_target) {
  GadgetInterface *gadget = GetView()->GetGadget();
  // If gadget is NULL then means that the view is not used in a gadget
  // (eg. in unittest), so it's ok to grant the permission.
  const Permissions *permissions = gadget ? gadget->GetPermissions() : NULL;
  if (permissions && permissions->IsRequiredAndGranted(Permissions::FILE_READ))
    impl_->drop_target_ = drop_target;
  else
    LOG("No permission to use basicElement.dropTarget.");
}

bool BasicElement::IsEnabled() const {
  return impl_->enabled_;
}

void BasicElement::SetEnabled(bool enabled) {
  if (impl_->enabled_ != enabled) {
    impl_->enabled_ = enabled;
    QueueDraw();
  }
}

std::string BasicElement::GetName() const {
  return impl_->name_;
}

Variant BasicElement::GetMask() const {
  return Variant(GetImageTag(impl_->mask_image_));
}

void BasicElement::SetMask(const Variant &mask) {
  if (mask != GetMask()) {
    impl_->SetMask(mask);
  }
}

const CanvasInterface *BasicElement::GetMaskCanvas() const {
  return impl_->GetMaskCanvas();
}

double BasicElement::GetPixelWidth() const {
  return impl_->width_;
}

void BasicElement::SetPixelWidth(double width) {
  impl_->width_specified_ = true;
  impl_->SetPixelWidth(width);
}

double BasicElement::GetPixelHeight() const {
  return impl_->height_;
}

void BasicElement::SetPixelHeight(double height) {
  impl_->height_specified_ = true;
  impl_->SetPixelHeight(height);
}

double BasicElement::GetRelativeWidth() const {
  return impl_->pwidth_;
}

double BasicElement::GetRelativeHeight() const {
  return impl_->pheight_;
}

double BasicElement::GetMinWidth() const {
  return impl_->min_width_;
}

void BasicElement::SetMinWidth(double min_width) {
  if (impl_->min_width_ != min_width) {
    impl_->min_width_ = std::max(0.0, min_width);
    impl_->WidthChanged();
  }
}

double BasicElement::GetMinHeight() const {
  return impl_->min_height_;
}

void BasicElement::SetMinHeight(double min_height) {
  if (impl_->min_height_ != min_height) {
    impl_->min_height_ = std::max(0.0, min_height);
    impl_->HeightChanged();
  }
}

double BasicElement::GetPixelX() const {
  return impl_->x_;
}

void BasicElement::SetPixelX(double x) {
  impl_->x_specified_ = true;
  impl_->SetPixelX(x);
}

double BasicElement::GetPixelY() const {
  return impl_->y_;
}

void BasicElement::SetPixelY(double y) {
  impl_->y_specified_ = true;
  impl_->SetPixelY(y);
}

double BasicElement::GetRelativeX() const {
  return impl_->px_;
}

void BasicElement::SetRelativeX(double x) {
  impl_->x_specified_ = true;
  impl_->SetRelativeX(x);
}

double BasicElement::GetRelativeY() const {
  return impl_->py_;
}

void BasicElement::SetRelativeY(double y) {
  impl_->y_specified_ = true;
  impl_->SetRelativeY(y);
}

double BasicElement::GetPixelPinX() const {
  return impl_->pin_x_;
}

void BasicElement::SetPixelPinX(double pin_x) {
  impl_->SetPixelPinX(pin_x);
}

double BasicElement::GetPixelPinY() const {
  return impl_->pin_y_;
}

void BasicElement::SetPixelPinY(double pin_y) {
  impl_->SetPixelPinY(pin_y);
}

void BasicElement::SetRelativeWidth(double width) {
  impl_->width_specified_ = true;
  impl_->SetRelativeWidth(width);
}

void BasicElement::SetRelativeHeight(double height) {
  impl_->height_specified_ = true;
  impl_->SetRelativeHeight(height);
}

double BasicElement::GetRelativePinX() const {
  return impl_->ppin_x_;
}

void BasicElement::SetRelativePinX(double x) {
  impl_->SetRelativePinX(x);
}

double BasicElement::GetRelativePinY() const {
  return impl_->ppin_y_;
}

void BasicElement::SetRelativePinY(double y) {
  return impl_->SetRelativePinY(y);
}

bool BasicElement::XIsRelative() const {
  return impl_->x_relative_;
}

bool BasicElement::YIsRelative() const {
  return impl_->y_relative_;
}

bool BasicElement::WidthIsRelative() const {
  return impl_->width_relative_;
}

bool BasicElement::HeightIsRelative() const {
  return impl_->height_relative_;
}

bool BasicElement::PinXIsRelative() const {
  return impl_->pin_x_relative_;
}

bool BasicElement::PinYIsRelative() const {
  return impl_->pin_y_relative_;
}

bool BasicElement::WidthIsSpecified() const {
  return impl_->width_specified_;
}

void BasicElement::ResetWidthToDefault() {
  impl_->ResetWidthToDefault();
}

bool BasicElement::HeightIsSpecified() const {
  return impl_->height_specified_;
}

void BasicElement::ResetHeightToDefault() {
  impl_->ResetHeightToDefault();
}

bool BasicElement::XIsSpecified() const {
  return impl_->x_specified_;
}

void BasicElement::ResetXToDefault() {
  impl_->ResetXToDefault();
}

bool BasicElement::YIsSpecified() const {
  return impl_->y_specified_;
}

void BasicElement::ResetYToDefault() {
  impl_->ResetYToDefault();
}

Variant BasicElement::GetX() const {
  return impl_->GetX();
}

void BasicElement::SetX(const Variant &x) {
  impl_->SetX(x);
}

Variant BasicElement::GetY() const {
  return impl_->GetY();
}

void BasicElement::SetY(const Variant &y) {
  impl_->SetY(y);
}

Variant BasicElement::GetWidth() const {
  return impl_->GetWidth();
}

void BasicElement::SetWidth(const Variant &width) {
  impl_->SetWidth(width);
}

Variant BasicElement::GetHeight() const {
  return impl_->GetHeight();
}

void BasicElement::SetHeight(const Variant &height) {
  impl_->SetHeight(height);
}

Variant BasicElement::GetPinX() const {
  return impl_->GetPinX();
}

void BasicElement::SetPinX(const Variant &pin_x) {
  impl_->SetPinX(pin_x);
}

Variant BasicElement::GetPinY() const {
  return impl_->GetPinY();
}

void BasicElement::SetPinY(const Variant &pin_y) {
  impl_->SetPinY(pin_y);
}

double BasicElement::GetClientWidth() const {
  return GetPixelWidth();
}

double BasicElement::GetClientHeight() const {
  return GetPixelHeight();
}

double BasicElement::GetRotation() const {
  return impl_->rotation_;
}

void BasicElement::SetRotation(double rotation) {
  impl_->SetRotation(rotation);
}

double BasicElement::GetOpacity() const {
  return impl_->opacity_;
}

void BasicElement::SetOpacity(double opacity) {
  if (opacity >= 0.0 && opacity <= 1.0) {
    impl_->SetOpacity(opacity);
  }
}

bool BasicElement::IsVisible() const {
  return impl_->visible_;
}

void BasicElement::SetVisible(bool visible) {
  impl_->SetVisible(visible);
}

bool BasicElement::IsReallyVisible() const {
  return impl_->IsReallyVisible(true);
}

bool BasicElement::IsReallyEnabled() const {
  return impl_->enabled_ && impl_->IsReallyVisible(false);
}

bool BasicElement::IsFullyOpaque() const {
  if (!HasOpaqueBackground() || impl_->mask_image_ != NULL)
    return false;

  double opacity = GetOpacity();
  const BasicElement *elm = GetParentElement();
  for (; elm != NULL; elm = elm->GetParentElement())
    opacity *= elm->GetOpacity();

  return opacity == 1.0;
}

BasicElement *BasicElement::GetParentElement() {
  return impl_->parent_;
}

const BasicElement *BasicElement::GetParentElement() const {
  return impl_->parent_;
}

void BasicElement::SetParentElement(BasicElement *parent) {
  impl_->parent_ = parent;
}

size_t BasicElement::GetIndex() const {
  return impl_->index_;
}

void BasicElement::SetIndex(size_t index) {
  impl_->index_ = index;
}

void BasicElement::EnableCanvasCache(bool enable) {
  impl_->cache_enabled_ = enable;
  if (!enable && impl_->cache_) {
    DestroyCanvas(impl_->cache_);
    impl_->cache_ = NULL;
  }
}

bool BasicElement::IsCanvasCacheEnabled() const {
  return impl_->cache_enabled_;
}

std::string BasicElement::GetTooltip() const {
  return impl_->tooltip_;
}

void BasicElement::SetTooltip(const std::string &tooltip) {
  impl_->tooltip_ = tooltip;
}

void BasicElement::ShowTooltip() {
  // Shows tooltip at center of this element.
  impl_->view_->ShowElementTooltipAtPosition(
      this, impl_->width_ / 2, impl_->height_ / 2);
}

BasicElement::FlipMode BasicElement::GetFlip() const {
  return impl_->flip_;
}

void BasicElement::SetFlip(FlipMode flip) {
  if (impl_->flip_ != flip) {
    impl_->flip_ = flip;
    QueueDraw();
  }
}

BasicElement::TextDirection BasicElement::GetTextDirection() const {
  return impl_->text_direction_;
}

void BasicElement::SetTextDirection(TextDirection text_direction) {
  if (impl_->text_direction_ != text_direction) {
    impl_->text_direction_ = text_direction;
    QueueDraw();
  }
}

bool BasicElement::IsTextRTL() const {
  switch (impl_->text_direction_) {
    case TEXT_DIRECTION_INHERIT_FROM_PARENT:
      return GetParentElement() ? GetParentElement()->IsTextRTL() :
                                  GetView()->IsTextRTL();
    case TEXT_DIRECTION_INHERIT_FROM_VIEW:
      return GetView()->IsTextRTL();
    default:
      return impl_->text_direction_ == TEXT_DIRECTION_RIGHT_TO_LEFT;
  }
}

Variant BasicElement::GetFocusOverlay() const {
  return Variant(GetImageTag(impl_->focus_overlay_));
}

void BasicElement::SetFocusOverlay(const Variant &image) {
  if (image != GetFocusOverlay()) {
    impl_->SetFocusOverlay(image);
  }
}

bool BasicElement::IsShowFocusOverlay() const {
  return impl_->IsShowFocusOverlay();
}

void BasicElement::SetShowFocusOverlay(bool show_focus_overlay) {
  impl_->SetShowFocusOverlay(show_focus_overlay);
}

bool BasicElement::IsTabStop() const {
  return impl_->tab_stop_set_ ? impl_->tab_stop_ : IsTabStopDefault();
}

void BasicElement::SetTabStop(bool tab_stop) {
  impl_->tab_stop_ = tab_stop;
  impl_->tab_stop_set_ = true;
}

void BasicElement::Focus() {
  impl_->view_->SetFocus(this);
}

void BasicElement::KillFocus() {
  impl_->view_->SetFocus(NULL);
}

bool BasicElement::IsTabStopDefault() const {
  return false;
}

void BasicElement::RecursiveLayout() {
  impl_->Layout();
}

void BasicElement::Draw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
}

void BasicElement::DrawChildren(CanvasInterface *canvas) {
  impl_->DrawChildren(canvas);
}

void BasicElement::CalculateSize() {
  if (impl_->children_)
    impl_->children_->CalculateSize();
  if (!impl_->width_specified_ || !impl_->height_specified_) {
    double width, height;
    GetDefaultSize(&width, &height);
    if (!impl_->width_specified_)
      impl_->width_ = width;
    if (!impl_->height_specified_)
      impl_->height_ = height;
  }
  if (!impl_->width_relative_ && impl_->width_ < impl_->min_width_)
    impl_->width_ = impl_->min_width_;
  if (!impl_->height_relative_ && impl_->height_ < impl_->min_height_)
    impl_->height_= impl_->min_height_;
}

void BasicElement::BeforeChildrenLayout() {
  // Do nothing.
}

void BasicElement::Layout() {
  // Do nothing.
}

void BasicElement::DoDraw(CanvasInterface * /* canvas */) {
  // Do nothing.
}

void BasicElement::ClearPositionChanged() {
  impl_->position_changed_ = false;
}

bool BasicElement::IsPositionChanged() const {
  return impl_->position_changed_;
}

void BasicElement::ClearSizeChanged() {
  impl_->size_changed_ = false;
}

bool BasicElement::IsSizeChanged() const {
  return impl_->size_changed_;
}

bool BasicElement::SetChildrenScrollable(bool scrollable) {
  if (impl_->children_) {
    impl_->children_->SetScrollable(scrollable);
    return true;
  }
  return false;
}

bool BasicElement::GetChildrenExtents(double *width, double *height) {
  if (impl_->children_) {
    impl_->children_->GetChildrenExtents(width, height);
    return true;
  }
  return false;
}

Rectangle BasicElement::GetExtentsInView() const {
  double r[8];
  SelfCoordToViewCoord(0, 0, &r[0], &r[1]);
  SelfCoordToViewCoord(0, GetPixelHeight(), &r[2], &r[3]);
  SelfCoordToViewCoord(GetPixelWidth(), GetPixelHeight(), &r[4], &r[5]);
  SelfCoordToViewCoord(GetPixelWidth(), 0, &r[6], &r[7]);
  return Rectangle::GetPolygonExtents(4, r);
}

Rectangle BasicElement::GetRectExtentsInView(const Rectangle &rect) const {
  Rectangle tmp(0, 0, GetPixelWidth(), GetPixelHeight());

  // If the rect is not intersected with the element's region, then just clear
  // the result rectangle.
  if (!tmp.Intersect(rect))
    tmp.w = tmp.h = 0;

  double r[8];
  SelfCoordToViewCoord(tmp.x, tmp.y, &r[0], &r[1]);
  SelfCoordToViewCoord(tmp.x, tmp.y + tmp.h, &r[2], &r[3]);
  SelfCoordToViewCoord(tmp.x + tmp.w, tmp.y + tmp.h, &r[4], &r[5]);
  SelfCoordToViewCoord(tmp.x + tmp.w, tmp.y, &r[6], &r[7]);
  return Rectangle::GetPolygonExtents(4, r);
}

Rectangle BasicElement::GetExtentsInParent() const {
  const double width = GetPixelWidth();
  const double height = GetPixelHeight();
  const double pin_x = GetPixelPinX();
  const double pin_y = GetPixelPinY();
  const double radians = DegreesToRadians(GetRotation());
  double left = 0;
  double right = 0;
  double top = 0;
  double bottom = 0;
  GetChildRectExtentInParent(GetPixelX(), GetPixelY(), pin_x, pin_y, radians,
                             0, 0, width, height,
                             &left, &top, &right, &bottom);
  return Rectangle(left, top, right - left, bottom - top);
}

Rectangle BasicElement::GetMinExtentsInParent() const {
  const double width = WidthIsRelative() ? GetMinWidth() : GetPixelWidth();
  const double height = HeightIsRelative() ? GetMinHeight() : GetPixelHeight();
  const double pin_x = PinXIsRelative() ?
      (width * GetRelativePinX()) : GetPixelPinX();
  const double pin_y = PinYIsRelative() ?
      (height * GetRelativePinY()) : GetPixelPinY();
  const double radians = DegreesToRadians(GetRotation());
  double left = 0;
  double right = 0;
  double top = 0;
  double bottom = 0;
  GetChildRectExtentInParent(GetPixelX(), GetPixelY(), pin_x, pin_y, radians,
                             0, 0, width, height,
                             &left, &top, &right, &bottom);
  return Rectangle(left, top, right - left, bottom - top);
}

void BasicElement::QueueDraw() {
  impl_->QueueDraw();
}

void BasicElement::QueueDrawRect(const Rectangle &rect) {
  impl_->QueueDrawRect(rect);
}

void BasicElement::QueueDrawRegion(const ClipRegion &region) {
  impl_->QueueDrawRegion(region);
}

void BasicElement::MarkRedraw() {
  impl_->MarkRedraw();
}

EventResult BasicElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                       BasicElement **fired_element,
                                       BasicElement **in_element,
                                       ViewInterface::HitTest *hittest) {
  return impl_->OnMouseEvent(event, direct, fired_element, in_element, hittest);
}

EventResult BasicElement::OnDragEvent(const DragEvent &event, bool direct,
                                      BasicElement **fired_element) {
  return impl_->OnDragEvent(event, direct, fired_element);
}

EventResult BasicElement::OnKeyEvent(const KeyboardEvent &event) {
  return impl_->OnKeyEvent(event);
}

EventResult BasicElement::OnOtherEvent(const Event &event) {
  return impl_->OnOtherEvent(event);
}

bool BasicElement::IsPointIn(double x, double y) const {
  if (!IsPointInElement(x, y, impl_->width_, impl_->height_))
    return false;

  const CanvasInterface *mask = GetMaskCanvas();
  if (!mask) return true;

  double opacity;

  // If failed to get the value of the point, then just return false, assuming
  // the point is out of the mask.
  if (!mask->GetPointValue(x, y, NULL, &opacity))
    return false;

  return opacity > 0;
}

void BasicElement::SelfCoordToChildCoord(const BasicElement *child,
                                         double x, double y,
                                         double *child_x,
                                         double *child_y) const {
  ParentCoordToChildCoord(x, y, child->GetPixelX(), child->GetPixelY(),
                          child->GetPixelPinX(), child->GetPixelPinY(),
                          DegreesToRadians(child->GetRotation()),
                          child_x, child_y);
  FlipMode flip = child->GetFlip();
  if (flip & FLIP_HORIZONTAL)
    *child_x = child->GetPixelWidth() - *child_x;
  if (flip & FLIP_VERTICAL)
    *child_y = child->GetPixelHeight() - *child_y;
}

void BasicElement::ChildCoordToSelfCoord(const BasicElement *child,
                                         double x, double y,
                                         double *self_x,
                                         double *self_y) const {
  FlipMode flip = child->GetFlip();
  if (flip & FLIP_HORIZONTAL)
    x = child->GetPixelWidth() - x;
  if (flip & FLIP_VERTICAL)
    y = child->GetPixelHeight() - y;
  ChildCoordToParentCoord(x, y, child->GetPixelX(), child->GetPixelY(),
                          child->GetPixelPinX(), child->GetPixelPinY(),
                          DegreesToRadians(child->GetRotation()),
                          self_x, self_y);
}

void BasicElement::SelfCoordToParentCoord(double x, double y,
                                          double *parent_x,
                                          double *parent_y) const {
  const BasicElement *parent = GetParentElement();
  if (parent) {
    parent->ChildCoordToSelfCoord(this, x, y, parent_x, parent_y);
  } else {
    FlipMode flip = GetFlip();
    if (flip & FLIP_HORIZONTAL)
      x = GetPixelWidth() - x;
    if (flip & FLIP_VERTICAL)
      y = GetPixelHeight() - y;
    ChildCoordToParentCoord(x, y, GetPixelX(), GetPixelY(),
                            GetPixelPinX(), GetPixelPinY(),
                            DegreesToRadians(GetRotation()),
                            parent_x, parent_y);
  }
}

void BasicElement::ParentCoordToSelfCoord(double parent_x, double parent_y,
                                          double *x, double *y) const {
  const BasicElement *parent = GetParentElement();
  if (parent) {
    parent->SelfCoordToChildCoord(this, parent_x, parent_y, x, y);
  } else {
    ParentCoordToChildCoord(parent_x, parent_y,
                            GetPixelX(), GetPixelY(),
                            GetPixelPinX(), GetPixelPinY(),
                            DegreesToRadians(GetRotation()),
                            x, y);
    FlipMode flip = GetFlip();
    if (flip & FLIP_HORIZONTAL)
      *x = GetPixelWidth() - *x;
    if (flip & FLIP_VERTICAL)
      *y = GetPixelHeight() - *y;
  }
}

void BasicElement::SelfCoordToViewCoord(double x, double y,
                                        double *view_x, double *view_y) const {
  const BasicElement *elm = this;
  while (elm) {
    elm->SelfCoordToParentCoord(x, y, &x, &y);
    elm = elm->GetParentElement();
  }
  if (view_x) *view_x = x;
  if (view_y) *view_y = y;
}

void BasicElement::ViewCoordToSelfCoord(double view_x, double view_y,
                                        double *self_x, double *self_y) const {
  std::vector<const BasicElement *> elements;
  for (const BasicElement *e = this; e != NULL; e = e->GetParentElement())
    elements.push_back(e);

  std::vector<const BasicElement *>::reverse_iterator it = elements.rbegin();
  for (; it != elements.rend(); ++it)
    (*it)->ParentCoordToSelfCoord(view_x, view_y, &view_x, &view_y);

  if (self_x) *self_x = view_x;
  if (self_y) *self_y = view_y;
}

void BasicElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width);
  ASSERT(height);
  *width = *height = 0;
}

void BasicElement::GetDefaultPosition(double *x, double *y) const {
  ASSERT(x && y);
  *x = *y = 0;
}

// Returns when input is: pixel: 0; relative: 1; 2: unspecified; invalid: -1.
BasicElement::ParsePixelOrRelativeResult
BasicElement::ParsePixelOrRelative(const Variant &input, double *output) {
  ASSERT(output);
  *output = 0;
  std::string str;

  if (!input.ConvertToString(&str) || str.empty()) {
    // Anything that can't be converted to string is not allowed.
    return PR_UNSPECIFIED;
  } else if (str.find_first_of("xnXN") != std::string::npos) {
    // INFINITY or NAN or hexidecimal floating-point numbers are not allowed.
    // NOTE: This statement relies on the printf output of a NaN or INF double
    // value. If the input is a double nan or inf, then it can be converted to
    // string and the result shall be "nan" or "inf".
    return PR_INVALID;
  } else if (input.ConvertToDouble(output)) {
    // If it can be converted to double directly then treat it as pixel value.
    return PR_PIXEL;
  } else {
    char *end_ptr;
    *output = strtod(str.c_str(), &end_ptr);
    // allows space and multiple % after double number.
    while(*end_ptr == ' ') ++end_ptr;
    bool relative = (*end_ptr == '%');
    while(*end_ptr == '%' || *end_ptr == ' ') ++end_ptr;
    if (relative && *end_ptr == '\0') {
      *output /= 100.0;
      return PR_RELATIVE;
    }
  }
  LOG("Invalid pixel or relative value: %s", input.Print().c_str());
  return PR_INVALID;
}

Variant BasicElement::GetPixelOrRelative(bool is_relative,
                                         bool is_specified,
                                         double pixel,
                                         double relative) {
  if (!is_specified)
    return Variant();

  if (is_relative) {
    // FIXME: Is it necessary to do round here?
    // Answer: to be compatible with Windows version.
    return Variant(StringPrintf("%d%%",
                                static_cast<int>(round(relative * 100))));
  } else {
    // FIXME: Is it necessary to do round here?
    // Answer: to be compatible with Windows version.
    return Variant(static_cast<int>(round(pixel)));
  }
}

void BasicElement::OnPopupOff() {
}

bool BasicElement::OnAddContextMenuItems(MenuInterface *menu) {
  ContextMenuEvent event(new ScriptableMenu(impl_->view_->GetGadget(), menu));
  ScriptableEvent scriptable_event(&event, this, NULL);
  impl_->view_->FireEvent(&scriptable_event, impl_->oncontextmenu_event_);
  return scriptable_event.GetReturnValue() != EVENT_RESULT_CANCELED;
}

bool BasicElement::IsChildInVisibleArea(const BasicElement *child) const {
  double min_x, min_y, max_x, max_y;
  GetChildRectExtentInParent(child->GetPixelX(), child->GetPixelY(),
                             child->GetPixelPinX(), child->GetPixelPinY(),
                             DegreesToRadians(child->GetRotation()),
                             0, 0,
                             child->GetPixelWidth(), child->GetPixelHeight(),
                             &min_x, &min_y, &max_x, &max_y);
  return max_x > 0 && max_y > 0 &&
         min_x < GetPixelWidth() && min_y < GetPixelHeight();
}

bool BasicElement::HasOpaqueBackground() const {
  return false;
}

void BasicElement::PostSizeEvent() {
  impl_->PostSizeEvent();
}

void BasicElement::SetDesignerMode(bool designer_mode) {
  GGL_UNUSED(designer_mode);
  impl_->designer_mode_ = true;
}

bool BasicElement::IsDesignerMode() const {
  return impl_->designer_mode_;
}

Connection *BasicElement::ConnectOnClickEvent(Slot0<void> *handler) {
  return impl_->onclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDblClickEvent(Slot0<void> *handler) {
  return impl_->ondblclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnRClickEvent(Slot0<void> *handler) {
  return impl_->onrclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnRDblClickEvent(Slot0<void> *handler) {
  return impl_->onrdblclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDragDropEvent(Slot0<void> *handler) {
  return impl_->ondragdrop_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDragOutEvent(Slot0<void> *handler) {
  return impl_->ondragout_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDragOverEvent(Slot0<void> *handler) {
  return impl_->ondragover_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnFocusInEvent(Slot0<void> *handler) {
  return impl_->onfocusin_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnFocusOutEvent(Slot0<void> *handler) {
  return impl_->onfocusout_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnKeyDownEvent(Slot0<void> *handler) {
  return impl_->onkeydown_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnKeyPressEvent(Slot0<void> *handler) {
  return impl_->onkeypress_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnKeyUpEvent(Slot0<void> *handler) {
  return impl_->onkeyup_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseDownEvent(Slot0<void> *handler) {
  return impl_->onmousedown_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseMoveEvent(Slot0<void> *handler) {
  return impl_->onmousemove_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseOverEvent(Slot0<void> *handler) {
  return impl_->onmouseover_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseOutEvent(Slot0<void> *handler) {
  return impl_->onmouseout_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseUpEvent(Slot0<void> *handler) {
  return impl_->onmouseup_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseWheelEvent(Slot0<void> *handler) {
  return impl_->onmousewheel_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnSizeEvent(Slot0<void> *handler) {
  return impl_->onsize_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnContextMenuEvent(Slot0<void> *handler) {
  return impl_->oncontextmenu_event_.Connect(handler);
}

Connection *BasicElement::ConnectOnContentChanged(Slot0<void> *handler) {
  return impl_->on_content_changed_signal_.Connect(handler);
}

EventResult BasicElement::HandleMouseEvent(const MouseEvent &event) {
  GGL_UNUSED(event);
  return EVENT_RESULT_UNHANDLED;
}
EventResult BasicElement::HandleDragEvent(const DragEvent &event) {
  GGL_UNUSED(event);
  return EVENT_RESULT_UNHANDLED;
}
EventResult BasicElement::HandleKeyEvent(const KeyboardEvent &event) {
  GGL_UNUSED(event);
  return EVENT_RESULT_UNHANDLED;
}
EventResult BasicElement::HandleOtherEvent(const Event &event) {
  GGL_UNUSED(event);
  return EVENT_RESULT_UNHANDLED;
}

void BasicElement::AggregateClipRegion(const Rectangle &boundary,
                                       ClipRegion *region) {
  impl_->AggregateClipRegion(boundary, region);
}

void BasicElement::AggregateMoreClipRegion(const Rectangle &boundary,
                                           ClipRegion *region) {
  GGL_UNUSED(boundary);
  GGL_UNUSED(region);
}

void BasicElement::EnsureAreaVisible(const Rectangle &rect,
                                     const BasicElement *source) {
  GGL_UNUSED(source);
  // First test if the rectangle is visible in this element.
  if (impl_->parent_ && rect.x + rect.w > 0 && rect.y + rect.h > 0 &&
      rect.x < GetPixelWidth() && rect.y < GetPixelHeight()) {
    double left, top, right, bottom;
    GetChildRectExtentInParent(GetPixelX(), GetPixelY(),
                               GetPixelPinX(), GetPixelPinY(),
                               DegreesToRadians(GetRotation()),
                               rect.x, rect.y,
                               rect.x + rect.w, rect.y + rect.h,
                               &left, &top, &right, &bottom);
    impl_->parent_->EnsureAreaVisible(
        Rectangle(left, top, right - left, bottom - top), this);
  }
}

void BasicElement::CalculateRelativeAttributes() {
  impl_->CalculateRelativeAttributes();
}

} // namespace ggadget
