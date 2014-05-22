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

#ifndef GGADGET_EVENT_H__
#define GGADGET_EVENT_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/scriptable_menu.h>
#include <ggadget/variant.h>

namespace ggadget {

/**
 * @defgroup Event Event
 * @ingroup View
 * Event related classes.
 * @{
 */

/** Used to indicate the result of an event handler. */
enum EventResult {
  /** The event is not handled in the handler. */
  EVENT_RESULT_UNHANDLED,
  /** The event is handled normally by the handler. */
  EVENT_RESULT_HANDLED,
  /** The handler wants the default action to be canceled. */
  EVENT_RESULT_CANCELED
};

/**
 * Class for holding event information. There are several subclasses for
 * events features in common.
 */
class Event {
 public:
  enum Type {
    EVENT_INVALID = 0,
    EVENT_SIMPLE_RANGE_START = 0,
    EVENT_CANCEL,
    EVENT_CLOSE,
    EVENT_DOCK,
    EVENT_MINIMIZE,
    EVENT_OK,
    EVENT_OPEN,
    EVENT_POPIN,
    EVENT_POPOUT,
    EVENT_RESTORE,
    EVENT_SIZE,
    EVENT_UNDOCK,
    EVENT_FOCUS_IN,
    EVENT_FOCUS_OUT,
    EVENT_CHANGE,
    EVENT_STATE_CHANGE,
    EVENT_MEDIA_CHANGE,
    EVENT_THEME_CHANGED,
    EVENT_SIMPLE_RANGE_END,

    EVENT_MOUSE_RANGE_START = 10000,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_CLICK,
    EVENT_MOUSE_DBLCLICK,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_OUT,
    EVENT_MOUSE_OVER,
    EVENT_MOUSE_WHEEL,
    EVENT_MOUSE_RCLICK,
    EVENT_MOUSE_RDBLCLICK,
    EVENT_MOUSE_RANGE_END,

    EVENT_KEY_RANGE_START = 20000,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_PRESS,
    EVENT_KEY_RANGE_END,

    EVENT_DRAG_RANGE_START = 30000,
    EVENT_DRAG_DROP,
    EVENT_DRAG_OUT,
    EVENT_DRAG_OVER,
    EVENT_DRAG_MOTION, // Only for dispatching in C++ code.
    EVENT_DRAG_RANGE_END,

    // Other uncategorized events.
    EVENT_UNCATEGORIZED = 40000,
    EVENT_SIZING,
    EVENT_OPTION_CHANGED,
    EVENT_TIMER,
    EVENT_PERFMON,
    EVENT_CONTEXT_MENU,
  };

  enum Modifier {
    MODIFIER_NONE = 0,
    MODIFIER_SHIFT = 1,
    MODIFIER_CONTROL = 2,
    MODIFIER_ALT = 4
  };

 protected:
  // This class is abstract.
  explicit Event(Type t, void *original = NULL)
      : type_(t), original_(original) { }

 public:
  Type GetType() const { return type_; }

  bool IsSimpleEvent() const { return type_ > EVENT_SIMPLE_RANGE_START &&
                                      type_ < EVENT_SIMPLE_RANGE_END; }
  bool IsMouseEvent() const { return type_ > EVENT_MOUSE_RANGE_START &&
                                     type_ < EVENT_MOUSE_RANGE_END; }
  bool IsKeyboardEvent() const { return type_ > EVENT_KEY_RANGE_START &&
                                        type_ < EVENT_KEY_RANGE_END; }
  bool IsDragEvent() const { return type_ > EVENT_DRAG_RANGE_START &&
                                    type_ < EVENT_DRAG_RANGE_END; }
  void *GetOriginalEvent() const { return original_; }
  void SetOriginalEvent(void *original) {
    original_ = original;
  }

 private:
  Type type_;
  void *original_;
};

class SimpleEvent : public Event {
 public:
  explicit SimpleEvent(Type t) : Event(t) { ASSERT(IsSimpleEvent()); }
};

class PositionEvent : public Event {
 public:
  double GetX() const { return x_; }
  double GetY() const { return y_; }

  void SetX(double x) { x_ = x; }
  void SetY(double y) { y_ = y; }

 protected:
  PositionEvent(Type t, double x, double y)
      : Event(t), x_(x), y_(y) {
  }

 private:
  double x_, y_;
};

/**
 * Class representing a mouse event.
 */
class MouseEvent : public PositionEvent {
 public:
  enum Button {
    BUTTON_NONE = 0,
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE = 4,
    BUTTON_RIGHT = 2,
    BUTTON_ALL = BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT
  };

