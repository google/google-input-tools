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

#include <algorithm>
#include <gdk/gdkkeysyms.h>
#include <ggadget/event.h>
#include "key_convert.h"

using ggadget::KeyboardEvent;

namespace ggadget {
namespace gtk {

struct KeyvalKeyCode {
  guint gtk_keyval;
  unsigned int key_code;
};

static KeyvalKeyCode keyval_key_code_map[] = {
  { GDK_Cancel,       KeyboardEvent::KEY_CANCEL },
  { GDK_BackSpace,    KeyboardEvent::KEY_BACK },
  { GDK_Tab,          KeyboardEvent::KEY_TAB },
  { GDK_KP_Tab,       KeyboardEvent::KEY_TAB },
  { GDK_ISO_Left_Tab, KeyboardEvent::KEY_TAB },  // Shift-TAB.
  { GDK_Clear,        KeyboardEvent::KEY_CLEAR },
  { GDK_Return,       KeyboardEvent::KEY_RETURN },
  { GDK_KP_Enter,     KeyboardEvent::KEY_RETURN },
  { GDK_Shift_L,      KeyboardEvent::KEY_SHIFT },
  { GDK_Shift_R,      KeyboardEvent::KEY_SHIFT },
  { GDK_Control_L,    KeyboardEvent::KEY_CONTROL },
  { GDK_Control_R,    KeyboardEvent::KEY_CONTROL },
  { GDK_Alt_L,        KeyboardEvent::KEY_ALT },
  { GDK_Alt_R,        KeyboardEvent::KEY_ALT },
  { GDK_Pause,        KeyboardEvent::KEY_PAUSE },
  { GDK_Caps_Lock,    KeyboardEvent::KEY_CAPITAL },
  { GDK_Escape,       KeyboardEvent::KEY_ESCAPE },
  { GDK_space,        KeyboardEvent::KEY_SPACE },
  { GDK_KP_Space,     KeyboardEvent::KEY_SPACE },
  { GDK_Page_Up,      KeyboardEvent::KEY_PAGE_UP },
  { GDK_KP_Page_Up,   KeyboardEvent::KEY_PAGE_UP },
  { GDK_Page_Down,    KeyboardEvent::KEY_PAGE_DOWN },
  { GDK_KP_Page_Down, KeyboardEvent::KEY_PAGE_DOWN },
  { GDK_End,          KeyboardEvent::KEY_END },
  { GDK_KP_End,       KeyboardEvent::KEY_END },
  { GDK_Home,         KeyboardEvent::KEY_HOME },
  { GDK_KP_Home,      KeyboardEvent::KEY_HOME },
  { GDK_Left,         KeyboardEvent::KEY_LEFT },
  { GDK_KP_Left,      KeyboardEvent::KEY_LEFT },
  { GDK_Up,           KeyboardEvent::KEY_UP },
  { GDK_KP_Up,        KeyboardEvent::KEY_UP },
  { GDK_Right,        KeyboardEvent::KEY_RIGHT },
  { GDK_KP_Right,     KeyboardEvent::KEY_RIGHT },
  { GDK_Down,         KeyboardEvent::KEY_DOWN },
  { GDK_KP_Down,      KeyboardEvent::KEY_DOWN },
  { GDK_Select,       KeyboardEvent::KEY_SELECT },
  { GDK_Print,        KeyboardEvent::KEY_PRINT },
  { GDK_Execute,      KeyboardEvent::KEY_TO_EXECUTE },
  { GDK_Insert,       KeyboardEvent::KEY_INSERT },
  { GDK_KP_Insert,    KeyboardEvent::KEY_INSERT },
  { GDK_Delete,       KeyboardEvent::KEY_DELETE },
  { GDK_KP_Delete,    KeyboardEvent::KEY_DELETE },
  { GDK_Help,         KeyboardEvent::KEY_HELP },
  { GDK_Menu,         KeyboardEvent::KEY_CONTEXT_MENU },
  { GDK_exclam,       '1' },
  { GDK_at,           '2' },
  { GDK_numbersign,   '3' },
  { GDK_dollar,       '4' },
  { GDK_percent,      '5' },
  { GDK_asciicircum,  '6' },
  { GDK_ampersand,    '7' },
  { GDK_asterisk,     '8' },
  { GDK_parenleft,    '9' },
  { GDK_parenright,   '0' },
  { GDK_colon,        KeyboardEvent::KEY_COLON },
  { GDK_semicolon,    KeyboardEvent::KEY_COLON },
  { GDK_plus,         KeyboardEvent::KEY_PLUS },
  { GDK_equal,        KeyboardEvent::KEY_PLUS },
  { GDK_comma,        KeyboardEvent::KEY_COMMA },
  { GDK_less,         KeyboardEvent::KEY_COMMA },
  { GDK_minus,        KeyboardEvent::KEY_MINUS },
  { GDK_underscore,   KeyboardEvent::KEY_MINUS },
  { GDK_period,       KeyboardEvent::KEY_PERIOD },
  { GDK_greater,      KeyboardEvent::KEY_PERIOD },
  { GDK_slash,        KeyboardEvent::KEY_SLASH },
  { GDK_question,     KeyboardEvent::KEY_SLASH },
  { GDK_grave,        KeyboardEvent::KEY_GRAVE },
  { GDK_asciitilde,   KeyboardEvent::KEY_GRAVE },
  { GDK_bracketleft,  KeyboardEvent::KEY_BRACKET_LEFT },
  { GDK_braceleft,    KeyboardEvent::KEY_BRACKET_LEFT },
  { GDK_backslash,    KeyboardEvent::KEY_BACK_SLASH },
  { GDK_bar,          KeyboardEvent::KEY_BACK_SLASH },
  { GDK_bracketright, KeyboardEvent::KEY_BRACKET_RIGHT },
  { GDK_braceright,   KeyboardEvent::KEY_BRACKET_RIGHT },
  { GDK_quotedbl,     KeyboardEvent::KEY_QUOTE_CHAR },
  { GDK_apostrophe,   KeyboardEvent::KEY_QUOTE_CHAR },
  { GDK_0,            '0' },
  { GDK_1,            '1' },
  { GDK_2,            '2' },
  { GDK_3,            '3' },
  { GDK_4,            '4' },
  { GDK_5,            '5' },
  { GDK_6,            '6' },
  { GDK_7,            '7' },
  { GDK_8,            '8' },
  { GDK_9,            '9' },
  { GDK_0,            '0' },
  { GDK_A,            'A' },
  { GDK_B,            'B' },
  { GDK_C,            'C' },
  { GDK_D,            'D' },
  { GDK_E,            'E' },
  { GDK_F,            'F' },
  { GDK_G,            'G' },
  { GDK_H,            'H' },
  { GDK_I,            'I' },
  { GDK_J,            'J' },
  { GDK_K,            'K' },
  { GDK_L,            'L' },
  { GDK_M,            'M' },
  { GDK_N,            'N' },
  { GDK_O,            'O' },
  { GDK_P,            'P' },
  { GDK_Q,            'Q' },
  { GDK_R,            'R' },
  { GDK_S,            'S' },
  { GDK_T,            'T' },
  { GDK_U,            'U' },
  { GDK_V,            'V' },
  { GDK_W,            'W' },
  { GDK_X,            'X' },
  { GDK_Y,            'Y' },
  { GDK_Z,            'Z' },
  { GDK_a,            'A' },
  { GDK_b,            'B' },
  { GDK_c,            'C' },
  { GDK_d,            'D' },
  { GDK_e,            'E' },
  { GDK_f,            'F' },
  { GDK_g,            'G' },
  { GDK_h,            'H' },
  { GDK_i,            'I' },
  { GDK_j,            'J' },
  { GDK_k,            'K' },
  { GDK_l,            'L' },
  { GDK_m,            'M' },
  { GDK_n,            'N' },
  { GDK_o,            'O' },
  { GDK_p,            'P' },
  { GDK_q,            'Q' },
  { GDK_r,            'R' },
  { GDK_s,            'S' },
  { GDK_t,            'T' },
  { GDK_u,            'U' },
  { GDK_v,            'V' },
  { GDK_w,            'W' },
  { GDK_x,            'X' },
  { GDK_y,            'Y' },
  { GDK_z,            'Z' },
  { GDK_KP_0,         KeyboardEvent::KEY_NUMPAD0 },
  { GDK_KP_1,         KeyboardEvent::KEY_NUMPAD1 },
  { GDK_KP_2,         KeyboardEvent::KEY_NUMPAD2 },
  { GDK_KP_3,         KeyboardEvent::KEY_NUMPAD3 },
  { GDK_KP_4,         KeyboardEvent::KEY_NUMPAD4 },
  { GDK_KP_5,         KeyboardEvent::KEY_NUMPAD5 },
  { GDK_KP_6,         KeyboardEvent::KEY_NUMPAD6 },
  { GDK_KP_7,         KeyboardEvent::KEY_NUMPAD7 },
  { GDK_KP_8,         KeyboardEvent::KEY_NUMPAD8 },
  { GDK_KP_9,         KeyboardEvent::KEY_NUMPAD9 },
  { GDK_KP_Multiply,  KeyboardEvent::KEY_MULTIPLY },
  { GDK_KP_Add,       KeyboardEvent::KEY_ADD },
  { GDK_KP_Separator, KeyboardEvent::KEY_SEPARATOR },
  { GDK_KP_Subtract,  KeyboardEvent::KEY_SUBTRACT },
  { GDK_KP_Decimal,   KeyboardEvent::KEY_DECIMAL },
  { GDK_KP_Divide,    KeyboardEvent::KEY_DIVIDE },
  { GDK_F1,           KeyboardEvent::KEY_F1 },
  { GDK_KP_F1,        KeyboardEvent::KEY_F1 },
  { GDK_F2,           KeyboardEvent::KEY_F2 },
  { GDK_KP_F2,        KeyboardEvent::KEY_F2 },
  { GDK_F3,           KeyboardEvent::KEY_F3 },
  { GDK_KP_F3,        KeyboardEvent::KEY_F3 },
  { GDK_F4,           KeyboardEvent::KEY_F4 },
  { GDK_KP_F4,        KeyboardEvent::KEY_F4 },
  { GDK_F5,           KeyboardEvent::KEY_F5 },
  { GDK_F6,           KeyboardEvent::KEY_F6 },
  { GDK_F7,           KeyboardEvent::KEY_F7 },
  { GDK_F8,           KeyboardEvent::KEY_F8 },
  { GDK_F9,           KeyboardEvent::KEY_F9 },
  { GDK_F10,          KeyboardEvent::KEY_F10 },
  { GDK_F11,          KeyboardEvent::KEY_F11 },
  { GDK_F12,          KeyboardEvent::KEY_F12 },
  { GDK_F13,          KeyboardEvent::KEY_F13 },
  { GDK_F14,          KeyboardEvent::KEY_F14 },
  { GDK_F15,          KeyboardEvent::KEY_F15 },
  { GDK_F16,          KeyboardEvent::KEY_F16 },
  { GDK_F17,          KeyboardEvent::KEY_F17 },
  { GDK_F18,          KeyboardEvent::KEY_F18 },
  { GDK_F19,          KeyboardEvent::KEY_F19 },
  { GDK_F20,          KeyboardEvent::KEY_F20 },
  { GDK_F21,          KeyboardEvent::KEY_F21 },
  { GDK_F22,          KeyboardEvent::KEY_F22 },
  { GDK_F23,          KeyboardEvent::KEY_F23 },
  { GDK_F24,          KeyboardEvent::KEY_F24 },
  { GDK_Num_Lock,     KeyboardEvent::KEY_NUMLOCK },
  { GDK_Scroll_Lock,  KeyboardEvent::KEY_SCROLL },
  { 0, 0 }, // Guard entry for checking the sorted state of this table.
};

static inline bool KeyvalCompare(const KeyvalKeyCode &v1,
                                  const KeyvalKeyCode &v2) {
  return v1.gtk_keyval < v2.gtk_keyval;
}

unsigned int ConvertGdkKeyvalToKeyCode(guint keyval) {
  if (keyval_key_code_map[0].gtk_keyval != 0) {
    std::sort(keyval_key_code_map,
              keyval_key_code_map + arraysize(keyval_key_code_map),
              KeyvalCompare);
  }

  ASSERT(keyval_key_code_map[0].gtk_keyval == 0);
  KeyvalKeyCode key = { keyval, 0 };
  const KeyvalKeyCode *pos = std::lower_bound(
      keyval_key_code_map, keyval_key_code_map + arraysize(keyval_key_code_map),
      key, KeyvalCompare);
  ASSERT(pos);
  return pos->gtk_keyval == keyval ? pos->key_code : 0;
}

int ConvertGdkModifierToModifier(guint state) {
  int mod = Event::MODIFIER_NONE;
  if (state & GDK_SHIFT_MASK) {
    mod |= Event::MODIFIER_SHIFT;
  }
  if (state & GDK_CONTROL_MASK) {
    mod |= Event::MODIFIER_CONTROL;
  }
  if (state & GDK_MOD1_MASK) {
    mod |= Event::MODIFIER_ALT;
  }
  return mod;
}

int ConvertGdkModifierToButton(guint state) {
  int button = MouseEvent::BUTTON_NONE;
  if (state & GDK_BUTTON1_MASK) {
    button |= MouseEvent::BUTTON_LEFT;
  }
  if (state & GDK_BUTTON2_MASK) {
    button |= MouseEvent::BUTTON_MIDDLE;
  }
  if (state & GDK_BUTTON3_MASK) {
    button |= MouseEvent::BUTTON_RIGHT;
  }
  return button;
}

} // namespace gtk
} // namespace ggadget
