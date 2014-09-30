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

// Enable it to print verbose debug info
// #define VIEW_VERBOSE_DEBUG
// #define EVENT_VERBOSE_DEBUG

#include <vector>
#include <algorithm>

#include "view.h"
#include "basic_element.h"
#include "clip_region.h"
#include "contentarea_element.h"
#include "content_item.h"
#include "details_view_data.h"
#include "element_factory.h"
#include "elements.h"
#include "event.h"
#include "file_manager_interface.h"
#include "file_manager_factory.h"
#include "main_loop_interface.h"
#include "gadget_consts.h"
#include "gadget_interface.h"
#include "graphics_interface.h"
#include "image_cache.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "menu_interface.h"
#include "options_interface.h"
#include "script_context_interface.h"
#include "scriptable_binary_data.h"
#include "scriptable_event.h"
#include "scriptable_helper.h"
#include "scriptable_image.h"
#include "scriptable_interface.h"
#include "scriptable_menu.h"
#include "slot.h"
#include "string_utils.h"
#include "texture.h"
#include "view_host_interface.h"
#include "xml_dom_interface.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"
#include "small_object.h"

namespace ggadget {

DECLARE_VARIANT_PTR_TYPE(CanvasInterface);

static const char *kResizableNames[] = { "false", "true", "zoom" };
static const Variant kConfirmDefaultArgs[] = { Variant(), Variant(false) };

class View::Impl : public SmallObject<> {
 public:
  /**
   * Callback object for timer watches.
   * if duration > 0 then it's a animation timer.
   * else if duration == 0 then it's a timeout timer.
   * else if duration < 0 then it's a interval timer.
   */
  class TimerWatchCallback : public WatchCallbackInterface {
   public:
    TimerWatchCallback(Impl *impl, Slot *slot, int start, int end,
                       int duration, uint64_t start_time,
                       bool is_event)
      : start_time_(start_time),
        last_finished_time_(0),
        impl_(impl),
        slot_(slot),
        destroy_connection_(NULL),
        event_(0, 0),
        scriptable_event_(&event_, NULL, NULL),
        start_(start),
        end_(end),
        duration_(duration),
        last_value_(start),
        is_event_(is_event) {
      destroy_connection_ = impl_->on_destroy_signal_.Connect(
          NewSlot(this, &TimerWatchCallback::OnDestroy));
    }

    ~TimerWatchCallback() {
      destroy_connection_->Disconnect();
      delete slot_;
    }

    void SetWatchId(int watch_id) {
      event_.SetToken(watch_id);
    }

    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(watch_id);
      ASSERT(event_.GetToken() == watch_id);
      ScopedLogContext log_context(impl_->gadget_);

      bool fire = true;
      bool ret = true;
      int value = end_; // In case of duration <= 0.
      uint64_t current_time = main_loop->GetCurrentTime();

      // Animation timer
      if (duration_ > 0) {
        double progress =
            static_cast<double>(current_time - start_time_) / duration_;
        progress = std::min(1.0, std::max(0.0, progress));
        value = start_ + static_cast<int>(round(progress * (end_ - start_)));
        fire = (value != last_value_);
        ret = (progress < 1.0);
        last_value_ = value;
      } else if (duration_ == 0) {
        ret = false;
      }

      // If ret is false then fire, to make sure that the last event will
      // always be fired.
      if (fire && (!ret || current_time - last_finished_time_ >
                   kMinTimeBetweenTimerCall)) {
        if (is_event_) {
          // Because timer events are still fired during modal dialog opened
          // in key/mouse event handlers, switch off the user interaction
          // flag when the timer event is handled, to prevent unexpected
          // openUrl() etc.
          bool old_interaction = impl_->gadget_ ?
              impl_->gadget_->SetInUserInteraction(false) : false;
          event_.SetValue(value);
          impl_->FireEventSlot(&scriptable_event_, slot_);
          if (impl_->gadget_)
            impl_->gadget_->SetInUserInteraction(old_interaction);
        } else {
          slot_->Call(NULL, 0, NULL);
        }
      }

      last_finished_time_ = main_loop->GetCurrentTime();
      return ret;
    }

    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      ASSERT(event_.GetToken() == watch_id);
      delete this;
    }

    void OnDestroy() {
      impl_->RemoveTimer(event_.GetToken());
    }

   private:
    uint64_t start_time_;
    uint64_t last_finished_time_;
    Impl *impl_;
    Slot *slot_;
    Connection *destroy_connection_;
    TimerEvent event_;
    ScriptableEvent scriptable_event_;
    int start_;
    int end_;
    int duration_;
    int last_value_;
    bool is_event_;
  };

  Impl(View *owner,
       ViewHostInterface *view_host,
       GadgetInterface *gadget,
       ElementFactory *element_factory,
       ScriptContextInterface *script_context)
    : width_(0),
      height_(0),
      default_width_(320),
      default_height_(240),
      min_width_(0),
      min_height_(0),
      zoom_(1.0),
      rtl_(false),
      resize_border_left_(0),
      resize_border_top_(0),
      resize_border_right_(0),
      resize_border_bottom_(0),
      owner_(owner),
      gadget_(gadget),
      element_factory_(element_factory),
      main_loop_(GetGlobalMainLoop()),
      view_host_(view_host),
      script_context_(script_context),
      onoptionchanged_connection_(NULL),
      onzoom_connection_(NULL),
      canvas_cache_(NULL),
      graphics_(NULL),
      scriptable_view_(NULL),
      clip_region_(0.9),
      children_(element_factory, NULL, owner),
#ifdef _DEBUG
      draw_count_(0),
      view_draw_count_(0),
      accum_draw_time_(0),
