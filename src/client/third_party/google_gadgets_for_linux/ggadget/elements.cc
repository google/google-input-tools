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

#include <vector>
#include <algorithm>

#include "basic_element.h"
#include "element_factory.h"
#include "elements.h"
#include "event.h"
#include "gadget_interface.h"
#include "graphics_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "scriptable_helper.h"
#include "signals.h"
#include "small_object.h"
#include "view.h"
#include "view_element.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace ggadget {

class Elements::Impl : public SmallObject<> {
 public:
  Impl(ElementFactory *factory, BasicElement *owner, View *view)
      : width_(.0), height_(.0),
        factory_(factory), owner_(owner), view_(view),
        scrollable_(false), element_removed_(false) {
    ASSERT(view);
  }

  ~Impl() {
    RemoveAllElements();
  }

#define ASSERT_ELEMENT_INDEX(element, index) \
  ASSERT((element)->GetIndex() == (index))

#define ASSERT_ELEMENT_INTEGRITY(element) \
  ASSERT((element)->GetIndex() < children_.size() && \
         children_[(element)->GetIndex()] == (element))

#ifdef _DEBUG
#define ASSERT_ELEMENTS_INTEGRITY \
  do { for (size_t i_i = 0; i_i < children_.size(); i_i++) \
    ASSERT_ELEMENT_INDEX(children_[i_i], i_i); } while (false)
#else
#define ASSERT_ELEMENTS_INTEGRITY do{} while (false)
#endif

  size_t GetCount() {
    return children_.size();
  }

  void UpdateIndexes(size_t index_after) {
    size_t size = children_.size();
    for (size_t i = index_after; i < size; i++)
      children_[i]->SetIndex(i);
  }

  bool IsChild(const BasicElement *element) {
    return element->GetIndex() != kInvalidIndex &&
           element->GetParentElement() == owner_;
  }

  const BasicElement *GetBeforeFromAfter(const BasicElement *after) {
    if (after) {
      ASSERT(IsChild(after));
      size_t index = after->GetIndex();
      if (index + 1 < children_.size())
        return children_[index + 1];
    } else if (children_.size()) {
      return children_[0];
    }
    return NULL;
  }

  bool InsertElementInternal(BasicElement *element,
                             const BasicElement *before) {
    element->SetParentElement(owner_);
    if (view_->OnElementAdd(element)) {
      element->QueueDraw();
      if (before) {
        ASSERT_ELEMENT_INTEGRITY(before);
        size_t index = before->GetIndex();
        children_.insert(children_.begin() + index, element);
        UpdateIndexes(index);
      } else {
        element->SetIndex(children_.size());
        children_.push_back(element);
      }
      ASSERT_ELEMENTS_INTEGRITY;
      if (on_element_added_.HasActiveConnections())
        on_element_added_(element);
      return true;
    }
    delete element;
    ASSERT_ELEMENTS_INTEGRITY;
    return false;
  }

  void RemoveElementInternal(BasicElement *element) {
    ASSERT_ELEMENT_INTEGRITY(element);
    element->QueueDraw();
    view_->OnElementRemove(element);
    size_t index = element->GetIndex();
    children_.erase(children_.begin() + index);
    UpdateIndexes(index);
    element_removed_ = true;
    ASSERT_ELEMENTS_INTEGRITY;
    if (on_element_removed_.HasActiveConnections())
        on_element_removed_(element);
  }

  bool InsertElement(BasicElement *element, const BasicElement *before) {
    ASSERT(element);
    if (element == before)
      return false;
    if (before) {
      if (!IsChild(before))
        return false;
      if (IsChild(element) && element->GetIndex() + 1 == before->GetIndex())
        return true;
    } else {
      if (IsChild(element) && element->GetIndex() == children_.size())
        return true;
    }
    if (element->GetView() != view_)
      return false;
    for (BasicElement *e = owner_; e; e = e->GetParentElement()) {
      if (e == element) {
        LOG("Can't insert an element under itself.");
        return false;
      }
    }

    if (element->GetIndex() == kInvalidIndex) {
      if (element->GetParentElement()) {
        LOG("Inner child can't be moved.");
        return false;
      }
    } else {
      BasicElement *parent = element->GetParentElement();
      // Detach the element from its original parent.
      Elements *elements = parent ?
                           parent->GetChildren() : view_->GetChildren();
      elements->impl_->RemoveElementInternal(element);
    }

    return InsertElementInternal(element, before);
  }

