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

#include "listbox_element.h"
#include "color.h"
#include "elements.h"
#include "event.h"
#include "item_element.h"
#include "logger.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

// Change kErrorItemExpected from constant to macro so it can be used with LOG
#define kErrorItemExpected "Incorrect element type: Item/ListItem expected."

// Default values obtained from the Windows version.
static const Color kDefaultItemOverColor(0xDE/255.0, 0xFB/255.0, 1.0);
static const Color kDefaultItemSelectedColor(0xC6/255.0,
                                             0xF7/255.0,
                                             0xF7/255.0);
static const Color kDefaultItemSepColor(0xF7/255.0, 0xF3/255.0, 0xF7/255.0);

class ListBoxElement::Impl : public SmallObject<> {
 public:
  Impl(ListBoxElement *owner, View *view) :
    owner_(owner),
    item_over_color_(new Texture(kDefaultItemOverColor, 1.0)),
    item_selected_color_(new Texture(kDefaultItemSelectedColor, 1.0)),
    item_separator_color_(new Texture(kDefaultItemSepColor, 1.0)),
    item_width_(1.0),
    item_height_(0),
    selected_index_(-2),
    pending_scroll_(0),
    item_width_specified_(false),
    item_height_specified_(false),
    item_width_relative_(true),
    item_height_relative_(false),
    multiselect_(false),
    item_separator_(false) {
    GGL_UNUSED(view);
  }

  ~Impl() {
    delete item_over_color_;
    item_over_color_ = NULL;
    delete item_selected_color_;
    item_selected_color_ = NULL;
    delete item_separator_color_;
    item_separator_color_ = NULL;
  }

  void SetPixelItemWidth(double width) {
    if (width >= 0.0 &&
        (width != item_width_ || item_width_relative_)) {
      item_width_ = width;
      item_width_relative_ = false;
      owner_->QueueDraw();
    }
  }

  void SetPixelItemHeight(double height) {
    if (height >= 0.0 &&
        (height != item_height_ || item_height_relative_)) {
      item_height_ = height;
      item_height_relative_ = false;
      owner_->QueueDraw();
    }
  }

  void SetRelativeItemWidth(double width) {
    if (width >= 0.0 && (width != item_width_ || !item_width_relative_)) {
      item_width_ = width;
      item_width_relative_ = true;
      owner_->QueueDraw();
    }
  }

  void SetRelativeItemHeight(double height) {
    if (height >= 0.0 &&
        (height != item_height_ || !item_height_relative_)) {
      item_height_ = height;
      item_height_relative_ = true;
      owner_->QueueDraw();
    }
  }

