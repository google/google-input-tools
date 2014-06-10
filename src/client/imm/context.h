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

#ifndef GOOPY_IMM_CONTEXT_H__
#define GOOPY_IMM_CONTEXT_H__

#include <atlbase.h>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/string_utils_win.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "ipc/constants.h"
#include "ipc/protos/ipc.pb.h"
#include "imm/candidate_info.h"
#include "imm/composition_string.h"
#include "imm/context_locker.h"
#include "imm/debug.h"
#include "imm/message_queue.h"
#include "imm/immdev.h"
#include "imm/input_context.h"

using ime_goopy::imm::CandidateInfo;
using ime_goopy::imm::CompositionString;
using ime_goopy::imm::HIMCLockerT;
using ime_goopy::imm::HIMCCLockerT;
using ime_goopy::imm::InputContext;
using ime_goopy::imm::MessageQueue;
using ime_goopy::imm::WindowsImmLockPolicy;

namespace ime_goopy {
namespace imm {

static int kInvalidCoordinate = -1;
static int kCompositionCandidatePadding = 4;

// Context maintains context data when the user is typing. This class
// communicates between engine and IMM framework, pass user input to engine and
// notify the framework when the engine status is changed.
template <class ImmLockPolicy, class MessageQueueType>
class ContextT : public ContextInterface {
 public:
  ContextT(HIMC himc, MessageQueueType* message_queue)
      : himc_(himc), message_queue_(message_queue), updating_(false) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" himc: " << himc_
             << L" context: " << this;
    ZeroMemory(&should_show_, sizeof(should_show_));
    composition_pos_.x = composition_pos_.y = kInvalidCoordinate;
  }

  ~ContextT() {
  }

  bool Initialize(EngineInterface* engine) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" engine: " << engine;
    assert(engine);
    engine_.reset(engine);
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return false;
    if (!context->Initialize()) return false;

    HIMCCLockerT<CompositionString, ImmLockPolicy> compstr(&context->hCompStr);
    if (!compstr) return false;
    if (!compstr->Initialize()) return false;

    HIMCCLockerT<CandidateInfo, ImmLockPolicy> candinfo(&context->hCandInfo);
    if (!candinfo) return false;
    if (!candinfo->Initialize()) return false;

    // Set the initial status of context.
    context->fdwConversion = IME_CMODE_NATIVE;
    context->fdwSentence = 0;
    context->fOpen = TRUE;
    engine->SetContext(this);
    return true;
  }

  void SetOpenStatus(bool open_status) {
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return;
    context->fOpen = open_status;
  }

  void DetachEngine() {
    engine_.release();
  }

  void UpdateComposition(const std::wstring& composition, int caret) OVERRIDE {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" himc: " << himc_;
    updating_ = true;
    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    if (!context) {
      updating_ = false;
      return;
    }
    HIMCCLockerT<CompositionString, ImmLockPolicy> compstr(context->hCompStr);
    if (!compstr) {
      updating_ = false;
      return;
    }
    if ((compstr->get_composition()[0] == 0) &&
        !composition.empty()) {
      message_queue_->AddMessage(WM_IME_STARTCOMPOSITION, 0, 0);
    }
    DWORD flag = GCS_COMP | GCS_CURSORPOS;
    message_queue_->AddMessage(WM_IME_COMPOSITION, 0, flag);
    if (composition.empty()) {
      message_queue_->AddMessage(WM_IME_ENDCOMPOSITION, 0, 0);
      composition_pos_.x = composition_pos_.y = kInvalidCoordinate;
    }
    compstr->set_composition(composition);
    compstr->set_result(L"");
    compstr->set_caret(caret);

    message_queue_->AddMessage(
        WM_IME_NOTIFY, IMN_PRIVATE, COMPONENT_COMPOSITION);
    message_queue_->Send();
  }

  void CommitResult(const std::wstring& result) OVERRIDE {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" himc: " << himc_;

    if (updating()) {
      pending_commits_ += result;
      return;
    }

    updating_ = true;

    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    if (!context) {
      updating_ = false;
      return;
    }
    HIMCCLockerT<CompositionString, ImmLockPolicy> compstr(context->hCompStr);
    if (!compstr) {
      updating_ = false;
      return;
    }

    if ((compstr->get_composition()[0] == 0))
      message_queue_->AddMessage(WM_IME_STARTCOMPOSITION, 0, 0);

    DWORD flag = GCS_COMP | GCS_CURSORPOS;
    if (result.size())
      flag = GCS_RESULTSTR | GCS_CURSORPOS;

    message_queue_->AddMessage(WM_IME_COMPOSITION, 0, flag);
    message_queue_->AddMessage(WM_IME_ENDCOMPOSITION, 0, 0);
    composition_pos_.x = composition_pos_.y = kInvalidCoordinate;

    compstr->set_composition(L"");
    compstr->set_result(result);
    compstr->set_caret(result.size());

    message_queue_->AddMessage(
        WM_IME_NOTIFY, IMN_PRIVATE, COMPONENT_COMPOSITION);
    message_queue_->Send();
  }

