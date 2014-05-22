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
#include <cmath>
#include "contentarea_element.h"
#include "canvas_interface.h"
#include "color.h"
#include "content_item.h"
#include "event.h"
#include "gadget_consts.h"
#include "graphics_interface.h"
#include "image_interface.h"
#include "main_loop_interface.h"
#include "menu_interface.h"
#include "messages.h"
#include "scriptable_array.h"
#include "scriptable_image.h"
#include "view.h"
#include "view_host_interface.h"
#include "signals.h"
#include "small_object.h"

namespace ggadget {

static const size_t kDefaultMaxContentItems = 25;
static const size_t kMaxContentItemsUpperLimit = 500;
static const Color kDefaultBackground(0.98, 0.98, 0.98);
static const Color kMouseOverBackground(0.83, 0.93, 0.98);
static const Color kMouseDownBackground(0.73, 0.83, 0.88);
static const Color kSelectedBackground(0.83, 0.93, 0.98);
static const unsigned int kRefreshInterval = 30000; // 30 seconds.

class ContentAreaElement::Impl : public SmallObject<> {
 public:
  typedef std::vector<ContentItem *> ContentItems;
  enum PinImageIndex {
    PIN_IMAGE_UNPINNED,
    PIN_IMAGE_UNPINNED_OVER,
    PIN_IMAGE_PINNED,
    PIN_IMAGE_COUNT,
  };

  Impl(ContentAreaElement *owner)
      : context_menu_time_(0),
        pin_image_max_width_(0),
        pin_image_max_height_(0),
        mouse_x_(-1.),
        mouse_y_(-1.),
        content_height_(0),
        background_opacity_(1.),
        mouseover_opacity_(1.),
        mousedown_opacity_(1.),
        background_color_(kDefaultBackground),
        mouseover_color_(kMouseOverBackground),
        mousedown_color_(kMouseDownBackground),
        owner_(owner),
        layout_canvas_(owner->GetView()->GetGraphics()->NewCanvas(5, 5)),
        target_connection_(NULL),
        mouse_over_item_(NULL),
        details_open_item_(NULL),
        death_detector_(NULL),
        background_color_src_(kDefaultBackground.ToString()),
        mouseover_color_src_(kMouseOverBackground.ToString()),
        mousedown_color_src_(kMouseDownBackground.ToString()),
        max_content_items_(kDefaultMaxContentItems),
        scrolling_line_step_(0),
        refresh_timer_(0),
        content_flags_(CONTENT_FLAG_NONE),
        target_(Gadget::TARGET_SIDEBAR),
        mouse_down_(false),
        mouse_over_pin_(false),
        modified_(false) {
    pin_images_[PIN_IMAGE_UNPINNED].Reset(new ScriptableImage(
        owner->GetView()->LoadImageFromGlobal(kContentItemUnpinned, false)));
    pin_images_[PIN_IMAGE_UNPINNED_OVER].Reset(new ScriptableImage(
        owner->GetView()->LoadImageFromGlobal(kContentItemUnpinnedOver, false)));
    pin_images_[PIN_IMAGE_PINNED].Reset(new ScriptableImage(
        owner->GetView()->LoadImageFromGlobal(kContentItemPinned, false)));

    // Schedule a interval timer to redraw the content area periodically,
    // to refresh the relative time stamps of the items.
    refresh_timer_ = owner->GetView()->SetInterval(
        NewSlot(this, &Impl::QueueDraw), kRefreshInterval);

    Gadget *gadget = GetGadget();
    if (gadget) {
      target_connection_ = gadget->ConnectOnDisplayTargetChanged(
          NewSlot(this, &Impl::DisplayTargetChangedHandler));
      target_ = gadget->GetDisplayTarget();
    }
  }

  ~Impl() {
    if (target_connection_)
      target_connection_->Disconnect();

    if (death_detector_) {
      // Inform the death detector that this element is dying.
      *death_detector_ = true;
    }

    owner_->GetView()->ClearInterval(refresh_timer_);
    refresh_timer_ = 0;
    RemoveAllContentItems();
    layout_canvas_->Destroy();
  }

  // Call through View::GetGadget() and downcast to Gadget*.
  Gadget *GetGadget() {
    GadgetInterface *gadget = owner_->GetView()->GetGadget();
    return (gadget && gadget->IsInstanceOf(Gadget::TYPE_ID)) ?
        down_cast<Gadget*>(gadget) : NULL;
  }