  void SetPendingSelection() {
    BasicElement *selected =
      owner_->GetChildren()->GetItemByIndex(selected_index_);
    if (selected) {
      if (selected->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(selected);

        // ClearSelection() is called to work around a bug in a sample
        // test gadget. The selectedItem property of listbox/combobox takes
        // precedence over the selected property of individual items.
        ClearSelection(item); // ignore return

        item->SetSelected(true);
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }

  // Returns true if anything was cleared.
  bool ClearSelection(ItemElement *avoid) {
    bool result = false;
    Elements *elements = owner_->GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child != avoid) {
        if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
          ItemElement *item = down_cast<ItemElement *>(child);
          if (item->IsSelected()) {
            result = true;
            item->SetSelected(false);

            // Clear pending scroll flag to avoid unexpected scrolling to
            // the top of the list.
            pending_scroll_ = 0;
          }
        } else {
          LOG(kErrorItemExpected);
        }
      }
    }
    return result;
  }

  void FireOnChangeEvent() {
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, owner_, NULL);
    owner_->GetView()->FireEvent(&s_event, onchange_event_);
  }

  ItemElement *FindItemByString(const char *str) {
    Elements *elements = owner_->GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        std::string text = item->GetLabelText();
        if (text == str) {
          return item;
        }
      }
    }
    return NULL;
  }

  int GetSelectedIndex() {
    const Elements *elements = owner_->GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      const BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        const ItemElement *item = down_cast<const ItemElement *>(child);
        if (item->IsSelected())
          return static_cast<int>(i);
      } else {
        LOG(kErrorItemExpected);
      }
    }

    return selected_index_ >= 0 ? selected_index_ : -1;
  }

  void SetSelectedIndex(int index) {
    if (index == -1) {
      selected_index_ = -1;
      SetSelectedItem(NULL);
      return;
    }

    BasicElement *item =
        owner_->GetChildren()->GetItemByIndex(static_cast<size_t>(index));
    if (!item) {
      // Only occurs when initializing from XML, selectedIndex is set before
      // items are added.
      if (selected_index_ == -2) {
        // Mark selection as pending.
        selected_index_ = index;
      }
      return;
    }

    if (item->IsInstanceOf(ItemElement::CLASS_ID)) {
      SetSelectedItem(down_cast<ItemElement *>(item));
    } else {
      LOG(kErrorItemExpected);
    }
  }

  ItemElement *GetSelectedItem() {
    Elements *elements = owner_->GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        if (item->IsSelected()) {
          return item;
        }
      } else {
        LOG(kErrorItemExpected);
      }
    }
    return NULL;
  }

  void SetSelectedItem(ItemElement *item) {
    bool changed = ClearSelection(item);
    if (item && !item->IsSelected()) {
      item->SetSelected(true);
      pending_scroll_ = 2;
      changed = true;
    }

    if (changed)
      FireOnChangeEvent();
  }

  void ShiftSelection(int distance, bool wrap) {
    int count = static_cast<int>(owner_->GetChildren()->GetCount());
    if (count == 0)
      return;

    int index = GetSelectedIndex();
    if (index >= 0) {
      index += distance;
      if (wrap) {
        index %= count;
        if (index < 0)
          index += count;
      } else {
        index = std::max(0, std::min(count - 1, index));
      }
    } else {
      index = 0;
    }
    SetSelectedIndex(index);
  }

  int GetPageItemCount() {
    return static_cast<int>(owner_->GetPixelHeight() /
                            owner_->GetItemPixelHeight());
  }

  // Update scroll to show the currently selected item. Item rotation will
  // be ignored.
  bool HandlePendingScroll() {
    int scroll_position = owner_->GetScrollYPosition();
    int selected_index = owner_->GetSelectedIndex();
    double item_height = owner_->GetItemPixelHeight();
    switch (pending_scroll_) {
      case 1:
        scroll_position = static_cast<int>(selected_index * item_height);
        break;
      case 2: {
        double top = selected_index * item_height;
        if (top < scroll_position) {
          scroll_position = static_cast<int>(top);
        } else {
          double bottom = top + item_height;
          double height = owner_->GetClientHeight();
          if (bottom > scroll_position + height)
            scroll_position = static_cast<int>(bottom - height);
        }
        break;
      }
      default:
        break;
    }

    pending_scroll_ = 0;
    if (scroll_position != owner_->GetScrollYPosition()) {
      owner_->SetScrollYPosition(scroll_position);
      return true;
    }
    return false;
  }

  ListBoxElement *owner_;
  Texture *item_over_color_;
  Texture *item_selected_color_;
  Texture *item_separator_color_;
  double item_width_;
  double item_height_;
  EventSignal onchange_event_;

  // Only used for when the index is specified in XML. This is an index
  // of an element "pending" to become selected. Initialized to -2.
  int selected_index_;

  // 0: no pending scroll;
  // 1: scroll the selected item to top;
  // 2: minimal scroll to make the selected item into view.
  // The actual scrolling action will be taken in Layout(). This is important
  // for correct scrolling if some items are added and then SetSelectedItem(),
  // SetSelectedIndex() or ScrollToSelectedItem() is called.
  // called.
  unsigned int pending_scroll_ : 2;
  bool item_width_specified_   : 1;
  bool item_height_specified_  : 1;
  bool item_width_relative_    : 1;
  bool item_height_relative_   : 1;
  bool multiselect_            : 1;
  bool item_separator_         : 1;
};

ListBoxElement::ListBoxElement(View *view,
                               const char *tag_name, const char *name)
    : DivElement(view, tag_name, name),
      impl_(new Impl(this, view)) {
  SetEnabled(true);
  SetXScrollable(false);
}

