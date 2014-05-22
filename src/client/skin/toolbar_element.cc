/*
  Copyright 2014 Google Inc.

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

#include "skin/toolbar_element.h"

#include <map>
#include <set>
#include <string>

#include "third_party/google_gadgets_for_linux/ggadget/button_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/canvas_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/elements.h"
#include "third_party/google_gadgets_for_linux/ggadget/linear_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/signals.h"
#include "third_party/google_gadgets_for_linux/ggadget/small_object.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"

namespace ime_goopy {
namespace skin {

class ToolbarElement::Impl : public ggadget::SmallObject<> {
 public:
  explicit Impl(ToolbarElement* owner);
  ~Impl();

  void Initialize();

  ggadget::BasicElement* AddButton(
      const char* button_name,
      ggadget::LinearElement::LayoutDirection direction,
      ButtonVisibilityType visibility);
  bool RemoveButton(const char* button_name);

  bool IsCollapsed();
  void SetCollapsed(bool is_collapsed);

 private:
  ToolbarElement* owner_;

  bool is_collapsed_;
  std::map<ggadget::BasicElement*, ButtonVisibilityType> button_visibility_;
  std::set<ggadget::BasicElement*> xml_imported_buttons_;

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

ToolbarElement::Impl::Impl(ToolbarElement* owner) : owner_(owner),
                                                    is_collapsed_(false) {
  owner_->SetHorizontalAutoSizing(true);
  owner_->SetVerticalAutoSizing(true);
  owner_->SetEnabled(true);
  owner_->SetOrientation(
      ggadget::LinearElement::ORIENTATION_HORIZONTAL);
}

ToolbarElement::Impl::~Impl() {
}

void ToolbarElement::Impl::Initialize() {
  xml_imported_buttons_.clear();
  ggadget::Elements* children = owner_->GetChildren();
  for (size_t i = 0; i < children->GetCount(); ++i) {
    ggadget::BasicElement* element = children->GetItemByIndex(i);
    element->SetVisible(false);
    element->SetEnabled(false);
    button_visibility_[element] = ALWAYS_INVISIBLE;
    xml_imported_buttons_.insert(element);
  }
}

ggadget::BasicElement* ToolbarElement::Impl::AddButton(
    const char* button_name,
    ggadget::LinearElement::LayoutDirection direction,
    ButtonVisibilityType visibility) {
  ggadget::ButtonElement* element =
      ggadget::down_cast<ggadget::ButtonElement*>(
          owner_->GetChildren()->GetItemByName(button_name));
  // If the element does not exist, create a new button.
  if (!element) {
    element = new ggadget::ButtonElement(owner_->GetView(), button_name);
    owner_->GetChildren()->AppendElement(element);
    owner_->SetChildLayoutDirection(element, direction);
    element->SetCursor(ggadget::ViewInterface::CURSOR_HAND);
  }

  // Set property visibility for this button.
  button_visibility_[element] = visibility;

  return element;
}

bool ToolbarElement::Impl::RemoveButton(const char* button_name) {
  ggadget::BasicElement* element =
      owner_->GetChildren()->GetItemByName(button_name);
  if (!element)
    return false;

  // If it is a xml imported button, hide it; otherwise, remove it.
  if (xml_imported_buttons_.find(element) !=
      xml_imported_buttons_.end()) {
    element->SetVisible(false);
    element->SetEnabled(false);
    button_visibility_[element] = ALWAYS_INVISIBLE;
  } else {
    button_visibility_.erase(element);
    owner_->GetChildren()->RemoveElement(element);
  }
  return true;
}

bool ToolbarElement::Impl::IsCollapsed() {
  return is_collapsed_;
}

void ToolbarElement::Impl::SetCollapsed(bool is_collapsed) {
  if (is_collapsed_ == is_collapsed)
    return;
  is_collapsed_ = is_collapsed;

  // Set the visibility of all the children.
  ggadget::Elements* children = owner_->GetChildren();
  for (size_t i = 0; i < children->GetCount(); ++i) {
    ggadget::BasicElement* element = children->GetItemByIndex(i);

    // If the visibility type of button is not normal,
    // do not change its visibility.
    std::map<ggadget::BasicElement*, ButtonVisibilityType>::iterator it =
        button_visibility_.find(element);
    if (it != button_visibility_.end() && it->second != NORMAL_VISIBILITY)
      continue;

    element->SetVisible(!is_collapsed_);
  }
}

ToolbarElement::ToolbarElement(
    ggadget::View* view, const char* name)
    : ggadget::LinearElement(view, "toolbar", name),
      impl_(new Impl(this)) {
}

ToolbarElement::~ToolbarElement() {
}

ggadget::BasicElement* ToolbarElement::CreateInstance(
    ggadget::View* view, const char* name) {
  return new ToolbarElement(view, name);
}

void ToolbarElement::Initialize() {
  impl_->Initialize();
}

ggadget::BasicElement* ToolbarElement::AddButton(
    const char* button_name,
    ggadget::LinearElement::LayoutDirection direction,
    ButtonVisibilityType visibility) {
  return impl_->AddButton(button_name, direction, visibility);
}

bool ToolbarElement::RemoveButton(const char* button_name) {
  return impl_->RemoveButton(button_name);
}

void ToolbarElement::SetCollapsed(bool is_collapsed) {
  impl_->SetCollapsed(is_collapsed);
}

bool ToolbarElement::IsCollapsed() {
  return impl_->IsCollapsed();
}

void ToolbarElement::DoClassRegister() {
  ggadget::LinearElement::DoClassRegister();

  RegisterProperty("collapsed",
                   ggadget::NewSlot(&ToolbarElement::IsCollapsed),
                   ggadget::NewSlot(&ToolbarElement::SetCollapsed));
}

}  // namespace skin
}  // namespace ime_goopy
