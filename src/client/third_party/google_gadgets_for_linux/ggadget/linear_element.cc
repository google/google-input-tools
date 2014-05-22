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

#include "linear_element.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "elements.h"
#include "logger.h"
#include "view.h"
#include "small_object.h"
#include "string_utils.h"

namespace {

static const char *kOrientationNames[] = {
  "horizontal", "vertical"
};

static const char *kHorizontalOrders[] = {
  "ltr", "rtl"
};

static const char *kLayoutDirections[] = {
  "forward", "backward",
};

static const char kLayoutDirectionProperty[] = "linearLayoutDir";
static const char kAutoStretchProperty[] = "autoStretch";

}  // anonymous namespace

namespace ggadget {

class LinearElement::Impl : public SmallObject<> {
 public:
  explicit Impl(LinearElement *owner)
      : owner_(owner),
        on_element_added_connection_(NULL),
        on_element_removed_connection_(NULL),
        padding_(0),
        min_width_(0),
        min_height_(0),
        orientation_(ORIENTATION_HORIZONTAL),
        h_auto_sizing_(false),
        v_auto_sizing_(false) {
    on_element_added_connection_ =
        owner_->GetChildren()->ConnectOnElementAdded(
            NewSlot(this, &Impl::OnElementAdded));
    on_element_removed_connection_ =
        owner_->GetChildren()->ConnectOnElementRemoved(
            NewSlot(this, &Impl::OnElementRemoved));
  }

  ~Impl() {
    on_element_added_connection_->Disconnect();
    on_element_removed_connection_->Disconnect();
  }

  Variant GetWidth() const {
    return h_auto_sizing_ ? Variant("auto") : owner_->GetWidth();
  }

  void SetWidth(const Variant &width) {
    if (width.type() == Variant::TYPE_STRING &&
        VariantValue<std::string>()(width) == "auto") {
      owner_->SetHorizontalAutoSizing(true);
    } else {
      h_auto_sizing_ = false;
      owner_->SetWidth(width);
    }
  }

  Variant GetHeight() const {
    return v_auto_sizing_ ? Variant("auto") : owner_->GetHeight();
  }

  void SetHeight(const Variant &height) {
    if (height.type() == Variant::TYPE_STRING &&
        VariantValue<std::string>()(height) == "auto") {
      owner_->SetVerticalAutoSizing(true);
    } else {
      v_auto_sizing_ = false;
      owner_->SetHeight(height);
    }
  }