#endif
      hittest_(HT_CLIENT),
      last_hittest_(HT_CLIENT),
      last_cursor_type_(CURSOR_DEFAULT),
      resizable_(RESIZABLE_ZOOM),
      dragover_result_(EVENT_RESULT_UNHANDLED),
      clip_region_enabled_(true),
      enable_cache_(true),
      show_caption_always_(false),
      draw_queued_(false),
      events_enabled_(true),
      need_redraw_(true),
      theme_changed_(false),
      resize_border_specified_(false),
      mouse_over_(false),
      view_focused_(false),
      safe_to_destroy_(true),
      content_changed_(true),
      auto_width_(false),
      auto_height_(false) {
    ASSERT(main_loop_);

    if (gadget_) {
      onoptionchanged_connection_ =
          gadget_->GetOptions()->ConnectOnOptionChanged(
              NewSlot(this, &Impl::OnOptionChanged));
    }
  }

  ~Impl() {
    ASSERT(event_stack_.empty());

    on_destroy_signal_.Emit(0, NULL);

    if (onoptionchanged_connection_) {
      onoptionchanged_connection_->Disconnect();
      onoptionchanged_connection_ = NULL;
    }

    if (onzoom_connection_) {
      onzoom_connection_->Disconnect();
      onzoom_connection_ = NULL;
    }

    if (canvas_cache_) {
      canvas_cache_->Destroy();
      canvas_cache_ = NULL;
    }

    if (view_host_) {
      view_host_->SetView(NULL);
      view_host_->Destroy();
      view_host_ = NULL;
    }
  }

  void RegisterProperties(RegisterableInterface *obj) {
    obj->RegisterProperty("caption",
                          NewSlot(owner_, &View::GetCaption),
                          NewSlot(owner_, &View::SetCaption));
    // Note: "event" property will be overrided in ScriptableView,
    // because ScriptableView will set itself to ScriptableEvent as SrcElement.
    obj->RegisterProperty("event", NewSlot(this, &Impl::GetEvent), NULL);
    obj->RegisterProperty("width",
                          NewSlot(this, &Impl::GetVariantWidth),
                          NewSlot(this, &Impl::SetVariantWidth));
    obj->RegisterProperty("height",
                          NewSlot(this, &Impl::GetVariantHeight),
                          NewSlot(this, &Impl::SetVariantHeight));
    obj->RegisterProperty("minWidth",
                          NewSlot(owner_, &View::GetMinWidth),
                          NewSlot(owner_, &View::SetMinWidth));
    obj->RegisterProperty("minHeight",
                          NewSlot(owner_, &View::GetMinHeight),
                          NewSlot(owner_, &View::SetMinHeight));
    obj->RegisterStringEnumProperty("resizable",
                          NewSlot(owner_, &View::GetResizable),
                          NewSlot(owner_, &View::SetResizable),
                          kResizableNames,
                          arraysize(kResizableNames));
    obj->RegisterProperty("showCaptionAlways",
                          NewSlot(owner_, &View::GetShowCaptionAlways),
                          NewSlot(owner_, &View::SetShowCaptionAlways));

    obj->RegisterVariantConstant("children", Variant(&children_));
    obj->RegisterMethod("appendElement",
                        NewSlot(&children_, &Elements::AppendElementVariant));
    // insertElement was deprecated by insertElementBehind.
    obj->RegisterMethod("insertElement",
                        NewSlot(&children_, &Elements::InsertElementVariant));
    obj->RegisterMethod("insertElementBehind",
                        NewSlot(&children_, &Elements::InsertElementVariant));
    // Added in 5.8 API.
    obj->RegisterMethod("insertElementInFrontOf",
                        NewSlot(&children_,
                                &Elements::InsertElementVariantAfter));
    obj->RegisterMethod("removeElement",
                        NewSlot(&children_, &Elements::RemoveElement));
    obj->RegisterMethod("removeAllElements",
                        NewSlot(&children_, &Elements::RemoveAllElements));

    // Here register ViewImpl::BeginAnimation because the Slot1<void, int> *
    // parameter in View::BeginAnimation can't be automatically reflected.
    obj->RegisterMethod("beginAnimation", NewSlot(this, &Impl::BeginAnimation));
    obj->RegisterMethod("cancelAnimation", NewSlot(this, &Impl::RemoveTimer));
    obj->RegisterMethod("setTimeout", NewSlot(this, &Impl::SetTimeout));
    obj->RegisterMethod("clearTimeout", NewSlot(this, &Impl::RemoveTimer));
    obj->RegisterMethod("setInterval", NewSlot(this, &Impl::SetInterval));
    obj->RegisterMethod("clearInterval", NewSlot(this, &Impl::RemoveTimer));

    obj->RegisterMethod("alert", NewSlot(owner_, &View::Alert));
    obj->RegisterMethod("confirm",
                        NewSlotWithDefaultArgs(NewSlot(owner_, &View::Confirm),
                                               kConfirmDefaultArgs));
    obj->RegisterMethod("prompt", NewSlot(owner_, &View::Prompt));

    obj->RegisterMethod("resizeBy", NewSlot(this, &Impl::ResizeBy));
    obj->RegisterMethod("resizeTo", NewSlot(this, &Impl::SetVariantSize));

    // Added in GDWin 5.8
    obj->RegisterProperty("resizeBorder",
                          NewSlot(this, &Impl::GetResizeBorder),
                          NewSlot(this, &Impl::SetResizeBorder));

    // Extended APIs.
#if 0
    obj->RegisterProperty("focusedElement",
                          NewSlot(owner_, &View::GetFocusedElement), NULL);
    obj->RegisterProperty("mouseOverElement",
                          NewSlot(owner_, &View::GetMouseOverElement), NULL);
#endif

    // Added for BIDI.
    obj->RegisterProperty("RTL",
                          NewSlot(owner_, &View::IsTextRTL),
                          NewSlot(owner_, &View::SetTextRTL));
    obj->RegisterSignal(kOnCancelEvent, &oncancel_event_);
    obj->RegisterSignal(kOnClickEvent, &onclick_event_);
    obj->RegisterSignal(kOnCloseEvent, &onclose_event_);
    obj->RegisterSignal(kOnDblClickEvent, &ondblclick_event_);
    obj->RegisterSignal(kOnRClickEvent, &onrclick_event_);
    obj->RegisterSignal(kOnRDblClickEvent, &onrdblclick_event_);
    obj->RegisterSignal(kOnDockEvent, &ondock_event_);
    obj->RegisterSignal(kOnKeyDownEvent, &onkeydown_event_);
    obj->RegisterSignal(kOnKeyPressEvent, &onkeypress_event_);
    obj->RegisterSignal(kOnKeyUpEvent, &onkeyup_event_);
    obj->RegisterSignal(kOnMinimizeEvent, &onminimize_event_);
    obj->RegisterSignal(kOnMouseDownEvent, &onmousedown_event_);
    obj->RegisterSignal(kOnMouseMoveEvent, &onmousemove_event_);
    obj->RegisterSignal(kOnMouseOutEvent, &onmouseout_event_);
    obj->RegisterSignal(kOnMouseOverEvent, &onmouseover_event_);
    obj->RegisterSignal(kOnMouseUpEvent, &onmouseup_event_);
    obj->RegisterSignal(kOnMouseWheelEvent, &onmousewheel_event_);
    obj->RegisterSignal(kOnOkEvent, &onok_event_);
    obj->RegisterSignal(kOnOpenEvent, &onopen_event_);
    obj->RegisterSignal(kOnOptionChangedEvent, &onoptionchanged_event_);
    obj->RegisterSignal(kOnPopInEvent, &onpopin_event_);
    obj->RegisterSignal(kOnPopOutEvent, &onpopout_event_);
    obj->RegisterSignal(kOnRestoreEvent, &onrestore_event_);
    obj->RegisterSignal(kOnSizeEvent, &onsize_event_);
    obj->RegisterSignal(kOnSizingEvent, &onsizing_event_);
    obj->RegisterSignal(kOnUndockEvent, &onundock_event_);
    // Not a standard signal.
    obj->RegisterSignal(kOnContextMenuEvent, &oncontextmenu_event_);
    // 5.8 API.
    obj->RegisterSignal(kOnThemeChangedEvent, &onthemechanged_event_);
  }

  void MapChildPositionEvent(const PositionEvent &org_event,
                             BasicElement *child,
                             PositionEvent *new_event) {
    ASSERT(child);
    double x, y;
    child->ViewCoordToSelfCoord(org_event.GetX(), org_event.GetY(), &x, &y);
    new_event->SetX(x);
    new_event->SetY(y);
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

  EventResult SendMouseEventToChildren(const MouseEvent &event) {
    Event::Type type = event.GetType();
    if (type == Event::EVENT_MOUSE_OVER) {
      // View's EVENT_MOUSE_OVER only applicable to itself.
      // Children's EVENT_MOUSE_OVER is triggered by other mouse events.
      return EVENT_RESULT_UNHANDLED;
    }

    BasicElement *temp, *temp1; // Used to receive unused output parameters.
    EventResult result = EVENT_RESULT_UNHANDLED;
    HitTest temp_hittest;
    // If some element is grabbing mouse, send all EVENT_MOUSE_MOVE,
    // EVENT_MOUSE_UP and EVENT_MOUSE_CLICK events to it directly, until
    // an EVENT_MOUSE_CLICK received, or any mouse event received without
    // left button down.
    // FIXME: Is it necessary to update hittest_ when mouse is grabbing?
    if (grabmouse_element_.Get()) {
      // We used to check IsReallyEnabled() here, which seems too strict
      // because the grabmouse_element_ may move out of the visible area, but
      // it should still grab the mouse.
      // EVENT_MOUSE_UP should always be fired no matter if the element is
      // enabled or not.
      if ((grabmouse_element_.Get()->IsEnabled() ||
           type == Event::EVENT_MOUSE_UP) &&
          (event.GetButton() & MouseEvent::BUTTON_LEFT) &&
          (type == Event::EVENT_MOUSE_MOVE || type == Event::EVENT_MOUSE_UP ||
           type == Event::EVENT_MOUSE_CLICK)) {
        MouseEvent new_event(event);
        MapChildMouseEvent(event, grabmouse_element_.Get(), &new_event);
        result = grabmouse_element_.Get()->OnMouseEvent(
            new_event, true, &temp, &temp1, &temp_hittest);
        // Set correct mouse cursor.
        if (grabmouse_element_.Get()) {
          owner_->SetCursor(grabmouse_element_.Get()->GetCursor());
        }
        // Release the grabbing on EVENT_MOUSE_CLICK not EVENT_MOUSE_UP,
        // otherwise the click event may be sent to wrong element.
        if (type == Event::EVENT_MOUSE_CLICK) {
          grabmouse_element_.Reset(NULL);
        }
        return result;
      } else {
        // Release the grabbing on any mouse event without left button down.
        grabmouse_element_.Reset(NULL);
      }
    }

    if (type == Event::EVENT_MOUSE_OUT) {
      // The mouse has been moved out of the view, clear the mouseover state.
      if (mouseover_element_.Get()) {
        MouseEvent new_event(event);
        MapChildMouseEvent(event, mouseover_element_.Get(), &new_event);
        result = mouseover_element_.Get()->OnMouseEvent(
            new_event, true, &temp, &temp1, &temp_hittest);
        mouseover_element_.Reset(NULL);
      }
      return result;
    }

    BasicElement *fired_element = NULL;
    BasicElement *in_element = NULL;
    HitTest child_hittest = HT_CLIENT;

    // Dispatch the event to children normally,
    // unless popup is active and event is inside popup element.
    bool outside_popup = true;
    if (popup_element_.Get()) {
      if (popup_element_.Get()->IsReallyVisible()) {
        MouseEvent new_event(event);
        MapChildMouseEvent(event, popup_element_.Get(), &new_event);
        if (popup_element_.Get()->IsPointIn(new_event.GetX(),
                                            new_event.GetY())) {
          // Not direct.
          result = popup_element_.Get()->OnMouseEvent(
              new_event, false, &fired_element, &in_element, &child_hittest);
          outside_popup = false;
        }
      } else {
        SetPopupElement(NULL);
      }
    }
    if (outside_popup) {
      result = children_.OnMouseEvent(event, &fired_element,
                                      &in_element, &child_hittest);
      // The following might hit if a grabbed element is
      // turned invisible or disabled while under grab.
      if (type == Event::EVENT_MOUSE_DOWN && result != EVENT_RESULT_CANCELED) {
        SetPopupElement(NULL);
      }
    }

    // If the mouse pointer moves out of the view after calling children's
    // mouse event handler, then just return the result without handling the
    // mouse over/out things.
    if (!mouse_over_)
      return result;

    ElementHolder in_element_holder(in_element);

    if (fired_element && type == Event::EVENT_MOUSE_DOWN &&
        (event.GetButton() & MouseEvent::BUTTON_LEFT)) {
      // Start grabbing.
      grabmouse_element_.Reset(fired_element);
      // Focus is handled in BasicElement.
    }

    if (fired_element != mouseover_element_.Get()) {
      BasicElement *old_mouseover_element = mouseover_element_.Get();
      // Store it early to prevent crash if fired_element is removed in
      // the mouseout handler.
      mouseover_element_.Reset(fired_element);

      if (old_mouseover_element) {
        MouseEvent mouseout_event(Event::EVENT_MOUSE_OUT,
                                  event.GetX(), event.GetY(),
                                  event.GetWheelDeltaX(),
                                  event.GetWheelDeltaY(),
                                  event.GetButton(),
                                  event.GetModifier());
        MapChildMouseEvent(event, old_mouseover_element, &mouseout_event);
        old_mouseover_element->OnMouseEvent(mouseout_event, true,
                                            &temp, &temp1, &temp_hittest);
      }

      if (mouseover_element_.Get()) {
        // Always fire the mouse over event even if the element's visibility
        // and enabled state changed during the above mouse out, to keep the
        // same behaviour as the Windows version.
        MouseEvent mouseover_event(Event::EVENT_MOUSE_OVER,
                                   event.GetX(), event.GetY(),
                                   event.GetWheelDeltaX(),
                                   event.GetWheelDeltaY(),
                                   event.GetButton(),
                                   event.GetModifier());
        MapChildMouseEvent(event, mouseover_element_.Get(), &mouseover_event);
        mouseover_element_.Get()->OnMouseEvent(mouseover_event, true,
                                               &temp, &temp1, &temp_hittest);
      }
    }

    if (in_element_holder.Get()) {
      hittest_ = child_hittest;
      if (type == Event::EVENT_MOUSE_MOVE &&
          in_element != tooltip_element_.Get()) {
        owner_->ShowElementTooltip(in_element);
      }
    } else {
      // FIXME: If HT_NOWHERE is more suitable?
      hittest_ = HT_TRANSPARENT;
      tooltip_element_.Reset(NULL);
    }

    // If in element has a special hittest value, then use its cursor instead
    // of mouseover element's cursor.
    if (hittest_ != HT_CLIENT && in_element_holder.Get()) {
      owner_->SetCursor(in_element_holder.Get()->GetCursor());
    } else if (mouseover_element_.Get()) {
      owner_->SetCursor(mouseover_element_.Get()->GetCursor());
    } else {
      owner_->SetCursor(CURSOR_DEFAULT);
    }

#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    if (in_element_holder.Get()) {
      DLOG("Mouse Event result(%p): In:%s type:%s, hitTest:%d, result: %d",
           owner_, in_element->GetName().c_str(), in_element->GetTagName(),
           hittest_, result);
    } else {
      DLOG("Mouse Event result(%p): hitTest:%d, result: %d",
           owner_, hittest_, result);
    }
#endif

    return result;
  }

  EventResult OnMouseEvent(const MouseEvent &event) {
    Event::Type type = event.GetType();
    double opacity;

    // Main views don't handle the mouse event if the pixel under the mouse
    // pointer is fully transparent and there is no element grabbing the mouse.
    // Options views and details views don't have this feature because they
    // look opaque.
    if (view_host_ &&
        view_host_->GetType() == ViewHostInterface::VIEW_HOST_MAIN &&
        type != Event::EVENT_MOUSE_OUT && !grabmouse_element_.Get() &&
        enable_cache_ && canvas_cache_ && canvas_cache_->GetPointValue(
            event.GetX(), event.GetY(), NULL, &opacity) && opacity == 0) {
      // Send out fake mouse out event if the pixel is fully transparent and
      // the mouse is over the view.
      if (mouse_over_) {
        MouseEvent new_event(Event::EVENT_MOUSE_OUT,
                             event.GetX(), event.GetY(), 0, 0,
                             MouseEvent::BUTTON_NONE,
                             MouseEvent::MODIFIER_NONE);
        OnMouseEvent(new_event);
      }
      hittest_ = HT_TRANSPARENT;
      return EVENT_RESULT_UNHANDLED;
    }

    // If the mouse is already out of the view, don't handle the mouse out
    // event again.
    if (type == Event::EVENT_MOUSE_OUT && !mouse_over_) {
      return EVENT_RESULT_UNHANDLED;
    }

    // If the mouse is already over the view, don't handle the mouse over
    // event again.
    if (type == Event::EVENT_MOUSE_OVER && mouse_over_) {
      return EVENT_RESULT_UNHANDLED;
    }

    // Send fake mouse over event if the pixel is not fully transparent and the
    // mouse over state is not set yet.
    if (type != Event::EVENT_MOUSE_OVER && type != Event::EVENT_MOUSE_OUT &&
        !mouse_over_) {
      MouseEvent new_event(Event::EVENT_MOUSE_OVER,
                           event.GetX(), event.GetY(), 0, 0,
                           MouseEvent::BUTTON_NONE, MouseEvent::MODIFIER_NONE);
      OnMouseEvent(new_event);
    }

    // Send event to view first.
    ScriptableEvent scriptable_event(&event, NULL, NULL);

    bool old_interactive = false;
    if (gadget_ && type != Event::EVENT_MOUSE_MOVE &&
        type != Event::EVENT_MOUSE_OVER && type != Event::EVENT_MOUSE_OUT)
      old_interactive = gadget_->SetInUserInteraction(true);

#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    if (type != Event::EVENT_MOUSE_MOVE)
      DLOG("%s(View): x:%g y:%g dx:%d dy:%d b:%d m:%d",
           scriptable_event.GetName(), event.GetX(), event.GetY(),
           event.GetWheelDeltaX(), event.GetWheelDeltaY(),
           event.GetButton(), event.GetModifier());
#endif
    switch (type) {
      case Event::EVENT_MOUSE_MOVE:
        // Put the high volume events near top.
        FireEvent(&scriptable_event, onmousemove_event_);
        break;
      case Event::EVENT_MOUSE_DOWN:
        FireEvent(&scriptable_event, onmousedown_event_);
        break;
      case Event::EVENT_MOUSE_UP:
        FireEvent(&scriptable_event, onmouseup_event_);
        break;
      case Event::EVENT_MOUSE_CLICK:
        FireEvent(&scriptable_event, onclick_event_);
        break;
      case Event::EVENT_MOUSE_DBLCLICK:
        FireEvent(&scriptable_event, ondblclick_event_);
        break;
      case Event::EVENT_MOUSE_RCLICK:
        FireEvent(&scriptable_event, onrclick_event_);
        break;
      case Event::EVENT_MOUSE_RDBLCLICK:
        FireEvent(&scriptable_event, onrdblclick_event_);
        break;
      case Event::EVENT_MOUSE_OUT:
        mouse_over_ = false;
        FireEvent(&scriptable_event, onmouseout_event_);
        break;
      case Event::EVENT_MOUSE_OVER:
        mouse_over_ = true;
        FireEvent(&scriptable_event, onmouseover_event_);
        break;
      case Event::EVENT_MOUSE_WHEEL:
        // 5.8 API added onmousewheel for view.
        FireEvent(&scriptable_event, onmousewheel_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED) {
      if (type == Event::EVENT_MOUSE_OVER) {
        // Translate the mouse over event to a mouse move event, to make sure
        // that the correct mouseover element will be set.
        MouseEvent new_event(Event::EVENT_MOUSE_MOVE,
                             event.GetX(), event.GetY(), 0, 0,
                             MouseEvent::BUTTON_NONE,
                             MouseEvent::MODIFIER_NONE);
        result = SendMouseEventToChildren(new_event);
      } else {
        result = SendMouseEventToChildren(event);
      }
    }

    if (mouse_over_ && result == EVENT_RESULT_UNHANDLED &&
        event.GetType() == Event::EVENT_MOUSE_RCLICK &&
        event.GetButton() == MouseEvent::BUTTON_RIGHT) {
      // Handle ShowContextMenu event.
      if (view_host_->ShowContextMenu(MouseEvent::BUTTON_RIGHT))
        result = EVENT_RESULT_HANDLED;
    }

    if (gadget_)
      gadget_->SetInUserInteraction(old_interactive);
    return result;
  }

  void SetFocusToFirstElement() {
    if (children_.GetCount()) {
      BasicElement *first = children_.GetItemByIndex(0);
      if (!first->IsReallyEnabled() || !first->IsTabStop())
        first = GetNextFocusElement(first);
      SetFocus(first);
    }
  }

  void SetFocusToLastElement() {
    size_t count = children_.GetCount();
    if (count) {
      BasicElement *last = children_.GetItemByIndex(count - 1);
      if (!last->IsReallyEnabled() || !last->IsTabStop())
        last = GetPreviousFocusElement(last);
      SetFocus(last);
    }
  }

  void MoveFocusForward() {
    BasicElement *current = focused_element_.Get();
    if (current) {
      // Try children first.
      BasicElement *next = GetFirstFocusInSubTrees(current);
      if (!next)
        next = GetNextFocusElement(current);
      if (next && next != current)
        SetFocus(next);
      // Otherwise leave the focus unchanged.
    } else {
      SetFocusToFirstElement();
    }
  }

  void MoveFocusBackward() {
    BasicElement *current = focused_element_.Get();
    if (current) {
      BasicElement *previous = GetPreviousFocusElement(current);
      if (previous && previous != current)
        SetFocus(previous);
      // Otherwise leave the focus unchanged.
    } else {
      SetFocusToLastElement();
    }
  }

  // Note: this method doesn't search in descendants.
  BasicElement *GetNextFocusElement(BasicElement *current) {
    // Try previous siblings first.
    BasicElement *parent = current->GetParentElement();
    Elements *elements = parent ? parent->GetChildren() : &children_;
    size_t index = current->GetIndex();
    if (index != kInvalidIndex) {
      for (size_t i = index + 1; i < elements->GetCount(); i++) {
        BasicElement *result = GetFirstFocusInTree(elements->GetItemByIndex(i));
        if (result)
          return result;
      }
    }
    // All next siblings and their children are not focusable, up to the parent.
    if (parent)
      return GetNextFocusElement(parent);

    // Now at the top level, wrap back to the first element.
    ASSERT(index != kInvalidIndex); // Otherwise it should have a parent.
    for (size_t i = 0; i <= index; i++) {
      BasicElement *result = GetFirstFocusInTree(children_.GetItemByIndex(i));
      if (result)
        return result;
    }
    return NULL;
  }

  BasicElement *GetPreviousFocusElement(BasicElement *current) {
    // Try previous siblings first.
    BasicElement *parent = current->GetParentElement();
    Elements *elements = parent ? parent->GetChildren() : &children_;
    size_t index = current->GetIndex();
    if (index != kInvalidIndex) {
      for (size_t i = index; i > 0; i--) {
        BasicElement *result = GetLastFocusInTree(
            elements->GetItemByIndex(i - 1));
        if (result)
          return result;
      }
    }
    // All previous siblings and their children are not focusable, up to the
    // parent.
    if (parent)
      return GetPreviousFocusElement(parent);

    // Now at the top level, wrap back to the last element.
    ASSERT(index != kInvalidIndex); // Otherwise it should have a parent.
    for (size_t i = children_.GetCount(); i > index; i--) {
      BasicElement *result =
          GetLastFocusInTree(children_.GetItemByIndex(i - 1));
      if (result)
        return result;
    }
    return NULL;
  }

  BasicElement *GetFirstFocusInTree(BasicElement *current) {
    return current->IsReallyEnabled() && current->IsTabStop() ?
           current : GetFirstFocusInSubTrees(current);
  }

  BasicElement *GetFirstFocusInSubTrees(BasicElement *current) {
    if (current->IsVisible()) {
      Elements *children = current->GetChildren();
      if (children) {
        size_t childcount = children->GetCount();
        for (size_t i = 0; i < childcount; i++) {
          BasicElement *result =
              GetFirstFocusInTree(children->GetItemByIndex(i));
          if (result)
            return result;
        }
      }
    }
    return NULL;
  }

  BasicElement *GetLastFocusInTree(BasicElement *current) {
    BasicElement *result = GetLastFocusInSubTrees(current);
    if (result)
      return result;
    return current->IsReallyEnabled() && current->IsTabStop() ? current : NULL;
  }

  BasicElement *GetLastFocusInSubTrees(BasicElement *current) {
    if (current->IsVisible()) {
      Elements *children = current->GetChildren();
      if (children) {
        for (size_t i = children->GetCount(); i > 0; i--) {
          BasicElement *result =
              GetLastFocusInTree(children->GetItemByIndex(i - 1));
          if (result)
            return result;
        }
      }
    }
    return NULL;
  }

  EventResult OnKeyEvent(const KeyboardEvent &event) {
    ScriptableEvent scriptable_event(&event, NULL, NULL);
#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    DLOG("%s(View): %d %d", scriptable_event.GetName(),
         event.GetKeyCode(), event.GetModifier());
#endif

    bool old_interactive =
        gadget_ ? gadget_->SetInUserInteraction(true) : false;

    BasicElement *old_focused_element = focused_element_.Get();

    switch (event.GetType()) {
      case Event::EVENT_KEY_DOWN:
        FireEvent(&scriptable_event, onkeydown_event_);
        break;
      case Event::EVENT_KEY_UP:
        FireEvent(&scriptable_event, onkeyup_event_);
        break;
      case Event::EVENT_KEY_PRESS:
        FireEvent(&scriptable_event, onkeypress_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED &&
        focused_element_.Get()) {
      if (!focused_element_.Get()->IsReallyEnabled()) {
        focused_element_.Get()->OnOtherEvent(
            SimpleEvent(Event::EVENT_FOCUS_OUT));
        focused_element_.Reset(NULL);
      } else {
        result = focused_element_.Get()->OnKeyEvent(event);
        if (result != EVENT_RESULT_CANCELED) {
          // From API 5.8, tab keys are not sent to elements, but move focus.
          if (event.GetType() == Event::EVENT_KEY_DOWN &&
              event.GetKeyCode() == KeyboardEvent::KEY_TAB &&
              // Only move focus when focus was not moved by the view's and
              // the focused element's event handler.
              old_focused_element == focused_element_.Get()) {
            if (event.GetModifier() & Event::MODIFIER_SHIFT)
              MoveFocusBackward();
            else
              MoveFocusForward();
            result = EVENT_RESULT_HANDLED;
          }
        }
      }
    }

    if (gadget_)
      gadget_->SetInUserInteraction(old_interactive);
    return result;
  }

  EventResult OnDragEvent(const DragEvent &event) {
    Event::Type type = event.GetType();
    if (type == Event::EVENT_DRAG_OUT || type == Event::EVENT_DRAG_DROP) {
      bool old_interactive = false;
      if (gadget_ && type == Event::EVENT_DRAG_DROP)
        old_interactive = gadget_->SetInUserInteraction(old_interactive);

      EventResult result = EVENT_RESULT_UNHANDLED;
      // Send the event and clear the dragover state.
      if (dragover_element_.Get()) {
        // If the element rejects the drop, send a EVENT_DRAG_OUT
        // on EVENT_DRAG_DROP.
        if (dragover_result_ != EVENT_RESULT_HANDLED)
          type = Event::EVENT_DRAG_OUT;
        DragEvent new_event(type, event.GetX(), event.GetY());
        new_event.SetDragFiles(event.GetDragFiles());
        new_event.SetDragUrls(event.GetDragUrls());
        new_event.SetDragText(event.GetDragText());
        MapChildPositionEvent(event, dragover_element_.Get(), &new_event);
        BasicElement *temp;
        result = dragover_element_.Get()->OnDragEvent(new_event, true, &temp);
        dragover_element_.Reset(NULL);
        dragover_result_ = EVENT_RESULT_UNHANDLED;
      }

      if (gadget_ && type == Event::EVENT_DRAG_DROP)
        gadget_->SetInUserInteraction(old_interactive);
      return result;
    }

    ASSERT(type == Event::EVENT_DRAG_MOTION);
    // Dispatch the event to children normally.
    BasicElement *fired_element = NULL;
    children_.OnDragEvent(event, &fired_element);
    if (fired_element != dragover_element_.Get()) {
      dragover_result_ = EVENT_RESULT_UNHANDLED;
      BasicElement *old_dragover_element = dragover_element_.Get();
      // Store it early to prevent crash if fired_element is removed in
      // the dragout handler.
      dragover_element_.Reset(fired_element);

      if (old_dragover_element) {
        DragEvent dragout_event(Event::EVENT_DRAG_OUT,
                                event.GetX(), event.GetY());
        dragout_event.SetDragFiles(event.GetDragFiles());
        dragout_event.SetDragUrls(event.GetDragUrls());
        dragout_event.SetDragText(event.GetDragText());
        MapChildPositionEvent(event, old_dragover_element, &dragout_event);
        BasicElement *temp;
        old_dragover_element->OnDragEvent(dragout_event, true, &temp);
      }

      if (dragover_element_.Get()) {
        // The visible state may change during event handling.
        if (!dragover_element_.Get()->IsReallyVisible()) {
          dragover_element_.Reset(NULL);
        } else {
          DragEvent dragover_event(Event::EVENT_DRAG_OVER,
                                   event.GetX(), event.GetY());
          dragover_event.SetDragFiles(event.GetDragFiles());
          dragover_event.SetDragUrls(event.GetDragUrls());
          dragover_event.SetDragText(event.GetDragText());
          MapChildPositionEvent(event, dragover_element_.Get(),
                                &dragover_event);
          BasicElement *temp;
          dragover_result_ = dragover_element_.Get()->OnDragEvent(
              dragover_event, true, &temp);
        }
      }
    }

    // Because gadget elements has no handler for EVENT_DRAG_MOTION, the
    // last return result of EVENT_DRAG_OVER should be used as the return result
    // of EVENT_DRAG_MOTION.
    return dragover_result_;
  }

  EventResult OnOtherEvent(const Event &event) {
    ScriptableEvent scriptable_event(&event, NULL, NULL);
#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    DLOG("%s(View)", scriptable_event.GetName());
#endif
    switch (event.GetType()) {
      case Event::EVENT_FOCUS_IN:
        view_focused_ = true;
        // Restore focus to the original focused element if it is still there.
        if (focused_element_.Get() &&
            (!focused_element_.Get()->IsReallyEnabled() ||
             focused_element_.Get()->OnOtherEvent(SimpleEvent(
                 Event::EVENT_FOCUS_IN)) == EVENT_RESULT_CANCELED)) {
          focused_element_.Reset(NULL);
        }
        break;
      case Event::EVENT_FOCUS_OUT:
        view_focused_ = false;
        if (focused_element_.Get()) {
          focused_element_.Get()->OnOtherEvent(SimpleEvent(
              Event::EVENT_FOCUS_OUT));
          // Don't clear focused_element_ so that when the focus come back
          // to this view, we can restore the focus to the element.
        }
        break;
      case Event::EVENT_CANCEL:
        FireEvent(&scriptable_event, oncancel_event_);
        break;
      case Event::EVENT_CLOSE:
        FireEvent(&scriptable_event, onclose_event_);
        break;
      case Event::EVENT_DOCK:
        FireEvent(&scriptable_event, ondock_event_);
        break;
      case Event::EVENT_MINIMIZE:
        FireEvent(&scriptable_event, onminimize_event_);
        break;
      case Event::EVENT_OK:
        FireEvent(&scriptable_event, onok_event_);
        break;
      case Event::EVENT_OPEN:
        SetFocusToFirstElement();
        FireEvent(&scriptable_event, onopen_event_);
        break;
      case Event::EVENT_POPIN:
        FireEvent(&scriptable_event, onpopin_event_);
        break;
      case Event::EVENT_POPOUT:
        FireEvent(&scriptable_event, onpopout_event_);
        break;
      case Event::EVENT_RESTORE:
        FireEvent(&scriptable_event, onrestore_event_);
        break;
      case Event::EVENT_SIZE:
        FireEvent(&scriptable_event, onsize_event_);
        break;
      case Event::EVENT_SIZING:
        FireEvent(&scriptable_event, onsizing_event_);
        break;
      case Event::EVENT_UNDOCK:
        FireEvent(&scriptable_event, onundock_event_);
        break;
      case Event::EVENT_THEME_CHANGED:
        MarkRedraw();
        owner_->QueueDraw();
        theme_changed_ = true;
        break;
      default:
        ASSERT(false);
    }
    return scriptable_event.GetReturnValue();
  }

  void SetVariantSize(const Variant &width, const Variant &height) {
    double pixel_width = 0;
    bool width_changed = false;
    if (width.type() == Variant::TYPE_STRING &&
        VariantValue<std::string>()(width) == "auto") {
      width_changed = !auto_width_;
      auto_width_ = true;
    } else if (width.ConvertToDouble(&pixel_width)) {
      width_changed = auto_width_ || (pixel_width != width_);
      auto_width_ = false;
    }

    double pixel_height = 0;
    bool height_changed = false;
    if (height.type() == Variant::TYPE_STRING &&
        VariantValue<std::string>()(height) == "auto") {
      height_changed = !auto_height_;
      auto_height_ = true;
    } else if (height.ConvertToDouble(&pixel_height)) {
      height_changed = auto_height_ || (pixel_height != height_);
      auto_height_ = false;
    }

    if ((width_changed && auto_width_) || (height_changed && auto_height_)) {
      double desired_width = 0;
      double desired_height = 0;
      GetDesiredAutoSize(&desired_width, &desired_height);
      if (auto_width_)
        pixel_width = desired_width;
      if (auto_height_)
        pixel_height = desired_height;
    }

    SetSize((width_changed ? pixel_width : width_),
            (height_changed ? pixel_height : height_));
  }

  void SetVariantWidth(const Variant &width) {
    SetVariantSize(width, auto_height_ ? Variant("auto") : Variant(height_));
  }

  Variant GetVariantWidth() const {
    return auto_width_ ? Variant("auto") : Variant(width_);
  }

  void SetVariantHeight(const Variant &height) {
    SetVariantSize(auto_width_ ? Variant("auto") : Variant(width_), height);
  }

  Variant GetVariantHeight() const {
    return auto_height_ ? Variant("auto") : Variant(height_);
  }

  void SetSize(double width, double height) {
    ScopedLogContext log_context(gadget_);
    width = std::max(width, min_width_);
    height = std::max(height, min_height_);
    if (width != width_ || height != height_) {
      // Invalidate the canvas cache.
      if (canvas_cache_) {
        canvas_cache_->Destroy();
        canvas_cache_ = NULL;
      }

      // Store default width and height if the size has not been set before.
      if (width_ == 0)
        default_width_ = width;
      if (height_ == 0)
        default_height_ = height;

      width_ = width;
      height_ = height;

      // In some case, QueueResize() may not cause redraw,
      // so do layout here to make sure the layout is correct.
      if (!draw_queued_)
        children_.Layout();

      SimpleEvent event(Event::EVENT_SIZE);
      ScriptableEvent scriptable_event(&event, NULL, NULL);
      FireEvent(&scriptable_event, onsize_event_);

      if (view_host_)
        view_host_->QueueResize();
    }
  }

  void ResizeBy(double width, double height) {
    auto_width_ = false;
    auto_height_ = false;
    SetSize(width_ + width, height_ + height);
  }

  double GetWidth() {
    return width_;
  }

  double GetHeight() {
    return height_;
  }

  void SetResizeBorder(const std::string &value) {
    resize_border_specified_ = false;
    if (!StringToBorderSize(value,
                            &resize_border_left_,
                            &resize_border_top_,
                            &resize_border_right_,
                            &resize_border_bottom_)) {
        LOG("Invalid resize border value: %s", value.c_str());
        return;
    }

    resize_border_specified_ = true;
    if (view_host_)
      view_host_->QueueResize();
  }

  std::string GetResizeBorder() {
    if (!resize_border_specified_) {
      return "";
    } else if (resize_border_left_ == resize_border_top_ &&
               resize_border_top_ == resize_border_right_ &&
               resize_border_right_ == resize_border_bottom_) {
      return StringPrintf("%.0f", resize_border_left_);
    } else if (resize_border_left_ == resize_border_right_ &&
               resize_border_top_ == resize_border_bottom_) {
      return StringPrintf("%.0f %.0f", resize_border_left_, resize_border_top_);
    } else {
      return StringPrintf("%.0f %.0f %.0f %.0f",
                          resize_border_left_, resize_border_top_,
                          resize_border_right_, resize_border_bottom_);
    }
  }

  void MarkRedraw() {
    need_redraw_ = true;
    children_.MarkRedraw();
  }

  void Layout() {
    // Any QueueDraw() called during Layout() will be ignored, because
    // draw_queued_ is true.
    draw_queued_ = true;
    if (theme_changed_ && events_enabled_) {
      SimpleEvent event(Event::EVENT_THEME_CHANGED);
      ScriptableEvent scriptable_event(&event, NULL, NULL);
      FireEvent(&scriptable_event, onthemechanged_event_);
      theme_changed_ = false;
    }

    children_.CalculateSize();

    AutoUpdateSize();
    children_.Layout();

    // Let posted events be processed after Layout() and before actual Draw().
    // This can prevent some flickers, for example, onsize of labels.
    // If Event isn't enabled then postpone these events.
    if (events_enabled_) {
      FirePostedSizeEvents();
    }
    draw_queued_ = false;

    Rectangle boundary(0, 0, width_, height_);
    if (!need_redraw_) {
      if (popup_element_.Get()) {
        popup_element_.Get()->AggregateClipRegion(boundary, &clip_region_);
      }
      children_.AggregateClipRegion(boundary, &clip_region_);
    } else {
      // Clear clip region if the whole view needs redrawing, so that view host
      // will draw the whole view correctly.
      clip_region_.Clear();
      clip_region_.AddRectangle(boundary);
      if (popup_element_.Get()) {
        popup_element_.Get()->AggregateClipRegion(Rectangle(), NULL);
      }
      children_.AggregateClipRegion(Rectangle(), NULL);
    }

    if (!clip_region_.IsEmpty()) {
      content_changed_ = true;
      if (on_add_rectangle_to_clip_region_.HasActiveConnections()) {
        size_t count = clip_region_.GetRectangleCount();
        for (size_t i = 0; i < count; ++i) {
          Rectangle r = clip_region_.GetRectangle(i);
          on_add_rectangle_to_clip_region_(r.x, r.y, r.w, r.h);
        }
      }
    }
  }

  void Draw(CanvasInterface *canvas) {
#if defined(_DEBUG) && defined(VIEW_VERBOSE_DEBUG)
    DLOG("host(%p) draw view(%p) on canvas %p with size: %f x %f",
         view_host_, owner_, canvas, canvas->GetWidth(), canvas->GetHeight());
    draw_count_ = 0;
    uint64_t start = main_loop_->GetCurrentTime();
#endif

    // no draw queued, so the draw request is initiated from host.
    // And because the canvas cache_ is valid, just need to paint the canvas
    // cache to the dest canvas.
    if (!content_changed_ && canvas_cache_ && !need_redraw_) {
#if defined(_DEBUG) && defined(VIEW_VERBOSE_DEBUG)
      DLOG("Draw View(%p) from canvas cache.", owner_);
#endif
      canvas->DrawCanvas(0, 0, canvas_cache_);
      return;
#if defined(_DEBUG) && defined(VIEW_VERBOSE_DEBUG)
    } else {
      DLOG("Redraw whole view: content changed: %d, "
           "canvas cache: %p, need redraw:%d",
           content_changed_, canvas_cache_, need_redraw_);
#endif
    }

    if (popup_element_.Get() && !popup_element_.Get()->IsReallyVisible())
      SetPopupElement(NULL);

#if defined(_DEBUG) && defined(VIEW_VERBOSE_DEBUG)
    clip_region_.PrintLog();
#endif

    bool reset_clip_region = false;
    if (enable_cache_ && !canvas_cache_ && graphics_) {
      canvas_cache_ = graphics_->NewCanvas(width_, height_);
      // If need_redraw_ was false, then clip region needs resetting.
      reset_clip_region = !need_redraw_;
      need_redraw_ = true;
    }

    if (reset_clip_region) {
      // Add whole view into clip region, so that all elements will be redrawn
      // correctly.
      clip_region_.Clear();
      clip_region_.AddRectangle(Rectangle(0, 0, width_, height_));
      if (on_add_rectangle_to_clip_region_.HasActiveConnections()) {
        on_add_rectangle_to_clip_region_(0, 0, width_, height_);
      }
    }

    CanvasInterface *target;
    if (canvas_cache_) {
      target = canvas_cache_;
      target->PushState();
      target->IntersectGeneralClipRegion(clip_region_);
      target->ClearRect(0, 0, width_, height_);
    } else {
      target = canvas;
      target->PushState();
    }

    BasicElement *popup = popup_element_.Get();
    double popup_rotation = 0;
    if (popup) {
      BasicElement *e = popup;
      for (; e != NULL; e = e->GetParentElement())
        popup_rotation += e->GetRotation();
    }

    // No need to draw children if there is a popup element and it's fully
    // opaque and the clip region is inside its extents.
    // If the popup element has non-horizontal/vertical orientation,
    // then the region of the popup element may not cover the whole clip
    // region. In this case it's not safe to skip drawing other children.
    if (!(canvas_cache_ && clip_region_enabled_ && popup &&
          popup->IsFullyOpaque() && fmod(popup_rotation, 90) == 0 &&
          clip_region_.IsInside(popup->GetExtentsInView()))) {
      children_.Draw(target);
    }

    if (popup && owner_->IsElementInClipRegion(popup)) {
      double pin_x = popup->GetPixelPinX();
      double pin_y = popup->GetPixelPinY();
      double abs_pin_x = 0;
      double abs_pin_y = 0;
      popup->SelfCoordToViewCoord(pin_x, pin_y, &abs_pin_x, &abs_pin_y);
      target->TranslateCoordinates(abs_pin_x, abs_pin_y);
      target->RotateCoordinates(DegreesToRadians(popup_rotation));
      target->TranslateCoordinates(-pin_x, -pin_y);
      popup->Draw(target);
    }

    target->PopState();

    if (target == canvas_cache_)
      canvas->DrawCanvas(0, 0, canvas_cache_);

#ifdef _DEBUG
    if (owner_->GetDebugMode() & DEBUG_CLIP_REGION)
      DrawClipRegionBox(clip_region_, canvas);
#endif

    clip_region_.Clear();
    need_redraw_ = false;
    content_changed_ = false;

#if defined(_DEBUG) && defined(VIEW_VERBOSE_DEBUG)
    uint64_t end = main_loop_->GetCurrentTime();
    if (end > 0 && start > 0) {
      accum_draw_time_ += (end - start);
      ++view_draw_count_;
      DLOG("Draw count: %d, time: %ju, average %lf",
           draw_count_, end - start,
           double(accum_draw_time_)/double(view_draw_count_));
    }
#endif
  }

#ifdef _DEBUG
  static bool DrawRectOnCanvasCallback(double x, double y, double w, double h,
                                       CanvasInterface *canvas) {
    static int color_index = 1;
    Color c(color_index & 1, (color_index >> 1) & 1, (color_index >> 2) & 1);
    canvas->DrawLine(x, y, x + w, y, 1, c);
    canvas->DrawLine(x + w, y, x + w, y + h, 1, c);
    canvas->DrawLine(x + w, y + h, x, y + h, 1, c);
    canvas->DrawLine(x, y + h, x, y, 1, c);
    color_index = (color_index >= 4 ? 1 : (color_index << 1));
    return true;
  }

  void DrawClipRegionBox(const ClipRegion &region, CanvasInterface *canvas) {
    region.EnumerateRectangles(NewSlot(DrawRectOnCanvasCallback, canvas));
  }
#endif

  bool OnAddContextMenuItems(MenuInterface *menu) {
    bool result = true;
    // Let the element handle context menu first, so that the element can
    // override view's menu.
    if (mouseover_element_.Get()) {
      if (mouseover_element_.Get()->IsReallyEnabled())
        result = mouseover_element_.Get()->OnAddContextMenuItems(menu);
      else
        mouseover_element_.Reset(NULL);
    }
    if (!result)
      return false;

    ContextMenuEvent event(new ScriptableMenu(gadget_, menu));
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    FireEvent(&scriptable_event, oncontextmenu_event_);
    if (scriptable_event.GetReturnValue() == EVENT_RESULT_CANCELED)
      return false;

    // If the view is main and the mouse over element doesn't have special menu
    // items, then add gadget's menu items.
    if (!view_host_) return false;
    if (gadget_ && view_host_->GetType() == ViewHostInterface::VIEW_HOST_MAIN)
      gadget_->OnAddCustomMenuItems(menu);

    return result;
  }

  bool OnSizing(double *width, double *height) {
    ASSERT(width);
    ASSERT(height);

    SizingEvent event(*width, *height);
    ScriptableEvent scriptable_event(&event, NULL, &event);

    FireEvent(&scriptable_event, onsizing_event_);
    bool result = (scriptable_event.GetReturnValue() != EVENT_RESULT_CANCELED);

#if defined(_DEBUG) && defined(EVENT_VERBOSE_DEBUG)
    DLOG("onsizing View %s, request: %lf x %lf, actual: %lf x %lf",
         (result ? "accepted" : "cancelled"),
         *width, *height, event.GetWidth(), event.GetHeight());
#endif

    if (result) {
      *width = event.GetWidth();
      *height = event.GetHeight();
      if (auto_width_ || auto_height_) {
        double desired_auto_width = 0;
        double desired_auto_height = 0;
        GetDesiredAutoSize(&desired_auto_width, &desired_auto_height);
        if (auto_width_)
          *width = desired_auto_width;
        if (auto_height_)
          *height = desired_auto_height;
      }

      *width = std::max(*width, min_width_);
      *height = std::max(*height, min_height_);
    }

    return result;
  }

  void FireEventSlot(ScriptableEvent *event, const Slot *slot) {
    ASSERT(event);
    ASSERT(slot);
    event->SetReturnValue(EVENT_RESULT_HANDLED);
    event_stack_.push_back(event);
    slot->Call(NULL, 0, NULL);
    event_stack_.pop_back();
  }

  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    if (events_enabled_ && event_signal.HasActiveConnections()) {
      SignalSlot slot(&event_signal);
      FireEventSlot(event, &slot);
    }
  }

  void FirePostedSizeEvents() {
    // Make a copy of posted_size_events_, because it may change during the
    // following loop.
    PostedSizeEvents posted_events_copy;
    std::swap(posted_size_events_, posted_events_copy);
    for (PostedSizeEvents::iterator it = posted_events_copy.begin();
         it != posted_events_copy.end(); ++it) {
      // Test if the event is still valid. If srcElement has been deleted,
      // it->first->GetSrcElement() should return NULL.
      if (it->first->GetSrcElement())
        FireEvent(it->first, *it->second);

      delete it->first->GetEvent();
      delete it->first;
    }
    posted_events_copy.clear();
  }

  void PostElementSizeEvent(BasicElement *element, const EventSignal &signal) {
    ASSERT(element);
    // Search if the size event has been posted for the element.
    for (PostedSizeEvents::const_iterator it = posted_size_events_.begin();
         it != posted_size_events_.end(); ++it) {
      if (it->first->GetSrcElement() == element) {
        // The size event already posted for the element.
        return;
      }
    }

    Event *event = new SimpleEvent(Event::EVENT_SIZE);
    ScriptableEvent *script_event = new ScriptableEvent(event, element, NULL);
    posted_size_events_.push_back(std::make_pair(script_event, &signal));
  }

  ScriptableEvent *GetEvent() {
    return event_stack_.empty() ? NULL : *(event_stack_.end() - 1);
  }

  BasicElement *GetElementByName(const char *name) {
    ElementsMap::iterator it = all_elements_.find(name);
    return it == all_elements_.end() ? NULL : it->second;
  }

  bool OnElementAdd(BasicElement *element) {
    ASSERT(element);
    if (element->IsInstanceOf(ContentAreaElement::CLASS_ID)) {
      if (content_area_element_.Get()) {
        LOG("Only one contentarea element is allowed in a view");
        return false;
      }
      content_area_element_.Reset(down_cast<ContentAreaElement *>(element));
    }

    std::string name = element->GetName();
    if (!name.empty() &&
        // Don't overwrite the existing element with the same name.
        all_elements_.find(name) == all_elements_.end())
      all_elements_[name] = element;
    return true;
  }

  // All references to this element should be cleared here.
  void OnElementRemove(BasicElement *element) {
    ASSERT(element);
    owner_->AddElementToClipRegion(element, NULL);

    // Clears tooltip immediately.
    if (element == tooltip_element_.Get() && view_host_)
      view_host_->ShowTooltip("");

    std::string name = element->GetName();
    if (!name.empty()) {
      ElementsMap::iterator it = all_elements_.find(name);
      if (it != all_elements_.end() && it->second == element)
        all_elements_.erase(it);
    }
  }

  void SetFocus(BasicElement *element) {
    if (element != focused_element_.Get() &&
        (!element || element->IsReallyEnabled())) {
      ElementHolder element_holder(element);
      // Remove the current focus first.
      if (!focused_element_.Get() ||
          focused_element_.Get()->OnOtherEvent(SimpleEvent(
              Event::EVENT_FOCUS_OUT)) != EVENT_RESULT_CANCELED) {
        ElementHolder old_focused_element(focused_element_.Get());
        focused_element_.Reset(element_holder.Get());
        // Only fire onfocusin event when the view is focused.
        if (view_focused_ && focused_element_.Get()) {
          // The enabled or visible state may changed, so check again.
          if (!focused_element_.Get()->IsReallyEnabled() ||
              focused_element_.Get()->OnOtherEvent(SimpleEvent(
                  Event::EVENT_FOCUS_IN)) == EVENT_RESULT_CANCELED) {
            // If the EVENT_FOCUS_IN is canceled, set focus back to the old
            // focused element.
            focused_element_.Reset(old_focused_element.Get());
            if (focused_element_.Get() &&
                focused_element_.Get()->OnOtherEvent(SimpleEvent(
                    Event::EVENT_FOCUS_IN)) == EVENT_RESULT_CANCELED) {
              focused_element_.Reset(NULL);
            }
          }
        }
      }
    }
  }

  void SetPopupElement(BasicElement *element) {
    if (popup_element_.Get()) {
      // To make sure the area covered by popup element can be redrawn
      // correctly.
      owner_->AddElementToClipRegion(popup_element_.Get(), NULL);
      popup_element_.Get()->OnPopupOff();
    }
    popup_element_.Reset(element);
    if (element) {
      element->QueueDraw();
    }
  }

  int BeginAnimation(Slot *slot, int start_value, int end_value,
                     int duration) {
    if (!slot) {
      DLOG("Invalid slot for animation.");
      return 0;
    }

    if (duration < 0) {
      DLOG("Invalid duration %d for animation.", duration);
      delete slot;
      return 0;
    }

    uint64_t current_time = main_loop_->GetCurrentTime();
    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, start_value, end_value,
                               duration, current_time, true);
    int id = main_loop_->AddTimeoutWatch(kAnimationInterval, watch);
    if (id > 0) {
      watch->SetWatchId(id);
    } else {
      DLOG("Failed to add animation timer.");
      delete watch;
    }
    return id;
  }

  int SetTimeout(Slot *slot, int timeout) {
    if (!slot) {
      LOG("Invalid slot for timeout.");
      return 0;
    }

    if (timeout < 0) {
      DLOG("Invalid timeout %d.", timeout);
      delete slot;
      return 0;
    }

    if (timeout < kMinTimeout)
      timeout = kMinTimeout;

    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, 0, 0, 0, 0, true);
    int id = main_loop_->AddTimeoutWatch(timeout, watch);
    if (id > 0) {
      watch->SetWatchId(id);
    } else {
      DLOG("Failed to add timeout timer.");
      delete watch;
    }
    return id;
  }

  int SetInterval(Slot *slot, int interval) {
    if (!slot) {
      LOG("Invalid slot for interval.");
      return 0;
    }

    if (interval < 0) {
      DLOG("Invalid interval %d.", interval);
      delete slot;
      return 0;
    }

    if (interval < kMinInterval)
      interval = kMinInterval;

    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, 0, 0, -1, 0, true);
    int id = main_loop_->AddTimeoutWatch(interval, watch);
    if (id > 0) {
      watch->SetWatchId(id);
    } else {
      DLOG("Failed to add interval timer.");
      delete watch;
    }
    return id;
  }

  void RemoveTimer(int token) {
    if (token > 0)
      main_loop_->RemoveWatch(token);
  }

  ImageInterface *LoadImage(const Variant &src, bool is_mask) {
    if (!graphics_) return NULL;

    switch (src.type()) {
      case Variant::TYPE_STRING: {
        const char *filename = VariantValue<const char*>()(src);
        FileManagerInterface *fm = owner_->GetFileManager();
        return image_cache_.LoadImage(graphics_, fm, filename, is_mask);
      }
      case Variant::TYPE_SCRIPTABLE: {
        const ScriptableBinaryData *binary =
            VariantValue<const ScriptableBinaryData *>()(src);
        return binary ? graphics_->NewImage("", binary->data(), is_mask) : NULL;
      }
      case Variant::TYPE_VOID:
        return NULL;
      default:
        LOG("Unsupported type of image src: '%s'", src.Print().c_str());
        DLOG("src=%s", src.Print().c_str());
        return NULL;
    }
  }

  ImageInterface *LoadImageFromGlobal(const char *name, bool is_mask) {
    return image_cache_.LoadImage(graphics_, NULL, name, is_mask);
  }

  Texture *LoadTexture(const Variant &src) {
    Color color;
    double opacity;
    if (src.type() == Variant::TYPE_STRING) {
      const char *name = VariantValue<const char *>()(src);
      if (name && !strchr(name, '.') &&
          Color::FromString(name, &color, &opacity))
        return new Texture(color, opacity);
    }

    ImageInterface *image = LoadImage(src, false);
    return image ? new Texture(image) : NULL;
  }

  void OnOptionChanged(const char *name) {
    ScopedLogContext log_context(gadget_);
    OptionChangedEvent event(name);
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    FireEvent(&scriptable_event, onoptionchanged_event_);
  }

  void AutoUpdateSize() {
    if (!auto_width_ && !auto_height_)
      return;

    double width = 0;
    double height = 0;
    GetDesiredAutoSize(&width, &height);
    SetSize(width, height);
  }

  void GetDesiredAutoSize(double *width, double *height) const {
    GetChildrenExtents(width, height);
    *width = auto_width_ ? *width : width_;
    *height = auto_height_ ? *height : height_;
  }

  void GetChildrenExtents(double *width, double *height) const {
    *width = 0;
    *height = 0;
    const size_t count = children_.GetCount();
    for (size_t i = 0; i < count; ++i) {
      const BasicElement *child = children_.GetItemByIndex(i);

      if (!child->IsVisible())
        continue;

      const double child_x = child->GetPixelX();
      const double child_y = child->GetPixelY();
      const Rectangle child_rect = child->GetMinExtentsInParent();
      const double child_right = child_rect.x + child_rect.w;
      const double child_bottom = child_rect.y + child_rect.h;

      if (child->XIsRelative()) {
        double child_extent = 0;
        const double relative_x = child->GetRelativeX();
        if (relative_x == 0) {
          child_extent = child_right - child->GetPixelX();
        } else if (relative_x == 1) {
          child_extent = child_x - child_rect.x;
        } else {
          child_extent = std::max(
              (child_x - child_rect.x) / relative_x,
              (child_right - child_x) / (1 - relative_x));
        }
        *width = std::max(*width, child_extent);
      } else {
        *width = std::max(*width, child_right);
      }

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
        *height = std::max(*height, child_extent);
      } else {
        *height = std::max(*height, child_bottom);
      }
    }
  }

  void SetGraphics(GraphicsInterface *graphics) {
    graphics_ = graphics;
    if (onzoom_connection_) {
      onzoom_connection_->Disconnect();
      onzoom_connection_ = NULL;
    }
    if (graphics_) {
      onzoom_connection_ = graphics_->ConnectOnZoom(NewSlot(this,
                                                           &Impl::OnZoom));
      zoom_ = graphics->GetZoom();
    }
  }

  void OnZoom(double zoom) {
    if (zoom_ == zoom) {
      return;
    }
    zoom_ = zoom;
    MarkRedraw();
  }

 public: // member variables
  double width_;
  double height_;
  double default_width_;
  double default_height_;
  double min_width_;
  double min_height_;
  double zoom_;
  bool rtl_;

  double resize_border_left_;
  double resize_border_top_;
  double resize_border_right_;
  double resize_border_bottom_;

  View *owner_;
  GadgetInterface *gadget_;
  ElementFactory *element_factory_;
  MainLoopInterface *main_loop_;
  ViewHostInterface *view_host_;
  ScriptContextInterface *script_context_;
  Connection *onoptionchanged_connection_;
  Connection *onzoom_connection_;
  CanvasInterface *canvas_cache_;
  GraphicsInterface *graphics_;
  ScriptableInterface *scriptable_view_;

  EventSignal oncancel_event_;
  EventSignal onclick_event_;
  EventSignal onclose_event_;
  EventSignal ondblclick_event_;
  EventSignal onrclick_event_;
  EventSignal onrdblclick_event_;
  EventSignal ondock_event_;
  EventSignal onkeydown_event_;
  EventSignal onkeypress_event_;
  EventSignal onkeyup_event_;
  EventSignal onminimize_event_;
  EventSignal onmousedown_event_;
  EventSignal onmousemove_event_;
  EventSignal onmouseout_event_;
  EventSignal onmouseover_event_;
  EventSignal onmouseup_event_;
  EventSignal onmousewheel_event_;
  EventSignal onok_event_;
  EventSignal onopen_event_;
  EventSignal onoptionchanged_event_;
  EventSignal onpopin_event_;
  EventSignal onpopout_event_;
  EventSignal onrestore_event_;
  EventSignal onsize_event_;
  EventSignal onsizing_event_;
  EventSignal onundock_event_;
  EventSignal oncontextmenu_event_;
  EventSignal onthemechanged_event_;

  Signal0<void> on_destroy_signal_;
  Signal4<void, double, double, double, double> on_add_rectangle_to_clip_region_;

  ImageCache image_cache_;

  // Note: though other things are case-insenstive, this map is case-sensitive,
  // to keep compatible with the Windows version.
  typedef LightMap<std::string, BasicElement *> ElementsMap;
  // Put all_elements_ here to make it the last member to be destructed,
  // because destruction of children_ needs it.
  ElementsMap all_elements_;

  ClipRegion clip_region_;

  Elements children_;

  ElementHolder focused_element_;
  ElementHolder mouseover_element_;
  ElementHolder grabmouse_element_;
  ElementHolder dragover_element_;
  ElementHolder tooltip_element_;
  ElementHolder popup_element_;
  ScriptableHolder<ContentAreaElement> content_area_element_;

  typedef std::vector<std::pair<ScriptableEvent *, const EventSignal *> >
      PostedSizeEvents;
  PostedSizeEvents posted_size_events_;
  std::vector<ScriptableEvent *> event_stack_;

  std::string caption_;

