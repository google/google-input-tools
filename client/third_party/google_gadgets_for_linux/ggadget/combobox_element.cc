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

#include <algorithm>

#include "combobox_element.h"
#include "canvas_interface.h"
#include "logger.h"
#include "edit_element_base.h"
#include "elements.h"
#include "event.h"
#include "listbox_element.h"
#include "math_utils.h"
#include "gadget_consts.h"
#include "image_interface.h"
#include "item_element.h"
#include "texture.h"
#include "scriptable_event.h"
#include "view.h"
#include "graphics_interface.h"
#include "element_factory.h"
#include "small_object.h"
#include "scrollbar_element.h"

namespace ggadget {

static const char *kTypeNames[] = {
  "dropdown", "droplist"
};

static const int kEditMargin = 2;

class Droplist : public ListBoxElement {
 public:
  Droplist(ComboBoxElement *combobox)
      : ListBoxElement(combobox->GetView(), "listbox", ""),
        combobox_(combobox),
        mouse_selection_mode_(false),
        item_over_color_(ListBoxElement::GetItemOverColor()) {
    SetParentElement(combobox);
  }

  virtual EventResult HandleKeyEvent(const KeyboardEvent &event) {
    if (mouse_selection_mode_ && event.GetType() == Event::EVENT_KEY_DOWN) {
      unsigned int code = event.GetKeyCode();
      if (code == KeyboardEvent::KEY_DOWN &&
          event.GetModifier() == Event::MODIFIER_CONTROL) {
        combobox_->SetDroplistVisible(true);
        return EVENT_RESULT_HANDLED;
      }
      // Select the mouse over item if the following key are pressed.
      if (code == KeyboardEvent::KEY_RETURN ||
          code == KeyboardEvent::KEY_UP || code == KeyboardEvent::KEY_DOWN ||
          code == KeyboardEvent::KEY_PAGE_UP ||
          code == KeyboardEvent::KEY_PAGE_DOWN) {
        BasicElement *mouse_over = GetView()->GetMouseOverElement();
        if (mouse_over && mouse_over->IsInstanceOf(ItemElement::CLASS_ID) &&
            mouse_over->GetParentElement()) {
          SetSelectedItem(down_cast<ItemElement *>(mouse_over));
        }
      }
      if (code == KeyboardEvent::KEY_RETURN ||
          code == KeyboardEvent::KEY_ESCAPE) {
        combobox_->SetDroplistVisible(false);
      }
    }
    return ListBoxElement::HandleKeyEvent(event);
  }

  // Set the selection mode whether selecting with mouse or keyboard.
  // If with mouse, draw the mouse over item using itemOverColor, otherwise,
  // draw the selected item using itemOverColor.
  void SetMouseSelectionMode(bool mode) {
    if (mode != mouse_selection_mode_) {
      mouse_selection_mode_ = mode;
      UpdateDroplistColors();
    }
  }

  void UpdateDroplistColors() {
    if (mouse_selection_mode_) {
      ListBoxElement::SetItemOverColor(item_over_color_.v());
      ListBoxElement::SetItemSelectedColor(Variant(""));
    } else {
      ListBoxElement::SetItemOverColor(Variant(""));
      ListBoxElement::SetItemSelectedColor(item_over_color_.v());
    }
  }

  void SetItemOverColor(const Variant &color) {
    item_over_color_ = ResultVariant(color);
    UpdateDroplistColors();
  }

  Variant GetItemOverColor() {
    Variant v = item_over_color_.v();
    // Don't return src if the color is from a ScriptableBinaryData object.
    return v.type() == Variant::TYPE_SCRIPTABLE ? Variant("") : v;
  }