  void DisplayTargetChangedHandler(int target) {
    target_ = static_cast<Gadget::DisplayTarget>(target);
  }

  void QueueDraw() {
    owner_->QueueDraw();
  }

  // Called when content items are added, removed or reordered.
  void Modified() {
    modified_ = true;
    mouse_over_item_ = NULL;
    QueueDraw();
  }

  // A class used to queue a redraw in the next iteration. This is necessary
  // since ContentAreas, unlike other elements, may be modified during
  // drawing due to the ability to add custom draw handlers. When modified, an
  // immediate redraw necessary.
  class QueueDrawCallback : public WatchCallbackInterface {
   public:
    QueueDrawCallback(ContentAreaElement *area) : area_(area) { ASSERT(area); }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      area_->QueueDraw();
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      delete this;
    }
   private:
    ContentAreaElement *area_; // not owned by this object
  };


  void Layout() {
    if (content_flags_ & CONTENT_FLAG_PINNABLE) {
      if (pin_image_max_width_ == 0) {
        for (size_t i = 0; i < arraysize(pin_images_); i++) {
          const ImageInterface *image = pin_images_[i].Get()->GetImage();
          if (image) {
            pin_image_max_width_ = std::max(
                pin_image_max_width_, image->GetWidth());
            pin_image_max_height_ = std::max(
                pin_image_max_height_, image->GetHeight());
          }
        }
      }
    } else {
      pin_image_max_width_ = pin_image_max_height_ = 0;
    }

    double y = 0;
    double width = owner_->GetClientWidth();
    double item_width = width - pin_image_max_width_;

    // Add a modification checker to detect whether the set of content items
    // or this object itself is modified during the following loop. If such
    // things happen, break the loop and return to ensure safety.
    modified_ = false;
    bool dead = false;
    death_detector_ = &dead;

    content_height_ = 0;
    size_t item_count = content_items_.size();
    if (content_flags_ & CONTENT_FLAG_MANUAL_LAYOUT) {
      scrolling_line_step_ = 1;
      for (size_t i = 0; i < item_count && !dead && !modified_; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        double item_x, item_y, item_width, item_height;
        bool x_relative, y_relative, width_relative, height_relative;
        item->GetRect(&item_x, &item_y, &item_width, &item_height,
                      &x_relative, &y_relative,
                      &width_relative, &height_relative);
        if (dead)
          break;

        double client_width = owner_->GetClientWidth() / 100;
        double client_height = owner_->GetClientHeight() / 100;
        if (x_relative)
          item_x = item_x * client_width;
        if (y_relative)
          item_y = item_y * client_height;
        if (width_relative)
          item_width = item_width * client_width;
        if (height_relative)
          item_height = item_height * client_height;
        item->SetLayoutRect(item_x, item_y, item_width, item_height);
        content_height_ = std::max(content_height_, item_y + item_height);
      }
    } else {
      scrolling_line_step_ = 0;
      // Pinned items first.
      if (content_flags_ & CONTENT_FLAG_PINNABLE) {
        for (size_t i = 0; i < item_count && !dead && !modified_ ; i++) {
          ContentItem *item = content_items_[i];
          ASSERT(item);
          int item_flags = item->GetFlags();
          if (item_flags & ContentItem::CONTENT_ITEM_FLAG_HIDDEN) {
            item->SetLayoutRect(0, 0, 0, 0);
          } else if (item_flags & ContentItem::CONTENT_ITEM_FLAG_PINNED) {
            double item_height = item->GetHeight(target_, layout_canvas_,
                                                 item_width);
            if (dead)
              break;
            item_height = std::max(item_height, pin_image_max_height_);
            // Note: SetRect still uses the width including pin_image,
            // while Draw and GetHeight use the width excluding pin_image.
            item->SetLayoutRect(0, y, width, item_height);
            y += item_height;
            if (!scrolling_line_step_ || scrolling_line_step_ > item_height)
              scrolling_line_step_ = static_cast<int>(ceil(item_height));
          }
        }
      }
      // Then unpinned items.
      for (size_t i = 0; i < item_count && !dead && !modified_ ; i++) {
        ContentItem *item = content_items_[i];
        ASSERT(item);
        int item_flags = item->GetFlags();
        if (!(item_flags & ContentItem::CONTENT_ITEM_FLAG_HIDDEN) &&
            (!(content_flags_ & CONTENT_FLAG_PINNABLE) ||
             !(item_flags & ContentItem::CONTENT_ITEM_FLAG_PINNED))) {
          double item_height = item->GetHeight(target_, layout_canvas_,
                                            item_width);
          if (dead)
            break;
          item_height = std::max(item_height, pin_image_max_height_);
          // Note: SetRect still uses the width including pin_image,
          // while Draw and GetHeight use the width excluding pin_image.
          item->SetLayoutRect(0, y, width, item_height);
          y += item_height;
          if (!scrolling_line_step_ || scrolling_line_step_ > item_height)
            scrolling_line_step_ = static_cast<int>(ceil(item_height));
        }
      }
      if (!dead)
        content_height_ = y;
    }

    if (!dead)
      death_detector_ = NULL;

    if (modified_) {
      // Need to queue another draw.
      GetGlobalMainLoop()->AddTimeoutWatch(0, new QueueDrawCallback(owner_));
    }
  }

