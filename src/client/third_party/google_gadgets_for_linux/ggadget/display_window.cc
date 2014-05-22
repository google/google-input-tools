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

#include "display_window.h"
#include "basic_element.h"
#include "button_element.h"
#include "checkbox_element.h"
#include "combobox_element.h"
#include "div_element.h"
#include "edit_element_base.h"
#include "elements.h"
#include "item_element.h"
#include "label_element.h"
#include "listbox_element.h"
#include "logger.h"
#include "scriptable_array.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "text_frame.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

static const int kLabelTextSize = -1;  // Default size.
static const int kListItemHeight = 17;
static const char *kControlBorderColor = "#A0A0A0";
static const char *kBackgroundColor = "#FFFFFF";
static const int kMinComboBoxHeight = 80;
static const int kMaxComboBoxHeight = 150;

class DisplayWindow::Impl : public SmallObject<> {
 public:
  enum ButtonId {
    ID_OK = 1,
    ID_CANCEL = 2
  };

  enum ControlClass {
    CLASS_LABEL = 0,
    CLASS_EDIT = 1,
    CLASS_LIST = 2,
    CLASS_BUTTON = 3
  };

  enum ControlType {
    TYPE_NONE = 0,
    TYPE_LIST_OPEN = 0,
    TYPE_LIST_DROP = 1,
    TYPE_BUTTON_PUSH = 2,
    TYPE_BUTTON_CHECK = 3,
    TYPE_EDIT_PASSWORD = 10,
  };

