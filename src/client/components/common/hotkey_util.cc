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

#include "components/common/hotkey_util.h"

#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
namespace components {

using ipc::proto::Message;

void HotkeyUtil::AddHotKey(ipc::KeyboardCode keycode, uint32 modifiers,
                           const char* command_string_id, uint32 component_id,
                           ipc::proto::HotkeyList* hotkey_list) {
  ipc::proto::Hotkey* hotkey = hotkey_list->add_hotkey();
  ipc::proto::KeyEvent* key_event = hotkey->add_key_event();
  Message* message = hotkey->add_message();
  key_event->set_keycode(keycode);
  key_event->set_type(ipc::proto::KeyEvent::DOWN);
  key_event->set_modifiers(modifiers);
  message->set_type(ipc::MSG_DO_COMMAND);
  message->set_reply_mode(Message::NO_REPLY);
  message->set_icid(ipc::kInputContextFocused);
  message->set_source(component_id);
  message->set_target(component_id);
  message->mutable_payload()->add_string(command_string_id);
}

}  // namespace components
}  // namespace ime_goopy