  ComboBoxElement *combobox_;
  bool mouse_selection_mode_;
  ResultVariant item_over_color_;
};

class ComboBoxElement::Impl : public SmallObject<> {
 public:
  Impl(ComboBoxElement *owner, View *view)
      : owner_(owner),
        droplist_(new Droplist(owner)),
        edit_(NULL),
        button_up_img_(view->LoadImageFromGlobal(kComboArrow, false)),
        button_down_img_(view->LoadImageFromGlobal(kComboArrowDown, false)),
        button_over_img_(view->LoadImageFromGlobal(kComboArrowOver, false)),
        background_(NULL),
        selection_refchange_connection_(NULL),
        selection_update_connection_(NULL),
        max_items_(10),
        item_pixel_height_(0),
        button_over_(false),
        button_down_(false),
        update_edit_value_(true),
        edit_has_focus_(false) {
    droplist_->SetPixelX(0);
    droplist_->SetVisible(false);
    droplist_->SetAutoscroll(true);
    droplist_->ConnectOnChangeEvent(NewSlot(this, &Impl::SelectionChanged));
    // When user clicks the drop list, let the view give focus to this element.
    droplist_->ConnectOnFocusInEvent(
        NewSlot(implicit_cast<BasicElement *>(owner_), &BasicElement::Focus));
    view->OnElementAdd(droplist_); // ListBox is exposed to the View.

    CreateEdit(); // COMBO_DROPDOWN is default.
  }

  ~Impl() {
    // Close listbox before destroying it to prevent
    // ComboBoxElement::GetPixelHeight() from calling listbox methods.
    droplist_->SetVisible(false);
    owner_->GetView()->OnElementRemove(droplist_);
    delete droplist_;
    DeleteEdit();
    delete background_;
    DestroyImage(button_up_img_);
    DestroyImage(button_down_img_);
    DestroyImage(button_over_img_);
  }

  std::string GetSelectedText() {
    const ItemElement *item = droplist_->GetSelectedItem();
    if (item) {
      return item->GetLabelText();
    }
    return std::string();
  }

  void SetDroplistVisible(bool visible) {
    if (droplist_->IsVisible() != visible) {
      if (visible) {
        droplist_->SetMouseSelectionMode(false);
        droplist_->ScrollToSelectedItem();
        droplist_->SetVisible(true);
        if (!owner_->IsDesignerMode())
          owner_->GetView()->SetPopupElement(owner_);
      } else if (owner_->IsDesignerMode()) {
        owner_->OnPopupOff();
      } else {
        // OnPopupOff() handler will turn off listbox.
        owner_->GetView()->SetPopupElement(NULL);
      }
      owner_->PostSizeEvent();
    }
  }

  void CreateEdit() {
    ElementFactory *factory = owner_->GetView()->GetElementFactory();
    edit_ = down_cast<EditElementBase*>(
        factory->CreateElement("edit", owner_->GetView(), ""));
    edit_->SetParentElement(owner_);
    owner_->GetView()->OnElementAdd(edit_);
    update_edit_value_ = true;
    if (edit_) {
      edit_->ConnectOnChangeEvent(NewSlot(this, &Impl::TextChanged));
      edit_->ConnectOnFocusInEvent(NewSlot(this, &Impl::OnEditFocusIn));
      edit_->ConnectOnFocusOutEvent(NewSlot(this, &Impl::OnEditFocusOut));
    } else {
      LOG("Failed to create EditElement.");
    }
  }

  void DeleteEdit() {
    if (edit_) {
      owner_->GetView()->OnElementRemove(edit_);
      delete edit_;
      edit_ = NULL;
    }
  }

  void OnEditFocusIn() {
    edit_has_focus_ = true;
    // Let the view still see the combobox as the focused element.
    owner_->Focus();
  }

  void OnEditFocusOut() {
    edit_has_focus_ = false;
  }

  void TextChanged() {
    ElementHolder self_holder(owner_);
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, ontextchange_event_);
    if (self_holder.Get() && edit_ && GetSelectedText() != edit_->GetValue()) {
      droplist_->SetSelectedIndex(-1);
      if (self_holder.Get())
        update_edit_value_ = false;
    }
  }