  class Control : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x811cc6d8013643f4, ScriptableInterface);

    Control(DisplayWindow *window, BasicElement *element)
        : window_(window), element_(element),
          checkbox_clicked_(false) {
    }

    virtual void DoClassRegister() {
      // Incompatibility: we don't allow chaning id of a control.
      RegisterProperty("id",
                       NewSlot(&BasicElement::GetName, &Control::element_),
                       NULL);
      RegisterProperty("enabled",
                       NewSlot(&BasicElement::IsEnabled, &Control::element_),
                       NewSlot(&Control::SetEnabled));
      RegisterProperty("text",
                       NewSlot(&Control::GetText),
                       NewSlot(&Control::SetText));
      RegisterProperty("value",
                       NewSlot(&Control::GetValue),
                       NewSlot(&Control::SetValue));
      RegisterProperty("x", NULL, NewSlot(DummySetter));
      RegisterProperty("y", NULL, NewSlot(DummySetter));
      RegisterProperty("width", NULL, NewSlot(DummySetter));
      RegisterProperty("height", NULL, NewSlot(DummySetter));
      RegisterClassSignal("onChanged", &Control::onchanged_signal_);
      RegisterClassSignal("onClicked", &Control::onclicked_signal_);
    }

    ScriptableArray *GetListBoxItems(ListBoxElement *listbox) {
      ScriptableArray *array = new ScriptableArray();
      size_t count = listbox->GetChildren()->GetCount();
      for (size_t i = 0; i < count; ++i) {
        BasicElement *item = listbox->GetChildren()->GetItemByIndex(i);
        if (item->IsInstanceOf(ItemElement::CLASS_ID))
          array->Append(Variant(down_cast<ItemElement*>(item)->GetLabelText()));
      }
      return array;
    }

    void SetEnabled(bool enabled) {
      element_->SetEnabled(enabled);
      element_->SetOpacity(enabled ? 1.0 : 0.7);
    }

    // Gets the full content of the control.
    Variant GetText() {
      if (element_->IsInstanceOf(ButtonElement::CLASS_ID)) {
        ButtonElement *button = down_cast<ButtonElement *>(element_);
        return Variant(button->GetTextFrame()->GetText());
      }
      if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
        return Variant(checkbox->GetTextFrame()->GetText());
      }
      if (element_->IsInstanceOf(LabelElement::CLASS_ID)) {
        LabelElement *label = down_cast<LabelElement *>(element_);
        return Variant(label->GetTextFrame()->GetText());
      }
      if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
        ListBoxElement *listbox = down_cast<ListBoxElement *>(element_);
        return Variant(GetListBoxItems(listbox));
      }
      if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
        ComboBoxElement *combobox = down_cast<ComboBoxElement *>(element_);
        return Variant(GetListBoxItems(combobox->GetDroplist()));
      }
      if (element_->IsInstanceOf(EditElementBase::CLASS_ID)) {
        EditElementBase *edit = down_cast<EditElementBase *>(element_);
        return Variant(edit->GetValue());
      }
      ASSERT(false);
      return Variant();
    }

    static const int kMaxListItems = 512;
    void SetListBoxItems(ListBoxElement *listbox, ScriptableInterface *array) {
      listbox->GetChildren()->RemoveAllElements();
      if (array) {
        int length;
        if (array->GetProperty("length").v().ConvertToInt(&length)) {
          if (length > kMaxListItems)
            length = kMaxListItems;
          for (int i = 0; i < length; i++) {
            ResultVariant v = array->GetPropertyByIndex(i);
            std::string str_value;
            if (v.v().ConvertToString(&str_value)) {
              listbox->AppendString(str_value.c_str());
            } else {
              LOG("Invalid type of array item(%s) for control %s",
                  v.v().Print().c_str(), element_->GetName().c_str());
            }
          }
        }
      }
    }

    void SetText(const Variant &text) {
      bool invalid = false;
      if (text.type() == Variant::TYPE_SCRIPTABLE) {
        ScriptableInterface *array =
             VariantValue<ScriptableInterface *>()(text);
        if (array) {
          if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
            SetListBoxItems(down_cast<ListBoxElement *>(element_), array);
          } else if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
            SetListBoxItems(
                down_cast<ComboBoxElement *>(element_)->GetDroplist(), array);
          } else {
            invalid = true;
          }
        }
      } else {
        std::string text_str;
        if (text.ConvertToString(&text_str)) {
          if (element_->IsInstanceOf(EditElementBase::CLASS_ID)) {
            EditElementBase *edit = down_cast<EditElementBase *>(element_);
            edit->SetValue(text_str.c_str());
          } else {
            // Erase hotkey indicator in the string.
            size_t pos = text_str.find('&');
            if (pos != text_str.npos)
              text_str.erase(pos, 1);
            if (element_->IsInstanceOf(ButtonElement::CLASS_ID)) {
              ButtonElement *button = down_cast<ButtonElement *>(element_);
              button->GetTextFrame()->SetText(text_str.c_str());
            } else if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
              CheckBoxElement *checkbox =
                  down_cast<CheckBoxElement *>(element_);
              checkbox->GetTextFrame()->SetText(text_str.c_str());
            } else if (element_->IsInstanceOf(LabelElement::CLASS_ID)) {
              LabelElement *label = down_cast<LabelElement *>(element_);
              TextFrame *text_frame = label->GetTextFrame();
              text_frame->SetText(text_str.c_str());

              text_frame->SetSize(kLabelTextSize);
              // Shrink the font size if the given rect can't enclose the text.
              double text_width, text_height;
              text_frame->GetExtents(element_->GetPixelWidth(),
                                     &text_width, &text_height);
              if (text_height > element_->GetPixelHeight())
                text_frame->SetSize(kLabelTextSize - 1);
            } else if (element_->IsInstanceOf(EditElementBase::CLASS_ID)) {
              EditElementBase *edit = down_cast<EditElementBase *>(element_);
              edit->SetValue(text_str.c_str());
            } else {
              invalid = true;
            }
          }
        } else {
          invalid = true;
        }
      }

      if (invalid) {
        LOG("Invalid type of text(%s) for control %s",
            text.Print().c_str(), element_->GetName().c_str());
      }
    }

    std::string GetListBoxValue(ListBoxElement *listbox) {
      ItemElement *item = listbox->GetSelectedItem();
      return item ? item->GetLabelText() : std::string();
    }

    // The current value of the control.
    Variant GetValue() {
      if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        // For check box it is a boolean idicating the check state.
        CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
        return Variant(checkbox->GetValue());
      }
      if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
        ListBoxElement *listbox = down_cast<ListBoxElement *>(element_);
        return Variant(GetListBoxValue(listbox));
      }
      if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
        ComboBoxElement *combobox = down_cast<ComboBoxElement *>(element_);
        return Variant(GetListBoxValue(combobox->GetDroplist()));
      }
      // For others it is the displayed text.
      return GetText();
    }

    bool SetListBoxValue(ListBoxElement *listbox, const Variant &value) {
      std::string value_str;
      if (value.ConvertToString(&value_str)) {
        ItemElement *item = listbox->FindItemByString(value_str.c_str());
        if (item)
          listbox->SetSelectedItem(item);
        return true;
      }
      return false;
    }

    void SetValue(const Variant &value) {
      bool valid = true;
      if (element_->IsInstanceOf(LabelElement::CLASS_ID) ||
          element_->IsInstanceOf(EditElementBase::CLASS_ID)) {
        SetText(value);
      } else if (element_->IsInstanceOf(ListBoxElement::CLASS_ID)) {
        valid = SetListBoxValue(down_cast<ListBoxElement *>(element_), value);
      } else if (element_->IsInstanceOf(ComboBoxElement::CLASS_ID)) {
        ComboBoxElement *combobox = down_cast<ComboBoxElement *>(element_);
        valid = SetListBoxValue(combobox->GetDroplist(), value);
      } else if (element_->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        bool value_bool;
        if (value.ConvertToBool(&value_bool)) {
          CheckBoxElement *checkbox = down_cast<CheckBoxElement *>(element_);
          checkbox->SetValue(value_bool);
        } else {
          valid = false;
        }
      } else {
        valid = false;
      }
      if (!valid) {
        LOG("Invalid type of value(%s) for control %s",
            value.Print().c_str(), element_->GetName().c_str());
      }
    }

    void SetRect(int x, int y, int width, int height) {
      element_->SetPixelX(x);
      element_->SetPixelY(y);
      element_->SetPixelWidth(width);
      element_->SetPixelHeight(height);
    }

    void OnChange() {
      onchanged_signal_(window_, this);
    }

    void OnClick() {
      onclicked_signal_(window_, this);
    }

    void OnCheckBoxClick() {
      checkbox_clicked_ = true;
    }

    void OnCheckBoxChange() {
      if (checkbox_clicked_) {
        onclicked_signal_(window_, this);
        checkbox_clicked_ = false;
      }
    }

    void OnSize(DivElement *div) {
      div->SetPixelWidth(element_->GetPixelWidth() + 2);
      div->SetPixelHeight(element_->GetPixelHeight() + 2);
    }

    DisplayWindow *window_;
    BasicElement *element_;
    bool checkbox_clicked_;
    Signal2<void, DisplayWindow *, Control *> onchanged_signal_;
    Signal2<void, DisplayWindow *, Control *> onclicked_signal_;
  };

  Impl(DisplayWindow *owner, View *view)
      : owner_(owner), view_(view),
        min_x_(INT_MAX), min_y_(INT_MAX), max_x_(0), max_y_(0) {
  }

  ~Impl() {
    for (ControlsMap::iterator it = controls_.begin();
         it != controls_.end(); ++it) {
      delete it->second;
    }
    controls_.clear();
  }

  DivElement *CreateFrameDiv(Elements *elements) {
    DivElement *div = down_cast<DivElement *>(elements->AppendElement("div",
                                                                      NULL));
    div->SetBackground(Variant(kControlBorderColor));
    return div;
  }

  Control *AddControl(ControlClass ctrl_class, ControlType ctrl_type,
                      const char *ctrl_id, const Variant &text,
                      int x, int y, int width, int height) {
    Control *control = NULL;
    Elements *elements = view_->GetChildren();
    DivElement *div = NULL;  // Some elements may need a frame.

    switch (ctrl_class) {
      case CLASS_LABEL: {
        LabelElement *element = down_cast<LabelElement *>(
            elements->AppendElement("label", ctrl_id));
        TextFrame *text_frame = element->GetTextFrame();
        text_frame->SetWordWrap(true);
        text_frame->SetSize(kLabelTextSize);
        control = new Control(owner_, element);
        break;
      }
      case CLASS_EDIT: {
        // Our border is thinner than Windows version, so thrink the height.
        y += 1;
        height -= 2;
        div = CreateFrameDiv(elements);
        EditElementBase *element = down_cast<EditElementBase *>(
            elements->AppendElement("edit", ctrl_id));
        if (ctrl_type == TYPE_EDIT_PASSWORD)
          element->SetPasswordChar("*");
        control = new Control(owner_, element);
        element->ConnectOnChangeEvent(NewSlot(control, &Control::OnChange));
        break;
      }
      case CLASS_LIST:
        switch (ctrl_type) {
          default:
          case TYPE_LIST_OPEN: {
            div = CreateFrameDiv(elements);
            ListBoxElement *element = down_cast<ListBoxElement *>(
                elements->AppendElement("listbox", ctrl_id));
            element->SetItemWidth(Variant("100%"));
            element->SetItemHeight(Variant(kListItemHeight));
            element->SetAutoscroll(true);
            element->SetBackground(Variant(kBackgroundColor));
            control = new Control(owner_, element);
            element->ConnectOnChangeEvent(NewSlot(control, &Control::OnChange));
            break;
          }
          case TYPE_LIST_DROP: {
            div = CreateFrameDiv(elements);
            ComboBoxElement *element = down_cast<ComboBoxElement *>(
                elements->AppendElement("combobox", ctrl_id));
            element->SetType(ComboBoxElement::COMBO_DROPLIST);
            element->GetDroplist()->SetItemWidth(Variant("100%"));
            element->GetDroplist()->SetItemHeight(Variant(kListItemHeight));
            element->SetBackground(Variant(kBackgroundColor));
            control = new Control(owner_, element);
            element->ConnectOnChangeEvent(NewSlot(control, &Control::OnChange));
            // Because our combobox can't pop out of the dialog box, we must
            // limit the height of the combobox
            height = std::max(std::min(height, kMaxComboBoxHeight),
                              kMinComboBoxHeight);
            break;
          }
        }
        break;
      case CLASS_BUTTON:
        switch (ctrl_type) {
          default:
          case TYPE_BUTTON_PUSH: {
            ButtonElement *element = down_cast<ButtonElement *>(
                elements->AppendElement("button", ctrl_id));
            element->SetDefaultRendering(true);
            element->GetTextFrame()->SetSize(kLabelTextSize);
            control = new Control(owner_, element);
            element->ConnectOnClickEvent(NewSlot(control, &Control::OnClick));
            break;
          }
          case TYPE_BUTTON_CHECK: {
            CheckBoxElement *element = down_cast<CheckBoxElement *>(
                elements->AppendElement("checkbox", ctrl_id));
            element->SetDefaultRendering(true);
            element->GetTextFrame()->SetSize(kLabelTextSize);
            // Default value of gadget checkbox element is false, but here
            // the default value should be false.
            element->SetValue(false);
            // The DisplayWindow expects the control has changed its value when
            // onClicked is fired, but our CheckBoxElement changes value after
            // "onclick", so the control must listen to the "onchange" event,
            // and check whether the change is caused by click or the program.
            control = new Control(owner_, element);
            element->ConnectOnClickEvent(NewSlot(control,
                                                 &Control::OnCheckBoxClick));
            element->ConnectOnChangeEvent(NewSlot(control,
                                                  &Control::OnCheckBoxChange));
            break;
          }
        }
        break;
      default:
        LOG("Unknown control class: %d", ctrl_class);
        break;
    }

    if (control) {
      if (div) {
        div->SetPixelX(x - 1);
        div->SetPixelY(y - 1);
        control->element_->ConnectOnSizeEvent(NewSlot(control,
                                                      &Control::OnSize, div));
      }
      control->SetRect(x, y, width, height);
      control->SetText(text);
      min_x_ = std::min(std::max(0, x), min_x_);
      min_y_ = std::min(std::max(0, y), min_y_);
      max_x_ = std::max(x + width, max_x_);
      max_y_ = std::max(y + height, max_y_);

      controls_.insert(make_pair(std::string(ctrl_id), control));
      return control;
    }

    return NULL;
  }

  Control *GetControl(const char *ctrl_id) {
    ControlsMap::iterator it = controls_.find(ctrl_id);
    return it == controls_.end() ? NULL : it->second;
  }

  void OnOk() {
    onclose_signal_(owner_, ID_OK);
  }

  void OnCancel() {
    onclose_signal_(owner_, ID_CANCEL);
  }

  DisplayWindow *owner_;
  View *view_;
  Signal2<void, DisplayWindow *, ButtonId> onclose_signal_;
  int min_x_, min_y_, max_x_, max_y_;
  typedef LightMultiMap<std::string, Control *,
                        GadgetStringComparator> ControlsMap;
  ControlsMap controls_;
};

DisplayWindow::DisplayWindow(View *view)
    : impl_(new Impl(this, view)) {
  ASSERT(view);
  view->ConnectOnOkEvent(NewSlot(impl_, &Impl::OnOk));
  view->ConnectOnCancelEvent(NewSlot(impl_, &Impl::OnCancel));
}

void DisplayWindow::DoClassRegister() {
  RegisterMethod("AddControl", NewSlot(&Impl::AddControl,
                                       &DisplayWindow::impl_));
  RegisterMethod("GetControl", NewSlot(&Impl::GetControl,
                                       &DisplayWindow::impl_));
  RegisterClassSignal("OnClose", &Impl::onclose_signal_, &DisplayWindow::impl_);
}

DisplayWindow::~DisplayWindow() {
  delete impl_;
  impl_ = NULL;
}

bool DisplayWindow::AdjustSize() {
  if (impl_->min_x_ != INT_MAX && impl_->min_y_ != INT_MAX) {
    // Add min_x_ and min_y_ to max_x_ and max_y_ to leave equal blank areas
    // along the four edges.
    impl_->view_->SetSize(impl_->max_x_ + impl_->min_x_,
                          impl_->max_y_ + impl_->min_y_);
    return true;
  }
  return false;
}

} // namespace ggadget