void ListBoxElement::DoClassRegister() {
  DivElement::DoClassRegister();
  RegisterProperty("itemHeight",
                   NewSlot(&ListBoxElement::GetItemHeight),
                   NewSlot(&ListBoxElement::SetItemHeight));
  RegisterProperty("itemWidth",
                   NewSlot(&ListBoxElement::GetItemWidth),
                   NewSlot(&ListBoxElement::SetItemWidth));
  RegisterProperty("itemOverColor",
                   NewSlot(&ListBoxElement::GetItemOverColor),
                   NewSlot(&ListBoxElement::SetItemOverColor));
  RegisterProperty("itemSelectedColor",
                   NewSlot(&ListBoxElement::GetItemSelectedColor),
                   NewSlot(&ListBoxElement::SetItemSelectedColor));
  RegisterProperty("itemSeparator",
                   NewSlot(&ListBoxElement::HasItemSeparator),
                   NewSlot(&ListBoxElement::SetItemSeparator));
  RegisterProperty("multiSelect",
                   NewSlot(&ListBoxElement::IsMultiSelect),
                   NewSlot(&ListBoxElement::SetMultiSelect));
  RegisterProperty("selectedIndex",
                   NewSlot(&ListBoxElement::GetSelectedIndex),
                   NewSlot(&ListBoxElement::SetSelectedIndex));
  RegisterProperty("selectedItem",
                   NewSlot(&Impl::GetSelectedItem, &ListBoxElement::impl_),
                   NewSlot(&ListBoxElement::SetSelectedItem));

  RegisterMethod("clearSelection",
                 NewSlot(&ListBoxElement::ClearSelection));

  // Version 5.5 newly added methods and properties.
  RegisterProperty("itemSeparatorColor",
                   NewSlot(&ListBoxElement::GetItemSeparatorColor),
                   NewSlot(&ListBoxElement::SetItemSeparatorColor));
  RegisterMethod("appendString",
                 NewSlot(&ListBoxElement::AppendString));
  RegisterMethod("insertStringAt",
                 NewSlot(&ListBoxElement::InsertStringAt));
  RegisterMethod("removeString",
                 NewSlot(&ListBoxElement::RemoveString));

  RegisterClassSignal(kOnChangeEvent, &Impl::onchange_event_,
                      &ListBoxElement::impl_);
}

ListBoxElement::~ListBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void ListBoxElement::ScrollToSelectedItem() {
  impl_->pending_scroll_ = 1;
  QueueDraw();
}

Connection *ListBoxElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

EventResult ListBoxElement::HandleKeyEvent(const KeyboardEvent &event) {
  EventResult result = EVENT_RESULT_UNHANDLED;
  if (event.GetType() == Event::EVENT_KEY_DOWN) {
    result = EVENT_RESULT_HANDLED;
    switch (event.GetKeyCode()) {
      case KeyboardEvent::KEY_UP:
        impl_->ShiftSelection(-1, true);
        break;
      case KeyboardEvent::KEY_DOWN:
        impl_->ShiftSelection(1, true);
        break;
      case KeyboardEvent::KEY_PAGE_UP:
        impl_->ShiftSelection(-impl_->GetPageItemCount(), false);
        break;
      case KeyboardEvent::KEY_PAGE_DOWN:
        impl_->ShiftSelection(impl_->GetPageItemCount(), false);
        break;
      case KeyboardEvent::KEY_HOME:
        SetSelectedIndex(0);
        break;
      case KeyboardEvent::KEY_END:
        SetSelectedIndex(static_cast<int>(GetChildren()->GetCount()) - 1);
        break;
      default:
        result = EVENT_RESULT_UNHANDLED;
        break;
    }
  }
  return result;
}

Variant ListBoxElement::GetItemWidth() const {
  return BasicElement::GetPixelOrRelative(impl_->item_width_relative_,
                                          impl_->item_width_specified_,
                                          impl_->item_width_,
                                          impl_->item_width_);
}

void ListBoxElement::SetItemWidth(const Variant &width) {
  double v;
  switch (BasicElement::ParsePixelOrRelative(width, &v)) {
    case BasicElement::PR_PIXEL:
      impl_->item_width_specified_ = true;
      impl_->SetPixelItemWidth(v);
      break;
    case BasicElement::PR_RELATIVE:
      impl_->item_width_specified_ = true;
      impl_->SetRelativeItemWidth(v);
      break;
    case BasicElement::PR_UNSPECIFIED:
      impl_->item_width_specified_ = false;
      impl_->SetRelativeItemWidth(1.0);
      break;
    default:
      break;
  }
}

