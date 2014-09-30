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

#include "skin/candidate_element.h"

#include <string>
#include "third_party/google_gadgets_for_linux/ggadget/button_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/elements.h"
#include "third_party/google_gadgets_for_linux/ggadget/event.h"
#include "third_party/google_gadgets_for_linux/ggadget/img_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/label_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/linear_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/menu_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_event.h"
#include "third_party/google_gadgets_for_linux/ggadget/small_object.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_frame.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"

namespace {

// Scriptable event signals name.
const char kCandidateSelectSignal[] = "oncandidateselect";
const char kCandidateMenuSignal[] = "oncandidatemenu";

}  // namespace

namespace ime_goopy {
namespace skin {

class CandidateElement::Impl : public ggadget::SmallObject<> {
 public:
  Impl(ggadget::View* view, const char* name, CandidateElement* owner);
  ~Impl();

  void OnCandidateMenuEvent();
  void OnCandidateClicked();
  void OnCandidateMenuClicked();

  // Called when mouse over/out.
  void OnMouseOver();
  void OnMouseOut();
  void SetMenuIconOpacity();

  CandidateElement* owner_;
  uint32_t id_;

  // Indicate if mouse in certain element.
  uint32_t mouse_over_counter_;

  // Sub UI elements.
  ggadget::LabelElement* text_element_;
  ggadget::ButtonElement* menu_icon_element_;

  // Scriptable event signal.
  // Fired if candidate is selected when:
  // - any sub element clicked.
  // - any sub element right clicked.
  ggadget::Signal2<void, uint32_t, bool> on_candidate_selected_event_;