  void Draw(CanvasInterface *canvas) {
    double height = owner_->GetClientHeight();
    if (background_opacity_ > 0.) {
      if (background_opacity_ != 1.) {
        canvas->PushState();
        canvas->MultiplyOpacity(background_opacity_);
      }
      double width = owner_->GetClientWidth();
      canvas->DrawFilledRect(0, 0, width, height, background_color_);
      if (background_opacity_ != 1.) {
        canvas->PopState();
      }
    }

    // Add a modification checker to detect whether the set of content items
    // or this object itself is modified during the following loop. If such
    // things happen, break the loop and return to ensure safety.
    modified_ = false;
    bool dead = false;
    death_detector_ = &dead;

    size_t item_count = content_items_.size();
    for (size_t i = 0; i < item_count && !dead && !modified_; i++) {
      ContentItem *item = content_items_[i];
      ASSERT(item);
      if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HIDDEN)
        continue;

      double item_x = 0, item_y = 0, item_width = 0, item_height = 0;
      item->GetLayoutRect(&item_x, &item_y, &item_width, &item_height);
      item_x -= owner_->GetScrollXPosition();
      item_y -= owner_->GetScrollYPosition();
      if (item_width > 0 && item_height > 0 && item_y < height) {
        bool mouse_over = mouse_x_ != -1. && mouse_y_ != -1. &&
                          mouse_x_ >= item_x &&
                          mouse_x_ < item_x + item_width &&
                          mouse_y_ >= item_y &&
                          mouse_y_ < item_y + item_height;
        bool mouse_over_pin = false;

        if ((content_flags_ & CONTENT_FLAG_PINNABLE) &&
            pin_image_max_width_ > 0 && pin_image_max_height_ > 0) {
          mouse_over_pin = mouse_over && mouse_x_ < pin_image_max_width_;
          if (mouse_over_pin) {
            const Color &color = mouse_down_ ?
                                  mousedown_color_ : mouseover_color_;
            canvas->DrawFilledRect(item_x, item_y,
                                   pin_image_max_width_, item_height,
                                   color);
          }

          const ImageInterface *pin_image = NULL;
          if (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED) {
            pin_image = pin_images_[PIN_IMAGE_PINNED].Get()->GetImage();
          } else {
            pin_image = pin_images_[mouse_over_pin ? PIN_IMAGE_UNPINNED_OVER :
                                    PIN_IMAGE_UNPINNED].Get()->GetImage();
          }
          if (pin_image) {
            double image_width = pin_image->GetWidth();
            double image_height = pin_image->GetHeight();
            pin_image->Draw(canvas,
                            item_x + (pin_image_max_width_ - image_width) / 2,
                            item_y + (item_height - image_height) / 2);
          }
          item_x += pin_image_max_width_;
          item_width -= pin_image_max_width_;
        }

        if (mouse_over &&
            !(item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_STATIC)) {
          const Color &color = mouse_down_ && !mouse_over_pin ?
                                mousedown_color_ : mouseover_color_;
          canvas->DrawFilledRect(item_x, item_y,
                                 item_width, item_height, color);
        }
        item->Draw(target_, canvas, item_x, item_y, item_width, item_height);
      }
    }
    if (!dead)
      death_detector_ = NULL;