  // Calculate the linear element's desired size based on its children's size.
  // Taking horizontal orientation as example, the width and height are
  // calculated by following rules:
  // - width
  //   Sum of all children's width and paddings between them. If some of the
  //   children have relative width, and the total relative width is less than
  //   100%, then the sum of all children's width is calculated by:
  //   (sum of children's pixel width) / (1 - sum of children's relative width)
  // - height
  //   Maximum vertical extent of all children. Children with relative height
  //   will be ignored. If a child has relative Y, then its vertical extent is
  //   calculated according to its height, pinY and PixelY to make sure all its
  //   content is inside the linear element's area.
  //
  // This method must be called after calling BasicElement::CalculateSize();
  // Note: this method cannot handle rotated children with relative size. If a
  // child must be rotated, then it must have pixel size.
  void CalculateSize(double *width, double *height) {
    Elements *children = owner_->GetChildren();
    const size_t count = children->GetCount();
    double total_children_relative_size = 0;
    double total_children_pixel_size = 0;
    double total_children_pixel_size_with_relative_min = 0;
    double max_size_required_by_relative_children = 0;
    double max_child_extent = 0;
    for (size_t i = 0; i < count; ++i) {
      BasicElement *child = children->GetItemByIndex(i);

      // Invisible children do not occupy space.
      if (!child->IsVisible())
        continue;

      const double child_x = child->GetPixelX();
      const double child_y = child->GetPixelY();
      const Rectangle child_rect = child->GetMinExtentsInParent();
      const bool is_auto_strecth = IsChildAutoStretch(child);
      if (orientation_ == ORIENTATION_HORIZONTAL) {
        if (!is_auto_strecth && child->WidthIsRelative()) {
          const double relative_width = child->GetRelativeWidth();
          total_children_relative_size += relative_width;
          if (child_rect.w > 0 && relative_width > 0) {
            max_size_required_by_relative_children = std::max(
                child_rect.w / relative_width,
                max_size_required_by_relative_children);
          }
        } else {
          total_children_pixel_size += child_rect.w;
        }

        total_children_pixel_size_with_relative_min += child_rect.w;

        const double child_bottom = child_rect.y + child_rect.h;
        if (child->YIsRelative()) {
          double child_extent = 0;
          const double relative_y = child->GetRelativeY();
          if (relative_y == 0) {
            child_extent = child_bottom - child_y;
          } else if (relative_y == 1) {
            child_extent = child_y - child_rect.y;
          } else {
            child_extent = std::max(
                (child_y - child_rect.y) / relative_y,
                (child_bottom - child_y) / (1 - relative_y));
          }
          max_child_extent = std::max(max_child_extent, child_extent);
        } else {
          max_child_extent = std::max(max_child_extent, child_bottom);
        }
      } else {
        if (!is_auto_strecth && child->HeightIsRelative()) {
          const double relative_height = child->GetRelativeHeight();
          total_children_relative_size += relative_height;
          if (child_rect.h > 0 && relative_height > 0) {
            max_size_required_by_relative_children = std::max(
                child_rect.h / relative_height,
                max_size_required_by_relative_children);
          }
        } else {
          total_children_pixel_size += child_rect.h;
        }

        total_children_pixel_size_with_relative_min += child_rect.h;

        const double child_right = child_rect.x + child_rect.w;
        if (child->XIsRelative()) {
          double child_extent = 0;
          const double relative_x = child->GetRelativeX();
          if (relative_x == 0) {
            child_extent = child_right - child_x;
          } else if (relative_x == 1) {
            child_extent = child_x - child_rect.x;
          } else {
            child_extent = std::max(
                (child_x - child_rect.x) / relative_x,
                (child_right - child_x) / (1 - relative_x));
          }
          max_child_extent = std::max(max_child_extent, child_extent);
        } else {
          max_child_extent = std::max(max_child_extent, child_right);
        }
      }
    }

    const double padding_size = (count <= 1 ? 0 : padding_ * (count - 1));
    total_children_pixel_size += padding_size;
    total_children_pixel_size_with_relative_min += padding_size;

    if (orientation_ == ORIENTATION_HORIZONTAL) {
      if (total_children_relative_size >= 1 || total_children_pixel_size <= 0) {
        *width = total_children_pixel_size_with_relative_min;
      } else {
        *width = std::max(
            total_children_pixel_size / (1 - total_children_relative_size),
            max_size_required_by_relative_children);
      }
      *height = max_child_extent;
    } else {
      if (total_children_relative_size >= 1 || total_children_pixel_size <= 0) {
        *height = total_children_pixel_size_with_relative_min;
      } else {
        *height = std::max(
            total_children_pixel_size / (1 - total_children_relative_size),
            max_size_required_by_relative_children);
      }
      *width = max_child_extent;
    }
  }

  void LayoutChild(BasicElement *child, bool forward, double *extent) {
    const double child_x = child->GetPixelX();
    const double child_y = child->GetPixelY();
    const double child_radians = DegreesToRadians(child->GetRotation());
    double left_in_parent = 0;
    double right_in_parent = 0;
    double top_in_parent = 0;
    double bottom_in_parent = 0;
    GetChildRectExtentInParent(child_x, child_y,
                               child->GetPixelPinX(), child->GetPixelPinY(),
                               child_radians,
                               0, 0,
                               child->GetPixelWidth(),
                               child->GetPixelHeight(),
                               &left_in_parent, &top_in_parent,
                               &right_in_parent, &bottom_in_parent);

    if (orientation_ == ORIENTATION_HORIZONTAL) {
      if (forward) {
        child->SetPixelX(*extent + (child_x - left_in_parent));
        *extent += (padding_ + (right_in_parent - left_in_parent));
      } else {
        *extent -= (right_in_parent - left_in_parent);
        child->SetPixelX(*extent + (child_x - left_in_parent));
        *extent -= padding_;
      }
    } else {
      if (forward) {
        child->SetPixelY(*extent + (child_y - top_in_parent));
        *extent += (padding_ + (bottom_in_parent - top_in_parent));
      } else {
        *extent -= (bottom_in_parent - top_in_parent);
        child->SetPixelY(*extent + (child_y - top_in_parent));
        *extent -= padding_;
      }
    }
  }

