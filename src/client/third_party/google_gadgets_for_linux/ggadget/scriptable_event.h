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

#ifndef GGADGET_SCRIPTABLE_EVENT_H__
#define GGADGET_SCRIPTABLE_EVENT_H__

#include <ggadget/common.h>
#include <ggadget/event.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>

namespace ggadget {

class BasicElement;

/**
 * @ingroup Event
 * @{
 */

//@{
/** Event names */
const char kOnCancelEvent[]        = "oncancel";
const char kOnClickEvent[]         = "onclick";
const char kOnCloseEvent[]         = "onclose";
const char kOnDblClickEvent[]      = "ondblclick";
const char kOnRClickEvent[]        = "onrclick";
const char kOnRDblClickEvent[]     = "onrdblclick";
const char kOnDragDropEvent[]      = "ondragdrop";
const char kOnDragOutEvent[]       = "ondragout";
const char kOnDragOverEvent[]      = "ondragover";
const char kOnFocusInEvent[]       = "onfocusin";
const char kOnFocusOutEvent[]      = "onfocusout";
const char kOnDockEvent[]          = "ondock";
const char kOnKeyDownEvent[]       = "onkeydown";
const char kOnKeyPressEvent[]      = "onkeypress";
const char kOnKeyUpEvent[]         = "onkeyup";
const char kOnMinimizeEvent[]      = "onminimize";
const char kOnMouseDownEvent[]     = "onmousedown";
const char kOnMouseMoveEvent[]     = "onmousemove";
const char kOnMouseOutEvent[]      = "onmouseout";
const char kOnMouseOverEvent[]     = "onmouseover";
const char kOnMouseUpEvent[]       = "onmouseup";
const char kOnMouseWheelEvent[]    = "onmousewheel";
const char kOnOkEvent[]            = "onok";
const char kOnOpenEvent[]          = "onopen";
const char kOnOptionChangedEvent[] = "onoptionchanged";
const char kOnPopInEvent[]         = "onpopin";
const char kOnPopOutEvent[]        = "onpopout";
const char kOnRestoreEvent[]       = "onrestore";
const char kOnSizeEvent[]          = "onsize";
const char kOnSizingEvent[]        = "onsizing";
const char kOnUndockEvent[]        = "onundock";
const char kOnChangeEvent[]        = "onchange";
const char kOnTextChangeEvent[]    = "ontextchange";
const char kOnContextMenuEvent[]   = "oncontextmenu";
const char kOnStateChangeEvent[]   = "onstatechange";
const char kOnMediaChangeEvent[]   = "onmediachange";
const char kOnThemeChangedEvent[]  = "onthemechanged";
//@}

/**
 * @ingroup ScriptableObjects
 * Scriptable decorator for @c Event.
 */
class ScriptableEvent : public ScriptableHelperNativeOwnedDefault {
 public:
  /**
   * Not using DEFINE_CLASS_ID because we use dynamic CLASS_IDs according to
   * actual type of the input event.
   */
  static const uint64_t CLASS_ID = UINT64_C(0x6732238aacb4468a);

 public:
  /**
   * @param event it's not declared as a const reference because sometimes we
   *     need dynamically allocated event (e.g. @c View::PostEvent()).
   * @param src_element the element or view from which the event is fired.
   * @param output_event only used for some events (such as
   *     @c Event::EVENT_SIZING) to store the output event.
   *     Can be @c NULL if the event has no output.
   */
  ScriptableEvent(const Event *event,
                  ScriptableInterface *src_element,
                  Event *output_event);
  virtual ~ScriptableEvent();

  virtual bool IsInstanceOf(uint64_t class_id) const;
  virtual uint64_t GetClassId() const;

 protected:
  virtual void DoClassRegister();

 public:
  const char *GetName() const;
  const Event *GetEvent() const;

  const Event *GetOutputEvent() const;
  Event *GetOutputEvent();

  ScriptableInterface *GetSrcElement();
  const ScriptableInterface *GetSrcElement() const;
  void SetSrcElement(ScriptableInterface *src_element);

  EventResult GetReturnValue() const;
  void SetReturnValue(EventResult return_value);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableEvent);
};

/** @} */

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_EVENT_H__
