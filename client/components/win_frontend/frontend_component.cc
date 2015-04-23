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

#include "components/win_frontend/frontend_component.h"
#include <windows.h>
#include <atlbase.h>
#include "appsensorapi/appsensor_helper.h"
#include "appsensorapi/common.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "base/synchronization/waitable_event.h"
#include "base/time.h"
#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/clipboard.h"
#include "common/debug.h"
#include "common/registry.h"
#include "common/shellutils.h"
#include "common/ui_utils.h"
#include "components/win_frontend/composition_window.h"
#include "components/win_frontend/composition_window_layouter.h"
#include "components/win_frontend/frontend_factory.h"
#include "frontend/text_styles.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "locale/locale_utils.h"
#include "tsf/tsf_utils.h"

namespace {

enum {
  // Imm will send this message to create input context.
  MSG_IMM_CREATE_INPUT_CONTEXT = ipc::MSG_USER_DEFINED_START,
  // The message is sent to switch to another input locale.
  MSG_IMM_SWITCH_INPUT_METHOD,
};

const uint32 kProduceMessages[] = {
  // User defined messgaes.
  MSG_IMM_CREATE_INPUT_CONTEXT,
  MSG_IMM_SWITCH_INPUT_METHOD,
  // IPC predefined messages.
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
  ipc::MSG_CONVERSION_MODE_CHANGED,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_DELETE_INPUT_CONTEXT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_HIDE_CANDIDATE_LIST_UI,
  ipc::MSG_HIDE_COMPOSITION_UI,
  ipc::MSG_HIDE_TOOLBAR_UI,
  ipc::MSG_LIST_INPUT_METHODS,
  ipc::MSG_QUERY_ACTIVE_CONSUMER,
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_REQUEST_CONSUMER,
  ipc::MSG_SELECT_CANDIDATE,
  ipc::MSG_SEND_KEY_EVENT,
  ipc::MSG_SHOW_CANDIDATE_LIST_UI,
  ipc::MSG_SHOW_COMPOSITION_UI,
  ipc::MSG_SHOW_TOOLBAR_UI,
  ipc::MSG_SWITCH_TO_INPUT_METHOD,
  ipc::MSG_UPDATE_INPUT_CARET,
};

const uint32 kConsumeMessages[] = {
  // User defined messgaes.
  MSG_IMM_CREATE_INPUT_CONTEXT,
  MSG_IMM_SWITCH_INPUT_METHOD,
  // IPC predefined messages.
  ipc::MSG_ACTIVE_CONSUMER_CHANGED,
  ipc::MSG_CANDIDATE_LIST_CHANGED,
  ipc::MSG_COMPOSITION_CHANGED,
  ipc::MSG_CONVERSION_MODE_CHANGED,
  ipc::MSG_INPUT_CONTEXT_DELETED,
  ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
  ipc::MSG_INPUT_CONTEXT_LOST_FOCUS,
  ipc::MSG_INPUT_METHOD_ACTIVATED,
  ipc::MSG_INSERT_TEXT,
  ipc::MSG_SYNTHESIZE_KEY_EVENT,
  ipc::MSG_ENABLE_FAKE_INLINE_COMPOSITION,
};

// String id of ime frontend component.
const char* frontend_ime_string_id = "com.google.ime.goopy.frontend";

// Legacy from engine.cc
const wchar_t kLastUseTimeName[] = L"ActiveTime";

// String keys for frontend component settings.
const char* kSettingsEnableDashboardKey = "EnableDashboard";
const char* kSettingsFullscreenAppFloatingStatusKey =
    "FullscreenAppNoFloatingStatus";
const char* kSettingsInlineModeKey = "InlineMode";

typedef struct {
  const char* key;
  int32 default_value;
} StringToIntegerTable;

const StringToIntegerTable kSettingsIntegerDefaultValues[] = {
  {kSettingsEnableDashboardKey, 0},
  {kSettingsFullscreenAppFloatingStatusKey, 0},
  {kSettingsInlineModeKey, 0},
};

}  // namespace