  MouseEvent(Type t, double x, double y,
             int wheel_delta_x, int wheel_delta_y,
             int button, int modifier,
             void *original = NULL)
      : PositionEvent(t, x, y),
        wheel_delta_x_(wheel_delta_x), wheel_delta_y_(wheel_delta_y),
        button_(button), modifier_(modifier) {
    ASSERT(IsMouseEvent());
    SetOriginalEvent(original);
  }

  int GetButton() const { return button_; }
  void SetButton(int button) { button_ = button; }

  int GetModifier() const { return modifier_; }
  void SetModifier(int m) { modifier_ = m; }

  int GetWheelDeltaX() const { return wheel_delta_x_; }
  void SetWheelDeltaX(int wheel_delta_x) {
    wheel_delta_x_ = wheel_delta_x;
  }

  int GetWheelDeltaY() const { return wheel_delta_y_; }
  void SetWheelDeltaY(int wheel_delta_y) {
    wheel_delta_y_ = wheel_delta_y;
  }

  static const int kWheelDelta = 120;

 private:
  int wheel_delta_x_;
  int wheel_delta_y_;
  int button_;
  int modifier_;
};

/**
 * Class representing a keyboard event.
 */
class KeyboardEvent : public Event {
 public:
  /**
   * Key codes compatible with the Windows version.
   * Most of the following names correspond VK_XXX macros in winuser.h, except
   * for some confusing names.
   *
   * They are only used in @c EVENT_KEY_DOWN and @c EVENT_KEY_UP events.
   * In @c EVENT_KEY_PRESS, the keyCode attribute is the the chararacter code.
   */
  enum KeyCode {
    KEY_CANCEL         = 3,
    KEY_BACK           = 8,
    KEY_TAB            = 9,
    KEY_CLEAR          = 12,
    KEY_RETURN         = 13,
    KEY_SHIFT          = 16,
    KEY_CONTROL        = 17,
    KEY_ALT            = 18,  // VK_MENU in winuser.h.
    KEY_PAUSE          = 19,
    KEY_CAPITAL        = 20,
    KEY_ESCAPE         = 31,
    KEY_SPACE          = 32,
    KEY_PAGE_UP        = 33,  // VK_PRIOR in winuser.h.
    KEY_PAGE_DOWN      = 34,  // VK_NEXT in winuser.h.
    KEY_END            = 35,
    KEY_HOME           = 36,
    KEY_LEFT           = 37,
    KEY_UP             = 38,
    KEY_RIGHT          = 39,
    KEY_DOWN           = 40,
    KEY_SELECT         = 41,
    KEY_PRINT          = 42,
    KEY_TO_EXECUTE     = 43,  // KEY_EXECUTE is a macro name on windows
    KEY_INSERT         = 45,
    KEY_DELETE         = 46,
    KEY_HELP           = 47,
    // Mixed decimal and hexadecimal to keep consistence with windows.h.
    // 0~9, A-Z and some punctuations are represented in their original ascii.
    KEY_CONTEXT_MENU   = 0x5D,  // VK_APPS in winuser.h.
    KEY_NUMPAD0        = 0x60,
    KEY_NUMPAD1        = 0x61,
    KEY_NUMPAD2        = 0x62,
    KEY_NUMPAD3        = 0x63,
    KEY_NUMPAD4        = 0x64,
    KEY_NUMPAD5        = 0x65,
    KEY_NUMPAD6        = 0x66,
    KEY_NUMPAD7        = 0x67,
    KEY_NUMPAD8        = 0x68,
    KEY_NUMPAD9        = 0x69,
    KEY_MULTIPLY       = 0x6A,
    KEY_ADD            = 0x6B,
    KEY_SEPARATOR      = 0x6C,
    KEY_SUBTRACT       = 0x6D,
    KEY_DECIMAL        = 0x6E,
    KEY_DIVIDE         = 0x6F,
    KEY_F1             = 0x70,
    KEY_F2             = 0x71,
    KEY_F3             = 0x72,
    KEY_F4             = 0x73,
    KEY_F5             = 0x74,
    KEY_F6             = 0x75,
    KEY_F7             = 0x76,
    KEY_F8             = 0x77,
    KEY_F9             = 0x78,
    KEY_F10            = 0x79,
    KEY_F11            = 0x7A,
    KEY_F12            = 0x7B,
    KEY_F13            = 0x7C,
    KEY_F14            = 0x7D,
    KEY_F15            = 0x7E,
    KEY_F16            = 0x7F,
    KEY_F17            = 0x80,
    KEY_F18            = 0x81,
    KEY_F19            = 0x82,
    KEY_F20            = 0x83,
    KEY_F21            = 0x84,
    KEY_F22            = 0x85,
    KEY_F23            = 0x86,
    KEY_F24            = 0x87,
    KEY_NUMLOCK        = 0x90,
    KEY_SCROLL         = 0x91,