    if (modified_) {
      // Need to queue another draw.
      GetGlobalMainLoop()->AddTimeoutWatch(0, new QueueDrawCallback(owner_));
    }
  }

  ScriptableArray *ScriptGetContentItems() {
    return ScriptableArray::Create(content_items_.begin(),
                                   content_items_.end());
  }

  void ScriptSetContentItems(ScriptableInterface *array) {
    RemoveAllContentItems();
    if (array) {
      Variant length_v = array->GetProperty("length").v();
      int length;
      if (length_v.ConvertToInt(&length)) {
        if (static_cast<size_t>(length) > max_content_items_)
          length = static_cast<int>(max_content_items_);

        for (int i = 0; i < length; i++) {
          ResultVariant v = array->GetPropertyByIndex(i);
          if (v.v().type() == Variant::TYPE_SCRIPTABLE) {
            ContentItem *item = VariantValue<ContentItem *>()(v.v());
            if (item)
              AddContentItem(item, ITEM_DISPLAY_IN_SIDEBAR);
          }
        }
      }
    }
    QueueDraw();
  }

  void GetPinImages(ScriptableImage **unpinned,
                    ScriptableImage **unpinned_over,
                    ScriptableImage **pinned) {
    ASSERT(unpinned && unpinned_over && pinned);
    *unpinned = pin_images_[PIN_IMAGE_UNPINNED].Get();
    *unpinned_over = pin_images_[PIN_IMAGE_UNPINNED_OVER].Get();
    *pinned = pin_images_[PIN_IMAGE_PINNED].Get();
  }

  void SetPinImage(PinImageIndex index, ScriptableImage *image) {
    if (image)
      pin_images_[index].Reset(image);
  }

  void SetPinImages(ScriptableImage *unpinned,
                    ScriptableImage *unpinned_over,
                    ScriptableImage *pinned) {
    SetPinImage(PIN_IMAGE_UNPINNED, unpinned);
    SetPinImage(PIN_IMAGE_UNPINNED_OVER, unpinned_over);
    SetPinImage(PIN_IMAGE_PINNED, pinned);
    // To be updated in Layout().
    pin_image_max_width_ = pin_image_max_height_ = 0;
    QueueDraw();
  }

  ScriptableArray *ScriptGetPinImages() {
    ScriptableArray *array = new ScriptableArray();
    array->Append(Variant(pin_images_[PIN_IMAGE_UNPINNED].Get()));
    array->Append(Variant(pin_images_[PIN_IMAGE_UNPINNED_OVER].Get()));
    array->Append(Variant(pin_images_[PIN_IMAGE_PINNED].Get()));
    return array;
  }

  void ScriptSetPinImage(PinImageIndex index, const ResultVariant &v) {
    if (v.v().type() == Variant::TYPE_SCRIPTABLE)
      SetPinImage(index, VariantValue<ScriptableImage *>()(v.v()));
  }

  void ScriptSetPinImages(ScriptableInterface *array) {
    if (array) {
      ScriptSetPinImage(PIN_IMAGE_UNPINNED, array->GetPropertyByIndex(0));
      ScriptSetPinImage(PIN_IMAGE_UNPINNED_OVER, array->GetPropertyByIndex(1));
      ScriptSetPinImage(PIN_IMAGE_PINNED, array->GetPropertyByIndex(2));
    }
  }

  bool SetMaxContentItems(size_t max_content_items) {
    max_content_items = std::min(std::max(size_t(1), max_content_items),
                                 kMaxContentItemsUpperLimit);
    if (max_content_items_ != max_content_items) {
      max_content_items_ = max_content_items;
      if (RemoveExtraItems(content_items_.begin())) {
        Modified();
        return true;
      }
    }
    return false;
  }

  bool AddContentItem(ContentItem *item, DisplayOptions options) {
    GGL_UNUSED(options);
    ContentItems::iterator it = std::find(content_items_.begin(),
                                          content_items_.end(),
                                          item);
    if (it == content_items_.end()) {
      item->AttachContentArea(owner_);
      content_items_.insert(content_items_.begin(), item);
      RemoveExtraItems(content_items_.begin() + 1);
      Modified();
      return true;
    }
    return false;
  }

  bool RemoveExtraItems(ContentItems::iterator begin) {
    if (content_items_.size() <= max_content_items_)
      return false;

    bool all_pinned = false;
    while (content_items_.size() > max_content_items_) {
      ContentItems::iterator it = content_items_.end() - 1;
      if (!all_pinned && (content_flags_ & CONTENT_FLAG_PINNABLE)) {
        // Find the first unpinned item which can be removed. If can't find
        // anything last item will be removed.
        for (; it > begin; --it) {
          if (!((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED))
            break;
        }
        if (it == begin &&
            ((*it)->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_PINNED)) {
          all_pinned = true;
          it = content_items_.end() - 1;
        }
      }

      (*it)->DetachContentArea(owner_);
      content_items_.erase(it);
    }
    return true;
  }

  // Call through Gadget::CloseDetailsView();
  void CloseDetailsView() {
    Gadget *gadget = GetGadget();
    if (gadget)
      gadget->CloseDetailsView();
  }

  bool RemoveContentItem(ContentItem *item) {
    ContentItems::iterator it = std::find(content_items_.begin(),
                                          content_items_.end(),
                                          item);
    if (it != content_items_.end()) {
      if (*it == details_open_item_)
        CloseDetailsView();

      (*it)->DetachContentArea(owner_);
      content_items_.erase(it);
      Modified();
      return true;
    }
    return false;
  }

  void RemoveAllContentItems() {
    for (ContentItems::iterator it = content_items_.begin();
         it != content_items_.end(); ++it) {
      (*it)->DetachContentArea(owner_);
    }
    content_items_.clear();
    if (details_open_item_)
      CloseDetailsView();
    Modified();
  }

  class DetailsViewFeedbackHandler {
   public:
    DetailsViewFeedbackHandler(Impl *impl, ContentItem *item)
      : impl_(impl), item_(item), content_area_(impl->owner_) {
    }
    bool operator()(int flags) const {
      if (content_area_.Get() && item_.Get()) {
        impl_->details_open_item_ = NULL;
        if (flags & ViewInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN)
          impl_->OnItemOpen(item_.Get());
        if (flags & ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK)
          impl_->OnItemNegativeFeedback(item_.Get());
        if (flags & ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON)
          impl_->OnItemRemove(item_.Get());
      }
      return true;
    }
    // Not used.
    bool operator==(const DetailsViewFeedbackHandler &) const {
      return false;
    }
   private:
    Impl *impl_;
    ScriptableHolder<ContentItem> item_;
    ElementHolder content_area_;
  };

  EventResult HandleMouseEvent(const MouseEvent &event) {
    bool queue_draw = false;
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (event.GetType() == Event::EVENT_MOUSE_OUT) {
      mouse_over_pin_ = false;
      mouse_over_item_ = NULL;
      mouse_x_ = mouse_y_ = -1.;
      mouse_down_ = false;
      queue_draw = true;
      result = EVENT_RESULT_HANDLED;
    } else {
      mouse_x_ = event.GetX();
      mouse_y_ = event.GetY();
      ContentItem *new_mouse_over_item = NULL;
      bool tooltip_required = false;
      for (ContentItems::iterator it = content_items_.begin();
           it != content_items_.end(); ++it) {
        int flags = (*it)->GetFlags();
        if (!(flags &
                (ContentItem::CONTENT_ITEM_FLAG_HIDDEN |
                 ContentItem::CONTENT_ITEM_FLAG_STATIC))) {
          double x, y, w, h;
          (*it)->GetLayoutRect(&x, &y, &w, &h);
          x -= owner_->GetScrollXPosition();
          y -= owner_->GetScrollYPosition();
          if (mouse_x_ >= x && mouse_x_ < x + w &&
              mouse_y_ >= y && mouse_y_ < y + h) {
            new_mouse_over_item = *it;
            tooltip_required = new_mouse_over_item->IsTooltipRequired(
                target_, layout_canvas_, x, y, w, h);
            break;
          }
        }
      }

      bool new_mouse_over_pin = (mouse_x_ < pin_image_max_width_);
      if (mouse_over_item_ != new_mouse_over_item) {
        mouse_over_item_ = new_mouse_over_item;
        mouse_over_pin_ = new_mouse_over_pin;
        std::string tooltip;
        if (tooltip_required)
          tooltip = new_mouse_over_item->GetTooltip();
        // Store the tooltip to let view display it when appropriate using
        // the default mouse-in logic.
        owner_->SetTooltip(tooltip);
        // Display the tooltip now, because view only displays tooltip when
        // the mouse-in element changes.
        owner_->GetView()->ShowElementTooltip(owner_);
        queue_draw = true;
      } else if (new_mouse_over_pin != mouse_over_pin_) {
        mouse_over_pin_ = new_mouse_over_pin;
        queue_draw = true;
      }

      if (event.GetType() != Event::EVENT_MOUSE_MOVE &&
          (event.GetButton() & MouseEvent::BUTTON_LEFT)) {
        result = EVENT_RESULT_HANDLED;

        switch (event.GetType()) {
          case Event::EVENT_MOUSE_DOWN:
            mouse_down_ = true;
            queue_draw = true;
            break;
          case Event::EVENT_MOUSE_UP:
            mouse_down_ = false;
            queue_draw = true;
            break;
          case Event::EVENT_MOUSE_CLICK:
            if (mouse_over_item_) {
              if (mouse_over_pin_) {
                mouse_over_item_->ToggleItemPinnedState();
              } else if (content_flags_ & CONTENT_FLAG_HAVE_DETAILS) {
                if (mouse_over_item_ == details_open_item_) {
                  CloseDetailsView();
                } else {
                  std::string title;
                  DetailsViewData *details_view_data = NULL;
                  int flags = 0;
                  Gadget *gadget = GetGadget();
                  if (gadget && !mouse_over_item_->OnDetailsView(
                         &title, &details_view_data, &flags) &&
                      details_view_data) {
                    gadget->ShowDetailsView(
                        details_view_data, title.c_str(), flags,
                        NewFunctorSlot<bool, int>(DetailsViewFeedbackHandler(
                            this, mouse_over_item_)));
                    details_open_item_ = mouse_over_item_;
                  }
                }
              }
            }
            break;
          case Event::EVENT_MOUSE_DBLCLICK:
            if (mouse_over_item_ && !mouse_over_pin_)
              mouse_over_item_->OpenItem();
            break;
          default:
            result = EVENT_RESULT_UNHANDLED;
            break;
        }
      }
    }

    if (queue_draw)
      QueueDraw();
    return result;
  }

  // Handler of the "Open" menu item.
  void OnItemOpen(ContentItem *item) {
    if (item)
      item->OpenItem();
  }

  // Handler of the "Remove" menu item.
  void OnItemRemove(ContentItem *item) {
    if (item) {
      bool dead = false;
      death_detector_ = &dead;
      if (!item->ProcessDetailsViewFeedback(
            ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON) &&
          !dead && !item->OnUserRemove() && !dead) {
        RemoveContentItem(item);
      }
      if (!dead)
        death_detector_ = NULL;
    }
  }

  // Handler of the "Don't show me ..." menu item.
  void OnItemNegativeFeedback(ContentItem *item) {
    if (item) {
      bool dead = false;
      death_detector_ = &dead;
      if (!item->ProcessDetailsViewFeedback(
            ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK) && !dead) {
        RemoveContentItem(item);
      }
      if (!dead)
        death_detector_ = NULL;
    }
  }

  class MenuItemHandler {
   public:
    MenuItemHandler(Impl *impl, void (Impl::*method)(ContentItem *),
                    ContentItem *item)
      : impl_(impl), method_(method), item_(item), content_area_(impl->owner_) {
    }
    void operator()(const char *) const {
      if (content_area_.Get() && item_.Get()) {
        // Make sure that openUrl can work.
        bool old_interaction = false;
        GadgetInterface *gadget = impl_->owner_->GetView()->GetGadget();
        if (gadget)
          old_interaction = gadget->SetInUserInteraction(true);
        (impl_->*method_)(item_.Get());
        if (gadget)
          gadget->SetInUserInteraction(old_interaction);
      }
    }
    // Not used.
    bool operator==(const MenuItemHandler &) const {
      return false;
    }
   private:
    Impl *impl_;
    void (Impl::*method_)(ContentItem *);
    ScriptableHolder<ContentItem> item_;
    ElementHolder content_area_;
  };

  bool OnAddContextMenuItems(MenuInterface *menu) {
    if (mouse_over_item_) {
      int item_flags = mouse_over_item_->GetFlags();
      if (!(item_flags & ContentItem::CONTENT_ITEM_FLAG_STATIC)) {
        if (mouse_over_item_->CanOpen()) {
          menu->AddItem(GM_("OPEN_CONTENT_ITEM"), 0,
                        MenuInterface::MENU_ITEM_ICON_OPEN,
                        NewFunctorSlot<void, const char *>(
                            MenuItemHandler(this, &Impl::OnItemOpen,
                                            mouse_over_item_)),
                        MenuInterface::MENU_ITEM_PRI_CLIENT);
        }
        if (!(item_flags & ContentItem::CONTENT_ITEM_FLAG_NO_REMOVE)) {
          menu->AddItem(GM_("REMOVE_CONTENT_ITEM"), 0,
                        MenuInterface::MENU_ITEM_ICON_DELETE,
                        NewFunctorSlot<void, const char *>(
                            MenuItemHandler(this, &Impl::OnItemRemove,
                                            mouse_over_item_)),
                        MenuInterface::MENU_ITEM_PRI_CLIENT);
        }
        if (item_flags & ContentItem::CONTENT_ITEM_FLAG_NEGATIVE_FEEDBACK) {
          menu->AddItem(GM_("DONT_SHOW_CONTENT_ITEM"), 0,
                        MenuInterface::MENU_ITEM_ICON_NO,
                        NewFunctorSlot<void, const char *>(
                            MenuItemHandler(this, &Impl::OnItemNegativeFeedback,
                                            mouse_over_item_)),
                        MenuInterface::MENU_ITEM_PRI_CLIENT);
        }
      }
    }
    // To keep compatible with the Windows version, don't show default menu
    // items.
    return false;
  }

  uint64_t context_menu_time_;
  double pin_image_max_width_;
  double pin_image_max_height_;
  double mouse_x_, mouse_y_;
  double content_height_;
  double background_opacity_;
  double mouseover_opacity_;
  double mousedown_opacity_;
  Color background_color_;
  Color mouseover_color_;
  Color mousedown_color_;

  ContentAreaElement *owner_;
  CanvasInterface *layout_canvas_; // Only used during Layout().
  Connection *target_connection_;
  ContentItem *mouse_over_item_;
  ContentItem *details_open_item_;
  bool *death_detector_;
  ContentItems content_items_;
  ScriptableHolder<ScriptableImage> pin_images_[PIN_IMAGE_COUNT];
  std::string background_color_src_;
  std::string mouseover_color_src_;
  std::string mousedown_color_src_;
  size_t max_content_items_;
  int scrolling_line_step_;
  int refresh_timer_;

  ContentFlag content_flags_    : 4;
  Gadget::DisplayTarget target_ : 2;
  bool mouse_down_              : 1;
  bool mouse_over_pin_          : 1;
  // Flags whether items were added, removed or reordered.
  bool modified_                : 1;
};

