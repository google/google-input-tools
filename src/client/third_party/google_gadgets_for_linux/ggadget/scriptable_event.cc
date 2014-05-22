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

#include "scriptable_event.h"
#include "basic_element.h"
#include "event.h"
#include "scriptable_array.h"
#include "scriptable_menu.h"
#include "small_object.h"

namespace ggadget {

static const uint64_t kMouseEventClassId = UINT64_C(0x06fccf33c75e4445);
static const uint64_t kKeyboardEventClassId = UINT64_C(0xf8f4522e6ad346a4);
static const uint64_t kDragEventClassId = UINT64_C(0x7fd0f2cdae9d4689);
static const uint64_t kSizingEventClassId = UINT64_C(0xba226642c2d94168);
static const uint64_t kOptionChangedEventClassId = UINT64_C(0x8c13c37976f0443d);
static const uint64_t kTimerEventClassId = UINT64_C(0xc7de1daa11a0489b);
static const uint64_t kPerfmonEventClassId = UINT64_C(0x4109a5fb49c84ae6);
static const uint64_t kContextMenuEventClassId = UINT64_C(0x5899c2c72f0e4f22);

class ScriptableEvent::Impl : public SmallObject<> {
 public:
  Impl(const Event *event, ScriptableInterface *src_element,
       Event *output_event)
       : event_(event),
         output_event_(output_event),
         src_element_(src_element),
         return_value_(EVENT_RESULT_UNHANDLED) {
    if (event->IsMouseEvent()) {
      class_id_ = kMouseEventClassId;
    } else if (event->IsKeyboardEvent()) {
      class_id_ = kKeyboardEventClassId;
    } else if (event->IsDragEvent()) {
      class_id_ = kDragEventClassId;
    } else {
      Event::Type type = event->GetType();
      switch (type) {
        case Event::EVENT_SIZING:
          class_id_ = kSizingEventClassId;
          break;
        case Event::EVENT_OPTION_CHANGED:
          class_id_ = kOptionChangedEventClassId;
          break;
        case Event::EVENT_TIMER:
          class_id_ = kTimerEventClassId;
          break;
        case Event::EVENT_PERFMON:
          class_id_ = kPerfmonEventClassId;
          break;
        case Event::EVENT_CONTEXT_MENU:
          class_id_ = kContextMenuEventClassId;
          break;
        default:
          class_id_ = ScriptableEvent::CLASS_ID;
          break;
      }
    }
  }

  ScriptableArray *ScriptGetDragFiles() {
    ASSERT(event_->IsDragEvent());
    const DragEvent *drag_event = static_cast<const DragEvent *>(event_);
    return ScriptableArray::Create(drag_event->GetDragFiles());
  }

  ScriptableArray *ScriptGetDragUrls() {
    ASSERT(event_->IsDragEvent());
    const DragEvent *drag_event = static_cast<const DragEvent *>(event_);
    return ScriptableArray::Create(drag_event->GetDragUrls());
  }

  bool ScriptGetReturnValue() {
    return return_value_ != EVENT_RESULT_CANCELED;
  }

  void ScriptSetReturnValue(bool value) {
    return_value_ = value ? EVENT_RESULT_HANDLED : EVENT_RESULT_CANCELED;
  }

  ScriptableInterface *GetSrcElement() {
    return src_element_.Get();
  }

  ScriptableMenu *GetMenu() {
    ASSERT(event_->GetType() == Event::EVENT_CONTEXT_MENU);
    return static_cast<const ContextMenuEvent *>(event_)->GetMenu();
  }

  DEFINE_DELEGATE_GETTER_CONST(
      GetEvent, src->impl_->event_, ScriptableEvent, Event);
  DEFINE_DELEGATE_GETTER_CONST(
      GetMouseEvent, static_cast<const MouseEvent *>(src->impl_->event_),
      ScriptableEvent, MouseEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetPositionEvent, static_cast<const PositionEvent *>(src->impl_->event_),
      ScriptableEvent, PositionEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetDragEvent, static_cast<const DragEvent *>(src->impl_->event_),
      ScriptableEvent, DragEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetKeyboardEvent, static_cast<const KeyboardEvent *>(src->impl_->event_),
      ScriptableEvent, KeyboardEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetSizingEvent, static_cast<const SizingEvent *>(src->impl_->event_),
      ScriptableEvent, SizingEvent);
  DEFINE_DELEGATE_GETTER_NO_CONST(
      GetOutputSizingEvent,
      static_cast<SizingEvent *>(src->impl_->output_event_),
      ScriptableEvent, SizingEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetOptionChangedEvent,
      static_cast<const OptionChangedEvent *>(src->impl_->event_),
      ScriptableEvent, OptionChangedEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetTimerEvent, static_cast<const TimerEvent *>(src->impl_->event_),
      ScriptableEvent, TimerEvent);
  DEFINE_DELEGATE_GETTER_CONST(
      GetPerfmonEvent, static_cast<const PerfmonEvent *>(src->impl_->event_),
      ScriptableEvent, PerfmonEvent);

