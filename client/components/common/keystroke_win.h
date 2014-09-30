/*
  Copyright 2014 Google Inc.

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
#ifndef GOOPY_COMPONENTS_COMMON_KEYSTROKE_WIN_H_
#define GOOPY_COMPONENTS_COMMON_KEYSTROKE_WIN_H_

// TODO(linxinfen): Removes dependence on windows.h, use text of KeyEvent to
// generate AsciiValue.
#include <windows.h>

#include "base/basictypes.h"
#include "ipc/keyboard_codes.h"

// English default input locale.
static const wchar_t kEnglishLocaleId[] = L"00000409";
// English input keyboard layout.
static HKL english_hkl = 0;

namespace ime_goopy {
namespace components {

class KeyStroke {
 public:
  KeyStroke()
      : vkey_(0),
        ch_(0),
        snapshot_(0) {
  }

  KeyStroke(uint32 vkey, const uint8* keystate)
      : vkey_(LOWORD(vkey)),
        snapshot_(0) {
    Construct(vkey, keystate, keystate[vkey_] & 0x80);
  }

  KeyStroke(uint32 vkey, const uint8* keystate, bool down)
      : vkey_(LOWORD(vkey)),
        snapshot_(0) {
    Construct(vkey, keystate, down);
  }

  // Copy constructor.
  KeyStroke(const KeyStroke &x)
      : vkey_(x.vkey_),
        ch_(x.ch_),
        snapshot_(x.snapshot_) {
  }

  bool IsDown() const {
    return GetBit(IS_DOWN);
  }

  bool IsUp() const {
    return !IsDown();
  }

  bool IsHome() const {
    return vkey_ == ipc::VKEY_HOME;
  }

  bool IsEnd() const {
    return vkey_ == ipc::VKEY_END;
  }

  bool IsReturn() const {
    return vkey_ == ipc::VKEY_RETURN;
  }

  bool IsEscape() const {
    return vkey_ == ipc::VKEY_ESCAPE;
  }

  bool IsBackspace() const {
    return vkey_ == ipc::VKEY_BACK;
  }

  bool IsDelete() const {
    return vkey_ == ipc::VKEY_DELETE;
  }

  bool IsWhitespace() const {
    return vkey_ == ipc::VKEY_SPACE;
  }

  bool IsCaptital() const {
    return vkey_ == ipc::VKEY_CAPITAL;
  }

  bool IsShift() const {
    return vkey_ == ipc::VKEY_SHIFT;
  }

  bool IsLeftShift() const {
    return GetBit(IS_LEFT_SHIFT);
  }

  bool IsRightShift() const {
    return GetBit(IS_RIGHT_SHIFT);
  }

  bool IsControl() const {
    return vkey_ == ipc::VKEY_CONTROL;
  }

  bool IsPageDown() const {
    return vkey_ == ipc::VKEY_NEXT;
  }

  bool IsPageUp() const {
    return vkey_ == ipc::VKEY_PRIOR;
  }

  bool IsPlus() const {
    return vkey_ == ipc::VKEY_OEM_PLUS;
  }

  bool IsMinus() const {
    return vkey_ == ipc::VKEY_OEM_MINUS;
  }

  bool IsComma() const {
    return vkey_ == ipc::VKEY_OEM_COMMA;
  }

  bool IsDot() const {
    return vkey_ == ipc::VKEY_OEM_PERIOD;
  }

  bool IsSubtract() const {
    return vkey_ == ipc::VKEY_SUBTRACT;
  }

  bool IsAdd() const {
    return vkey_ == ipc::VKEY_ADD;
  }

  bool IsLeftControl() const {
    return GetBit(IS_LEFT_CONTROL);
  }

  bool IsRightControl() const {
    return GetBit(IS_RIGHT_CONTROL);
  }

  bool IsPeriod() const {
    return vkey_ == ipc::VKEY_OEM_PERIOD;
  }

  bool IsShifted() const {
    return GetBit(IS_SHIFTED);
  }

  bool IsTab() const {
    return vkey_ == ipc::VKEY_TAB;
  }

  bool IsCaplocked() const {
    return GetBit(IS_CAPLOCKED);
  }

  bool IsCtrled() const {
    return GetBit(IS_CTRLED);
  }

  bool IsMenued() const {
    return GetBit(IS_MENUED);
  }

  bool IsAscii() const {
    const char ch = AsciiValue();
    return isascii(ch);
  }

  char AsciiValue() const {
    return static_cast<char>(ch_);
  }

  bool IsAlpha() const {
    return AsciiValue() >= 'A' && AsciiValue() <= 'Z' ||
           AsciiValue() >= 'a' && AsciiValue() <= 'z';
  }

  bool IsDigit() const {
    return AsciiValue() >= '0' && AsciiValue() <= '9';
  }

  bool IsNumPad() const {
    return vkey_ >= ipc::VKEY_NUMPAD0 && vkey_ <= ipc::VKEY_NUMPAD9;
  }

  bool IsDecimal() const {
    return vkey_ == ipc::VKEY_DECIMAL;
  }

  int DigitValue() const {
    return AsciiValue() - '0';
  }

  bool IsVisible() const {
    return AsciiValue() >= ' ';
  }

  bool IsMoveLeft() const {
    return vkey_ == ipc::VKEY_LEFT;
  }

  bool IsMoveRight() const {
    return vkey_ == ipc::VKEY_RIGHT;
  }

  bool IsMoveEnd() const {
    return vkey_ == ipc::VKEY_END;
  }

  bool IsMoveUp() const {
    return vkey_ == ipc::VKEY_UP;
  }

  bool IsMoveDown() const {
    return vkey_ == ipc::VKEY_DOWN;
  }

  bool IsCtrledDigit() const {
    // When Ctrl key is pressed, the ASCII value will always be 0. So we need
    // to use the virtual key directly.
    if (vkey_ >= ipc::VKEY_1 && vkey_ <= ipc::VKEY_9 && IsCtrled())
      return true;
    return false;
  }

  uint16 vkey() const {
    return vkey_;
  }

#ifdef DEBUG
  string debug_string() const {
    char buf[128] = { 0 };
    _snprintf(buf, sizeof(buf), "vkey:%d ch:%d snap:%d", vkey_, ch_,
              snapshot_);
    return buf;
  }
#endif  // DEBUG

 private:
  // The number shift the ascii char to its responding full shape char.
  static const int kFullShapeShiftNumber = 65248;
  typedef enum BitOffset {
    IS_DOWN = 0,
    IS_CAPLOCKED = 1,
    IS_SHIFTED = 2,
    IS_CTRLED = 3,
    IS_MENUED = 4,
    IS_LEFT_SHIFT = 5,
    IS_LEFT_CONTROL = 6,
    IS_RIGHT_SHIFT = 7,
    IS_RIGHT_CONTROL = 8,
  };

  void Construct(uint32 vkey, const uint8* keystate, bool down) {
    SetBit(IS_LEFT_SHIFT, keystate[ipc::VKEY_LSHIFT] & 0x80);
    SetBit(IS_LEFT_CONTROL, keystate[ipc::VKEY_LCONTROL] & 0x80);
    SetBit(IS_RIGHT_SHIFT, keystate[ipc::VKEY_RSHIFT] & 0x80);
    SetBit(IS_RIGHT_CONTROL, keystate[ipc::VKEY_RCONTROL] & 0x80);
    SetBit(IS_DOWN, down);
    SetBit(IS_CAPLOCKED, keystate[ipc::VKEY_CAPITAL] & 0x1);
    SetBit(IS_SHIFTED, keystate[ipc::VKEY_SHIFT] & 0x80);
    SetBit(IS_CTRLED, keystate[ipc::VKEY_CONTROL] & 0x80);
    SetBit(IS_MENUED, keystate[ipc::VKEY_MENU] & 0x80);
    WORD wch = L'\0';
    if (!english_hkl)
      english_hkl = LoadKeyboardLayout(kEnglishLocaleId, KLF_NOTELLSHELL);
    ToAsciiEx(vkey_,
              MapVirtualKey(vkey_, MAPVK_VK_TO_VSC),
              keystate,
              &wch,
              0,
              english_hkl);
    ch_ = (wch < 128) ? static_cast<char>(wch) : 0;
  }

  void SetBit(BitOffset bit_offset, int value) {
    int v = (value != 0) ? 1 : 0;
    snapshot_ &= ~(0x1 << static_cast<int>(bit_offset));
    snapshot_ |= (v << bit_offset);
  }

  bool GetBit(BitOffset bit_offset) const {
    int v = snapshot_;
    v &= (static_cast<int>(0x1) << bit_offset);
    return v != 0;
  }

  uint16 vkey_;
  uint16 ch_;
  int snapshot_;
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_KEYSTROKE_WIN_H_