ContentAreaElement::ContentAreaElement(View *view, const char *name)
    : ScrollingElement(view, "contentarea", name, false),
      impl_(new Impl(this)) {
  SetEnabled(true);
  SetAutoscroll(true);
}

void ContentAreaElement::DoClassRegister() {
  ScrollingElement::DoClassRegister();
  RegisterProperty("contentFlags", NULL, // Write only.
                   NewSlot(&ContentAreaElement::SetContentFlags));
  RegisterProperty("maxContentItems",
                   NewSlot(&ContentAreaElement::GetMaxContentItems),
                   NewSlot(&ContentAreaElement::SetMaxContentItems));
  RegisterProperty("backgroundColor",
                   NewSlot(&ContentAreaElement::GetBackgroundColor),
                   NewSlot(&ContentAreaElement::SetBackgroundColor));
  RegisterProperty("overColor",
                   NewSlot(&ContentAreaElement::GetOverColor),
                   NewSlot(&ContentAreaElement::SetOverColor));
  RegisterProperty("downColor",
                   NewSlot(&ContentAreaElement::GetDownColor),
                   NewSlot(&ContentAreaElement::SetDownColor));
  RegisterProperty("contentItems",
                   NewSlot(&Impl::ScriptGetContentItems,
                           &ContentAreaElement::impl_),
                   NewSlot(&Impl::ScriptSetContentItems,
                           &ContentAreaElement::impl_));
  RegisterProperty("pinImages",
                   NewSlot(&Impl::ScriptGetPinImages,
                           &ContentAreaElement::impl_),
                   NewSlot(&Impl::ScriptSetPinImages,
                           &ContentAreaElement::impl_));
  RegisterMethod("addContentItem",
                 NewSlot(&ContentAreaElement::AddContentItem));
  RegisterMethod("removeContentItem",
                 NewSlot(&ContentAreaElement::RemoveContentItem));
  RegisterMethod("removeAllContentItems",
                 NewSlot(&ContentAreaElement::RemoveAllContentItems));
}