    KEY_COLON          = 0xBA,  // VK_OEM_1 in winuser.h, ;: in the keyboard.
    KEY_PLUS           = 0xBB,  // =+ in the keyboard.
    KEY_COMMA          = 0xBC,  // ,< in the keyboard.
    KEY_MINUS          = 0xBD,  // -_ in the keyboard.
    KEY_PERIOD         = 0xBE,  // .> in the keyboard.
    KEY_SLASH          = 0xBF,  // VK_OEM_2 in winuser.h, /? in the keyboard.
    KEY_GRAVE          = 0xC0,  // VK_OEM_3 in winuser.h, `~ in the keyboard.
    KEY_BRACKET_LEFT   = 0xDB,  // VK_OEM_4 in winuser.h, [{ in the keyboard.
    KEY_BACK_SLASH     = 0xDC,  // VK_OEM_5 in winuser.h, \| in the keyboard.
    KEY_BRACKET_RIGHT  = 0xDD,  // VK_OEM_6 in winuser.h, ]} in the keyboard.
    KEY_QUOTE_CHAR     = 0xDE   // VK_OEM_7 in winuser.h, '" in the keyboard.
                                // KEY_QUOTE is a macro on windows.
  };

  KeyboardEvent(Type t, unsigned int key_code, int modifier, void *original)
      : Event(t, original), key_code_(key_code), modifier_(modifier) {
    ASSERT(IsKeyboardEvent());
  }

  unsigned int GetKeyCode() const { return key_code_; }
  void SetKeyCode(unsigned int key_code) { key_code_ = key_code; }

  int GetModifier() const { return modifier_; }
  void SetModifier(int m) { modifier_ = m; }

 private:
  unsigned int key_code_;
  int modifier_;
};

/**
 * Class representing a drag & drop event.
 */
class DragEvent : public PositionEvent {
 public:
  DragEvent(Type t, double x, double y)
    : PositionEvent(t, x, y), drag_files_(NULL),
      drag_urls_(NULL), drag_text_(NULL) {
    ASSERT(IsDragEvent());
  }

  const char *const *GetDragFiles() const { return drag_files_; }
  void SetDragFiles(const char *const *drag_files) { drag_files_ = drag_files; }

  const char *const *GetDragUrls() const { return drag_urls_; }
  void SetDragUrls(const char *const *drag_urls) { drag_urls_ = drag_urls; }

  const char *GetDragText() const { return drag_text_; }
  void SetDragText(const char *drag_text) { drag_text_ = drag_text; }

 private:
  const char *const *drag_files_;
  const char *const *drag_urls_;
  const char *drag_text_;
};

/**
 * Class representing a sizing event.
 */
class SizingEvent : public Event {
 public:
  SizingEvent(double width, double height)
      : Event(EVENT_SIZING), width_(width), height_(height) {
  }

  double GetWidth() const { return width_; }
  double GetHeight() const { return height_; }

  void SetWidth(double width) { width_ = width; }
  void SetHeight(double height) { height_ = height; }

 private:
  double width_, height_;
};

/**
 * Class representing a changed option.
 */
class OptionChangedEvent : public Event {
 public:
  OptionChangedEvent(const char *property_name)
      : Event(EVENT_OPTION_CHANGED), property_name_(property_name) {
  }

  std::string GetPropertyName() const { return property_name_; }
  void SetPropertyname(
      const char *property_name) { property_name_ = property_name; }

 private:
  std::string property_name_;
};

/**
 * Class representing a timer event.
 */
class TimerEvent : public Event {
 public:
  TimerEvent(int token, int value)
      : Event(EVENT_TIMER), token_(token), value_(value) {
  }

  int GetToken() const { return token_; }
  void SetToken(int token) { token_ = token; }
  int GetValue() const { return value_; }
  void SetValue(int value) { value_ = value; }

 private:
  int token_;
  int value_;
};

/**
 * Class representing a perfmon event.
 */
class PerfmonEvent : public Event {
 public:
  PerfmonEvent(const Variant &value)
      : Event(EVENT_PERFMON), value_(value) {
  }

  Variant GetValue() const { return value_; }
  void SetValue(const Variant &value) { value_ = value; }

 private:
  Variant value_;
};

/**
 * Class representing a context menu event.
 */
class ContextMenuEvent : public Event {
 public:
  ContextMenuEvent(ScriptableMenu *menu)
      : Event(EVENT_CONTEXT_MENU), menu_(menu) {
    menu_->Ref();
  }
  ~ContextMenuEvent() {
    menu_->Unref();
  }

  ScriptableMenu *GetMenu() const { return menu_; }

 private:
  ScriptableMenu *menu_;
};

/** @} */

} // namespace ggadget

#endif // GGADGET_EVENT_H__