namespace ime_goopy {
namespace components {

using win_frontend::CompositionWindowList;
using win_frontend::CompositionWindowLayouter;

namespace {

// Returns the keyboard layout (IMM) or language profile (TSF) for a package.
std::wstring GetKeyboardLayoutForPack(const std::wstring& pack_name,
                                      DWORD* lang_id) {
  std::wstring key =
      WideStringPrintf(L"%s\\%s\\%s",
                       kInputRegistryKey, kPacksSubKey, pack_name.c_str());
  REGSAM flags = KEY_READ | KEY_WOW64_64KEY;
  scoped_ptr<RegistryKey> registry(
      RegistryKey::OpenKey(HKEY_LOCAL_MACHINE, key.c_str(), flags));
  if (!registry.get())
    return L"";

  std::wstring value;
  if (registry->QueryStringValue(kPackKeyboardLayoutValue, &value) !=
      ERROR_SUCCESS) {
    return L"";
  }
  if (lang_id) {
    registry->QueryDWORDValue(kPackLanguageId, *lang_id);
  }
  return value;
}

std::wstring GetCurrentKeyboardLayoutName() {
  if (ShellUtils::SupportTSF()) {
    return tsf::TSFUtils::GetCurrentLanguageProfile();
  } else {
    wchar_t layout[MAX_PATH] = { 0 };
    ::GetKeyboardLayoutName(layout);
    return layout;
  }
}

void SwitchToKeyboardLayout(const std::wstring& keyboard_layout, DWORD lang) {
  if (ShellUtils::SupportTSF()) {
    tsf::TSFUtils::SwitchToTIP(lang, keyboard_layout);
  } else {
    HKL hkl = ::LoadKeyboardLayout(keyboard_layout.c_str(), KLF_ACTIVATE);
    DCHECK(hkl);
  }
}

bool IsRTLSystem() {
  // We can't detect the layout of application in ipc_console, so we use the
  // layout of shell window as the default layout.
  return ::GetWindowLong(::GetShellWindow(), GWL_EXSTYLE) &
         WS_EX_LAYOUTRTL;
}

}  // namespace

FrontendComponent::FrontendComponent(Delegate* delegate)
    : context_(NULL),
      caret_(0),
      caret_in_window_(0),
      composition_terminating_(false),
      icid_(0),
      is_rtl_language_(false),
      ime_loaded_(false),
      ui_loaded_(false),
      waiting_process_key_(false),
      conversion_mode_(CONVERSION_MODE_CHINESE),
      settings_client_(new ipc::SettingsClient(this, this)),
      delegate_(delegate),
      console_pid_(0),
      switching_input_method_by_toolbar_(false),
      enable_fake_inline_composition_(false) {
  composition_window_list_.reset(CompositionWindowList::CreateInstance());
  composition_window_list_->Initialize();
}

FrontendComponent::~FrontendComponent() {
  RemoveFromHost();
  while (!cached_messages_.empty()) {
    delete cached_messages_.front();
    cached_messages_.pop();
  }
  while (!cached_produced_messages_.empty()) {
    delete cached_produced_messages_.front();
    cached_produced_messages_.pop();
  }
}

void FrontendComponent::OnValueChanged(const std::string& key,
                                       const ipc::proto::VariableArray& array) {
  for (size_t i = 0; i < arraysize(kSettingsIntegerDefaultValues); ++i) {
    if (key == kSettingsIntegerDefaultValues[i].key) {
      DCHECK_EQ(array.variable_size(), 1);
      DCHECK_EQ(array.variable(0).type(), ipc::proto::Variable::INTEGER);
      settings_integers_[kSettingsIntegerDefaultValues[i].key] =
          array.variable(0).integer();
      break;
    }
  }
}

void FrontendComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  char buf[MAX_PATH] = {0};
  snprintf(buf, MAX_PATH, "%s_%d_%llx",
           frontend_ime_string_id,
           GetCurrentThreadId(),
           reinterpret_cast<uint64>(this));
  info->set_string_id(buf);
  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);

  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);

  GetSubComponentsInfo(info);
}

void FrontendComponent::OnRegistered() {
  // Registers observers for interested settings.
  ipc::SettingsClient::KeyList key_list;
  for (int i = 0; i < arraysize(kSettingsIntegerDefaultValues); ++i) {
    key_list.push_back(kSettingsIntegerDefaultValues[i].key);
  }
  if (!settings_client_->AddChangeObserverForKeys(key_list)) {
    DLOG(ERROR) << "AddChangeObserverForKeys failed";
    OnIPCDisconnected();
    return;
  }
  int64 pid = 0;
  if (!settings_client_->GetIntegerValue(kSettingsIPCConsolePID, &pid)) {
    DLOG(ERROR) << "Failed to get the process id for ipc console";
    OnIPCDisconnected();
    return;
  }
  // This setting only changes when ipc_console crashes and creates a new
  // instance.
  console_pid_ = static_cast<DWORD>(pid);
  // The current thread may be not same as the one IPC window belongs to, so
  // deliver a message to IPC window to ensure it.
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(MSG_IMM_CREATE_INPUT_CONTEXT,
                 ipc::kInputContextNone, false));
  mptr->set_source(id());
  mptr->set_target(id());
  if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send failed type = MSG_CREATE_INPUT_CONTEXT";
    OnIPCDisconnected();
  }
}

void FrontendComponent::OnDeregistered() {
  OnIPCDisconnected();
  console_pid_ = 0;
}

void FrontendComponent::Handle(ipc::proto::Message* message) {
  if (waiting_process_key_ &&
      (message->type() == ipc::MSG_COMPOSITION_CHANGED ||
       message->type() == ipc::MSG_CANDIDATE_LIST_CHANGED ||
       message->type() == ipc::MSG_INSERT_TEXT)) {
    // Message of specific types should be cached until |ProcessKey| is called.
    CacheMessage(message);
    return;
  }
  DoHandle(message);
}