ContentAreaElement::~ContentAreaElement() {
  delete impl_;
  impl_ = NULL;
}

std::string ContentAreaElement::GetBackgroundColor() const {
  return impl_->background_color_src_;
}

void ContentAreaElement::SetBackgroundColor(const char *color) {
  if (impl_->background_color_src_ != color) {
    if (Color::FromString(color, &impl_->background_color_,
                          &impl_->background_opacity_)) {
      impl_->background_color_src_ = color;

      QueueDraw();
    }
  }
}

std::string ContentAreaElement::GetDownColor() const {
  return impl_->mousedown_color_src_;
}

void ContentAreaElement::SetDownColor(const char *color) {
  if (impl_->mousedown_color_src_ != color) {
    if (Color::FromString(color, &impl_->mousedown_color_,
                          &impl_->mousedown_opacity_)) {
      impl_->mousedown_color_src_ = color;

      QueueDraw();
    }
  }
}

std::string ContentAreaElement::GetOverColor() const {
  return impl_->mouseover_color_src_;
}

void ContentAreaElement::SetOverColor(const char *color) {
  if (impl_->mouseover_color_src_ != color) {
    if (Color::FromString(color, &impl_->mouseover_color_,
                          &impl_->mouseover_opacity_)) {
      impl_->mouseover_color_src_ = color;

      QueueDraw();
    }
  }
}

