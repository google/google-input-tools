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

#include <string>

#include "item_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "graphics_interface.h"
#include "label_element.h"
#include "listbox_element.h"
#include "combobox_element.h"
#include "logger.h"
#include "texture.h"
#include "text_frame.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class ItemElement::Impl : public SmallObject<> {
 public:
  Impl(ItemElement *owner)
    : owner_(owner),
      background_(NULL),
      selected_(false),
      mouseover_(false),
      drawoverlay_(true) {
  }

  ~Impl() {
    delete background_;
    background_ = NULL;
  }

  ListBoxElement *GetListBox() {
    BasicElement *parent = owner_->GetParentElement();
    if (parent && parent->IsInstanceOf(ListBoxElement::CLASS_ID))
      return down_cast<ListBoxElement *>(parent);
    LOG("Item element is not contained inside a parent of the correct type.");
    return NULL;
  }

  ComboBoxElement *GetComboBox() {
    ListBoxElement *listbox = GetListBox();
    if (!listbox)
      return NULL;

    BasicElement *grandparent = listbox->GetParentElement();
    if (grandparent && grandparent->IsInstanceOf(ComboBoxElement::CLASS_ID))
      return down_cast<ComboBoxElement *>(grandparent);
    return NULL;
  }

  ScriptableInterface *GetScriptableParent() {
    ComboBoxElement *combobox = GetComboBox();
    if (combobox)
      return combobox;
    BasicElement *parent = owner_->GetParentElement();
    return parent ? parent : owner_->GetView()->GetScriptable();
  }

  ItemElement *owner_;
  Texture *background_;

  bool selected_    : 1;
  bool mouseover_   : 1;
  bool drawoverlay_ : 1;
};

ItemElement::ItemElement(View *view, const char *tagname, const char *name)
    : BasicElement(view, tagname, name, true),
      impl_(new Impl(this)) {
  SetEnabled(true);
}

void ItemElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("parentElement",
                   NewSlot(&Impl::GetScriptableParent, &ItemElement::impl_),
                   NULL);
  RegisterProperty("background",
                   NewSlot(&ItemElement::GetBackground),
                   NewSlot(&ItemElement::SetBackground));
  RegisterProperty("selected",
                   NewSlot(&ItemElement::IsSelected),
                   NewSlot(&ItemElement::SetSelected));
  // Disable getters and setters for position and size because they are
  // automatically set by the parent. The script can still use offsetXXXX
  // to get actual place of items.
  RegisterProperty("x", NULL, NewSlot(DummySetter));
  RegisterProperty("y", NULL, NewSlot(DummySetter));
  RegisterProperty("width", NULL, NewSlot(DummySetter));
  RegisterProperty("height", NULL, NewSlot(DummySetter));
}

ItemElement::~ItemElement() {
  delete impl_;
  impl_ = NULL;
}

void ItemElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->background_) {
    impl_->background_->Draw(canvas, 0, 0, GetPixelWidth(), GetPixelHeight());
  }

  ListBoxElement *listbox = impl_->GetListBox();
  if (impl_->drawoverlay_ && listbox) {
    const Texture *overlay = NULL;
    if (impl_->selected_)
      overlay = listbox->GetItemSelectedTexture();
    if (!overlay && impl_->mouseover_)
      overlay = listbox->GetItemOverTexture();
    if (overlay)
      overlay->Draw(canvas, 0, 0, GetPixelWidth(), GetPixelHeight());
  }

  DrawChildren(canvas);

  if (impl_->drawoverlay_ && listbox && listbox->HasItemSeparator()) {
    const Texture *item_separator = listbox->GetItemSeparatorTexture();
    if (item_separator)
      item_separator->Draw(canvas, 0, GetPixelHeight() - 2, GetPixelWidth(), 2);
  }
}

void ItemElement::SetDrawOverlay(bool draw) {
  if (draw != impl_->drawoverlay_) {
    impl_->drawoverlay_ = draw;
  }
}

bool ItemElement::IsMouseOver() const {
  return impl_->mouseover_;
}

Variant ItemElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->background_));
}