bool FrontendComponent::ShouldProcessKey(const ipc::proto::KeyEvent& key) {
  // As the major components in the framework lives in the process of
  // ipc_console, we need to allow the ipc console process to set foreground
  // window in case some component in the ipc console process needs to pop up
  // a window and get focused. As the API AllowSetForegroundWindow() only take
  // effect before receiving next input, so we need to call it every time there
  // is a chance that ipc_console will pop up UI. And there are 3 ways for user
  // to trigger an UI popup: using menu or button on status bar, or hotkeys.
  // So we call this API in ShouldProcessKey and win menu component, because
  // clicking on the status bar won't generate input to the application.
  ::AllowSetForegroundWindow(console_pid_);
  if (!ime_loaded_ || !ui_loaded_)
    return false;

  // Bypass twice call of ShouldProcessKey which happens in TSF.
  if (waiting_process_key_)
    return true;

  if (key.type() == ipc::proto::KeyEvent::UP)
    DLOG(INFO) << "key up catched";

  // NOTE(haicsun): T13n don't need to consider capital status again, but goopy
  // does, consider it again when integrating goopy.
  ipc::proto::Message* message = NewMessage(ipc::MSG_SEND_KEY_EVENT,
                                            icid_,
                                            true);
  message->mutable_payload()->mutable_key_event()->CopyFrom(key);

  ipc::proto::Message* reply = NULL;
  waiting_process_key_ = true;
  if (!SendWithReply(message, -1, &reply)) {
    DLOG(ERROR) << "SendMessage failed type = MSG_SEND_KEY_EVENT";
    OnIPCDisconnected();
    return false;
  }

  bool should_process = reply->payload().boolean(0);
  DVLOG(3) << __SHORT_FUNCTION__
           << L" should process: " << should_process;
  delete reply;

  if (!should_process) {
    // Clean up cached messages.
    waiting_process_key_ = false;
    while (!cached_messages_.empty()) {
      DoHandle(cached_messages_.front());
      cached_messages_.pop();
    }
  }
  return should_process;
}

void FrontendComponent::ProcessKey(const ipc::proto::KeyEvent& key) {
  // We may receive vkey 0 in some rare cases, see bug 1814414 for more info.
  if (key.keycode() == 0)
    return;
  if (!ime_loaded_ || !ui_loaded_)
    return;

  if (!waiting_process_key_) {
    // It's not supposed to happen, but in case it happened, send a
    // message to ime.
    ipc::proto::Message* message = NewMessage(ipc::MSG_SEND_KEY_EVENT,
                                              icid_,
                                              true);
    message->mutable_payload()->mutable_key_event()->CopyFrom(key);

    ipc::proto::Message* reply = NULL;
    if (!SendWithReply(message, -1, &reply)) {
      DLOG(ERROR) << "SendWithReply failed type = MSG_SEND_KEY_EVENT";
      OnIPCDisconnected();
      return;
    }
    DCHECK(reply && reply->payload().boolean_size() &&
           reply->payload().boolean(0));
    delete reply;
  }

  waiting_process_key_ = false;
  // Handle cached messages received during call of |ShouldProcessKey|.
  while (!cached_messages_.empty()) {
    ipc::proto::Message* message = cached_messages_.front();
    DCHECK(message->type() == ipc::MSG_COMPOSITION_CHANGED ||
           message->type() == ipc::MSG_CANDIDATE_LIST_CHANGED ||
           message->type() == ipc::MSG_INSERT_TEXT);

    DoHandle(cached_messages_.front());
    cached_messages_.pop();
  }
}

void FrontendComponent::SelectCandidate(int index, bool commit) {
  DCHECK(candidates_.id());
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(ipc::MSG_SELECT_CANDIDATE,
                                                  icid_,
                                                  false));
  mptr->mutable_payload()->add_uint32(candidates_.id());
  mptr->mutable_payload()->add_uint32(index);
  mptr->mutable_payload()->add_boolean(commit);

  if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send failed: type = MSG_SELECT_CANDIDATE";
    OnIPCDisconnected();
  }
}

void FrontendComponent::EndComposition(bool commit) {
  if (composition_terminating_) return;
  composition_terminating_ = true;
  if (!ime_loaded_ || !ui_loaded_)
    return;
  if (commit) {
    scoped_ptr<ipc::proto::Message> mptr(
        NewMessage(ipc::MSG_COMPLETE_COMPOSITION, icid_, true));
    ipc::proto::Message* reply = NULL;
    if (!SendWithReply(mptr.release(), -1, &reply)) {
      DLOG(ERROR) << "SendMessage failed: type = MSG_COMPLETE_COMPOSITION";
      OnIPCDisconnected();
    }
    delete reply;
  } else {
    scoped_ptr<ipc::proto::Message> mptr(
        NewMessage(ipc::MSG_CANCEL_COMPOSITION, icid_, true));
    ipc::proto::Message* reply = NULL;
    if (!SendWithReply(mptr.release(), -1, &reply)) {
      DLOG(ERROR) << "SendMessage failed: type = MSG_CANCEL_COMPOSITION";
      OnIPCDisconnected();
    }
    delete reply;
  }
  composition_terminating_ = false;
}

void FrontendComponent::FocusInputContext() {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_FOCUS_INPUT_CONTEXT, icid_, false));

  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = MSG_FOCUS_INPUT_CONTEXT";
    OnIPCDisconnected();
  }
  wchar_t path[MAX_PATH] = { 0 };
  ::GetModuleFileName(NULL, path, MAX_PATH);
  ::PathStripPath(path);
  wchar_t wndcls[MAX_PATH] = { 0 };
  ::RealGetWindowClass(::GetForegroundWindow(), wndcls, MAX_PATH);
}

void FrontendComponent::BlurInputContext() {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_BLUR_INPUT_CONTEXT, icid_, false));

  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = MSG_BLUR_INPUT_CONTEXT";
    OnIPCDisconnected();
  }
  EnableCandidateWindow(false);
  EnableCompositionWindow(false);
}

void FrontendComponent::EnableCompositionWindow(bool enable) {
  if (enable && enable_fake_inline_composition_) {
    RECT rect = {0};
    UpdateInlineCompositionWindow(&rect);
  } else {
    composition_window_list_->Hide();
  }
  bool enable_composition_ui = enable && !enable_fake_inline_composition_;
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      enable_composition_ui ? ipc::MSG_SHOW_COMPOSITION_UI :
                              ipc::MSG_HIDE_COMPOSITION_UI,
      icid_,
      false));
  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = "
                << (enable_composition_ui ?
                    "MSG_SHOW_COMPOSITION_UI" : "MSG_HIDE_COMPOSITION_UI");
    OnIPCDisconnected();
    return;
  }
}

