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

#include "components/input_method_switch/input_method_switch_component.h"

#include "base/logging.h"
#include "base/resource_bundle.h"
#include "common/string_utils.h"
#include "components/common/constants.h"
#include "components/common/file_utils.h"
#include "components/common/hotkey_util.h"
#include "components/common/virtual_keyboard_and_character_picker_consts.h"
#include "components/input_method_switch/input_method_switch.grh"
#include "ipc/constants.h"
#include "ipc/keyboard_codes.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"
#include "ipc/protos/ipc.pb.h"

namespace {

// The message types input method switch component can produce.
const uint32 kProduceMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_ADD_HOTKEY_LIST,
  ipc::MSG_ACTIVATE_HOTKEY_LIST,
  ipc::MSG_DO_COMMAND,
  ipc::MSG_QUERY_ACTIVE_INPUT_METHOD,
  ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,
  ipc::MSG_SWITCH_TO_INPUT_METHOD,
  ipc::MSG_REQUEST_CONSUMER,
  ipc::MSG_LIST_INPUT_METHODS,
};

// The message types input method switch component can consume.
const uint32 kConsumeMessages[] = {
  ipc::MSG_INPUT_CONTEXT_CREATED,
  ipc::MSG_DO_COMMAND,
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_INPUT_METHOD_ACTIVATED,
};

static const char kComponentStringId[] =
    "com.google.input_tools.input_method_switch";
static const char kDirectInputModeCommandStringId[] =
    "toggle_direct_input_mode_command";
static const char kDirectInputModeCommandHotKey[] = "\tCtrl+G";
static const char kPreviousImeCommandStringId[] = "previous_ime_command";
static const char kPreviousImeCommandHotKey[] = "\tCtrl+J";
static const char kSwitchToCommandStringId[] = "switch_to_command";

static const uint32 kHotkeyListId = 1;
static const char kSeperator[] = "seperator";
static const char kResourcePackPathPattern[] =
    "/input_method_switch_[LANG].pak";
const char kVirtualKeyboardComponentPrefix[] =
    "com.google.input_tools.virtual_keyboard";

inline std::string GetLocalizedString(int id) {
  DCHECK(ime_goopy::ResourceBundle::HasSharedInstance());
  return ime_goopy::WideToUtf8(
      ime_goopy::ResourceBundle::GetSharedInstance().GetLocalizedString(id));
}

// Compare function for ComponentInfo.
static bool ComponentInfoCmp(const ipc::proto::ComponentInfo& left,
                             const ipc::proto::ComponentInfo& right) {
  return left.string_id() < right.string_id();
}
}  // namespace

namespace ime_goopy {
namespace components {

using ipc::proto::Message;

class InputMethodSwitchComponent::Impl {
 public:
  explicit Impl(InputMethodSwitchComponent* owner);
  virtual ~Impl();

  void OnMsgDoCommand(Message* message);
  void OnMsgInputMethodActivated(Message* message);
  void OnMsgInputContextCreated(Message* message);
  void AddHotkeyList();

  void SetInputMethodList(uint32 icid, ipc::proto::CommandList* command_list);
  void SetCommandList(uint32 icid);
  void ActivateHotkeyList(uint32 icid);

 private:
  void HandleToggleDirectInputModeCommand(uint32 icid);
  void HandlePreviousImeCommand(uint32 icid);
  void HandleSelectInputMethod(uint32 icid,
                               const std::string& selected_input_method_id);