#ifdef _DEBUG
  int draw_count_;
  int view_draw_count_;
  uint64_t accum_draw_time_;
#endif
#if defined(OS_WIN)
  // MSVC treats enum bit fields as signed! That means if a 2-bit enum bit
  // field is assigned with an enum value of 3, when reading the field, the
  // result will be -1 instead of 3, because the highest bit is considered as
  // a sign bit. So surprisingly the result isn't equal to the original enum
  // value.
  HitTest hittest_;
  HitTest last_hittest_;
  CursorType last_cursor_type_;
  ResizableMode resizable_;
  EventResult dragover_result_ ;
#else
  HitTest hittest_              : 6;
  HitTest last_hittest_         : 6;
  CursorType last_cursor_type_  : 4;
  ResizableMode resizable_      : 2;
  EventResult dragover_result_  : 2;
#endif
  bool clip_region_enabled_     : 1;
  bool enable_cache_            : 1;
  bool show_caption_always_     : 1;
  bool draw_queued_             : 1;
  bool events_enabled_          : 1;
  bool need_redraw_             : 1;
  bool theme_changed_           : 1;
  bool resize_border_specified_ : 1;
  bool mouse_over_              : 1;
  bool view_focused_            : 1;
  bool safe_to_destroy_         : 1;
  bool content_changed_         : 1;
  bool auto_width_              : 1;
  bool auto_height_             : 1;

  static const int kAnimationInterval = 40;
  static const int kMinTimeout = 10;
  static const int kMinInterval = 10;
  static const uint64_t kMinTimeBetweenTimerCall = 5;
};