void FrontendComponent::EnableCandidateWindow(bool enable) {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      enable ? ipc::MSG_SHOW_CANDIDATE_LIST_UI: ipc::MSG_HIDE_CANDIDATE_LIST_UI,
      icid_,
      false));
  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = "
                << (enable ?
                "MSG_SHOW_CANDIDATE_LIST_UI" : "MSG_HIDE_CANDIDATE_LIST_UI");
    OnIPCDisconnected();
  }
}

void FrontendComponent::EnableToolbarWindow(bool enable) {
  int32 int_value = 0;
  // Don't show status view in fullscreen apps.
  if (enable && UIUtils::IsInFullScreenWindow(::GetFocus()) &&
      !ShellUtils::CheckWindows8()) {
    GetIntegerValue(kSettingsFullscreenAppFloatingStatusKey, &int_value);
    if (!int_value)
      return;
  }

  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      enable ? ipc::MSG_SHOW_TOOLBAR_UI: ipc::MSG_HIDE_TOOLBAR_UI,
      icid_,
      false));

  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = "
                << (enable ?
                "MSG_SHOW_TOOLBAR_UI" : "MSG_HIDE_CANDIDATE_LIST_UI");
    OnIPCDisconnected();
  }
}

void FrontendComponent::UpdateInputCaret() {
  DVLOG(4) << __SHORT_FUNCTION__;
  RECT last_composition_rect = {0};

  // If composition window is empty, in most cases it doesn't need to update
  // the input caret, but if it really needs, returns the input caret rect to
  // keep it right.
  // If our inline composition window shows, the candidate position should be
  // placed in the bottom left or bottom right (depends on display direction)
  // of last composition window.
  if (composition_.empty() ||
      !context_->ShouldShow(ContextInterface::UI_COMPONENT_COMPOSITION) ||
      !enable_fake_inline_composition_) {
    if (!context_->GetCaretRectForCandidate(&last_composition_rect))
      return;
  } else if (!UpdateInlineCompositionWindow(&last_composition_rect)) {
    return;
  }

  {
    // Verify caret position.
    // if it's out of screen in both dimensions, don't update the input caret.
    // TODO(haicsun): support bidi.
    RECT monitor_rect = {0};
    POINT pt = {0};
    pt.x = last_composition_rect.left;
    pt.y = last_composition_rect.bottom;
    HMONITOR monitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    if (::GetMonitorInfo(monitor, &monitor_info)) {
      monitor_rect = monitor_info.rcWork;
      if (!::PtInRect(&monitor_rect, pt))
        return;
    }
  }

  scoped_ptr<ipc::proto::Message> mptr(new ipc::proto::Message());
  mptr->set_type(ipc::MSG_UPDATE_INPUT_CARET);
  mptr->set_source(id());
  mptr->set_target(ipc::kComponentBroadcast);
  mptr->set_icid(icid_);
  mptr->set_reply_mode(ipc::proto::Message::NO_REPLY);

  ipc::proto::InputCaret* caret =
      mptr->mutable_payload()->mutable_input_caret();
  caret->mutable_rect()->set_x(last_composition_rect.left);
  caret->mutable_rect()->set_y(last_composition_rect.top);
  caret->mutable_rect()->set_width(
      last_composition_rect.right - last_composition_rect.left);
  caret->mutable_rect()->set_height(
      last_composition_rect.bottom - last_composition_rect.top);

  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = MSG_UPDATE_INPUT_CARET";
    OnIPCDisconnected();
  }
}

void FrontendComponent::ResizeCandidatePage(int new_size) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE, icid_, false));
  mptr->mutable_payload()->add_uint32(candidates_.id());
  mptr->mutable_payload()->add_uint32(new_size);
  mptr->mutable_payload()->add_uint32(1);
  if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "SendMessage failed: type = MSG_CANCEL_COMPOSITION";
    OnIPCDisconnected();
  }
}

int FrontendComponent::GetTextStyleIndex(TextState text_state) {
  switch (text_state) {
    case TEXTSTATE_COMPOSING:
      return STOCKSTYLE_UNDERLINED;
    case TEXTSTATE_HOVER:
      // Currently we don't have the hover state.
      DLOG(ERROR);
      return STOCKSTYLE_NORMAL;
    default:
      DLOG(ERROR);
      return STOCKSTYLE_NORMAL;
  }
}

