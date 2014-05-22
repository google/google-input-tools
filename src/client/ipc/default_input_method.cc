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

#include "ipc/default_input_method.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"

namespace {

// Messages can be consumed by this built-in component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_PROCESS_KEY_EVENT,
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
};
const size_t kConsumeMessagesSize = arraysize(kConsumeMessages);

const uint32 kProduceMessages[] = {
  ipc::MSG_INSERT_TEXT,
};
const size_t kProduceMessagesSize = arraysize(kProduceMessages);

const char kName[] = "Default Input Method";

}  // namespace

namespace ipc {

const char kDefaultInputMethodStringID[] =
    "com.google.imp.hub.default-input-method";

DefaultInputMethod::DefaultInputMethod(Hub* hub)
    : hub_(hub),
      id_(kComponentDefault) {
  // Register this built-in component.
  hub->Attach(this);

  proto::Message* message = new proto::Message();
  message->set_type(MSG_REGISTER_COMPONENT);
  message->set_reply_mode(proto::Message::NEED_REPLY);

  proto::ComponentInfo* info =
      message->mutable_payload()->add_component_info();

  info->set_string_id(kDefaultInputMethodStringID);
  info->set_name(kName);

  for(size_t i = 0; i < kConsumeMessagesSize; ++i)
    info->add_consume_message(kConsumeMessages[i]);

  for(size_t i = 0; i < kProduceMessagesSize; ++i)
    info->add_produce_message(kProduceMessages[i]);

  bool result = hub_->Dispatch(this, message);
  DCHECK(result);
}

DefaultInputMethod::~DefaultInputMethod() {
  hub_->Detach(this);
  // |self_| will be deleted automatically when detaching from Hub.
}

bool DefaultInputMethod::Send(proto::Message* message) {
  switch (message->type()) {
    case MSG_ATTACH_TO_INPUT_CONTEXT:
      ConvertToBooleanReplyMessage(message, true);
      hub_->Dispatch(this, message);
      return true;
    case MSG_REGISTER_COMPONENT:
      if (message->reply_mode() == proto::Message::IS_REPLY)
        id_ = message->payload().component_info(0).id();
      break;
    case MSG_PROCESS_KEY_EVENT:
      OnMsgProcessKeyEvent(message);
      return true;
    default:
      break;
  }
  delete message;
  return true;
}

void DefaultInputMethod::OnMsgProcessKeyEvent(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);

  // Simply returns false to the application.
  if (message->reply_mode() == proto::Message::NEED_REPLY) {
    ConvertToBooleanReplyMessage(message, false);
    hub_->Dispatch(this, mptr.release());
  }
}

}  // namespace ipc