  void SelectionChanged() {
    if (!edit_) {
      owner_->QueueDrawRect(Rectangle(0, 0, owner_->GetPixelWidth(),
                                      droplist_->GetPixelY()));
    } else {
      edit_->QueueDraw();
    }

    // From now on, draws the selected item using itemOverColor.
    droplist_->SetMouseSelectionMode(false);
    update_edit_value_ = true;

    // The source's destructor is being called.
    if (selection_refchange_connection_) {
      selection_refchange_connection_->Disconnect();
      selection_refchange_connection_ = NULL;
      selection_update_connection_->Disconnect();
      selection_update_connection_ = NULL;
    }
    ItemElement *item = droplist_->GetSelectedItem();
    if (item) {
      selection_refchange_connection_ = item->ConnectOnReferenceChange(
          NewSlot(this, &Impl::OnSelectionRefChange));
      selection_update_connection_ = item->ConnectOnContentChanged(
          NewSlot(this, &Impl::OnSelectionUpdate));
    }

    // Relay this event to combobox's listeners.
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, onchange_event_);
  }

  void SetListBoxHeight() {
    double height = std::max(
        0.0, owner_->BasicElement::GetPixelHeight() - item_pixel_height_);
    if (max_items_ > 0) {
      size_t items = std::min(droplist_->GetChildren()->GetCount(), max_items_);
      height = std::min(height,
                        static_cast<double>(items) * item_pixel_height_);
    }
    droplist_->SetPixelHeight(height);
  }

  ImageInterface *GetButtonImage() {
    if (button_down_) {
      return button_down_img_;
    } else if (button_over_) {
      return button_over_img_;
    } else {
      return button_up_img_;
    }
  }

  Rectangle GetButtonRect() {
    Rectangle rect;
    ImageInterface *img = GetButtonImage();
    if (img) {
      rect.w = img->GetWidth();
      rect.x = owner_->GetPixelWidth() - rect.w - 1;
      rect.h = item_pixel_height_ - 2;
      rect.y = 1;
    }
    return rect;
  }

  void MarkRedraw() {
    if (edit_)
      edit_->MarkRedraw();
    droplist_->MarkRedraw();
  }

  void OnSelectionRefChange(int ref_count, int change) {
    GGL_UNUSED(ref_count);
    if (change == 0) {
      // The source's destructor is being called.
      selection_refchange_connection_->Disconnect();
      selection_refchange_connection_ = NULL;
      selection_update_connection_ = NULL;
    }
  }

  void OnSelectionUpdate() {
    if (!edit_) {
      owner_->QueueDrawRect(Rectangle(0, 0, owner_->GetPixelWidth(),
                                      droplist_->GetPixelY()));
    }
  }

  ScrollBarElement *GetScrollBar() {
    return droplist_->GetScrollBar();
  }

  DEFINE_DELEGATE_GETTER(GetListBox, src->impl_->droplist_,
                         ComboBoxElement, ListBoxElement);

  ComboBoxElement *owner_;
  Droplist *droplist_;
  EditElementBase *edit_; // is NULL if and only if COMBO_DROPLIST mode
  ImageInterface *button_up_img_;
  ImageInterface *button_down_img_;
  ImageInterface *button_over_img_;
  Texture *background_;
  Connection *selection_refchange_connection_;
  Connection *selection_update_connection_;
  size_t max_items_;
  double item_pixel_height_;

  EventSignal onchange_event_;
  EventSignal ontextchange_event_;

  bool button_over_        : 1;
  bool button_down_        : 1;
  bool update_edit_value_  : 1;
  bool edit_has_focus_     : 1;
};

ComboBoxElement::ComboBoxElement(View *view, const char *name)
    : BasicElement(view, "combobox", name, false),
      impl_(new Impl(this, view)) {
  SetEnabled(true);
}

void ComboBoxElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("scrollbar",
                   NewSlot(&Impl::GetScrollBar, &ComboBoxElement::impl_),
                   NULL);
  RegisterProperty("background",
                   NewSlot(&ComboBoxElement::GetBackground),
                   NewSlot(&ComboBoxElement::SetBackground));
  RegisterProperty("itemHeight",
                   NewSlot(&ListBoxElement::GetItemHeight,
                           Impl::GetListBoxConst),
                   NewSlot(&ListBoxElement::SetItemHeight, Impl::GetListBox));
  RegisterProperty("itemWidth",
                   NewSlot(&ListBoxElement::GetItemWidth,
                           Impl::GetListBoxConst),
                   NewSlot(&ListBoxElement::SetItemWidth, Impl::GetListBox));
  RegisterProperty("itemOverColor",
                   NewSlot(&ComboBoxElement::GetItemOverColor),
                   NewSlot(&ComboBoxElement::SetItemOverColor));
  RegisterProperty("itemSeparator",
                   NewSlot(&ListBoxElement::HasItemSeparator,
                           Impl::GetListBoxConst),
                   NewSlot(&ListBoxElement::SetItemSeparator,
                           Impl::GetListBox));
  RegisterProperty("selectedIndex",
                   NewSlot(&ListBoxElement::GetSelectedIndex,
                           Impl::GetListBoxConst),
                   NewSlot(&ListBoxElement::SetSelectedIndex,
                           Impl::GetListBox));
  RegisterProperty("selectedItem",
                   NewSlot(&ListBoxElement::GetSelectedItem,
                           Impl::GetListBoxConst),
                   NewSlot(&ListBoxElement::SetSelectedItem, Impl::GetListBox));
  RegisterProperty("droplistVisible",
                   NewSlot(&ComboBoxElement::IsDroplistVisible),
                   NewSlot(&ComboBoxElement::SetDroplistVisible));
  RegisterProperty("maxDroplistItems",
                   NewSlot(&ComboBoxElement::GetMaxDroplistItems),
                   NewSlot(&ComboBoxElement::SetMaxDroplistItems));
  RegisterProperty("value",
                   NewSlot(&ComboBoxElement::GetValue),
                   NewSlot(&ComboBoxElement::SetValue));
  RegisterStringEnumProperty("type",
                             NewSlot(&ComboBoxElement::GetType),
                             NewSlot(&ComboBoxElement::SetType),
                             kTypeNames, arraysize(kTypeNames));

  RegisterMethod("clearSelection",
                 NewSlot(&ListBoxElement::ClearSelection, Impl::GetListBox));

  // Version 5.5 newly added methods and properties.
  RegisterProperty("itemSeparatorColor",
                   NewSlot(&ListBoxElement::GetItemSeparatorColor,
                           Impl::GetListBoxConst),
                   NewSlot(&ListBoxElement::SetItemSeparatorColor,
                           Impl::GetListBox));
  RegisterMethod("appendString",
                 NewSlot(&ListBoxElement::AppendString, Impl::GetListBox));
  RegisterMethod("insertStringAt",
                 NewSlot(&ListBoxElement::InsertStringAt, Impl::GetListBox));
  RegisterMethod("removeString",
                 NewSlot(&ListBoxElement::RemoveString, Impl::GetListBox));

  // Linux specific, not standard API:
  RegisterProperty("edit",
                   NewSlot(static_cast<EditElementBase *(ComboBoxElement::*)()>
                       (&ComboBoxElement::GetEdit)),
                   NULL);
  RegisterProperty("droplist",
                   NewSlot(static_cast<ListBoxElement *(ComboBoxElement::*)()>
                       (&ComboBoxElement::GetDroplist)),
                   NULL);

  RegisterClassSignal(kOnChangeEvent, &Impl::onchange_event_,
                      &ComboBoxElement::impl_);
  RegisterClassSignal(kOnTextChangeEvent, &Impl::ontextchange_event_,
                      &ComboBoxElement::impl_);
}

ComboBoxElement::~ComboBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ComboBoxElement::MarkRedraw() {
  BasicElement::MarkRedraw();
  impl_->MarkRedraw();
}