void FrontendComponent::NotifyConversionModeChange(uint32 conversion_mode) {
  DWORD new_conversion_mode = 0;
  // Some application like Fetion 2008 does not set all IME_CMODE_* bits
  // correctly. Partically, IME_CMODE_NATIVE is reset unintentionally. so we
  // ignore setting CONVERSION_MODE_CHINESE which represents IME_CMODE_NATIVE.
  new_conversion_mode |= (conversion_mode_ & CONVERSION_MODE_CHINESE);
  new_conversion_mode |= (conversion_mode & CONVERSION_MODE_FULL_SHAPE);
  new_conversion_mode |= (conversion_mode & CONVERSION_MODE_FULL_PUNCT);

  if (conversion_mode_ == new_conversion_mode)
    return;

  conversion_mode_ = new_conversion_mode;

  scoped_ptr<ipc::proto::Message> mptr(new ipc::proto::Message());
  mptr->set_type(ipc::MSG_CONVERSION_MODE_CHANGED);
  mptr->set_reply_mode(ipc::proto::Message::NO_REPLY);
  mptr->set_source(id());
  mptr->set_target(ipc::kComponentBroadcast);
  mptr->set_icid(icid_);
  mptr->mutable_payload()->add_boolean(
      conversion_mode_ & CONVERSION_MODE_CHINESE);
  mptr->mutable_payload()->add_boolean(
      conversion_mode_ & CONVERSION_MODE_FULL_SHAPE);
  mptr->mutable_payload()->add_boolean(
      conversion_mode_ & CONVERSION_MODE_FULL_PUNCT);
  if (!ime_loaded_ || !ui_loaded_) {
    // Wait for context ready.
    cached_produced_messages_.push(mptr.release());
  } else {if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "Send error type = MSG_CONVERSION_MODE_CHANGED";
    OnIPCDisconnected();
  }
}

uint32 FrontendComponent::GetConversionMode() {
  return conversion_mode_;
}

void FrontendComponent::SetContext(ContextInterface* context) {
  context_ = context;
  // This function is called with |context| != NULL only when user switches to
  // our input method, and there are two situations as below:
  //   1. When user is switching to our input methods for the first time.
  //   2. When user is switching between two of our input methods.
  // In the first case, frontend component will create a hub input context when
  // registered and switch set active input method of hub when the hub input
  // context is created. In such case, |icid| is 0 and nothing special needs to
  // be done here.
  // In the second case, a hub input context is already created, and there are
  // two subcases:
  //   2.1 User switches IME using our toolbar. In this case, UI component
  //       will switch the active input method in hub and hub will inform the
  //       frontend component of the change, frontend component then shelve
  //       itself and switch the system input locale to match the active input
  //       method, and a new IMM input context will be created and set to the
  //       frontend component. As the active input method of the hub input
  //       context already matches the system input locale, we only need to set
  //       |switching_input_method_by_toolbar_| to false to indicate that the
  //       switching process is finished.
  //   2.2 User switches IMEs using system language bar. In this case,
  //       the frontend component will be shelved when IMM deselects our input
  //       method, then it will be unshelved when IMM selects our input method
  //       again and the new IMM input context will be set to the frontend
  //       component. Frontend component should inform hub to activate the new
  //       input method for the hub input context owned by the frontend
  //       component to match the system input locale.
  if (icid_ && context_) {
    // Case 2.
    if (switching_input_method_by_toolbar_) {
      // Case 2.1.
      switching_input_method_by_toolbar_ = false;
    } else {
      // Case 2.2.
      IPCSwitchToInputMethod();
    }
  }
}

void FrontendComponent::IPCCreateInputContext() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_CREATE_INPUT_CONTEXT,
      ipc::kInputContextNone,
      true));

  ipc::proto::Message* reply = NULL;
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << "SendWithReply error: MSG_CREATE_INPUT_CONTEXT";
    OnIPCDisconnected();
    return;
  }

  DCHECK(reply);
  icid_ = reply->icid();
  delegate_->InputContextCreated(icid_);
  delete reply;
}

bool FrontendComponent::IPCSwitchToInputMethod() {
  // Get system current keyboard layout.
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_LIST_INPUT_METHODS,
      icid_,
      true));

  scoped_ptr<ipc::proto::Message> list_input_methods_reply;
  {
    // Construct reply message.
    ipc::proto::Message* message = NULL;
    if (!SendWithReply(mptr.release(), -1, &message)) {
      DLOG(ERROR) << "SendWithReply error type = MSG_LIST_INPUT_METHODS";
      OnIPCDisconnected();
      return false;
    }
    DCHECK(message);
    list_input_methods_reply.reset(message);
  }

  if (!list_input_methods_reply->payload().component_info_size())
    return false;

  std::string current_language = LocaleUtils::GetKeyboardLayoutLocaleName();
  int new_input_method = ipc::kComponentDefault;
  std::wstring system_keyboard_layout = GetCurrentKeyboardLayoutName();
  bool found = false;
  for (int i = 0;
       !found && i < list_input_methods_reply->payload().component_info_size();
       ++i) {
    const ipc::proto::ComponentInfo& info =
        list_input_methods_reply->payload().component_info(i);

    for (int lang_idx = 0; lang_idx < info.language_size(); ++lang_idx) {
      if (system_keyboard_layout ==
          GetKeyboardLayoutForPack(Utf8ToWide(info.string_id()), NULL)) {
        // If the entry in the registry shows that the component is register for
        // the locale, switch to this input method.
        new_input_method = info.id();
        found = true;
        break;
      } else if (LocaleUtils::PrimaryLocaleEquals(info.language(lang_idx),
                                                  current_language)) {
        // Else if the language shares the same primary language with current
        // input language, we save it as the backup input method and switch
        // to it if no perfect match found.
        new_input_method = info.id();
      }
    }
  }

  if (new_input_method != ipc::kComponentDefault) {
    mptr.reset(NewMessage(ipc::MSG_SWITCH_TO_INPUT_METHOD, icid_, false));
    mptr->mutable_payload()->add_uint32(new_input_method);
    if (!Send(mptr.release(), NULL)) {
      DLOG(ERROR) << "Send error type = MSG_SWITCH_TO_INPUT_METHOD";
      OnIPCDisconnected();
      return false;
    }
    return true;
  }
  return false;
}