int ContentAreaElement::GetContentFlags() const {
  return impl_->content_flags_;
}

void ContentAreaElement::SetContentFlags(int flags) {
  if (impl_->content_flags_ != flags) {
    // Casting to ContentFlags to avoid conversion warning when
    // compiling by the latest gcc.
    impl_->content_flags_ = static_cast<ContentFlag>(flags);
    QueueDraw();
  }
}

size_t ContentAreaElement::GetMaxContentItems() const {
  return impl_->max_content_items_;
}

void ContentAreaElement::SetMaxContentItems(size_t max_content_items) {
  impl_->SetMaxContentItems(max_content_items);
}

const std::vector<ContentItem *> &ContentAreaElement::GetContentItems() {
  return impl_->content_items_;
}

void ContentAreaElement::GetPinImages(ScriptableImage **unpinned,
                                      ScriptableImage **unpinned_over,
                                      ScriptableImage **pinned) {
  impl_->GetPinImages(unpinned, unpinned_over, pinned);
}

void ContentAreaElement::SetPinImages(ScriptableImage *unpinned,
                                      ScriptableImage *unpinned_over,
                                      ScriptableImage *pinned) {
  impl_->SetPinImages(unpinned, unpinned_over, pinned);
}

void ContentAreaElement::AddContentItem(ContentItem *item,
                                        DisplayOptions options) {
  impl_->AddContentItem(item, options);
}

