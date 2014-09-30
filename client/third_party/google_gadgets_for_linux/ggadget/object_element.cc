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

#include "object_element.h"

#include <string>

#include "canvas_interface.h"
#include "elements.h"
#include "element_factory.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class ObjectElement::Impl : public SmallObject<> {
 public:
  Impl() : object_(NULL) { }
  ~Impl() { delete object_; }

  BasicElement *object_;
  std::string classid_;
};

ObjectElement::ObjectElement(View *view, const char *name)
    : BasicElement(view, "object", name, false),
      impl_(new Impl()) {
  SetEnabled(true);
}

ObjectElement::~ObjectElement() {
  delete impl_;
}

void ObjectElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("classId",
                   NewSlot(&ObjectElement::GetObjectClassId),
                   NewSlot(&ObjectElement::SetObjectClassId));
  RegisterProperty("object", NewSlot(&ObjectElement::GetObject), NULL);
}

BasicElement *ObjectElement::CreateInstance(View *view, const char *name) {
  return new ObjectElement(view, name);
}

BasicElement *ObjectElement::GetObject() {
  return impl_->object_;
}

const std::string& ObjectElement::GetObjectClassId() const {
  return impl_->classid_;
}

void ObjectElement::SetObjectClassId(const std::string& classid) {
  ASSERT(!impl_->object_);
  if (impl_->object_) {
    LOG("Object already had a classId: %s", impl_->classid_.c_str());
  } else {
    impl_->object_ = GetView()->GetElementFactory()->CreateElement(
        classid.c_str(), GetView(), GetName().c_str());
    if (!impl_->object_) {
      LOG("Failed to get the object with classid: %s.", classid.c_str());
      return;
    }
    impl_->object_->SetParentElement(this);
    // Trigger property registration.
    impl_->object_->GetProperty("");
    impl_->classid_ = classid;
  }
}

void ObjectElement::Layout() {
  BasicElement::Layout();
  if (impl_->object_)
    impl_->object_->RecursiveLayout();
}

void ObjectElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->object_)
    impl_->object_->Draw(canvas);
}

EventResult ObjectElement::HandleMouseEvent(const MouseEvent &event) {
  BasicElement *fired, *in;
  ViewInterface::HitTest hittest;
  return impl_->object_ ?
      impl_->object_->OnMouseEvent(event, true, &fired, &in, &hittest) :
      EVENT_RESULT_UNHANDLED;
}

EventResult ObjectElement::HandleDragEvent(const DragEvent &event) {
  BasicElement *fired;
  return impl_->object_ ?
      impl_->object_->OnDragEvent(event, true, &fired) :
      EVENT_RESULT_UNHANDLED;
}

EventResult ObjectElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->object_ ? impl_->object_->OnKeyEvent(event) :
      EVENT_RESULT_UNHANDLED;
}

EventResult ObjectElement::HandleOtherEvent(const Event &event) {
  return impl_->object_ ? impl_->object_->OnOtherEvent(event) :
      EVENT_RESULT_UNHANDLED;
}

void ObjectElement::AggregateMoreClipRegion(const Rectangle &boundary,
                                            ClipRegion *region) {
  if (impl_->object_)
    impl_->object_->AggregateClipRegion(boundary, region);
}

} // namespace ggadget