  uint64_t class_id_;
  const Event *event_;
  Event *output_event_;
  ScriptableHolder<ScriptableInterface> src_element_;
  EventResult return_value_;
};

ScriptableEvent::ScriptableEvent(const Event *event,
                                 ScriptableInterface *src_element,
                                 Event *output_event)
    : impl_(new Impl(event, src_element, output_event)) {
}

void ScriptableEvent::DoClassRegister() {
  RegisterProperty("returnValue",
                   NewSlot(&Impl::ScriptGetReturnValue,
                           &ScriptableEvent::impl_),
                   NewSlot(&Impl::ScriptSetReturnValue,
                           &ScriptableEvent::impl_));
  RegisterProperty("srcElement",
                   NewSlot(&Impl::GetSrcElement, &ScriptableEvent::impl_),
                   NULL);
  RegisterProperty("type", NewSlot(&ScriptableEvent::GetName), NULL);

  switch (impl_->class_id_) {
    case kMouseEventClassId:
      RegisterProperty("x",
          NewSlot(&PositionEvent::GetX, Impl::GetPositionEvent), NULL);
      RegisterProperty("y",
          NewSlot(&PositionEvent::GetY, Impl::GetPositionEvent), NULL);
      RegisterProperty("button",
          NewSlot(&MouseEvent::GetButton, Impl::GetMouseEvent), NULL);
      RegisterProperty("wheelDelta",
          NewSlot(&MouseEvent::GetWheelDeltaY, Impl::GetMouseEvent), NULL);
      RegisterProperty("wheelDeltaX",
          NewSlot(&MouseEvent::GetWheelDeltaX, Impl::GetMouseEvent), NULL);
      RegisterProperty("wheelDeltaY",
          NewSlot(&MouseEvent::GetWheelDeltaY, Impl::GetMouseEvent), NULL);
      break;
    case kKeyboardEventClassId:
      RegisterProperty("keyCode",
          NewSlot(&KeyboardEvent::GetKeyCode, Impl::GetKeyboardEvent), NULL);
      break;
    case kDragEventClassId:
      RegisterProperty("x",
          NewSlot(&PositionEvent::GetX, Impl::GetPositionEvent), NULL);
      RegisterProperty("y",
          NewSlot(&PositionEvent::GetY, Impl::GetPositionEvent), NULL);
      RegisterProperty("dragFiles",
          NewSlot(&Impl::ScriptGetDragFiles, &ScriptableEvent::impl_), NULL);
      RegisterProperty("dragUrls",
          NewSlot(&Impl::ScriptGetDragUrls, &ScriptableEvent::impl_), NULL);
      RegisterProperty("dragText",
          NewSlot(&DragEvent::GetDragText, Impl::GetDragEvent), NULL);
      break;
    case kSizingEventClassId:
      ASSERT(impl_->output_event_ &&
             impl_->output_event_->GetType() == Event::EVENT_SIZING);
      RegisterProperty("width",
          NewSlot(&SizingEvent::GetWidth, Impl::GetSizingEvent),
          NewSlot(&SizingEvent::SetWidth, Impl::GetOutputSizingEvent));
      RegisterProperty("height",
          NewSlot(&SizingEvent::GetHeight, Impl::GetSizingEvent),
          NewSlot(&SizingEvent::SetHeight, Impl::GetOutputSizingEvent));
      break;
    case kOptionChangedEventClassId:
      RegisterProperty("propertyName",
                       NewSlot(&OptionChangedEvent::GetPropertyName,
                               Impl::GetOptionChangedEvent),
                       NULL);
      break;
    case kTimerEventClassId:
      RegisterProperty("cookie",
          NewSlot(&TimerEvent::GetToken, Impl::GetTimerEvent), NULL);
      RegisterProperty("value",
          NewSlot(&TimerEvent::GetValue, Impl::GetTimerEvent), NULL);
      break;
    case kPerfmonEventClassId:
      RegisterProperty("value",
          NewSlot(&PerfmonEvent::GetValue, Impl::GetPerfmonEvent), NULL);
      break;
    case kContextMenuEventClassId:
      RegisterProperty("menu",
                       NewSlot(&Impl::GetMenu, &ScriptableEvent::impl_), NULL);
    default:
      break;
  }
}

ScriptableEvent::~ScriptableEvent() {
  delete impl_;
  impl_ = NULL;
}

bool ScriptableEvent::IsInstanceOf(uint64_t class_id) const {
  return class_id == impl_->class_id_ || class_id == CLASS_ID ||
         ScriptableInterface::IsInstanceOf(class_id);
}

uint64_t ScriptableEvent::GetClassId() const {
  return impl_->class_id_;
}

const char *ScriptableEvent::GetName() const {
  switch (impl_->event_->GetType()) {
    case Event::EVENT_CANCEL: return kOnCancelEvent;
    case Event::EVENT_CLOSE: return kOnCloseEvent;
    case Event::EVENT_DOCK: return kOnDockEvent;
    case Event::EVENT_MINIMIZE: return kOnMinimizeEvent;
    case Event::EVENT_OK: return kOnOkEvent;
    case Event::EVENT_OPEN: return kOnOpenEvent;
    case Event::EVENT_POPIN: return kOnPopInEvent;
    case Event::EVENT_POPOUT: return kOnPopOutEvent;
    case Event::EVENT_RESTORE: return kOnRestoreEvent;
    case Event::EVENT_SIZE: return kOnSizeEvent;
    case Event::EVENT_UNDOCK: return kOnUndockEvent;
    case Event::EVENT_FOCUS_IN: return kOnFocusInEvent;
    case Event::EVENT_FOCUS_OUT: return kOnFocusOutEvent;
    case Event::EVENT_CHANGE: return kOnChangeEvent;
    // Windows version returns "onchange" for "ontextchange" events,
    // so we do the same. Both "onchange" and "ontextchange" are of
    // EVENT_CHANGE type.
    case Event::EVENT_STATE_CHANGE: return kOnStateChangeEvent;
    case Event::EVENT_MEDIA_CHANGE: return kOnMediaChangeEvent;
    case Event::EVENT_THEME_CHANGED: return kOnThemeChangedEvent;

    case Event::EVENT_MOUSE_DOWN: return kOnMouseDownEvent;
    case Event::EVENT_MOUSE_UP: return kOnMouseUpEvent;
    case Event::EVENT_MOUSE_CLICK: return kOnClickEvent;
    case Event::EVENT_MOUSE_DBLCLICK: return kOnDblClickEvent;
    case Event::EVENT_MOUSE_RCLICK: return kOnRClickEvent;
    case Event::EVENT_MOUSE_RDBLCLICK: return kOnRDblClickEvent;
    case Event::EVENT_MOUSE_MOVE: return kOnMouseMoveEvent;
    case Event::EVENT_MOUSE_OUT: return kOnMouseOutEvent;
    case Event::EVENT_MOUSE_OVER: return kOnMouseOverEvent;
    case Event::EVENT_MOUSE_WHEEL: return kOnMouseWheelEvent;

    case Event::EVENT_KEY_DOWN: return kOnKeyDownEvent;
    case Event::EVENT_KEY_UP: return kOnKeyUpEvent;
    case Event::EVENT_KEY_PRESS: return kOnKeyPressEvent;

    case Event::EVENT_DRAG_DROP: return kOnDragDropEvent;
    case Event::EVENT_DRAG_OUT: return kOnDragOutEvent;
    case Event::EVENT_DRAG_OVER: return kOnDragOverEvent;

    case Event::EVENT_SIZING: return kOnSizingEvent;
    case Event::EVENT_OPTION_CHANGED: return kOnOptionChangedEvent;
    case Event::EVENT_TIMER: return "";  // Windows version does the same.
    case Event::EVENT_PERFMON: return ""; // FIXME: Is it correct?
    case Event::EVENT_CONTEXT_MENU: return kOnContextMenuEvent;
    default: ASSERT(false); return "";
  }
}

const Event *ScriptableEvent::GetEvent() const {
  return impl_->event_;
}

const Event *ScriptableEvent::GetOutputEvent() const {
  return impl_->output_event_;
}
Event *ScriptableEvent::GetOutputEvent() {
  return impl_->output_event_;
}

ScriptableInterface *ScriptableEvent::GetSrcElement() {
  return impl_->src_element_.Get();
}
const ScriptableInterface *ScriptableEvent::GetSrcElement() const {
  return impl_->src_element_.Get();
}
void ScriptableEvent::SetSrcElement(ScriptableInterface *src_element) {
  impl_->src_element_.Reset(src_element);
}

EventResult ScriptableEvent::GetReturnValue() const {
  return impl_->return_value_;
}

void ScriptableEvent::SetReturnValue(EventResult return_value) {
  impl_->return_value_ = return_value;
}

} // namespace ggadget