void FrontendComponent::IPCRequestConsumer() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_REQUEST_CONSUMER,
      icid_,
      false));

  for (int i = 0; i < arraysize(kProduceMessages); ++i)
    mptr->mutable_payload()->add_uint32(kProduceMessages[i]);

  if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Send error type = MSG_REQUEST_CONSUMER";
    OnIPCDisconnected();
  }
}

void FrontendComponent::DoHandle(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);

  if (HandleMessageBySubComponents(mptr.get()))
    mptr.release();

  switch (mptr->type()) {
    case MSG_IMM_CREATE_INPUT_CONTEXT:
      IPCCreateInputContext();
      if (!IPCSwitchToInputMethod()) {
        // It shouldn't happen, but in case it happens, It's up to hub to
        // decide which IME should be employed.
        DLOG(ERROR) << "Can't switch to proper ime";
      }
      IPCRequestConsumer();
      break;
    case ipc::MSG_ACTIVE_CONSUMER_CHANGED:
      OnMsgActiveConsumerChanged(mptr.release());
      break;
    case ipc::MSG_INPUT_CONTEXT_LOST_FOCUS:
      OnMsgInputContextLostFocus(mptr.release());
      break;
    case ipc::MSG_INPUT_CONTEXT_GOT_FOCUS:
      OnMsgInputContextGotFocus(mptr.release());
      break;
    case ipc::MSG_INPUT_CONTEXT_DELETED:
      OnMsgInputContextDeleted(mptr.release());
      break;
    case ipc::MSG_INPUT_METHOD_ACTIVATED:
      OnMsgInputMethodActivated(mptr.release());
      break;
    case ipc::MSG_COMPOSITION_CHANGED:
      OnMsgCompositionChanged(mptr.release());
      break;
    case ipc::MSG_CANDIDATE_LIST_CHANGED:
      OnMsgCandidateListChanged(mptr.release());
      break;
    case ipc::MSG_CONVERSION_MODE_CHANGED:
      OnMsgConversionModeChanged(mptr.release());
      break;
    case ipc::MSG_INSERT_TEXT:
      OnMsgInsertText(mptr.release());
      break;
    case ipc::MSG_SYNTHESIZE_KEY_EVENT:
      OnMsgSynthesizeKeyEvent(mptr.release());
      break;
    case ipc::MSG_ENABLE_FAKE_INLINE_COMPOSITION:
      OnMsgEnableFakeInlineComposition(mptr.release());
      break;
    default:
      DLOG(ERROR) << "Can't handle message type: " << mptr->type();
      ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE, "");
      break;
  }
  if (mptr.get())
    ReplyTrue(mptr.release());
}

void FrontendComponent::OnMsgActiveConsumerChanged(
    ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  scoped_ptr<ipc::proto::Message> mptr(message);
  DCHECK_GT(mptr->payload().uint32_size(), 0);
  if (mptr->icid() != icid_)
    return;

  const int uint32_size = mptr->payload().uint32_size();
  for (int i = 0; i < uint32_size; ++i) {
    if (mptr->payload().uint32(i) == ipc::MSG_PROCESS_KEY_EVENT) {
      ime_loaded_ = true;
      break;
    } else if (mptr->payload().uint32(i) == ipc::MSG_SHOW_COMPOSITION_UI) {
      ui_loaded_ = true;
      break;
    }
  }

  if (ime_loaded_ && ui_loaded_) {
    // Record last use time to settings store.
    if (!settings_client_->SetIntegerValue(
        WideToUtf8(kLastUseTimeName), base::Time::Now().ToInternalValue())) {
      DLOG(ERROR) << "Can't set settings";
      return;
    }

    // Send all cached messages.
    while (!cached_produced_messages_.empty()) {
      ipc::proto::Message* cached_message = cached_produced_messages_.front();
      DCHECK(!cached_message->icid() || cached_message->icid() == icid_);
      // The messages in |cached_produced_messages_| may be created before input
      // context is created, so |icid_| is not properly set, they must be set
      // again before sending.
      cached_message->set_icid(icid_);
      cached_message->set_source(id());

      if (!Send(cached_produced_messages_.front(), NULL)) {
        DLOG(ERROR) << "Send() failed type= "
                    << cached_produced_messages_.front()->type();
        OnIPCDisconnected();
      }
      cached_produced_messages_.pop();
    }
  }

  ReplyTrue(mptr.release());
}

void FrontendComponent::OnMsgInputContextLostFocus(
    ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  ReplyTrue(message);
}

void FrontendComponent::OnMsgInputContextGotFocus(
    ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  ReplyTrue(message);
}

void FrontendComponent::OnMsgInputContextDeleted(ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  ReplyTrue(message);
}

void FrontendComponent::OnMsgInputMethodActivated(
    ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;

  scoped_ptr<ipc::proto::Message> mptr(message);
  DCHECK_EQ(mptr->payload().component_info_size(), 1);

  if (mptr->payload().component_info_size() != 1 || mptr->icid() != icid_)
    return;

  const ipc::proto::ComponentInfo& component_info =
      mptr->payload().component_info(0);
  if (component_info.string_id() != current_input_method_.string_id()) {
    current_input_method_.CopyFrom(component_info);
    is_rtl_language_ = LocaleUtils::IsRTLLanguage(component_info.language(0));

    DCHECK_NE(component_info.language_size(), 0);
    // If in IMM call stack, the switching should be postponed until imm call
    // is finished to avoid potential bugs.
    // context_ is NULL when the an input keyboard layout is switching, so also
    // need to postpone the switch if already have one.
    if (!waiting_process_key_ && context_) {
      SwitchInputMethod();
    } else {
      // SwitchInputMethod will be called when the message is handled.
      mptr.reset(NewMessage(MSG_IMM_SWITCH_INPUT_METHOD, icid_, false));
      if (!Send(mptr.release(), NULL)) {
        DLOG(ERROR) << "Send failed type = MSG_IMM_SWITCH_INPUT_METHOD.";
        OnIPCDisconnected();
      }
    }
  }
}