View::View(ViewHostInterface *view_host,
           GadgetInterface *gadget,
           ElementFactory *element_factory,
           ScriptContextInterface *script_context)
    : impl_(new Impl(this, view_host, gadget, element_factory, script_context)) {
  // Make sure that the view is initialized when attaching to the ViewHost.
  if (view_host) {
    if (!impl_->graphics_)
      impl_->SetGraphics(view_host->NewGraphics());
    view_host->SetView(this);
  }
}

View::~View() {
  GraphicsInterface *tmp = impl_->graphics_;
  delete impl_;
  delete tmp;
  impl_ = NULL;
}

GadgetInterface *View::GetGadget() const {
  return impl_->gadget_;
}

ScriptContextInterface *View::GetScriptContext() const {
  return impl_->script_context_;
}

FileManagerInterface *View::GetFileManager() const {
  GadgetInterface *gadget = GetGadget();
  return gadget ? gadget->GetFileManager() : NULL;
}

void View::Layout() {
  impl_->Layout();
}

GraphicsInterface *View::GetGraphics() const {
  return impl_->graphics_;
}

void View::RegisterProperties(RegisterableInterface *obj) const {
  impl_->RegisterProperties(obj);
}

void View::SetScriptable(ScriptableInterface *obj) {
  impl_->scriptable_view_ = obj;
  if (obj)
    RegisterProperties(obj->GetRegisterable());
}