  BasicElement *InsertElement(const char *tag_name,
                              const BasicElement *before,
                              const char *name) {
    if (!factory_)
      return NULL;
    if (before && !IsChild(before))
      return NULL;

    BasicElement *e = factory_->CreateElement(tag_name, view_, name);
    if (e == NULL)
      return NULL;
    return InsertElementInternal(e, before) ? e : NULL;
  }

  bool RemoveElement(BasicElement *element) {
    if (!element || !IsChild(element))
      return false;

    RemoveElementInternal(element);
    delete element;
    return true;
  }

  void RemoveAllElements() {
    if (children_.size()) {
      for (Children::iterator ite = children_.begin();
           ite != children_.end(); ++ite) {
        view_->OnElementRemove(*ite);
        if (on_element_removed_.HasActiveConnections())
          on_element_removed_(*ite);
        delete *ite;
      }
      Children v;
      children_.swap(v);
      element_removed_ = true;
    }
    // The caller should call QueueDraw() at proper time.
  }

  BasicElement *GetItem(const Variant &index_or_name) {
    switch (index_or_name.type()) {
      case Variant::TYPE_INT64:
        return GetItemByIndex(VariantValue<size_t>()(index_or_name));
      case Variant::TYPE_STRING:
        return GetItemByName(VariantValue<const char *>()(index_or_name));
      case Variant::TYPE_DOUBLE:
        return GetItemByIndex(
            static_cast<size_t>(VariantValue<double>()(index_or_name)));
      default:
        return NULL;
    }
  }

  BasicElement *GetItemByIndex(size_t index) {
    if (index < children_.size()) {
      ASSERT_ELEMENT_INDEX(children_[index], index);
      return children_[index];
    }
    return NULL;
  }

  BasicElement *GetItemByName(const char *name) {
    if (!name || !*name)
      return NULL;
    for (Children::const_iterator ite = children_.begin();
         ite != children_.end(); ++ite) {
      if (GadgetStrCmp((*ite)->GetName().c_str(), name) == 0)
        return *ite;
    }
    return NULL;
  }

  void MapChildPositionEvent(const PositionEvent &org_event,
                             BasicElement *child,
                             PositionEvent *new_event) {
    double child_x, child_y;
    ASSERT(owner_ == child->GetParentElement());
    child->ParentCoordToSelfCoord(org_event.GetX(), org_event.GetY(),
                                  &child_x, &child_y);
    new_event->SetX(child_x);
    new_event->SetY(child_y);
  }

  void MapChildMouseEvent(const MouseEvent &org_event,
                          BasicElement *child,
                          MouseEvent *new_event) {
    MapChildPositionEvent(org_event, child, new_event);
    BasicElement::FlipMode flip = child->GetFlip();
    if (flip & BasicElement::FLIP_HORIZONTAL)
      new_event->SetWheelDeltaX(-org_event.GetWheelDeltaX());
    if (flip & BasicElement::FLIP_VERTICAL)
      new_event->SetWheelDeltaY(-org_event.GetWheelDeltaY());
  }

  EventResult OnMouseEvent(const MouseEvent &event,
                           BasicElement **fired_element,
                           BasicElement **in_element,
                           ViewInterface::HitTest *hittest) {
    // The following event types are processed directly in the view.
    ASSERT(event.GetType() != Event::EVENT_MOUSE_OVER &&
           event.GetType() != Event::EVENT_MOUSE_OUT);

    ElementHolder in_element_holder(*in_element);
    *fired_element = NULL;
    ViewInterface::HitTest in_hittest = *hittest;
    MouseEvent new_event(event);
    // Iterate in reverse since higher elements are listed last.
    for (Children::reverse_iterator ite = children_.rbegin();
         ite != children_.rend(); ++ite) {
      BasicElement *child = *ite;
      // Don't use child->IsReallyVisible() because here we don't need to check
      // visibility of ancestors.
      if (!child->IsVisible() || child->GetOpacity() == 0.0)
        continue;
      MapChildMouseEvent(event, child, &new_event);
      if (child->IsPointIn(new_event.GetX(), new_event.GetY())) {
        BasicElement *child = *ite;
        ElementHolder child_holder(*ite);
        BasicElement *descendant_in_element = NULL;
        ViewInterface::HitTest descendant_hittest = *hittest;
        EventResult result = child->OnMouseEvent(new_event, false,
                                                 fired_element,
                                                 &descendant_in_element,
                                                 &descendant_hittest);
        // The child has been removed by some event handler, can't continue.
        if (!child_holder.Get()) {
          *in_element = in_element_holder.Get();
          *hittest = in_hittest;
          return result;
        }
        if (!in_element_holder.Get()) {
          in_element_holder.Reset(descendant_in_element);
          in_hittest = descendant_hittest;
        }
        if (*fired_element) {
          *in_element = in_element_holder.Get();
          *hittest = in_hittest;
          return result;
        }
      }
    }
    *in_element = in_element_holder.Get();
    *hittest = in_hittest;
    return EVENT_RESULT_UNHANDLED;
  }