void FrontendComponent::OnMsgCompositionChanged(ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  scoped_ptr<ipc::proto::Message> mptr(message);

  raw_composition_.CopyFrom(mptr->payload().composition());
  DCHECK_EQ(raw_composition_.selection().start(),
            raw_composition_.selection().end());
  caret_in_window_ = raw_composition_.selection().end();
  composition_in_window_ = Utf8ToWide(raw_composition_.text().text());

  ReplyTrue(mptr.release());

  // |composition_| may be different from |composition_in_window_| in
  // inline_mode, when inline_text field is provided in the message.
  if (raw_composition_.has_inline_text()) {
    composition_ = Utf8ToWide(raw_composition_.inline_text().text());
    caret_ = raw_composition_.inline_selection().end();
  } else {
    composition_ = composition_in_window_;
    caret_ = caret_in_window_;
  }

  UpdateComposition();
}

void FrontendComponent::OnMsgCandidateListChanged(
    ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  scoped_ptr<ipc::proto::Message> mptr(message);
  candidates_.CopyFrom(mptr->payload().candidate_list());
  // TODO(haicsun): remove help_tips_.
  if (candidates_.has_footnote())
    help_tips_ = Utf8ToWide(candidates_.footnote().text());

  context_->UpdateCandidates(candidates_.candidate_size() > 0, candidates_);
  ReplyTrue(mptr.release());
}

void FrontendComponent::OnMsgInsertText(ipc::proto::Message* message) {
  DVLOG(3) << __SHORT_FUNCTION__;
  scoped_ptr<ipc::proto::Message> mptr(message);
  std::wstring result = Utf8ToWide(mptr->payload().composition().text().text());
  DCHECK(context_);
  if (result.find(L'\n') != wstring::npos) {
    // The result is a multi-line text. We should use clipboard to send the
    // text.
    Clipboard clipboard;

    // Transform the linefeed format of the result and write to clipboard.
    clipboard.WriteText(ToWindowsCRLF(result));
    DCHECK(clipboard.IsFormatAvailable(CF_UNICODETEXT));
    clipboard.Destroy();

    // Simulate <Shift>+<Insert> to paste the clipboard text to application.
    // NOTE: Several attempts had been made but no perfect solution here.
    // WM_PASTE doesn't work for non-standard edit controls. <Ctrl>+V could be
    // redefined in other games and editors (emacs). <Shift>+<Insert> seems to
    // be the most general solution here since <Insert> is a key with special
    // meaning and less likely to be redefined.
    //
    // The fallback of this method also includes that we cannot restore the
    // original content in clipboard, because SendInput returns before the
    // paste actually happens.
    INPUT input[4] = { 0 };
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_SHIFT;
    input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_INSERT;
    input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input[2] = input[1];
    input[2].ki.dwFlags |= KEYEVENTF_KEYUP;
    input[3] = input[0];
    input[3].ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(arraysize(input), input, sizeof(input[0]));
  }

  caret_ = result.size();
  composition_.clear();
  caret_in_window_ = 0;
  composition_in_window_.clear();
  candidates_.Clear();
  help_tips_.clear();
  candidates_.set_id(0);
  context_->CommitResult(result);

  ReplyTrue(mptr.release());
}

void FrontendComponent::OnMsgConversionModeChanged(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  DCHECK_EQ(mptr->payload().boolean_size(), 3);
  uint32 conversion = 0;
  bool native = mptr->payload().boolean(0);
  bool full_shape = mptr->payload().boolean(1);
  bool full_punct = mptr->payload().boolean(2);
  conversion |= native ? CONVERSION_MODE_CHINESE : 0;
  conversion |= full_shape ? CONVERSION_MODE_FULL_SHAPE : 0;
  conversion |= full_punct ? CONVERSION_MODE_FULL_PUNCT : 0;
  if (conversion != conversion_mode_) {
    conversion_mode_ = conversion;
    context_->UpdateStatus(native, full_shape, full_punct);
  }
}

void FrontendComponent::OnMsgSynthesizeKeyEvent(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  DCHECK(mptr->payload().has_key_event());

  INPUT input;
  ZeroMemory(&input, sizeof(input));
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = mptr->payload().key_event().keycode();
  if (mptr->payload().key_event().type() == ipc::proto::KeyEvent::UP)
    input.ki.dwFlags = KEYEVENTF_KEYUP;
  // KEYEVENTF_EXTENDEDKEY should be added, otherwise, it will affect the
  // subsequent key events in vista. (i.e. some keyup event will be translated
  // as keydown).
  if ((mptr->payload().key_event().modifiers() & ipc::kControlKeyMask) ||
      (mptr->payload().key_event().modifiers() & ipc::kShiftKeyMask)) {
    input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
  }
  SendInput(1, &input, sizeof(INPUT));
}

void FrontendComponent::UpdateComposition() {
  context_->UpdateComposition(composition_, caret_);
}