  void UpdateCandidates(
      bool is_compositing, const ipc::proto::CandidateList& candidate_list)
      OVERRIDE {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" himc: " << himc_;
    updating_ = true;

    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    if (!context) {
      updating_ = false;
      return;
    }
    HIMCCLockerT<CandidateInfo, ImmLockPolicy> candinfo(context->hCandInfo);
    if (!candinfo) {
      updating_ = false;
      return;
    }

    bool was_empty = (candinfo->count() == 0);
    int count = candidate_list.candidate_size();
    candinfo->set_count(count);
    candinfo->set_selection(candidate_list.selected_candidate());
    if (is_compositing) {
      for (int i = 0; i < count; i++) {
        const ipc::proto::Candidate& candidate = candidate_list.candidate(i);
        const std::wstring& text_to_commit = candidate.has_actual_text() ?
            Utf8ToWide(candidate.actual_text().text()) :
            Utf8ToWide(candidate.text().text());
        candinfo->set_candidate(i, text_to_commit);
      }
      // Send an IMN_OPENCANDIDATE even the candidate is already opened,
      // otherwise, Dreamweaver will not update candidate position. I don't
      // know the impact to other applications of adding this extra message.
      message_queue_->AddMessage(WM_IME_NOTIFY, IMN_OPENCANDIDATE, 1);
      message_queue_->AddMessage(WM_IME_NOTIFY, IMN_CHANGECANDIDATE, 1);
    } else if (!was_empty) {
      message_queue_->AddMessage(WM_IME_NOTIFY, IMN_CLOSECANDIDATE, 1);
    }
    message_queue_->AddMessage(
        WM_IME_NOTIFY, IMN_PRIVATE, COMPONENT_CANDIDATES);
    message_queue_->Send();
  }

  void UpdateStatus(bool native, bool full_shape, bool full_punct) OVERRIDE {
    DWORD imm_conversion_status_bits = 0;
    imm_conversion_status_bits |= native ? IME_CMODE_CHINESE : 0;

    imm_conversion_status_bits |= full_shape ? IME_CMODE_FULLSHAPE : 0;

    imm_conversion_status_bits |= full_punct ? IME_CMODE_SYMBOL : 0;

    // Sentence bits is not concerned right now.
    ImmSetConversionStatus(
        himc_,
        imm_conversion_status_bits,
        0);
    message_queue_->AddMessage(WM_IME_NOTIFY, IMN_PRIVATE, COMPONENT_STATUS);
    message_queue_->Send();
  }

  Platform GetPlatform() OVERRIDE {
    return PLATFORM_WINDOWS_IMM;
  }

  EngineInterface* GetEngine() OVERRIDE {
    return engine_.get();
  }

  bool GetClientRect(RECT* rect) OVERRIDE {
    HIMCLockerT<InputContext, WindowsImmLockPolicy> context(himc_);
    HWND original_window = context->hWnd;
    RECT client_rect = {0};
    if (!::GetClientRect(original_window, &client_rect))
      return false;

    POINT top_left = {0};
    top_left.x = client_rect.left;
    top_left.y = client_rect.top;

    POINT bottom_right = {0};
    bottom_right.x = client_rect.right;
    bottom_right.y = client_rect.bottom;

    if (!::ClientToScreen(original_window, &top_left) ||
        !::ClientToScreen(original_window, &bottom_right)) {
      return false;
    }

    rect->left = top_left.x;
    rect->right = bottom_right.x;
    rect->top = top_left.y;
    rect->bottom = bottom_right.y;

    return true;
  }

  bool GetCaretRectForComposition(RECT* rect) OVERRIDE {
    if (ShouldShow(UI_COMPONENT_COMPOSITION)) {
      DVLOG(3) << __SHORT_FUNCTION__ << L" Getting rect from composition";
      HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
      if (context->GetCaretRectFromComposition(rect)) {
        DVLOG(3) << __SHORT_FUNCTION__ << L" Got rect from composition";
        return true;
      }
    }

    DVLOG(3) << __SHORT_FUNCTION__ << L" Nope...failed.";
    return false;
  }