void ItemElement::SetBackground(const Variant &background) {
  if (background != GetBackground()) {
    delete impl_->background_;
    impl_->background_ = GetView()->LoadTexture(background);
    QueueDraw();
  }
}

bool ItemElement::IsSelected() const {
  return impl_->selected_;
}

void ItemElement::SetSelected(bool selected) {
  if (impl_->selected_ != selected) {
    impl_->selected_ = selected;
    QueueDraw();
  }
}

std::string ItemElement::GetLabelText() const {
  const Elements *elements = GetChildren();
  size_t childcount = elements->GetCount();
  for (size_t i = 0; i < childcount; i++) {
    const BasicElement *e = elements->GetItemByIndex(i);
    if (e && e->IsInstanceOf(LabelElement::CLASS_ID)) {
      const LabelElement *label = down_cast<const LabelElement *>(e);
      return label->GetTextFrame()->GetText();
    }
  }

  LOG("Label element not found inside Item element %s", GetName().c_str());
  return std::string();
}

void ItemElement::SetLabelText(const char *text) {
  Elements *elements = GetChildren();
  size_t childcount = elements->GetCount();
  for (size_t i = 0; i < childcount; i++) {
    BasicElement *e = elements->GetItemByIndex(i);
    if (e && e->IsInstanceOf(LabelElement::CLASS_ID)) {
      LabelElement *label = down_cast<LabelElement *>(e);
      label->GetTextFrame()->SetText(text);
      return;
    }
  }

  LOG("Label element not found inside Item element %s", GetName().c_str());
}

bool ItemElement::AddLabelWithText(const char *text) {
  BasicElement *e = GetChildren()->AppendElement("label", "");
  if (e) {
    ASSERT(e->IsInstanceOf(LabelElement::CLASS_ID));
    LabelElement *label = down_cast<LabelElement *>(e);
    label->GetTextFrame()->SetText(text);

    QueueDraw();
    return true;
  }

  return false;
}

BasicElement *ItemElement::CreateInstance(View *view, const char *name) {
  return new ItemElement(view, "item", name);
}

BasicElement *ItemElement::CreateListItemInstance(View *view,
                                                  const char *name) {
  return new ItemElement(view, "listitem", name);
}

void ItemElement::GetDefaultSize(double *width, double *height) const {
  ListBoxElement *listbox = impl_->GetListBox();
  if (listbox) {
    *width  = listbox->GetItemPixelWidth();
    *height = listbox->GetItemPixelHeight();
  } else {
    *width = *height = 0;
  }
}

void ItemElement::GetDefaultPosition(double *x, double *y) const {
  *x = 0;
  *y = static_cast<double>(GetIndex()) * GetPixelHeight();
}

EventResult ItemElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_CLICK: {
      ListBoxElement *listbox = impl_->GetListBox();
      if (listbox) {
        ElementHolder self_holder(this);
        // Need to invoke selection through parent, since
        // parent knows about multiselect status.
        if (event.GetModifier() & Event::MODIFIER_SHIFT) {
          listbox->SelectRange(this);
        } else if (event.GetModifier() & Event::MODIFIER_CONTROL) {
          listbox->AppendSelection(this);
        } else {
          listbox->SetSelectedItem(this);
        }

        // If inside combobox, turn off droplist on click.
        if (self_holder.Get()) {
          ComboBoxElement *combobox = impl_->GetComboBox();
          if (combobox) {
            combobox->SetDroplistVisible(false);
            combobox->Focus();
          }
        }
      }
      break;
    }
    case Event::EVENT_MOUSE_OUT:
      impl_->mouseover_ = false;
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_OVER:
      impl_->mouseover_ = true;
      QueueDraw();
      break;
    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

EventResult ItemElement::HandleKeyEvent(const KeyboardEvent &event) {
  ListBoxElement *listbox = impl_->GetListBox();
  return listbox ? listbox->HandleKeyEvent(event) : EVENT_RESULT_UNHANDLED;
}

bool ItemElement::HasOpaqueBackground() const {
  return impl_->background_ ? impl_->background_->IsFullyOpaque() : false;
}

} // namespace ggadget