void FrontendComponent::OnMsgEnableFakeInlineComposition(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  DCHECK_GT(mptr->payload().boolean_size(), 0);
  enable_fake_inline_composition_ = mptr->payload().boolean(0);
  UpdateComposition();
  ReplyTrue(mptr.release());
}

void FrontendComponent::GetIntegerValue(std::string key, int32* int_value) {
  DCHECK(int_value);

  if (settings_integers_.count(key)) {
    *int_value = settings_integers_[key];
    return;
  }

  // Set to default value before getting it through settings client.
  for (int i = 0; i < arraysize(kSettingsIntegerDefaultValues); ++i) {
    if (key == kSettingsIntegerDefaultValues[i].key) {
      *int_value = kSettingsIntegerDefaultValues[i].default_value;
      break;
    }
  }

  // Load all interested settings at once.
  ipc::proto::VariableArray value_array;
  std::vector<std::string> key_list;
  for (int i = 0; i < arraysize(kSettingsIntegerDefaultValues); ++i) {
    key_list.push_back(kSettingsIntegerDefaultValues[i].key);
  }
  if (!settings_client_->GetValues(key_list, &value_array))
    return;

  DCHECK_EQ(arraysize(kSettingsIntegerDefaultValues),
            value_array.variable_size());
  // Extracts existing values to |settings_integers_|.
  for (int i = 0; i < value_array.variable_size(); ++i) {
    const ipc::proto::Variable& value = value_array.variable(i);
    if (value.type() == ipc::proto::Variable::NONE)
      continue;
    DCHECK(value.type() == ipc::proto::Variable::INTEGER &&
           value.has_integer());
    settings_integers_[key_list[i]] =
        static_cast<int32>(value.integer());
  }

  if (settings_integers_.count(key))
    *int_value = settings_integers_[key];
}

void FrontendComponent::OnIPCDisconnected() {
  // Clear context related states if ipc fails.
  candidates_.Clear();
  raw_composition_.Clear();
  composition_ = L"";
  caret_ = 0;
  composition_in_window_ = L"";
  caret_in_window_ = 0;
  composition_terminating_ = false;
  help_tips_ = L"";
  icid_ = 0;
  is_rtl_language_ = false;
  ime_loaded_ = false;
  ui_loaded_ = false;
  waiting_process_key_ = false;
  conversion_mode_ = IME_CMODE_NATIVE;
  while (!cached_messages_.empty()) {
    delete cached_messages_.front();
    cached_messages_.pop();
  }
  while (!cached_produced_messages_.empty()) {
    delete cached_produced_messages_.front();
    cached_produced_messages_.pop();
  }
}

void FrontendComponent::CacheMessage(ipc::proto::Message* message) {
  // If a message that is not MSG_INSERT_TEXT comes after MSG_INSERT_TEXT, then
  // all previous messages must be from other source like softkeyboard,
  // which must be handled during the call.
  if (!cached_messages_.empty() &&
      cached_messages_.back()->type() == ipc::MSG_INSERT_TEXT &&
      message->type() != ipc::MSG_INSERT_TEXT) {
    while (!cached_messages_.empty()) {
      DoHandle(cached_messages_.front());
      cached_messages_.pop();
    }
  }
  cached_messages_.push(message);
}

void FrontendComponent::SwitchInputMethod() {
  DCHECK_NE(current_input_method_.language_size(), 0);
  DWORD lang_id = 0;
  std::wstring new_layout = GetKeyboardLayoutForPack(
      Utf8ToWide(current_input_method_.string_id()), &lang_id);
  std::wstring old_layout = GetCurrentKeyboardLayoutName();
  if (new_layout.empty() || new_layout == old_layout) {
      // Already using right input keyboard layout.
      return;
  }
  if (!ShellUtils::SupportTSF()) {
    // Don't shelve frontend in tsf mode.
    FrontendFactory::ShelveFrontend(context_->GetId(), this);
    context_->DetachEngine();
    context_ = NULL;
  }
  // There are two ways to switch active input method of a context: using
  // language selection button on toolbar or using system language bar, and this
  // function is only called in the former case.
  // Please refer to
  // http://b/6429168 for detail information of this variable
  // |switching_input_method_by_toolbar_| and why it must be placed behind
  // |LoadKeyboardLayout|.
  switching_input_method_by_toolbar_ = true;
  SwitchToKeyboardLayout(new_layout, lang_id);
  DCHECK(context_);
}

bool FrontendComponent::UpdateInlineCompositionWindow(RECT* composition_rect) {
  RECT client_rect = {0};
  RECT caret_rect = {0};
  LOGFONT log_font = {0};

  if (!context_->GetClientRect(&client_rect) ||
      !context_->GetCaretRectForComposition(&caret_rect) ||
      !context_->GetCompositionFont(&log_font)) {
    DLOG(ERROR) << "Can't get information from context to build the"
                << "composition window layout.";
    return false;
  }

  // Compute layouts.
  std::vector<CompositionWindowList::CompositionWindowLayout> layouts;

  CompositionWindowLayouter layouter;
  layouter.Layout(
      client_rect, caret_rect, log_font, composition_, caret_, &layouts);
  // Update caret rect.
  if (!layouts.empty()) {
    CompositionWindowList::CompositionWindowLayout last_window_layout =
        layouts.back();
    *composition_rect = last_window_layout.window_position_in_screen_coordinate;
  } else {
    return false;
  }
  composition_window_list_->UpdateLayout(layouts);
  return true;
}

}  // namespace components
}  // namespace ime_goopy