  EventResult OnDragEvent(const DragEvent &event,
                          BasicElement **fired_element) {
    // Only the following event type is dispatched along the element tree.
    ASSERT(event.GetType() == Event::EVENT_DRAG_MOTION);

    *fired_element = NULL;
    DragEvent new_event(event);
    // Iterate in reverse since higher elements are listed last.
    for (Children::reverse_iterator ite = children_.rbegin();
         ite != children_.rend(); ++ite) {
      BasicElement *child = (*ite);
      if (!child->IsReallyVisible())
        continue;

      MapChildPositionEvent(event, child, &new_event);
      if (child->IsPointIn(new_event.GetX(), new_event.GetY())) {
        ElementHolder child_holder(child);
        EventResult result = (*ite)->OnDragEvent(new_event, false,
                                                 fired_element);
        // The child has been removed by some event handler, can't continue.
        if (!child_holder.Get() || *fired_element)
          return result;
      }
    }
    return EVENT_RESULT_UNHANDLED;
  }

  // Update the maximum children extent.
  void UpdateChildExtent(BasicElement *child,
                         double *extent_width, double *extent_height) {
    double x = child->GetPixelX();
    double y = child->GetPixelY();
    double pin_x = child->GetPixelPinX();
    double pin_y = child->GetPixelPinY();
    double width = child->GetPixelWidth();
    double height = child->GetPixelHeight();
    // Estimate the biggest possible extent with low cost.
    double est_maximum_extent = std::max(pin_x, width - pin_x) +
                                std::max(pin_y, height - pin_y);
    double child_extent_width = x + est_maximum_extent;
    double child_extent_height = y + est_maximum_extent;
    // Calculate the actual extent only if the estimated value is bigger than
    // current extent.
    if (child_extent_width > *extent_width ||
        child_extent_height > *extent_height) {
      GetChildExtentInParent(x, y, pin_x, pin_y, width, height,
                             DegreesToRadians(child->GetRotation()),
                             &child_extent_width, &child_extent_height);
      if (child_extent_width > *extent_width)
        *extent_width = child_extent_width;
      if (child_extent_height > *extent_height)
        *extent_height = child_extent_height;
    }
  }

  void CalculateSize() {
    Children::iterator it = children_.begin();
    Children::iterator end = children_.end();
    for (; it != end; ++it)
      (*it)->CalculateSize();
  }

  void Layout() {
    Children::iterator it = children_.begin();
    Children::iterator end = children_.end();
    bool need_update_extents = element_removed_;
    for (; it != end; ++it) {
      (*it)->RecursiveLayout();
      if ((*it)->IsPositionChanged() || (*it)->IsSizeChanged())
        need_update_extents = true;
      // Clear the size and position changed state here, because children's
      // Draw() method might not be called.
      (*it)->ClearPositionChanged();
      (*it)->ClearSizeChanged();
    }

    if (scrollable_) {
      if (need_update_extents) {
        width_ = height_ = 0;
        for (it = children_.begin(); it != end; ++it)
          UpdateChildExtent(*it, &width_, &height_);
      }
    } else if (owner_) {
      // If not scrollable, the canvas size is the same as the parent.
      width_ = owner_->GetPixelWidth();
      height_ = owner_->GetPixelHeight();
    } else {
      width_ = view_->GetWidth();
      height_ = view_->GetHeight();
    }

    element_removed_ = false;
  }

  void AggregateClipRegion(const Rectangle &boundary, ClipRegion *region) {
    Children::iterator it = children_.begin();
    Children::iterator end = children_.end();
    BasicElement *popup = view_->GetPopupElement();
    for (; it != end; ++it) {
      // Skip popup element, it'll be handled in view.
      if (popup != (*it)) {
        (*it)->AggregateClipRegion(boundary, region);
      }
    }
  }