ScriptableInterface *View::GetScriptable() const {
  return impl_->scriptable_view_;
}

bool View::IsSafeToDestroy() const {
  return impl_->event_stack_.empty() && impl_->safe_to_destroy_;
}

void View::SetAutoWidth(bool auto_width) {
  if (impl_->auto_width_ != auto_width) {
    impl_->auto_width_ = auto_width;
    impl_->AutoUpdateSize();
  }
}

bool View::IsAutoWidth() const {
  return impl_->auto_width_;
}

void View::SetAutoHeight(bool auto_height) {
  if (impl_->auto_height_ != auto_height) {
    impl_->auto_height_ = auto_height;
    impl_->AutoUpdateSize();
  }
}

bool View::IsAutoHeight() const {
  return impl_->auto_height_;
}

double View::GetMinWidth() const {
  return impl_->min_width_;
}

void View::SetMinWidth(double min_width) {
  if (impl_->min_width_ != min_width) {
    impl_->min_width_ = std::max(0.0, min_width);
    if (impl_->width_ < impl_->min_width_)
      impl_->SetSize(min_width, GetHeight());
  }
}

double View::GetMinHeight() const {
  return impl_->min_height_;
}

void View::SetMinHeight(double min_height) {
  if (impl_->min_height_ != min_height) {
    impl_->min_height_ = std::max(0.0, min_height);
    if (impl_->height_ < impl_->min_height_)
      impl_->SetSize(GetWidth(), min_height);
  }
}