  bool GetCaretRectForCandidate(RECT* rect) OVERRIDE {
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return false;

    if (ShouldShow(UI_COMPONENT_CANDIDATES)) {
      DVLOG(3) << __SHORT_FUNCTION__ << L" Getting rect from candidates";
      if (context->GetCaretRectFromCandidate(rect)) {
        DVLOG(3) << __SHORT_FUNCTION__ << L" Got rect from candidates";
        return true;
      }
    } else if (ShouldShow(UI_COMPONENT_COMPOSITION)) {
      DVLOG(3) << __SHORT_FUNCTION__ << L" Getting rect from composition";
      if (context->GetCaretRectFromComposition(rect)) {
        DVLOG(3) << __SHORT_FUNCTION__ << L" Got rect from composition";
        return true;
      }
    }

    DVLOG(3) << __SHORT_FUNCTION__ << L" Nope...failed.";
    return false;
  }

  bool GetCandidatePos(POINT* point) OVERRIDE {
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return false;
    if (composition_pos_.x != kInvalidCoordinate) {
      *point = composition_pos_;
      point->y += context->GetFontHeight() + kCompositionCandidatePadding;
      return true;
    }
    if (!context->GetCompositionPos(point))
      return false;
    point->y += context->GetFontHeight() + kCompositionCandidatePadding;
    return true;
  }

  bool GetCompositionPos(POINT* point) OVERRIDE {
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return false;
    if (composition_pos_.x != kInvalidCoordinate) {
      *point = composition_pos_;
      return true;
    }
    return context->GetCompositionPos(point);
  }

  bool GetCompositionBoundary(RECT *rect) OVERRIDE {
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return false;
    return context->GetCompositionBoundary(rect);
  }

  bool GetCompositionFont(LOGFONT *font) OVERRIDE {
    HIMCLockerT<InputContext, ImmLockPolicy> context(himc_);
    if (!context) return false;
    return context->GetCompositionFont(font);
  }

  bool ShouldShow(UIComponent ui_type) OVERRIDE {
    // The show/hide status of status window is not associated to input context.
    // Try to avoid the misusing.
    assert(ui_type != UI_COMPONENT_STATUS);
    return should_show_[ui_type];
  }

  // Used by UIWindow to set the UI status.
  void SetShouldShow(UIComponent ui_type, bool value) {
    // The show/hide status of status window is not associated to input context.
    // Try to avoid the misusing.
    assert(ui_type != UI_COMPONENT_STATUS);
    should_show_[ui_type] = value;
  }

  // IMM
  BOOL OnProcessKey(UINT virtual_key, LPARAM lparam, CONST LPBYTE key_state) {
    DVLOG(1) << __SHORT_FUNCTION__;
    // If the open status is false, for example in a password control, we send
    // nothing to IME.
    if (!GetOpenStatus()) return FALSE;
    if (!engine_.get()) return FALSE;

    // In IE8.0 protect mode, key up/down info is wrong in key_state, we should
    // get up/down status from the transition state bit (highest bit).
    // KeyStroke key(virtual_key, key_state, !(lparam & 0x80000000));
    return engine_->ShouldProcessKey(ConvertToIPCKey(
        virtual_key, key_state, !(lparam & 0x80000000))) ? TRUE : FALSE;
  }

  BOOL OnNotifyIME(DWORD action, DWORD index, DWORD value) {
    DVLOG(1) << __SHORT_FUNCTION__;
    switch (action) {
      case NI_OPENCANDIDATE:
        break;
      case NI_CLOSECANDIDATE:
        break;
      case NI_SELECTCANDIDATESTR:
        break;
      case NI_SETCANDIDATE_PAGESTART:
        break;
      case NI_SETCANDIDATE_PAGESIZE:
        break;
      case NI_CHANGECANDIDATELIST:
        break;
      case NI_CONTEXTUPDATED:
        switch (value) {
          case IMC_SETCONVERSIONMODE:
            break;
          case IMC_SETOPENSTATUS:
            DVLOG(1) << L" open status: " << ImmGetOpenStatus(himc_);
            break;
          case IMC_SETCANDIDATEPOS:
            break;
          case IMC_SETCOMPOSITIONFONT:
            break;
          case IMC_SETCOMPOSITIONWINDOW:
            break;
          default:
            return FALSE;
            break;
        }
        break;
      case NI_COMPOSITIONSTR:
        switch (index) {
          case CPS_COMPLETE:
            if (engine_.get())
              engine_->EndComposition(false);
            break;
          case CPS_CONVERT:
            break;
          case CPS_REVERT:
            break;
          case CPS_CANCEL:
            if (engine_.get())
              engine_->EndComposition(false);
            break;
          default:
            return FALSE;
            break;
        }
        break;
      default:
        return FALSE;
        break;
    }
    return TRUE;
  }

  bool GetOpenStatus() {
    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    if (!context) return false;
    return context->fOpen;
  }

  bool IsVisible() {
    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    return context->hWnd &&
           ::IsWindow(context->hWnd) &&
           ::IsWindowVisible(context->hWnd);
  }