Variant ListBoxElement::GetItemHeight() const {
  return BasicElement::GetPixelOrRelative(impl_->item_height_relative_,
                                          impl_->item_height_specified_,
                                          impl_->item_height_,
                                          impl_->item_height_);
}

void ListBoxElement::SetItemHeight(const Variant &height) {
  double v;
  switch (BasicElement::ParsePixelOrRelative(height, &v)) {
    case BasicElement::PR_PIXEL:
      impl_->item_height_specified_ = true;
      impl_->SetPixelItemHeight(v);
      break;
    case BasicElement::PR_RELATIVE:
      impl_->item_height_specified_ = true;
      impl_->SetRelativeItemHeight(v);
      break;
    case BasicElement::PR_UNSPECIFIED:
      impl_->item_height_specified_ = false;
      impl_->SetPixelItemHeight(0);
      break;
    default:
      break;
  }
}

double ListBoxElement::GetItemPixelWidth() const {
  return impl_->item_width_relative_ ?
         impl_->item_width_ * GetClientWidth() : impl_->item_width_;
}

double ListBoxElement::GetItemPixelHeight() const {
  return impl_->item_height_relative_ ?
         impl_->item_height_ * GetClientHeight() : impl_->item_height_;
}

Variant ListBoxElement::GetItemOverColor() const {
  return Variant(Texture::GetSrc(impl_->item_over_color_));
}

const Texture *ListBoxElement::GetItemOverTexture() const {
  return impl_->item_over_color_;
}

void ListBoxElement::SetItemOverColor(const Variant &color) {
  if (color != GetItemOverColor()) {
    delete impl_->item_over_color_;
    impl_->item_over_color_ = GetView()->LoadTexture(color);

    Elements *elements = GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        if (item->IsMouseOver()) {
          item->QueueDraw();
          break;
        }
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }
}

Variant ListBoxElement::GetItemSelectedColor() const {
  return Variant(Texture::GetSrc(impl_->item_selected_color_));
}

const Texture *ListBoxElement::GetItemSelectedTexture() const {
  return impl_->item_selected_color_;
}

void ListBoxElement::SetItemSelectedColor(const Variant &color) {
  if (color != GetItemSelectedColor()) {
    delete impl_->item_selected_color_;
    impl_->item_selected_color_ = GetView()->LoadTexture(color);

    Elements *elements = GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        if (item->IsSelected()) {
          item->QueueDraw();
        }
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }
}

Variant ListBoxElement::GetItemSeparatorColor() const {
  return Variant(Texture::GetSrc(impl_->item_separator_color_));
}

const Texture *ListBoxElement::GetItemSeparatorTexture() const {
  return impl_->item_separator_color_;
}

void ListBoxElement::SetItemSeparatorColor(const Variant &color) {
  if (color != GetItemSeparatorColor()) {
    delete impl_->item_separator_color_;
    impl_->item_separator_color_ = GetView()->LoadTexture(color);

    Elements *elements = GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        item->QueueDraw();
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }
}

bool ListBoxElement::HasItemSeparator() const {
  return impl_->item_separator_;
}

void ListBoxElement::SetItemSeparator(bool separator) {
  if (separator != impl_->item_separator_) {
    impl_->item_separator_ = separator;

    Elements *elements = GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        item->QueueDraw();
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }
}

bool ListBoxElement::IsMultiSelect() const {
  return impl_->multiselect_;
}

void ListBoxElement::SetMultiSelect(bool multiselect) {
  impl_->multiselect_ = multiselect; // No need to redraw.
}

int ListBoxElement::GetSelectedIndex() const {
  return impl_->GetSelectedIndex();
}

void ListBoxElement::SetSelectedIndex(int index) {
  impl_->SetSelectedIndex(index);
}

ItemElement *ListBoxElement::GetSelectedItem() const {
  return impl_->GetSelectedItem();
}

void ListBoxElement::SetSelectedItem(ItemElement *item) {
  impl_->SetSelectedItem(item);
}