  // Fired if candidate require a pop-out menu when:
  // - elements other than image menu is right clicked.
  // - image menu clicked.
  ggadget::Signal2<void, uint32_t, ggadget::MenuInterface*>
      on_candidate_menu_event_;
};

CandidateElement::Impl::Impl(ggadget::View* view,
                             const char* name,
                             CandidateElement* owner)
    : owner_(owner),
      id_(0),
      mouse_over_counter_(0),
      text_element_(NULL),
      menu_icon_element_(NULL) {
  owner_->SetHorizontalAutoSizing(true);
  owner_->SetVerticalAutoSizing(true);
  owner_->SetEnabled(true);
  owner_->SetTextDirection(
      ggadget::BasicElement::TEXT_DIRECTION_INHERIT_FROM_PARENT);
  // Insert text element as owner's public child.
  text_element_ = new ggadget::LabelElement(view, "");
  text_element_->SetRelativeHeight(1.0);
  text_element_->SetTextDirection(
      ggadget::BasicElement::TEXT_DIRECTION_INHERIT_FROM_PARENT);
  owner_->GetChildren()->AppendElement(text_element_);

  // Insert image menu element.
  menu_icon_element_ = new ggadget::ButtonElement(view, "");
  menu_icon_element_->SetStretchMiddle(true);
  menu_icon_element_->SetRelativePinY(1.0);
  menu_icon_element_->SetRelativeY(1.0);
  menu_icon_element_->SetOpacity(0.0);
  menu_icon_element_->SetEnabled(true);
  owner_->GetChildren()->AppendElement(menu_icon_element_);
  owner_->SetChildLayoutDirection(
      menu_icon_element_, ggadget::LinearElement::LAYOUT_BACKWARD);

  owner_->SetEnabled(true);
  owner_->SetCursor(ggadget::ViewInterface::CURSOR_HAND);

  // Handle mouse over/out events for sub elements.
  owner_->ConnectOnMouseOverEvent(
      ggadget::NewSlot(this, &Impl::OnMouseOver));
  owner_->ConnectOnMouseOutEvent(
      ggadget::NewSlot(this, &Impl::OnMouseOut));
  menu_icon_element_->ConnectOnMouseOverEvent(
      ggadget::NewSlot(this, &Impl::OnMouseOver));
  menu_icon_element_->ConnectOnMouseOutEvent(
      ggadget::NewSlot(this, &Impl::OnMouseOut));

  // Connect context menu event for all elements.
  menu_icon_element_->ConnectOnContextMenuEvent(
      ggadget::NewSlot(this, &Impl::OnCandidateMenuEvent));
  owner_->ConnectOnContextMenuEvent(
      ggadget::NewSlot(this, &Impl::OnCandidateMenuEvent));

  // Connect menu icon click event.
  menu_icon_element_->ConnectOnClickEvent(
      ggadget::NewSlot(this, &Impl::OnCandidateMenuClicked));
  owner_->ConnectOnClickEvent(
      ggadget::NewSlot(this, &Impl::OnCandidateClicked));
}

CandidateElement::Impl::~Impl() {
}

void CandidateElement::Impl::OnCandidateMenuEvent() {
  // Get ggadget::MenuInterface.
  ggadget::ScriptableEvent* scriptable_event =
      owner_->GetView()->GetEvent();
  ASSERT(scriptable_event);
  const ggadget::Event* event = scriptable_event->GetEvent();
  ASSERT(event->GetType() == ggadget::Event::EVENT_CONTEXT_MENU);
  const ggadget::ContextMenuEvent* context_menu_event =
      static_cast<const ggadget::ContextMenuEvent*>(event);
  ASSERT(context_menu_event);

  ggadget::ScriptableMenu* scriptable_menu = context_menu_event->GetMenu();
  ASSERT(scriptable_menu);
  ggadget::MenuInterface* menu_interface = scriptable_menu->GetMenu();
  // Fire candidate context menu event.
  scriptable_menu->SetPositionHint(owner_);
  on_candidate_menu_event_(id_, menu_interface);
  scriptable_event->SetReturnValue(ggadget::EVENT_RESULT_CANCELED);
}

void CandidateElement::Impl::OnCandidateClicked() {
  ggadget::ScriptableEvent* scriptable_event = owner_->GetView()->GetEvent();
  scriptable_event->SetReturnValue(ggadget::EVENT_RESULT_HANDLED);
  on_candidate_selected_event_(id_, true);
}

void CandidateElement::Impl::OnCandidateMenuClicked() {
  owner_->GetView()->GetViewHost()->ShowContextMenu(
      ggadget::MouseEvent::BUTTON_LEFT);
}

void CandidateElement::Impl::OnMouseOver() {
  ++mouse_over_counter_;
  SetMenuIconOpacity();
}

void CandidateElement::Impl::OnMouseOut() {
  --mouse_over_counter_;
  SetMenuIconOpacity();
}

void CandidateElement::Impl::SetMenuIconOpacity() {
  menu_icon_element_->SetOpacity(mouse_over_counter_ > 0 ? 1.0 : 0.0);
}

CandidateElement::CandidateElement(ggadget::View* view, const char* name)
    : ggadget::LinearElement(view, "candidate", name),
      impl_(new Impl(view, name, this)) {
}

CandidateElement::~CandidateElement() {
  delete impl_;
  impl_ = NULL;
}

void CandidateElement::CalculateSize() {
  double text_width, text_height;
  impl_->text_element_->GetTextFrame()->SetRTL(IsTextRTL());
  impl_->text_element_->GetTextFrame()->GetSimpleExtents(
      &text_width, &text_height);
  impl_->text_element_->SetPixelWidth(text_width);
  impl_->text_element_->SetMinHeight(text_height);
  ggadget::LinearElement::CalculateSize();
}

void CandidateElement::DoClassRegister() {
  ggadget::LinearElement::DoClassRegister();

  RegisterProperty("id",
                   ggadget::NewSlot(&CandidateElement::GetId),
                   ggadget::NewSlot(&CandidateElement::SetId));
  RegisterProperty("text",
                   ggadget::NewSlot(&CandidateElement::GetText),
                   ggadget::NewSlot(&CandidateElement::SetText));
  RegisterProperty("menuWidth",
                   ggadget::NewSlot(&CandidateElement::GetMenuWidth),
                   ggadget::NewSlot(&CandidateElement::SetMenuWidth));
  RegisterProperty("menuHeight",
                   ggadget::NewSlot(&CandidateElement::GetMenuHeight),
                   ggadget::NewSlot(&CandidateElement::SetMenuHeight));
  RegisterProperty("menuImage",
                   ggadget::NewSlot(&CandidateElement::GetMenuIcon),
                   ggadget::NewSlot(&CandidateElement::SetMenuIcon));
  RegisterProperty("menuDownIcon",
                   ggadget::NewSlot(&CandidateElement::GetMenuDownIcon),
                   ggadget::NewSlot(&CandidateElement::SetMenuDownIcon));
  RegisterProperty("menuOverIcon",
                   ggadget::NewSlot(&CandidateElement::GetMenuOverIcon),
                   ggadget::NewSlot(&CandidateElement::SetMenuOverIcon));

  RegisterClassSignal(kCandidateSelectSignal,
                 &Impl::on_candidate_selected_event_,
                 &CandidateElement::impl_);
  RegisterClassSignal(kCandidateMenuSignal,
                 &Impl::on_candidate_menu_event_,
                 &CandidateElement::impl_);
}

uint32_t CandidateElement::GetId() const {
  return impl_->id_;
}

void CandidateElement::SetId(uint32_t id) {
  impl_->id_ = id;
}

std::string CandidateElement::GetText() const {
  return impl_->text_element_->GetTextFrame()->GetText();
}

void CandidateElement::SetText(const std::string& text) {
  impl_->text_element_->GetTextFrame()->SetText(text);
}

void CandidateElement::SetFormats(const ggadget::TextFormats& formats) {
  impl_->text_element_->GetTextFrame()->SetTextWithFormats(GetText(), formats);
}

void CandidateElement::SetDefaultFormat(
    const ggadget::TextFormat& default_format) {
  impl_->text_element_->GetTextFrame()->SetDefaultFormat(default_format);
}

double CandidateElement::GetMenuWidth() const {
  return impl_->menu_icon_element_->GetPixelWidth();
}

void CandidateElement::SetMenuWidth(double width) {
  impl_->menu_icon_element_->SetPixelWidth(width);
}

double CandidateElement::GetMenuHeight() const {
  return impl_->menu_icon_element_->GetPixelHeight();
}

void CandidateElement::SetMenuHeight(double height) {
  impl_->menu_icon_element_->SetPixelHeight(height);
}

ggadget::Variant CandidateElement::GetMenuIcon() const {
  return impl_->menu_icon_element_->GetImage();
}

void CandidateElement::SetMenuIcon(const ggadget::Variant& img) {
  impl_->menu_icon_element_->SetImage(img);
}

ggadget::Variant CandidateElement::GetMenuDownIcon() const {
  return impl_->menu_icon_element_->GetDownImage();
}

void CandidateElement::SetMenuDownIcon(const ggadget::Variant& img) {
  impl_->menu_icon_element_->SetDownImage(img);
}

ggadget::Variant CandidateElement::GetMenuOverIcon() const {
  return impl_->menu_icon_element_->GetOverImage();
}

void CandidateElement::SetMenuOverIcon(const ggadget::Variant& img) {
  impl_->menu_icon_element_->SetOverImage(img);
}

void CandidateElement::ConnectOnCandidateSelected(
    ggadget::Slot2<void, uint32_t, bool>* handler) {
  impl_->on_candidate_selected_event_.Connect(handler);
}

void CandidateElement::ConnectOnCandidateContextMenu(
    ggadget::Slot2<void, uint32_t, ggadget::MenuInterface*>* handler) {
  impl_->on_candidate_menu_event_.Connect(handler);
}

ggadget::BasicElement* CandidateElement::CreateInstance(
    ggadget::View* view, const char* name) {
  CandidateElement* candidate_element = new CandidateElement(view, name);

  return candidate_element;
}

}  // namespace skin
}  // namespace ime_goopy