  // Layouts children along the specified orientation.
  void LayoutChildren() {
    Elements *children = owner_->GetChildren();
    const size_t count = children->GetCount();
    std::vector<BasicElement*> backward_children;
    double extent = 0;
    double min_extent = 0;
    double max_extent = (orientation_ == ORIENTATION_HORIZONTAL) ?
        owner_->GetPixelWidth() : owner_->GetPixelHeight();
    bool reverse = (orientation_ == ORIENTATION_HORIZONTAL &&
                    owner_->IsTextRTL());

    // Layout forward children.
    for (size_t i = 0; i < count; ++i) {
      BasicElement *child = children->GetItemByIndex(i);

      // Invisible children do not occupy space, thus no need to layout them.
      if (!child->IsVisible())
        continue;
      if (GetChildLayoutDirection(child) == LinearElement::LAYOUT_FORWARD)
        LayoutChild(child, !reverse, reverse ? &max_extent: &min_extent);
      else
        backward_children.push_back(child);
    }

    extent = (orientation_ == ORIENTATION_HORIZONTAL) ?
        owner_->GetPixelWidth() : owner_->GetPixelHeight();

    // Layout backward children.
    for (size_t i = backward_children.size(); i > 0; --i) {
      LayoutChild(backward_children[i - 1],
                  reverse,
                  reverse ? &min_extent : &max_extent);
    }
  }

  // Adjusts the auto stretch children's size.
  // The auto stretch children will take up all the space that is not occupied
  // by other children.
  // Now we don't support rotated auto stretch children.
  // TODO(synch): currently, if more than 1 auto stretch children have min size,
  //    this function may have bugs that the calculated size may less than the
  //    min size, we should fix it later in CalculateSize.
  void AdjustAutoStretchChild() {
    if (auto_stretch_children_.empty()) {
      return;
    }
    double non_stretch_children_size = 0;
    int count = owner_->GetChildren()->GetCount();
    for (int i = 0; i < count; ++i) {
      BasicElement *element = owner_->GetChildren()->GetItemByIndex(i);
      if (owner_->IsChildAutoStretch(element))
        continue;
      if (!element->IsVisible())
        continue;
      element->CalculateRelativeAttributes();
      Rectangle rect = element->GetExtentsInParent();
      if (orientation_ == ORIENTATION_HORIZONTAL)
        non_stretch_children_size += rect.w;
      else
        non_stretch_children_size += rect.h;
    }
    const double padding_size = (count <= 1 ? 0 : padding_ * (count - 1));
    non_stretch_children_size += padding_size;

    double auto_stretch_children_total_size = 0;
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      auto_stretch_children_total_size =
        owner_->GetClientWidth() - non_stretch_children_size;
    } else {
      auto_stretch_children_total_size =
          owner_->GetClientHeight() - non_stretch_children_size;
    }
    double total_relative_percent = 0;
    for (AutoStretchChildrenMap::const_iterator it =
            auto_stretch_children_.begin();
         it != auto_stretch_children_.end();
         ++it) {
      if (!it->first || !it->first->IsVisible())
        continue;
      total_relative_percent += it->second;
    }
    for (AutoStretchChildrenMap::const_iterator it =
             auto_stretch_children_.begin();
         it != auto_stretch_children_.end();
         ++it) {
      if (!it->first || !it->first->IsVisible())
        continue;
      double child_size = 0;
      if (total_relative_percent <= 0) {
        child_size = auto_stretch_children_total_size /
            auto_stretch_children_.size();
      } else {
        child_size = auto_stretch_children_total_size * it->second /
            total_relative_percent;
      }
      if (orientation_ == ORIENTATION_HORIZONTAL)
        it->first->SetPixelWidth(child_size);
      else
        it->first->SetPixelHeight(child_size);
    }
  }

  // Updates the size of auto stretch children to relative.
  // Because we set the pixel size of these children in AdjustChildrenSize,
  // we need to recover them into relative size. Also, if user sets the relative
  // size, we will update the value which we saved in the map.
  void UpdateAutoStretchChildrenRelativeSize() {
    for (AutoStretchChildrenMap::iterator it =
             auto_stretch_children_.begin();
         it != auto_stretch_children_.end();
         ++it) {
      if (orientation_ == ORIENTATION_HORIZONTAL) {
        if (it->first->WidthIsRelative())
          it->second = it->first->GetRelativeWidth();
        else
          it->first->SetRelativeWidth(it->second);
      } else {
        if (it->first->HeightIsRelative())
          it->second = it->first->GetRelativeHeight();
        else
          it->first->SetRelativeHeight(it->second);
      }
    }
  }