void ComboBoxElement::DoDraw(CanvasInterface *canvas) {
  bool expanded = impl_->droplist_->IsVisible();
  double elem_width = GetPixelWidth();

  if (impl_->background_) {
    // Crop before drawing background.
    double crop_height = impl_->item_pixel_height_;
    if (expanded) {
      crop_height += impl_->droplist_->GetPixelHeight();
    }
    impl_->background_->Draw(canvas, 0, 0, elem_width, crop_height);
  }

  if (impl_->edit_) {
    impl_->edit_->Draw(canvas);
  } else {
    // Draw item
    ItemElement *item = impl_->droplist_->GetSelectedItem();
    if (item) {
      item->SetDrawOverlay(false);
      // Support rotations, masks, etc. here. Windows version supports these,
      // but is this really intended?
      double rotation = item->GetRotation();
      double pinx = item->GetPixelPinX(), piny = item->GetPixelPinY();
      bool transform = (rotation != 0 || pinx != 0 || piny != 0);
      canvas->PushState();
      canvas->IntersectRectClipRegion(0, 0, elem_width,
                                      impl_->item_pixel_height_);
      if (transform) {
        canvas->RotateCoordinates(DegreesToRadians(rotation));
        canvas->TranslateCoordinates(-pinx, -piny);
      }

      GetView()->EnableClipRegion(false);
      item->Draw(canvas);
      GetView()->EnableClipRegion(true);

      canvas->PopState();
      item->SetDrawOverlay(true);
    }
  }

  // Draw button
  ImageInterface *img = impl_->GetButtonImage();
  if (img) {
    Rectangle rect = impl_->GetButtonRect();
    // Windows default color is 206 203 206 and leaves a 1px margin.
    canvas->DrawFilledRect(rect.x, rect.y, rect.w, rect.h,
                           Color::FromChars(206, 203, 206));
    img->Draw(canvas, rect.x,
              rect.y + (rect.h - img->GetHeight()) / 2);
  }

  // Draw listbox
  if (expanded) {
    canvas->TranslateCoordinates(0, impl_->item_pixel_height_);
    impl_->droplist_->Draw(canvas);
  }
}

EditElementBase *ComboBoxElement::GetEdit() {
  return impl_->edit_;
}

const EditElementBase *ComboBoxElement::GetEdit() const {
  return impl_->edit_;
}

ListBoxElement *ComboBoxElement::GetDroplist() {
  return impl_->droplist_;
}

const ListBoxElement *ComboBoxElement::GetDroplist() const {
  return impl_->droplist_;
}

Variant ComboBoxElement::GetItemOverColor() const {
  return impl_->droplist_->GetItemOverColor();
}

void ComboBoxElement::SetItemOverColor(const Variant &color) {
  impl_->droplist_->SetItemOverColor(color);
}

const Elements *ComboBoxElement::GetChildren() const {
  return impl_->droplist_->GetChildren();
}

Elements *ComboBoxElement::GetChildren() {
  return impl_->droplist_->GetChildren();
}

bool ComboBoxElement::IsDroplistVisible() const {
  return impl_->droplist_->IsVisible();
}

void ComboBoxElement::SetDroplistVisible(bool visible) {
  impl_->SetDroplistVisible(visible);
}

size_t ComboBoxElement::GetMaxDroplistItems() const {
  return impl_->max_items_;
}

void ComboBoxElement::SetMaxDroplistItems(size_t max_droplist_items) {
  if (max_droplist_items != impl_->max_items_) {
    impl_->max_items_ = max_droplist_items;
    QueueDraw();
  }
}

ComboBoxElement::Type ComboBoxElement::GetType() const {
  if (impl_->edit_) {
    return COMBO_DROPDOWN;
  } else {
    return COMBO_DROPLIST;
  }
}

void ComboBoxElement::SetType(Type type) {
  if (type == COMBO_DROPDOWN) {
    if (!impl_->edit_) {
      impl_->CreateEdit();
      QueueDraw();
    }
  } else if (impl_->edit_) {
    impl_->DeleteEdit();
    QueueDraw();
  }
}

std::string ComboBoxElement::GetValue() const {
  if (impl_->edit_) {
    return impl_->edit_->GetValue();
  } else {
    // The release notes are wrong here: the value property can be read
    // but not modified in droplist mode.
    return impl_->GetSelectedText();
  }
}

void ComboBoxElement::SetValue(const char *value) {
  if (impl_->edit_) {
    impl_->edit_->SetValue(value);
  }
  // The release notes are wrong here: the value property can be read
  // but not modified in droplist mode.
}

Variant ComboBoxElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->background_));
}

void ComboBoxElement::SetBackground(const Variant &background) {
  if (background != GetBackground()) {
    delete impl_->background_;
    impl_->background_ = GetView()->LoadTexture(background);
    QueueDraw();
  }
}