bool View::IsTextRTL() const {
  return impl_->rtl_;
}

void View::SetTextRTL(bool rtl) {
  if (impl_->rtl_ != rtl) {
    impl_->rtl_ = rtl;
    QueueDraw();
  }
}

void View::SetWidth(double width) {
  SetSize(width, GetHeight());
}

void View::SetHeight(double height) {
  SetSize(GetWidth(), height);
}

void View::SetSize(double width, double height) {
  impl_->SetSize(width, height);
}

double View::GetWidth() const {
  return impl_->width_;
}

double View::GetHeight() const {
  return impl_->height_;
}

void View::GetDefaultSize(double *width, double *height) const {
  if (width) *width = impl_->default_width_;
  if (height) *height = impl_->default_height_;
}

void View::SetResizable(ViewInterface::ResizableMode resizable) {
  if (impl_->resizable_ != resizable) {
    impl_->resizable_ = resizable;

    if (!impl_->resize_border_specified_ && resizable == RESIZABLE_TRUE) {
      SetResizeBorder(8, 8, 8, 8);
    }

    if (impl_->view_host_)
      impl_->view_host_->SetResizable(resizable);
  }
}

ViewInterface::ResizableMode View::GetResizable() const {
  return impl_->resizable_;
}

void View::SetCaption(const std::string &caption) {
  impl_->caption_ = caption;
  if (impl_->view_host_)
    impl_->view_host_->SetCaption(caption);
}