  void ResetChildrenXY() {
    Elements *children = owner_->GetChildren();
    const size_t count = children->GetCount();
    for (size_t i = 0; i < count; ++i) {
      BasicElement *child = children->GetItemByIndex(i);
      child->SetPixelX(0);
      child->SetPixelY(0);
    }
  }

  void SetChildLayoutDirection(LinearElement::LayoutDirection dir,
                               BasicElement *child) {
    ASSERT(child);
    ASSERT(child->GetParentElement() == owner_);

    if (dir == LinearElement::LAYOUT_BACKWARD)
      backward_children_.insert(child);
    else
      backward_children_.erase(child);
  }

  LinearElement::LayoutDirection GetChildLayoutDirection(
      BasicElement *child) const {
    ASSERT(child);
    ASSERT(child->GetParentElement() == owner_);
    return backward_children_.count(child) ?
        LinearElement::LAYOUT_BACKWARD : LinearElement::LAYOUT_FORWARD;
  }

  void SetChildAutoStretch(bool auto_stretch, BasicElement *child) {
    ASSERT(child);
    ASSERT(child->GetParentElement() == owner_);
    if (auto_stretch) {
      double relative_size = 0;
      if (orientation_ == ORIENTATION_HORIZONTAL) {
        if (child->WidthIsRelative())
          relative_size = child->GetRelativeWidth();
        else
          relative_size = 1.0;
      } else {
        if (child->HeightIsRelative())
          relative_size = child->GetRelativeHeight();
        else
          relative_size = 1.0;
      }
      auto_stretch_children_.insert(
          AutoStretchChildrenMap::value_type(child, relative_size));
    } else {
      auto_stretch_children_.erase(child);
    }
  }

  bool IsChildAutoStretch(BasicElement *child) const {
    ASSERT(child);
    ASSERT(child->GetParentElement() == owner_);
    return auto_stretch_children_.count(child);
  }

  void OnElementAdded(BasicElement *element) {
    element->RegisterStringEnumProperty(
        kLayoutDirectionProperty,
        NewSlot(this, &Impl::GetChildLayoutDirection, element),
        NewSlot(this, &Impl::SetChildLayoutDirection, element),
        kLayoutDirections, arraysize(kLayoutDirections));
    element->RegisterProperty(
        kAutoStretchProperty,
        NewSlot(this, &Impl::IsChildAutoStretch, element),
        NewSlot(this, &Impl::SetChildAutoStretch, element));
  }

  void OnElementRemoved(BasicElement *element) {
    element->RemoveProperty(kLayoutDirectionProperty);
    element->RemoveProperty(kAutoStretchProperty);
    auto_stretch_children_.erase(element);
    backward_children_.erase(element);
  }

  LinearElement *owner_;
  Connection *on_element_added_connection_;
  Connection *on_element_removed_connection_;

  // All children that need layouting backward.
  std::set<BasicElement*> backward_children_;
  typedef std::map<BasicElement* /* child*/, double /* relative size */>
          AutoStretchChildrenMap;
  AutoStretchChildrenMap auto_stretch_children_;

  double padding_;
  double min_width_;
  double min_height_;

  LinearElement::Orientation orientation_;
  bool h_auto_sizing_ : 1;
  bool v_auto_sizing_ : 1;
};

LinearElement::LinearElement(View *view, const char *name)
    : DivElement(view, "linear", name),
      impl_(new Impl(this)) {
}

LinearElement::LinearElement(View *view, const char *tag_name, const char *name)
    : DivElement(view, tag_name, name),
      impl_(new Impl(this)) {
}

LinearElement::Orientation LinearElement::GetOrientation() const {
  return impl_->orientation_;
}

void LinearElement::SetOrientation(Orientation orientation) {
  if (impl_->orientation_ != orientation) {
    impl_->orientation_ = orientation;
    impl_->ResetChildrenXY();
    QueueDraw();
  }
}

double LinearElement::GetPadding() const {
  return impl_->padding_;
}

void LinearElement::SetPadding(double padding) {
  if (impl_->padding_ != padding) {
    impl_->padding_ = padding;
    QueueDraw();
  }
}

