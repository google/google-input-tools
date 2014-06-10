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

#include "tsf/key_event_sink.h"

#include "base/string_utils_win.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "ipc/constants.h"
#include "tsf/context_event_sink.h"
#include "tsf/edit_session.h"

namespace ime_goopy {
namespace tsf {
namespace {
ipc::proto::KeyEvent ConvertToIPCKeyImpl(UINT virtual_key,
                                         LPBYTE key_state,
                                         bool down,
                                         bool with_modifier,
                                         HKL opt_hkl) {
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
  if (with_modifier) {
    ::ToUnicodeEx(virtual_key,
                  key_event.hardware_keycode(),
                  key_state,
                  unicode_text,
                  arraysize(unicode_text), 0, opt_hkl);
  } else {
    ::ToUnicodeEx(virtual_key,
                  key_event.hardware_keycode(),
                  key_state_without_modifier,
                  unicode_text,
                  arraysize(unicode_text), 0, opt_hkl);
  }
  
  key_event.set_text(ime_goopy::WideToUtf8(unicode_text));
  return key_event;
}

ipc::proto::KeyEvent ConvertToIPCKey(UINT virtual_key,
                                     LPBYTE key_state,
                                     bool down) {
  return ConvertToIPCKeyImpl(virtual_key, key_state, down, false, NULL);
}

}  // namespace
const HKL KeyEventSink::english_hkl_ = ::LoadKeyboardLayout(L"00000409", 0);
KeyEventSink::KeyEventSink()
    : context_event_sink_(NULL),
      client_id_(TF_CLIENTID_NULL),
      engine_(NULL),
      enabled_(true),
      engine_eaten_(false) {
}

KeyEventSink::~KeyEventSink() {
  Uninitialize();
}

HRESULT KeyEventSink::Initialize(ContextEventSink *context_event_sink) {
  DVLOG(1) << __SHORT_FUNCTION__;
  context_event_sink_ = context_event_sink;
  context_ = context_event_sink->context();
  client_id_ = context_event_sink->client_id();
  engine_ = context_event_sink->GetEngine();

  HRESULT hr = E_FAIL;
  hr = key_event_sink_advisor_.Advise(context_, this);
  if (FAILED(hr)) return hr;

  return S_OK;
}

HRESULT KeyEventSink::Uninitialize() {
  DVLOG(1) << __SHORT_FUNCTION__;
  key_event_sink_advisor_.Unadvise();
  engine_ = NULL;
  client_id_ = TF_CLIENTID_NULL;
  context_.Release();
  context_event_sink_ = NULL;

  return S_OK;
}

// Called by the system to query this service wants a potential keystroke.
// Set eaten to true if we want to process this key.
// In some application, like wordpad, the application never call OnTestKeyXXX,
// all keyboard events are directly passed to OnKeyXXX. But in some other
// applications, like notpad, it will first call OnTestKeyXXX, and if the eaten
// parameter is set to FALSE, it will not call OnKeyXXX. This behavior is not
// the same as they documented in MSDN.
// Our solution here is, always call OnTestKeyXXX in the beginning of OnKeyXXX.
HRESULT KeyEventSink::OnTestKeyDown(WPARAM wparam, LPARAM lparam, BOOL *eaten) {
  DVLOG(3) << __SHORT_FUNCTION__
           << L" wparam: " << wparam
           << L" lparam: " << hex << lparam;
  assert(engine_);
  assert(eaten);
  if (!eaten) return E_INVALIDARG;
  *eaten = FALSE;
  // A short path for win key. Seem that if we took too long to process win key,
  // windows will discard it.
  if (wparam == VK_LWIN || wparam == VK_RWIN)
    return S_OK;
  if (!enabled_) return S_OK;

  BYTE key_state[256];
  bool succeeded = GetKeyboardState(key_state);
  assert(succeeded);
  ipc::proto::KeyEvent key = ConvertToIPCKey(wparam, key_state, true);
  ipc::proto::KeyEvent key_in_english = ConvertToIPCKeyImpl(
      wparam, key_state, true, true, english_hkl_);
  engine_eaten_ = engine_->ShouldProcessKey(key);
  bool visible = !key_in_english.text().empty() &&
                 isprint(key_in_english.text()[0]);
  *eaten = engine_eaten_ || visible;

  return S_OK;
}

// Called by the system to offer this service a keystroke.
// If *eaten == TRUE on exit, the application will not handle the keystroke.
// If eaten is FALSE and it's composing, caller will terminate the composition.
HRESULT KeyEventSink::OnKeyDown(WPARAM wparam, LPARAM lparam, BOOL *eaten) {
  DVLOG(3) << __SHORT_FUNCTION__
           << L" wparam: " << wparam
           << L" lparam: 0x" << hex << lparam;
  assert(engine_);
  assert(eaten);
  if (!eaten) return E_INVALIDARG;

  // Always call OnTestKeyXXX, in case the application hasn't called it.
  OnTestKeyDown(wparam, lparam, eaten);
  if (!*eaten) return S_OK;

  BYTE key_state[256];
  if (!GetKeyboardState(key_state)) return E_FAIL;

  HRESULT hr = RequestEditSession(
      context_, client_id_, this->GetUnknown(),
      NewPermanentCallback(this, &KeyEventSink::ProcessKeySession),
      ConvertToIPCKeyImpl(wparam, key_state, true, true, english_hkl_),
      TF_ES_SYNC | TF_ES_READWRITE);
  if (FAILED(hr)) {
    DVLOG(1) << "RequestEditSession failed in " << __SHORT_FUNCTION__;
    return hr;
  }
  return S_OK;
}

// Called by the system to query this service wants a potential keystroke.
// Set eaten to true if we want to process this key.
HRESULT KeyEventSink::OnTestKeyUp(WPARAM wparam, LPARAM lparam, BOOL *eaten) {
  DVLOG(3) << __SHORT_FUNCTION__
           << L" wparam: " << wparam
           << L" lparam: " << hex << lparam;
  assert(engine_);
  assert(eaten);
  if (!eaten) return E_INVALIDARG;

  *eaten = FALSE;
  // A short path for win key. Seem that if we took too long to process win key,
  // windows will discard it.
  if (wparam == VK_LWIN || wparam == VK_RWIN)
    return S_OK;
  if (!enabled_) return S_OK;

  BYTE key_state[256];
  if (!GetKeyboardState(key_state)) return E_FAIL;

  // It's not good to assume here the relationship of an UP key and the
  // corresponding DOWN key. It should be the engine who can decide.
  // That's to say, a DOWN key not being eaten doesn't necessarily mean
  // the corresponding UP key should not be eaten as well. Sticky key is
  // a good example.

  // It's curious that Microsoft word 2003/2007 will call OnTestKeyUp for
  // a DOWN key. We must filter those invalid keystrokes here.

  if (engine_->ShouldProcessKey(ConvertToIPCKey(wparam, key_state, false))) {
    *eaten = TRUE;
  }

  return S_OK;
}

// Called by the system to offer this service a keystroke.
// If *eaten == TRUE on exit, the application will not handle the keystroke.
HRESULT KeyEventSink::OnKeyUp(WPARAM wparam, LPARAM lparam, BOOL *eaten) {
  DVLOG(3) << __SHORT_FUNCTION__
           << L" wparam: " << wparam
           << L" lparam: " << hex << lparam;
  assert(engine_);
  assert(eaten);
  if (!eaten) return E_INVALIDARG;

  // Always call OnTestKeyXXX, in case the application hasn't called it.
  OnTestKeyUp(wparam, lparam, eaten);
  if (!*eaten) return S_OK;

  BYTE key_state[256];
  if (!GetKeyboardState(key_state)) return E_FAIL;
  DVLOG(3) << __SHORT_FUNCTION__ << " keyup processed.";
  HRESULT hr = RequestEditSession(
      context_, client_id_, this->GetUnknown(),
      NewPermanentCallback(this, &KeyEventSink::ProcessKeySession),
      ConvertToIPCKey(wparam, key_state, false),
      TF_ES_SYNC | TF_ES_READWRITE);
  if (FAILED(hr)) {
    DVLOG(1) << "RequestEditSession failed in " << __SHORT_FUNCTION__;
    return hr;
  }
  return S_OK;
}

HRESULT KeyEventSink::ProcessKeySession(TfEditCookie cookie,
                                        ipc::proto::KeyEvent key) {
  DVLOG(3) << __SHORT_FUNCTION__
    << L" key: " << key.keycode() << "L " << key.type();
  DCHECK(context_event_sink_->write_cookie() == TF_INVALID_COOKIE);
  context_event_sink_->set_write_cookie(cookie);
  if (engine_eaten_) {
    engine_->ProcessKey(key);
  } else {
    context_event_sink_->CommitResult(Utf8ToWide(key.text()));
  }
  context_event_sink_->set_write_cookie(TF_INVALID_COOKIE);
  return S_OK;
}
}  // namespace tsf
}  // namespace ime_goopy