  void Draw(CanvasInterface *canvas) {
    ASSERT(canvas);
    if (children_.empty() || !width_ || !height_)
      return;

    size_t child_count = children_.size();

    BasicElement *popup = view_->GetPopupElement();
    for (size_t i = 0; i < child_count; i++) {
      BasicElement *element = children_[i];
      // Doesn't draw popup element here.
      if (element == popup) continue;

      // Doesn't draw elements that outside visible area.
      // Conditions to determine if an element is outside visible area:
      // 1. If it's outside view's clip region
      // 2. If it's outside parent's visible area.
      if (!view_->IsElementInClipRegion(element) ||
          (owner_ && !owner_->IsChildInVisibleArea(element))) {
        //DLOG("pass child: %p(%s)", element, element->GetName().c_str());
        continue;
      }

      canvas->PushState();
      if (element->GetRotation() == .0) {
        canvas->TranslateCoordinates(
            element->GetPixelX() - element->GetPixelPinX(),
            element->GetPixelY() - element->GetPixelPinY());
      } else {
        canvas->TranslateCoordinates(element->GetPixelX(),
                                     element->GetPixelY());
        canvas->RotateCoordinates(
            DegreesToRadians(element->GetRotation()));
        canvas->TranslateCoordinates(-element->GetPixelPinX(),
                                     -element->GetPixelPinY());
      }

      element->Draw(canvas);
      canvas->PopState();
    }

#ifdef _DEBUG
    if (view_->GetDebugMode() & ViewInterface::DEBUG_CONTAINER) {
      // Draw bounding box for debug.
      canvas->DrawLine(0, 0, 0, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(0, 0, width_, 0, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, height_, 0, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, height_, width_, 0, 1, Color(0, 0, 0));
      canvas->DrawLine(0, 0, width_, height_, 1, Color(0, 0, 0));
      canvas->DrawLine(width_, 0, 0, height_, 1, Color(0, 0, 0));
    }
#endif
  }

  void SetScrollable(bool scrollable) {
    scrollable_ = scrollable;
  }

  void MarkRedraw() {
    Children::iterator it = children_.begin();
    for (; it != children_.end(); ++it)
      (*it)->MarkRedraw();
  }

  double width_;
  double height_;
  ElementFactory *factory_;
  BasicElement *owner_;
  View *view_;
  typedef std::vector<BasicElement *> Children;
  Children children_;
  Signal1<void, BasicElement*> on_element_added_;
  Signal1<void, BasicElement*> on_element_removed_;

  bool scrollable_      : 1;
  bool element_removed_ : 1;
};

Elements::Elements(ElementFactory *factory,
                   BasicElement *owner, View *view)
    : impl_(new Impl(factory, owner, view)) {
}

void Elements::DoClassRegister() {
  RegisterProperty("count", NewSlot(&Impl::GetCount, &Elements::impl_), NULL);
  RegisterMethod("item", NewSlot(&Impl::GetItem, &Elements::impl_));
  // Register the "default" method, allowing this object be called directly
  // as a function.
  RegisterMethod("", NewSlot(&Impl::GetItem, &Elements::impl_));
}

Elements::~Elements() {
  delete impl_;
}

size_t Elements::GetCount() const {
  return impl_->GetCount();
}

BasicElement *Elements::GetItemByIndex(size_t child) {
  return impl_->GetItemByIndex(child);
}

BasicElement *Elements::GetItemByName(const char *child) {
  return impl_->GetItemByName(child);
}

const BasicElement *Elements::GetItemByIndex(size_t child) const {
  return impl_->GetItemByIndex(child);
}

const BasicElement *Elements::GetItemByName(const char *child) const {
  return impl_->GetItemByName(child);
}

BasicElement *Elements::AppendElement(const char *tag_name, const char *name) {
  return impl_->InsertElement(tag_name, NULL, name);
}

BasicElement *Elements::InsertElement(const char *tag_name,
                                      const BasicElement *before,
                                      const char *name) {
  return impl_->InsertElement(tag_name, before, name);
}

BasicElement *Elements::InsertElementAfter(const char *tag_name,
                                           const BasicElement *after,
                                           const char *name) {
  if (after && !impl_->IsChild(after))
    return NULL;
  return impl_->InsertElement(tag_name, impl_->GetBeforeFromAfter(after), name);
}

bool Elements::AppendElement(BasicElement *element) {
  return InsertElement(element, NULL);
}

bool Elements::InsertElement(BasicElement *element,
                             const BasicElement *before) {
  return impl_->InsertElement(element, before);
}

bool Elements::InsertElementAfter(BasicElement *element,
                                  const BasicElement *after) {
  if (after && !impl_->IsChild(after))
    return false;
  if (after == element)
    return false;
  const BasicElement *before = impl_->GetBeforeFromAfter(after);
  if (before == element)
    return true;
  return impl_->InsertElement(element, before);
}

BasicElement *Elements::AppendElementFromXML(const std::string &xml) {
  return InsertElementFromXML(xml, NULL);
}

BasicElement *Elements::InsertElementFromXML(const std::string &xml,
                                             const BasicElement *before) {
  if (before && !impl_->IsChild(before))
    return NULL;

  DOMDocumentInterface *xmldoc = GetXMLParser()->CreateDOMDocument();
  xmldoc->Ref();
  GadgetInterface *gadget = impl_->view_->GetGadget();
  bool success = false;
  if (gadget) {
    success = gadget->ParseLocalizedXML(xml, xml.c_str(), xmldoc);
  } else {
    // For unittest. Parse without encoding fallback and localization.
    success = GetXMLParser()->ParseContentIntoDOM(xml, NULL, xml.c_str(),
                                                  NULL, NULL, NULL,
                                                  xmldoc, NULL, NULL);
  }

  BasicElement *result = NULL;
  if (success) {
    DOMElementInterface *xml_element = xmldoc->GetDocumentElement();
    if (!xml_element) {
      LOG("No root element in xml definition: %s", xml.c_str());
    } else {
      // Disable events during parsing XML into Elements.
      impl_->view_->EnableEvents(false);
      result = InsertElementFromDOM(this, impl_->view_->GetScriptContext(),
                                    xml_element, before, "");
      impl_->view_->EnableEvents(true);
    }
  }

  ASSERT(xmldoc->GetRefCount() == 1);
  xmldoc->Unref();
  return result;
}

BasicElement *Elements::InsertElementFromXMLAfter(const std::string &xml,
                                             const BasicElement *after) {
  if (after && !impl_->IsChild(after))
    return NULL;
  return InsertElementFromXML(xml, impl_->GetBeforeFromAfter(after));
}

BasicElement *Elements::AppendElementVariant(const Variant &element) {
  return InsertElementVariant(element, NULL);
}

BasicElement *Elements::InsertElementVariant(const Variant &element,
                                             const BasicElement *before) {
  BasicElement *result = NULL;
  if (element.type() == Variant::TYPE_STRING) {
    result = InsertElementFromXML(VariantValue<std::string>()(element), before);
  } else if (element.type() == Variant::TYPE_SCRIPTABLE) {
    BasicElement *elm = VariantValue<BasicElement *>()(element);
    if (elm && InsertElement(elm, before))
      result = elm;
  }
  return result;
}

BasicElement *Elements::InsertElementVariantAfter(const Variant &element,
                                                  const BasicElement *after) {
  if (after && !impl_->IsChild(after))
    return NULL;
  return InsertElementVariant(element, impl_->GetBeforeFromAfter(after));
}

bool Elements::RemoveElement(BasicElement *element) {
  return impl_->RemoveElement(element);
}

void Elements::RemoveAllElements() {
  impl_->RemoveAllElements();
  if (impl_->element_removed_) {
    if (impl_->owner_)
      impl_->owner_->QueueDraw();
    else
      impl_->view_->QueueDraw();
  }
}

void Elements::CalculateSize() {
  impl_->CalculateSize();
}

void Elements::Layout() {
  impl_->Layout();
}

void Elements::Draw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
}