bool LinearElement::IsHorizontalAutoSizing() const {
  return impl_->h_auto_sizing_;
}

void LinearElement::SetHorizontalAutoSizing(bool auto_sizing) {
  if (impl_->h_auto_sizing_ != auto_sizing) {
    impl_->h_auto_sizing_ = auto_sizing;
    QueueDraw();
  }
}

bool LinearElement::IsVerticalAutoSizing() const {
  return impl_->v_auto_sizing_;
}

void LinearElement::SetVerticalAutoSizing(bool auto_sizing) {
  if (impl_->v_auto_sizing_ != auto_sizing) {
    impl_->v_auto_sizing_ = auto_sizing;
    QueueDraw();
  }
}

LinearElement::LayoutDirection LinearElement::GetChildLayoutDirection(
    BasicElement *child) const {
  return impl_->GetChildLayoutDirection(child);
}

void LinearElement::SetChildLayoutDirection(BasicElement *child,
                                            LayoutDirection dir) {
  impl_->SetChildLayoutDirection(dir, child);
}

bool LinearElement::IsChildAutoStretch(BasicElement *child) const {
  return impl_->IsChildAutoStretch(child);
}

void LinearElement::SetChildAutoStretch(BasicElement *child,
                                        bool auto_stretch) {
  impl_->SetChildAutoStretch(auto_stretch, child);
}

void LinearElement::DoClassRegister() {
  DivElement::DoClassRegister();
  RegisterStringEnumProperty("orientation",
                   NewSlot(&LinearElement::GetOrientation),
                   NewSlot(&LinearElement::SetOrientation),
                   kOrientationNames, arraysize(kOrientationNames));
  RegisterProperty("padding",
                   NewSlot(&LinearElement::GetPadding),
                   NewSlot(&LinearElement::SetPadding));

  // Overrides "width" and "height" properties to make sure they won't be
  // changed by script when "autoSizing" is true.
  // Note: we cannot override BasicElement::{Get|Set}{Width|Height} methods, as
  // they are not virtual.
  RegisterProperty("width",
                   NewSlot(&Impl::GetWidth, &LinearElement::impl_),
                   NewSlot(&Impl::SetWidth, &LinearElement::impl_));
  RegisterProperty("height",
                   NewSlot(&Impl::GetHeight, &LinearElement::impl_),
                   NewSlot(&Impl::SetHeight, &LinearElement::impl_));
}

LinearElement::~LinearElement() {
  delete impl_;
  impl_ = NULL;
}

void LinearElement::CalculateSize() {
  impl_->UpdateAutoStretchChildrenRelativeSize();
  BasicElement::CalculateSize();
  impl_->CalculateSize(&impl_->min_width_, &impl_->min_height_);
  // Take the space occupied by the scrollbar into account.
  impl_->min_width_ += (GetPixelWidth() - GetClientWidth());
  // Take the space occupied by the scrollbar into account.
  // Though horizontal scrollbar is not supported for now, having this code
  // is harmless.
  impl_->min_height_ += (GetPixelHeight() - GetClientHeight());
  // TODO(synch): figure out how to layout div element with scroll bar and
  //     children with relative size.
  if (impl_->h_auto_sizing_) {
    SetPixelWidth(GetMinWidth());
  } else if (!WidthIsRelative()) {
    // If the pixel width is set explicitly, then we allow the width to be
    // shrunk freely.
    impl_->min_width_ = 0;
  }
  if (impl_->v_auto_sizing_) {
    SetPixelHeight(GetMinHeight());
  } else if (!HeightIsRelative()) {
    // If the pixel height is set explicitly, then we allow the height to be
    // shrunk freely.
    impl_->min_height_ = 0;
  }
}

void LinearElement::BeforeChildrenLayout() {
  impl_->AdjustAutoStretchChild();
}

void LinearElement::Layout() {
  impl_->LayoutChildren();
  DivElement::Layout();
}

double LinearElement::GetMinWidth() const {
  return std::max(impl_->min_width_, DivElement::GetMinWidth());
}

double LinearElement::GetMinHeight() const {
  return std::max(impl_->min_height_, DivElement::GetMinHeight());
}

BasicElement *LinearElement::CreateInstance(View *view, const char *name) {
  return new LinearElement(view, name);
}

} // namespace ggadget