  InputMethodSwitchComponent* owner_;
  struct InputMethodUsage {
    std::string previous_input_method;
    std::string current_input_method;
  };
  typedef std::map<uint32 /* icid*/, InputMethodUsage> InputMethodUsageMap;
  InputMethodUsageMap input_method_usage_;

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

InputMethodSwitchComponent::Impl::Impl(InputMethodSwitchComponent* owner)
    : owner_(owner) {
}

InputMethodSwitchComponent::Impl::~Impl() {
}

void InputMethodSwitchComponent::Impl::OnMsgDoCommand(Message* message) {
  scoped_ptr<Message> mptr(message);
  DCHECK_EQ(mptr->payload().string_size(), 1);
  DCHECK(!mptr->payload().string(0).empty());
  uint32 icid = mptr->icid();
  const std::string& command_string_id = mptr->payload().string(0);
  if (command_string_id == kDirectInputModeCommandStringId) {
    HandleToggleDirectInputModeCommand(icid);
  } else if (command_string_id == kPreviousImeCommandStringId) {
    HandlePreviousImeCommand(icid);
  } else {
    HandleSelectInputMethod(icid, command_string_id);
  }
}

void InputMethodSwitchComponent::Impl::OnMsgInputMethodActivated(
    Message* message) {
  uint32 icid = message->icid();
  if (message->has_payload() && message->payload().component_info_size() == 1) {
    SetCommandList(icid);
    std::string input_method = message->payload().component_info(0).string_id();
    if (input_method.find(kVirtualKeyboardNamePrefix) == -1) {
      InputMethodUsageMap::iterator it = input_method_usage_.find(icid);
      if (it != input_method_usage_.end()) {
        input_method_usage_[icid].previous_input_method =
            input_method_usage_[icid].current_input_method;
      }
      input_method_usage_[icid].current_input_method = input_method;
    }
  }
  owner_->ReplyTrue(message);
}

void InputMethodSwitchComponent::Impl::OnMsgInputContextCreated(
    Message* message) {
  if (message->has_payload() && message->payload().has_input_context_info()) {
    scoped_ptr<Message> mptr(
        owner_->NewMessage(ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
                           message->payload().input_context_info().id(),
                           true));
    owner_->Send(mptr.release(), NULL);
  }
  owner_->ReplyTrue(message);
}

void InputMethodSwitchComponent::Impl::AddHotkeyList() {
  scoped_ptr<Message> mptr(owner_->NewMessage(ipc::MSG_ADD_HOTKEY_LIST,
                                              ipc::kInputContextNone, false));
  ipc::proto::HotkeyList* hotkey_list =
      mptr->mutable_payload()->add_hotkey_list();

  hotkey_list->set_id(kHotkeyListId);
  // Ctrl+G for switching between keyboard input mode and current language input
  // mode.
  HotkeyUtil::AddHotKey(ipc::VKEY_G,
                        ipc::kControlKeyMask,
                        kDirectInputModeCommandStringId,
                        owner_->id(),
                        hotkey_list);
  // F12 works the same with Ctrl+G.
  HotkeyUtil::AddHotKey(ipc::VKEY_F12, 0, kDirectInputModeCommandStringId,
                        owner_->id(), hotkey_list);
  // Ctrl+J for switching to previous input method.
  HotkeyUtil::AddHotKey(ipc::VKEY_J, ipc::kControlKeyMask,
                        kPreviousImeCommandStringId, owner_->id(), hotkey_list);
  owner_->Send(mptr.release(), NULL);
}

void InputMethodSwitchComponent::Impl::SetInputMethodList(
    uint32 icid, ipc::proto::CommandList* command_list) {
  ipc::proto::Command* switch_to_command = command_list->add_command();
  switch_to_command->set_id(kSwitchToCommandStringId);
  switch_to_command->mutable_title()->set_text(GetLocalizedString(
      IDS_SWITCHTO_COMMAND_TITLE));
  ipc::proto::CommandList* sub_commands =
      switch_to_command->mutable_sub_commands();

  ipc::proto::Command* sub_command = sub_commands->add_command();
  sub_command->set_id(kDirectInputModeCommandStringId);
  sub_command->mutable_title()->set_text(
      GetLocalizedString(IDS_TOGGLE_DIRECT_INPUT_MODE) +
      kDirectInputModeCommandHotKey);

  sub_command = sub_commands->add_command();
  sub_command->set_id(kPreviousImeCommandStringId);
  sub_command->mutable_title()->set_text(
      GetLocalizedString(IDS_SWITCH_TO_PREVIOUS_IME) +
      kPreviousImeCommandHotKey);

  sub_command = sub_commands->add_command();
  sub_command->set_id(kSeperator);
  sub_command->set_state(ipc::proto::Command::SEPARATOR);

  ipc::proto::ComponentInfo active_input_method;
  Message* reply = NULL;
  // Lists active input method.
  scoped_ptr<Message> mptr(
      owner_->NewMessage(ipc::MSG_QUERY_ACTIVE_INPUT_METHOD, icid, true));
  if (!owner_->SendWithReplyNonRecursive(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    if (reply->has_payload() && reply->payload().component_info_size() == 1)
      active_input_method.CopyFrom(reply->payload().component_info(0));
    delete reply;
  }
  // Lists input methods.
  mptr.reset(owner_->NewMessage(ipc::MSG_LIST_INPUT_METHODS, icid, true));
  if (!owner_->SendWithReplyNonRecursive(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReplyNonRecursive failed";
  } else {
    if (reply->has_payload() && reply->payload().component_info_size()) {
      ComponentInfos input_method_list = reply->payload().component_info();

      // Since the size of input_method_list is small, bubble sort is enough.
      for (int i = 0; i < input_method_list.size() - 1; ++i) {
        for (int j = i + 1; j < input_method_list.size(); ++j) {
          if (!ComponentInfoCmp(input_method_list.Get(i),
                input_method_list.Get(j)))
            input_method_list.SwapElements(i, j);
        }
      }
      for (int i = 0; i < input_method_list.size(); ++i) {
        // TODO(synch): find a technique to allow ime components to prevent
        // themselves from appearing in the language list.
        if (input_method_list.Get(i).string_id().find(
            kVirtualKeyboardComponentPrefix) == 0) {
          continue;
        }
        sub_command = sub_commands->add_command();
        sub_command->set_id(input_method_list.Get(i).string_id());
        sub_command->mutable_title()->set_text(input_method_list.Get(i).name());
        if (active_input_method.language_size() &&
            (input_method_list.Get(i).language(0) ==
                active_input_method.language(0))) {
          sub_command->set_state(ipc::proto::Command::CHECKED);
        }
      }
    }
    delete reply;
  }
}

void InputMethodSwitchComponent::Impl::SetCommandList(uint32 icid) {
  if (icid != ipc::kInputContextNone) {
    ipc::proto::CommandList command_list;
    SetInputMethodList(icid, &command_list);
    scoped_ptr<Message> mptr(
        owner_->NewMessage(ipc::MSG_SET_COMMAND_LIST, icid, false));
    mptr->mutable_payload()->add_command_list()->CopyFrom(command_list);
    owner_->Send(mptr.release(), NULL);
  }
}

void InputMethodSwitchComponent::Impl::ActivateHotkeyList(uint32 icid) {
  scoped_ptr<Message> mptr(
      owner_->NewMessage(ipc::MSG_ACTIVATE_HOTKEY_LIST, icid, false));
  mptr->mutable_payload()->add_uint32(kHotkeyListId);
  owner_->Send(mptr.release(), NULL);
}

void InputMethodSwitchComponent::Impl::HandleToggleDirectInputModeCommand(
    uint32 icid) {
  scoped_ptr<Message> new_message(
      owner_->NewMessage(ipc::MSG_QUERY_ACTIVE_INPUT_METHOD, icid, true));
  Message* reply = NULL;
  if (!owner_->SendWithReplyNonRecursive(new_message.release(), -1, &reply)) {
    DLOG(ERROR) << "SendWithReply failed";
    return;
  }
  DCHECK(reply);
  if (reply->has_payload() &&
      reply->payload().component_info_size() &&
      reply->payload().component_info(0).language_size()) {
    const std::string& component_string_id =
        reply->payload().component_info(0).string_id();
    // Switches to previous ime if current active ime is keyboard input.
    if (component_string_id == kKeyboardInputComponentStringId) {
      scoped_ptr<Message> switch_message(owner_->NewMessage(
            ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD, icid, false));
      owner_->Send(switch_message.release(), NULL);
    } else {
      scoped_ptr<Message> switch_message(owner_->NewMessage(
          ipc::MSG_SWITCH_TO_INPUT_METHOD, icid, false));
      switch_message->mutable_payload()->add_string(
          kKeyboardInputComponentStringId);
      owner_->Send(switch_message.release(), NULL);
    }
  }
  delete reply;
}

void InputMethodSwitchComponent::Impl::HandlePreviousImeCommand(uint32 icid) {
  InputMethodUsageMap::iterator it = input_method_usage_.find(icid);
  if (it != input_method_usage_.end() &&
      !it->second.previous_input_method.empty()) {
    scoped_ptr<Message> switch_message(owner_->NewMessage(
        ipc::MSG_SWITCH_TO_INPUT_METHOD, icid, false));
    switch_message->mutable_payload()->add_string(
        it->second.previous_input_method);
    owner_->Send(switch_message.release(), NULL);
  }
}

void InputMethodSwitchComponent::Impl::HandleSelectInputMethod(
    uint32 icid, const std::string& selected_input_method_id) {
  scoped_ptr<ipc::proto::Message> mptr(
      owner_->NewMessage(ipc::MSG_SWITCH_TO_INPUT_METHOD, icid, false));
  mptr->mutable_payload()->add_string(selected_input_method_id);
  owner_->Send(mptr.release(), NULL);
}

InputMethodSwitchComponent::InputMethodSwitchComponent() {
  std::string path_prefix =
      FileUtils::GetDataPathForComponent(kComponentStringId) +
      kResourcePackPathPattern;
  if (!ResourceBundle::HasSharedInstance())
    ResourceBundle::InitSharedInstanceWithSystemLocale();
  ResourceBundle::GetSharedInstance().AddDataPackToSharedInstance(path_prefix);

  impl_.reset(new Impl(this));
}

InputMethodSwitchComponent::~InputMethodSwitchComponent() {
}

void InputMethodSwitchComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(kComponentStringId);
  for (int i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);

  for (int i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

void InputMethodSwitchComponent::Handle(Message* message) {
  DCHECK(message);
  scoped_ptr<Message> mptr(message);

  switch (mptr->type()) {
    case ipc::MSG_INPUT_CONTEXT_CREATED:
      impl_->OnMsgInputContextCreated(mptr.release());
      break;
    case ipc::MSG_ATTACH_TO_INPUT_CONTEXT:
      OnMsgAttachToInputContext(mptr.release());
      break;
    case ipc::MSG_DO_COMMAND:
      impl_->OnMsgDoCommand(mptr.release());
      break;
    case ipc::MSG_INPUT_METHOD_ACTIVATED:
      impl_->OnMsgInputMethodActivated(mptr.release());
      break;
    default: {
      DLOG(ERROR) << "Unexpected message received:"
                  << "type = " << mptr->type()
                  << "icid = " << mptr->icid();
      ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE,
                 "unknown type");
      break;
    }
  }
}

void InputMethodSwitchComponent::OnRegistered() {
  impl_->AddHotkeyList();
}

void InputMethodSwitchComponent::OnMsgAttachToInputContext(Message* message) {
  uint32 icid = message->icid();
  ReplyTrue(message);

  scoped_ptr<Message> mptr(
      NewMessage(ipc::MSG_REQUEST_CONSUMER, icid, true));
  for (int i = 0; i < arraysize(kProduceMessages); ++i)
    mptr->mutable_payload()->add_uint32(kProduceMessages[i]);
  Message* reply = NULL;
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << "SendWithReply failed with type = MSG_REQUEST_CONSUMER "
                << "icid = " << icid;
    return;
  }
  DCHECK(reply);
  DCHECK(!ipc::MessageIsErrorReply(reply));
  delete reply;

  impl_->SetCommandList(icid);
  impl_->ActivateHotkeyList(icid);
}
}  // namespace components
}  // namespace ime_goopy