void ComboBoxElement::Layout() {
  BasicElement::Layout();
  impl_->item_pixel_height_ = impl_->droplist_->GetItemPixelHeight();
  double elem_width = GetPixelWidth();
  impl_->droplist_->SetPixelY(impl_->item_pixel_height_);
  impl_->droplist_->SetPixelWidth(elem_width);
  impl_->SetListBoxHeight();
  impl_->droplist_->Layout();
  if (impl_->edit_) {
    ImageInterface *img = impl_->GetButtonImage();
    impl_->edit_->SetPixelWidth(elem_width - (img ? img->GetWidth() : 0));
    impl_->edit_->SetPixelHeight(impl_->item_pixel_height_);

    if (impl_->update_edit_value_) {
      impl_->edit_->SetValue(impl_->GetSelectedText().c_str());
    }

    impl_->edit_->Layout();
  }
  impl_->update_edit_value_ = false;
}

EventResult ComboBoxElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                          BasicElement **fired_element,
                                          BasicElement **in_element,
                                          ViewInterface::HitTest *hittest) {
  if (direct) {
    // In case that mouse clicked in area other than the edit and drop list.
    return BasicElement::OnMouseEvent(event, direct, fired_element,
                                      in_element, hittest);
  }

  // From now on, draws the mouse over item using itemOverColor.
  impl_->droplist_->SetMouseSelectionMode(true);

  double x = event.GetX();
  double y = event.GetY();
  double y_in_droplist = y - impl_->droplist_->GetPixelY();
  if (y_in_droplist < 0) {
    // kEditMargin around (inside) the edit box are excluded from the
    // edit box, to keep the same behavior as GDWin.
    if (impl_->edit_ && y >= kEditMargin && x >= kEditMargin &&
        y_in_droplist < -kEditMargin &&
        x < impl_->edit_->GetPixelWidth() - kEditMargin) {
      return impl_->edit_->OnMouseEvent(event, direct, fired_element,
                                        in_element, hittest);
    }
    return BasicElement::OnMouseEvent(event, direct, fired_element,
                                      in_element, hittest);
  }
  if (!impl_->droplist_->IsVisible()) {
    // The mouse is in the listbox area while the listbox is invisible.
    // This combobox will need to appear to be transparent for this area.
    return EVENT_RESULT_UNHANDLED;
  }

  // Send event to the drop list.
  MouseEvent new_event(event);
  new_event.SetY(y_in_droplist);
  return impl_->droplist_->OnMouseEvent(new_event, direct, fired_element,
                                        in_element, hittest);
}

EventResult ComboBoxElement::OnDragEvent(const DragEvent &event, bool direct,
                                     BasicElement **fired_element) {
  double new_y = event.GetY() - impl_->droplist_->GetPixelY();
  if (!direct) {
    if (new_y >= 0) {
      // In the listbox region.
      if (impl_->droplist_->IsVisible()) {
        DragEvent new_event(event);
        new_event.SetY(new_y);
        EventResult r = impl_->droplist_->OnDragEvent(new_event,
                                                      direct, fired_element);
        if (*fired_element == impl_->droplist_) {
          *fired_element = this;
        }
        return r;
      } else  {
        // This combobox will need to appear to be transparent to the elements
        // below it if listbox is invisible.
        return EVENT_RESULT_UNHANDLED;
      }
    } else if (impl_->edit_ && event.GetX() < impl_->edit_->GetPixelWidth()) {
      // In editbox.
      EventResult r = impl_->edit_->OnDragEvent(event, direct, fired_element);
      if (*fired_element == impl_->edit_) {
        *fired_element = this;
      }
      return r;
    }
  }

  return BasicElement::OnDragEvent(event, direct, fired_element);
}