void ContentAreaElement::RemoveContentItem(ContentItem *item) {
  impl_->RemoveContentItem(item);
}

void ContentAreaElement::RemoveAllContentItems() {
  impl_->RemoveAllContentItems();
}

void ContentAreaElement::Layout() {
  static int recurse_depth = 0;

  ScrollingElement::Layout();
  impl_->Layout();

  // Set reasonable scrolling step length.
  SetYPageStep(static_cast<int>(round(GetClientHeight())));
  SetYLineStep(impl_->scrolling_line_step_);

  int y_range = static_cast<int>(ceil(impl_->content_height_ -
                                      GetClientHeight()));
  if (y_range < 0) y_range = 0;

  // See DivElement::Layout() impl for the reason of recurse_depth.
  if (UpdateScrollBar(0, y_range) && (y_range > 0 || recurse_depth < 2)) {
    recurse_depth++;
    // Layout again to reflect change of the scroll bar.
    Layout();
    recurse_depth++;
  }
}

void ContentAreaElement::DoDraw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
  DrawScrollbar(canvas);
}

EventResult ContentAreaElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = impl_->HandleMouseEvent(event);
  return result == EVENT_RESULT_UNHANDLED ?
         ScrollingElement::HandleMouseEvent(event) : result;
}

BasicElement *ContentAreaElement::CreateInstance(View *view, const char *name) {
  return new ContentAreaElement(view, name);
}

bool ContentAreaElement::OnAddContextMenuItems(MenuInterface *menu) {
  return ScrollingElement::OnAddContextMenuItems(menu) &&
         impl_->OnAddContextMenuItems(menu);
}

ScriptableArray *ContentAreaElement::ScriptGetContentItems() {
  return impl_->ScriptGetContentItems();
}

void ContentAreaElement::ScriptSetContentItems(ScriptableInterface *array) {
  impl_->ScriptSetContentItems(array);
}

ScriptableArray *ContentAreaElement::ScriptGetPinImages() {
  return impl_->ScriptGetPinImages();
}

void ContentAreaElement::ScriptSetPinImages(ScriptableInterface *array) {
  impl_->ScriptSetPinImages(array);
}

bool ContentAreaElement::HasOpaqueBackground() const {
  return true;
}

} // namespace ggadget