void ListBoxElement::ClearSelection() {
  if (impl_->ClearSelection(NULL))
    impl_->FireOnChangeEvent();
}

void ListBoxElement::AppendSelection(ItemElement *item) {
  ASSERT(item);

  if (!impl_->multiselect_) {
    SetSelectedItem(item);
    return;
  }

  if (!item->IsSelected()) {
    item->SetSelected(true);
    impl_->FireOnChangeEvent();
  }
}

void ListBoxElement::SelectRange(ItemElement *endpoint) {
  ASSERT(endpoint);

  if (!impl_->multiselect_) {
    SetSelectedItem(endpoint);
    return;
  }

  bool changed = false;
  ItemElement *endpoint2 = GetSelectedItem();
  if (endpoint2 == NULL || endpoint == endpoint2) {
    if (!endpoint->IsSelected()) {
      changed = true;
      endpoint->SetSelected(true);
    }
  } else {
    bool started = false;
    Elements *elements = GetChildren();
    size_t childcount = elements->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = elements->GetItemByIndex(i);
      if (child->IsInstanceOf(ItemElement::CLASS_ID)) {
        ItemElement *item = down_cast<ItemElement *>(child);
        if (item == endpoint || item == endpoint2) {
          started = !started;
          if (!started) { // started has just been turned off
            if (!item->IsSelected()) {
              item->SetSelected(true);
              changed = true;
            }
            break;
          }
        }

        if (started && !item->IsSelected()) {
          item->SetSelected(true);
          changed = true;
        }
      } else {
        LOG(kErrorItemExpected);
      }
    }
  }

  if (changed) {
    impl_->FireOnChangeEvent();
  }
}

bool ListBoxElement::AppendString(const char *str) {
  Elements *elements = GetChildren();
  BasicElement *child = elements->AppendElement("item", "");
  if (!child) {
    return false;
  }

  ASSERT(child->IsInstanceOf(ItemElement::CLASS_ID));
  ItemElement *item = down_cast<ItemElement *>(child);
  bool result = item->AddLabelWithText(str);
  if (!result) {
    // Cleanup on failure.
    elements->RemoveElement(child);
  }
  return result;
}

bool ListBoxElement::InsertStringAt(const char *str, size_t index) {
  Elements *elements = GetChildren();
  if (elements->GetCount() == index) {
    return AppendString(str);
  }

  const BasicElement *before = elements->GetItemByIndex(index);
  if (!before) {
    return false;
  }

  BasicElement *child = elements->InsertElement("item", before, "");
  if (!child) {
    return false;
  }

  ASSERT(child->IsInstanceOf(ItemElement::CLASS_ID));
  ItemElement *item = down_cast<ItemElement *>(child);
  bool result = item->AddLabelWithText(str);
  if (!result) {
    // Cleanup on failure.
    elements->RemoveElement(child);
  }
  return result;
}

void ListBoxElement::RemoveString(const char *str) {
  ItemElement *item = FindItemByString(str);
  if (item) {
    GetChildren()->RemoveElement(item);
  }
}

void ListBoxElement::Layout() {
  impl_->SetPendingSelection();
  // This field is no longer used after the first layout.
  impl_->selected_index_ = -1;

  // Call parent Layout() after SetIndex().
  DivElement::Layout();

  if (impl_->HandlePendingScroll()) {
    // Call Layout() again to let the scrollbar layout.
    DivElement::Layout();
  }

  // Set appropriate scrolling step distance.
  double item_height = GetItemPixelHeight();
  double box_height = GetClientHeight();
  double page_step = ::floor(box_height/item_height) * item_height;
  SetYPageStep(static_cast<int>(page_step > 0 ? page_step : box_height));
  SetYLineStep(static_cast<int>(std::min(item_height, box_height)));
}

ItemElement *ListBoxElement::FindItemByString(const char *str) {
  return impl_->FindItemByString(str);
}

const ItemElement *ListBoxElement::FindItemByString(const char *str) const {
  return impl_->FindItemByString(str);
}

BasicElement *ListBoxElement::CreateInstance(View *view, const char *name) {
  return new ListBoxElement(view, "listbox", name);
}

} // namespace ggadget