EventResult ComboBoxElement::HandleMouseEvent(const MouseEvent &event) {
  // Only events NOT in the listbox region are ever passed to this handler.
  // So it's save to assume that these events are not for the listbox, with the
  // exception of mouse wheel events.
  EventResult r = EVENT_RESULT_HANDLED;
  double button_width =
      impl_->button_up_img_ ? impl_->button_up_img_->GetWidth() : 0;
  bool in_button = event.GetY() < impl_->droplist_->GetPixelY() &&
        event.GetX() >= (GetPixelWidth() - button_width);
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_MOVE:
      r = EVENT_RESULT_UNHANDLED;
      // Fall through.
    case Event::EVENT_MOUSE_OVER:
      if (impl_->button_over_ != in_button) {
        impl_->button_over_ = in_button;
        QueueDrawRect(impl_->GetButtonRect());
      }
      break;
    case Event::EVENT_MOUSE_UP:
      if (impl_->button_down_) {
        impl_->button_down_ = false;
        QueueDrawRect(impl_->GetButtonRect());
      }
      break;
    case Event::EVENT_MOUSE_DOWN:
      if (in_button && event.GetButton() & MouseEvent::BUTTON_LEFT) {
        impl_->button_down_ = true;
        QueueDrawRect(impl_->GetButtonRect());
      }
      break;
    case Event::EVENT_MOUSE_CLICK:
      // Toggle droplist visibility.
      SetDroplistVisible(!impl_->droplist_->IsVisible());
      break;
    case Event::EVENT_MOUSE_OUT:
      if (impl_->button_over_) {
        impl_->button_over_ = false;
        QueueDrawRect(impl_->GetButtonRect());
      }
      break;
    case Event::EVENT_MOUSE_WHEEL:
      if (impl_->droplist_->IsVisible()) {
        BasicElement *dummy1, *dummy2;
        ViewInterface::HitTest dummy3;
        r = impl_->droplist_->OnMouseEvent(event, true, &dummy1,
                                           &dummy2, &dummy3);
      } else {
        r = EVENT_RESULT_UNHANDLED;
      }
      break;
    default:
      r = EVENT_RESULT_UNHANDLED;
      break;
  }

  return r;
}

EventResult ComboBoxElement::HandleKeyEvent(const KeyboardEvent &event) {
  if (impl_->edit_ && impl_->edit_has_focus_ &&
      event.GetType() == Event::EVENT_KEY_DOWN) {
    unsigned int code = event.GetKeyCode();
    if (IsDroplistVisible()) {
      EventResult result = impl_->droplist_->HandleKeyEvent(event);
      if (result == EVENT_RESULT_UNHANDLED)
        return impl_->edit_->OnKeyEvent(event);
      return result;
    }
    if (code != KeyboardEvent::KEY_UP && code != KeyboardEvent::KEY_DOWN) {
      return impl_->edit_->OnKeyEvent(event);
    }
  }
  return impl_->droplist_->HandleKeyEvent(event);
}

EventResult ComboBoxElement::HandleOtherEvent(const Event &event) {
  if (impl_->edit_) {
    Event::Type type = event.GetType();
    if ((type == Event::EVENT_FOCUS_IN && !impl_->edit_has_focus_) ||
        (type == Event::EVENT_FOCUS_OUT && impl_->edit_has_focus_)) {
      // Send a fake focus in/out event to the edit so that it can show/hide
      // the caret.
      return impl_->edit_->OnOtherEvent(event);
    }
  }
  return EVENT_RESULT_UNHANDLED;
}

void ComboBoxElement::AggregateMoreClipRegion(const Rectangle &boundary,
                                              ClipRegion *region) {
  if (impl_->droplist_)
    impl_->droplist_->AggregateClipRegion(boundary, region);
  if (impl_->edit_)
    impl_->edit_->AggregateClipRegion(boundary, region);
}

void ComboBoxElement::OnPopupOff() {
  QueueDraw();
  impl_->droplist_->SetVisible(false);
  PostSizeEvent();
}

double ComboBoxElement::GetPixelHeight() const {
  return impl_->item_pixel_height_ +
      (impl_->droplist_->IsVisible() ? impl_->droplist_->GetPixelHeight() : 0);
}

bool ComboBoxElement::IsChildInVisibleArea(const BasicElement *child) const {
  ASSERT(child);

  if (child == impl_->edit_)
    return true;
  else if (child == impl_->droplist_)
    return impl_->droplist_->IsVisible();

  return impl_->droplist_->IsVisible() &&
         impl_->droplist_->IsChildInVisibleArea(child);
}

bool ComboBoxElement::HasOpaqueBackground() const {
  return impl_->background_ ? impl_->background_->IsFullyOpaque() : false;
}

Connection *ComboBoxElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

bool ComboBoxElement::IsTabStopDefault() const {
  return impl_->edit_ != NULL;
}

BasicElement *ComboBoxElement::CreateInstance(View *view, const char *name) {
  return new ComboBoxElement(view, name);
}

} // namespace ggadget