EventResult Elements::OnMouseEvent(const MouseEvent &event,
                                   BasicElement **fired_element,
                                   BasicElement **in_element,
                                   ViewInterface::HitTest *hittest) {
  return impl_->OnMouseEvent(event, fired_element, in_element, hittest);
}

EventResult Elements::OnDragEvent(const DragEvent &event,
                                  BasicElement **fired_element) {
  return impl_->OnDragEvent(event, fired_element);
}

void Elements::SetScrollable(bool scrollable) {
  impl_->SetScrollable(scrollable);
}

void Elements::GetChildrenExtents(double *width, double *height) {
  ASSERT(width && height);
  *width = impl_->width_;
  *height = impl_->height_;
}

void Elements::MarkRedraw() {
  impl_->MarkRedraw();
}

void Elements::AggregateClipRegion(const Rectangle &boundary,
                                   ClipRegion *region) {
  impl_->AggregateClipRegion(boundary, region);
}

Connection *Elements::ConnectOnElementAdded(Slot1<void, BasicElement*> *slot) {
  return impl_->on_element_added_.Connect(slot);
}

Connection *Elements::ConnectOnElementRemoved(
    Slot1<void, BasicElement*> *slot) {
  return impl_->on_element_removed_.Connect(slot);
}

} // namespace ggadget