  UINT OnToAsciiEx(UINT virtual_key,
                   UINT scan_code,
                   CONST LPBYTE key_state,
                   LPTRANSMSGLIST trans_list,
                   UINT state) {
    DVLOG(1) << __SHORT_FUNCTION__;

    // Don't call IME if the open status is false.
    if (!GetOpenStatus()) return 0;
    if (!engine_.get()) return 0;

    assert(key_state);
    assert(trans_list);

    message_queue_->Attach(trans_list);
    // In IE8.0 protect mode, key up/down info is wrong in key_state, we should
    // get up/down status from scan_code.
    // KeyStroke key(virtual_key, key_state, !(scan_code & 0x8000));
    engine_->ProcessKey(
        ConvertToIPCKey(virtual_key, key_state, !(scan_code & 0x8000)));
    return message_queue_->Detach();
  }

  void OnSystemStatusChange() {
    DVLOG(1) << __SHORT_FUNCTION__;
    DWORD conversion = 0, sentence = 0;
    if (ImmGetConversionStatus(himc_, &conversion, &sentence) != 0) {
      DWORD conversion_mode = 0;
      conversion_mode |= ((conversion & IME_CMODE_FULLSHAPE) ?
                          EngineInterface::CONVERSION_MODE_FULL_SHAPE : 0);
      conversion_mode |= ((conversion & IME_CMODE_SYMBOL) ?
          EngineInterface::CONVERSION_MODE_FULL_PUNCT : 0);
      if (engine_.get())
        engine_->NotifyConversionModeChange(conversion_mode);
    } else {
      DLOG(ERROR) << "Cannot get conversion status for 0x" << hex << himc_;
    }
  }

  bool updating() {
    return updating_;
  }

  void FinishUpdate() {
    updating_ = false;
    if (!pending_commits_.empty()) {
      std::wstring text = pending_commits_;
      pending_commits_.clear();
      CommitResult(text);
    }
  }

  bool OnImportDictionary(const wchar_t* filename) {
    if (!engine_.get()) return false;
    return engine_->ImportDictionary(filename);
  }

  ContextId GetId() {
    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    return reinterpret_cast<ContextId>(context->hWnd);
  }

 private:
  ipc::proto::KeyEvent ConvertToIPCKey(UINT virtual_key,
                                       LPBYTE key_state,
                                       bool down) const {
    ipc::proto::KeyEvent key_event;
    key_event.set_keycode(virtual_key);
    key_event.set_type(down ?
                       ipc::proto::KeyEvent::DOWN : ipc::proto::KeyEvent::UP);
    key_event.set_hardware_keycode(MapVirtualKey(virtual_key, MAPVK_VK_TO_VSC));
    uint32 modifiers = 0;
    modifiers |= (key_state[VK_SHIFT] & 0x80) ? ipc::kShiftKeyMask : 0;
    modifiers |= (key_state[VK_CONTROL] & 0x80) ? ipc::kControlKeyMask : 0;
    modifiers |= (key_state[VK_MENU] & 0x80) ? ipc::kAltKeyMask : 0;
    modifiers |= (key_state[VK_CAPITAL] & 0x1) ? ipc::kCapsLockMask : 0;
    if (virtual_key == VK_SHIFT) {
      modifiers |= ipc::kShiftKeyMask;
      key_event.set_is_modifier(true);
    } else if (virtual_key == VK_CONTROL) {
      modifiers |= ipc::kControlKeyMask;
      key_event.set_is_modifier(true);
    } else if (virtual_key == VK_MENU) {
      modifiers |= ipc::kAltKeyMask;
      key_event.set_is_modifier(true);
    } else if (virtual_key == VK_CAPITAL) {
      modifiers |= ipc::kCapsLockMask;
      key_event.set_is_modifier(true);
    }
    key_event.set_modifiers(modifiers);
    wchar_t unicode_text[MAX_PATH] = {0};
    BYTE key_state_without_modifier[256] = {0};
    key_state_without_modifier[virtual_key & 0xFF] = 0x80;
    ::ToUnicode(virtual_key,
                key_event.hardware_keycode(),
                key_state_without_modifier,
                unicode_text,
                arraysize(unicode_text), 0);
    key_event.set_text(ime_goopy::WideToUtf8(unicode_text));
    return key_event;
  }

  HIMC himc_;
  scoped_ptr<EngineInterface> engine_;
  scoped_ptr<MessageQueueType> message_queue_;
  bool should_show_[ContextInterface::UI_COMPONENT_COUNT];
  bool updating_;
  LOGFONT composition_font_;
  POINT composition_pos_;
  std::wstring pending_commits_;
  DISALLOW_EVIL_CONSTRUCTORS(ContextT);
};

typedef ContextT<WindowsImmLockPolicy, MessageQueue> Context;
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_CONTEXT_H__