std::string View::GetCaption() const {
  return impl_->caption_;
}

void View::SetShowCaptionAlways(bool show_always) {
  impl_->show_caption_always_ = show_always;
  if (impl_->view_host_)
    impl_->view_host_->SetShowCaptionAlways(show_always);
}

bool View::GetShowCaptionAlways() const {
  return impl_->show_caption_always_;
}

void View::SetResizeBorder(double left, double top,
                           double right, double bottom) {
  impl_->resize_border_specified_ = true;
  impl_->resize_border_left_ = (left > 0 ? left : 0);
  impl_->resize_border_top_ = (top > 0 ? top : 0);
  impl_->resize_border_right_ = (right > 0 ? right : 0);
  impl_->resize_border_bottom_ = (bottom > 0 ? bottom : 0);
  if (impl_->view_host_)
    impl_->view_host_->QueueResize();
}

bool View::GetResizeBorder(double *left, double *top,
                           double *right, double *bottom) const {
  if (left)
    *left = impl_->resize_border_left_;
  if (top)
    *top = impl_->resize_border_top_;
  if (right)
    *right = impl_->resize_border_right_;
  if (bottom)
    *bottom = impl_->resize_border_bottom_;
  return impl_->resize_border_specified_;
}

void View::MarkRedraw() {
  impl_->MarkRedraw();
}

void View::Draw(CanvasInterface *canvas) {
  ScopedLogContext log_context(impl_->gadget_);
  impl_->Draw(canvas);
}

const ClipRegion *View::GetClipRegion() const {
  return &impl_->clip_region_;
}

EventResult View::OnMouseEvent(const MouseEvent &event) {
  ScopedLogContext log_context(impl_->gadget_);
  return impl_->OnMouseEvent(event);
}

EventResult View::OnKeyEvent(const KeyboardEvent &event) {
  ScopedLogContext log_context(impl_->gadget_);
  return impl_->OnKeyEvent(event);
}

EventResult View::OnDragEvent(const DragEvent &event) {
  ScopedLogContext log_context(impl_->gadget_);
  return impl_->OnDragEvent(event);
}

EventResult View::OnOtherEvent(const Event &event) {
  ScopedLogContext log_context(impl_->gadget_);
  return impl_->OnOtherEvent(event);
}

ViewInterface::HitTest View::GetHitTest() const {
  return impl_->hittest_;
}

bool View::OnAddContextMenuItems(MenuInterface *menu) {
  ScopedLogContext log_context(impl_->gadget_);
  return impl_->OnAddContextMenuItems(menu);
}

bool View::OnSizing(double *width, double *height) {
  ScopedLogContext log_context(impl_->gadget_);
  return impl_->OnSizing(width, height);
}

void View::FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
  impl_->FireEvent(event, event_signal);
}

void View::PostElementSizeEvent(BasicElement *element,
                                const EventSignal &signal) {
  impl_->PostElementSizeEvent(element, signal);
}

ScriptableEvent *View::GetEvent() const {
  return impl_->GetEvent();
}

void View::EnableEvents(bool enable_events) {
  impl_->events_enabled_ = enable_events;
}

void View::EnableCanvasCache(bool enable_cache) {
  impl_->enable_cache_ = enable_cache;
  if (impl_->canvas_cache_ && !enable_cache) {
    impl_->canvas_cache_->Destroy();
    impl_->canvas_cache_ = NULL;
    QueueDraw();
  }
}

ElementFactory *View::GetElementFactory() const {
  return impl_->element_factory_;
}

Elements *View::GetChildren() const {
  return &impl_->children_;
}

BasicElement *View::GetElementByName(const char *name) const {
  return impl_->GetElementByName(name);
}

bool View::OnElementAdd(BasicElement *element) {
  return impl_->OnElementAdd(element);
}

void View::OnElementRemove(BasicElement *element) {
  impl_->OnElementRemove(element);
}

void View::SetFocus(BasicElement *element) {
  impl_->SetFocus(element);
}

void View::SetPopupElement(BasicElement *element) {
  impl_->SetPopupElement(element);
}

BasicElement *View::GetPopupElement() const {
  return impl_->popup_element_.Get();
}

BasicElement *View::GetFocusedElement() const {
  return impl_->focused_element_.Get();
}

BasicElement *View::GetMouseOverElement() const {
  return impl_->mouseover_element_.Get();
}

ContentAreaElement *View::GetContentAreaElement() const {
  return impl_->content_area_element_.Get();
}

bool View::IsElementInClipRegion(const BasicElement *element) const {
  return !impl_->clip_region_enabled_ ||
      impl_->clip_region_.Overlaps(element->GetExtentsInView());
}


void View::AddElementToClipRegion(BasicElement *element,
                                  const Rectangle *rect) {
  Rectangle element_rect = rect ? element->GetRectExtentsInView(*rect) :
      element->GetExtentsInView();
  element_rect.Integerize(true);
  impl_->clip_region_.AddRectangle(element_rect);
}

void View::EnableClipRegion(bool enable) {
  impl_->clip_region_enabled_ = enable;
}

bool View::IsClipRegionEnabled() const {
  return impl_->clip_region_enabled_;
}

void View::AddRectangleToClipRegion(const Rectangle &rect) {
  if (!impl_->enable_cache_) {
    Rectangle view_rect(0, 0, impl_->width_, impl_->height_);
    if (view_rect.Intersect(rect)) {
      view_rect.Integerize(true);
      impl_->clip_region_.AddRectangle(view_rect);
      if (impl_->on_add_rectangle_to_clip_region_.HasActiveConnections()) {
        impl_->on_add_rectangle_to_clip_region_(
            view_rect.x, view_rect.y, view_rect.w, view_rect.h);
      }
    }
  }
}

void View::IncreaseDrawCount() {
#ifdef _DEBUG
  impl_->draw_count_++;
#endif
}

int View::BeginAnimation(Slot0<void> *slot,
                         int start_value,
                         int end_value,
                         int duration) {
  return impl_->BeginAnimation(slot, start_value, end_value, duration);
}

void View::CancelAnimation(int token) {
  impl_->RemoveTimer(token);
}

int View::SetTimeout(Slot0<void> *slot, int timeout) {
  return impl_->SetTimeout(slot, timeout);
}

void View::ClearTimeout(int token) {
  impl_->RemoveTimer(token);
}

int View::SetInterval(Slot0<void> *slot, int interval) {
  return impl_->SetInterval(slot, interval);
}

void View::ClearInterval(int token) {
  impl_->RemoveTimer(token);
}

ImageInterface *View::LoadImage(const Variant &src, bool is_mask) const {
  return impl_->LoadImage(src, is_mask);
}

ImageInterface *
View::LoadImageFromGlobal(const char *name, bool is_mask) const {
  return impl_->LoadImageFromGlobal(name, is_mask);
}

Texture *View::LoadTexture(const Variant &src) const {
  return impl_->LoadTexture(src);
}

void *View::GetNativeWidget() const {
  return impl_->view_host_ ? impl_->view_host_->GetNativeWidget() : NULL;
}

// Note! view should not change between different kinds of view hosts,
// since the graphics compatibility issue
ViewHostInterface *View::SwitchViewHost(ViewHostInterface *new_host) {
  ViewHostInterface *old_host = impl_->view_host_;
  old_host->SetView(NULL);
  if (impl_->canvas_cache_) {
    impl_->canvas_cache_->Destroy();
    impl_->canvas_cache_ = NULL;
  }
  impl_->view_host_ = new_host;
  if (new_host) {
    if (!impl_->graphics_)
      impl_->SetGraphics(new_host->NewGraphics());
    new_host->SetView(this);
    MarkRedraw();
    new_host->QueueDraw();
  }
  return old_host;
}

ViewHostInterface *View::GetViewHost() const {
  return impl_->view_host_;
}

void View::ViewCoordToNativeWidgetCoord(double x, double y, double *widget_x,
                                        double *widget_y) const {
  if (impl_->view_host_)
    impl_->view_host_->ViewCoordToNativeWidgetCoord(x, y, widget_x, widget_y);
}

void View::NativeWidgetCoordToViewCoord(double x, double y,
                                        double *view_x, double *view_y) const {
  if (impl_->view_host_)
    impl_->view_host_->NativeWidgetCoordToViewCoord(x, y, view_x, view_y);
}

void View::QueueDraw() {
  if (!impl_->draw_queued_ && impl_->view_host_) {
    impl_->draw_queued_ = true;
    impl_->view_host_->QueueDraw();
  }
}

int View::GetDebugMode() const {
  return impl_->view_host_ ? impl_->view_host_->GetDebugMode() : DEBUG_DISABLED;
}

bool View::OpenURL(const char *url) const {
  return impl_->gadget_ ? impl_->gadget_->OpenURL(url) : false;
}

void View::Alert(const char *message) const {
  if (impl_->view_host_) {
    impl_->safe_to_destroy_ = false;
    bool old_interaction =
      impl_->gadget_ ? impl_->gadget_->SetInUserInteraction(true) : false;
    impl_->view_host_->Alert(this, message);
    if (impl_->gadget_)
      impl_->gadget_->SetInUserInteraction(old_interaction);
    impl_->safe_to_destroy_ = true;
  }
}

ViewHostInterface::ConfirmResponse View::Confirm(const char *message,
                                                 bool cancel_button) const {
  ViewHostInterface::ConfirmResponse result =
    cancel_button ? ViewHostInterface::CONFIRM_CANCEL :
                    ViewHostInterface::CONFIRM_NO;
  if (impl_->view_host_) {
    impl_->safe_to_destroy_ = false;
    bool old_interaction =
      impl_->gadget_ ? impl_->gadget_->SetInUserInteraction(true) : false;
    result = impl_->view_host_->Confirm(this, message, cancel_button);
    if (impl_->gadget_)
      impl_->gadget_->SetInUserInteraction(old_interaction);
    impl_->safe_to_destroy_ = true;
  }
  return result;
}

std::string View::Prompt(const char *message,
                         const char *default_result) const {
  std::string result;
  if (impl_->view_host_) {
    impl_->safe_to_destroy_ = false;
    bool old_interaction =
      impl_->gadget_ ? impl_->gadget_->SetInUserInteraction(true) : false;
    result = impl_->view_host_->Prompt(this, message, default_result);
    if (impl_->gadget_)
      impl_->gadget_->SetInUserInteraction(old_interaction);
    impl_->safe_to_destroy_ = true;
  }
  return result;
}

uint64_t View::GetCurrentTime() const {
  return impl_->main_loop_->GetCurrentTime();
}

void View::ShowElementTooltip(BasicElement *element) {
  ASSERT(element);
  ASSERT(element->GetView() == this);
  impl_->tooltip_element_.Reset(element);
  if (impl_->view_host_)
    impl_->view_host_->ShowTooltip(element->GetTooltip());
}

void View::ShowElementTooltipAtPosition(BasicElement *element,
                                        double x, double y) {
  ASSERT(element);
  ASSERT(element->GetView() == this);
  impl_->tooltip_element_.Reset(element);
  if (impl_->view_host_) {
    element->SelfCoordToViewCoord(x, y, &x, &y);
    impl_->view_host_->ShowTooltipAtPosition(element->GetTooltip(), x, y);
  }
}

void View::SetCursor(ViewInterface::CursorType type) {
  if (impl_->view_host_ && (impl_->last_cursor_type_ != type ||
                            impl_->last_hittest_ != impl_->hittest_)) {
    impl_->last_cursor_type_ = type;
    impl_->last_hittest_ = impl_->hittest_;
    impl_->view_host_->SetCursor(type);
  }
}

bool View::ShowView(bool modal, int flags, Slot1<bool, int> *feedback_handler) {
  return impl_->view_host_ ?
         impl_->view_host_->ShowView(modal, flags, feedback_handler) :
         false;
}

void View::CloseView() {
  if (impl_->view_host_)
    impl_->view_host_->CloseView();
}

int View::GetDefaultFontSize() const {
  return impl_->gadget_ ?
         impl_->gadget_->GetDefaultFontSize() : kDefaultFontSize;
}

bool View::IsFocused() const {
  return impl_->view_focused_;
}

Connection *View::ConnectOnCancelEvent(Slot0<void> *handler) {
  return impl_->oncancel_event_.Connect(handler);
}
Connection *View::ConnectOnClickEvent(Slot0<void> *handler) {
  return impl_->onclick_event_.Connect(handler);
}
Connection *View::ConnectOnCloseEvent(Slot0<void> *handler) {
  return impl_->onclose_event_.Connect(handler);
}
Connection *View::ConnectOnDblClickEvent(Slot0<void> *handler) {
  return impl_->ondblclick_event_.Connect(handler);
}
Connection *View::ConnectOnRClickEvent(Slot0<void> *handler) {
  return impl_->onrclick_event_.Connect(handler);
}
Connection *View::ConnectOnRDblClickCancelEvent(Slot0<void> *handler) {
  return impl_->onrdblclick_event_.Connect(handler);
}
Connection *View::ConnectOnDockEvent(Slot0<void> *handler) {
  return impl_->ondock_event_.Connect(handler);
}
Connection *View::ConnectOnKeyDownEvent(Slot0<void> *handler) {
  return impl_->onkeydown_event_.Connect(handler);
}
Connection *View::ConnectOnPressEvent(Slot0<void> *handler) {
  return impl_->onkeypress_event_.Connect(handler);
}
Connection *View::ConnectOnKeyUpEvent(Slot0<void> *handler) {
  return impl_->onkeyup_event_.Connect(handler);
}
Connection *View::ConnectOnMinimizeEvent(Slot0<void> *handler) {
  return impl_->onminimize_event_.Connect(handler);
}
Connection *View::ConnectOnMouseDownEvent(Slot0<void> *handler) {
  return impl_->onmousedown_event_.Connect(handler);
}
Connection *View::ConnectOnMouseMoveEvent(Slot0<void> *handler) {
  return impl_->onmousemove_event_.Connect(handler);
}
Connection *View::ConnectOnMouseOverEvent(Slot0<void> *handler) {
  return impl_->onmouseover_event_.Connect(handler);
}
Connection *View::ConnectOnMouseOutEvent(Slot0<void> *handler) {
  return impl_->onmouseout_event_.Connect(handler);
}
Connection *View::ConnectOnMouseUpEvent(Slot0<void> *handler) {
  return impl_->onmouseup_event_.Connect(handler);
}
Connection *View::ConnectOnMouseWheelEvent(Slot0<void> *handler) {
  return impl_->onmousewheel_event_.Connect(handler);
}
Connection *View::ConnectOnOkEvent(Slot0<void> *handler) {
  return impl_->onok_event_.Connect(handler);
}
Connection *View::ConnectOnOpenEvent(Slot0<void> *handler) {
  return impl_->onopen_event_.Connect(handler);
}
Connection *View::ConnectOnOptionChangedEvent(Slot0<void> *handler) {
  return impl_->onoptionchanged_event_.Connect(handler);
}
Connection *View::ConnectOnPopInEvent(Slot0<void> *handler) {
  return impl_->onpopin_event_.Connect(handler);
}
Connection *View::ConnectOnPopOutEvent(Slot0<void> *handler) {
  return impl_->onpopout_event_.Connect(handler);
}
Connection *View::ConnectOnRestoreEvent(Slot0<void> *handler) {
  return impl_->onrestore_event_.Connect(handler);
}
Connection *View::ConnectOnSizeEvent(Slot0<void> *handler) {
  return impl_->onsize_event_.Connect(handler);
}
Connection *View::ConnectOnSizingEvent(Slot0<void> *handler) {
  return impl_->onsizing_event_.Connect(handler);
}
Connection *View::ConnectOnUndockEvent(Slot0<void> *handler) {
  return impl_->onundock_event_.Connect(handler);
}
Connection *View::ConnectOnContextMenuEvent(Slot0<void> *handler) {
  return impl_->oncontextmenu_event_.Connect(handler);
}
Connection *View::ConnectOnThemeChangedEvent(Slot0<void> *handler) {
  return impl_->onthemechanged_event_.Connect(handler);
}
Connection *View::ConnectOnAddRectangleToClipRegion(
    Slot4<void, double, double, double, double> *handler) {
  return impl_->on_add_rectangle_to_clip_region_.Connect(handler);
}

} // namespace ggadget
