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

#include "ipc/hub_input_context_manager.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/hub_impl.h"
#include "ipc/message_util.h"

namespace {

// Messages can be consumed by this built-in component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_DELETE_INPUT_CONTEXT,
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACH_FROM_INPUT_CONTEXT,

  ipc::MSG_QUERY_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,

  ipc::MSG_ACTIVATE_COMPONENT,
  ipc::MSG_ASSIGN_ACTIVE_CONSUMER,
  ipc::MSG_RESIGN_ACTIVE_CONSUMER,
  ipc::MSG_QUERY_ACTIVE_CONSUMER,
  ipc::MSG_REQUEST_CONSUMER,

  ipc::MSG_UPDATE_INPUT_CARET,
  ipc::MSG_QUERY_INPUT_CARET,
};
const size_t kConsumeMessagesSize = arraysize(kConsumeMessages);

const char kStringID[] = "com.google.ime.goopy.ipc.hub.input-context-manager";
const char kName[] = "Goopy IPC Hub Input Context Manager";

}  // namespace

namespace ipc {
namespace hub {

HubInputContextManager::HubInputContextManager(HubImpl* hub)
    : self_(NULL),
      hub_(hub) {
  // Register this built-in component.
  hub->Attach(this);

  proto::ComponentInfo info;
  info.set_string_id(kStringID);
  info.set_name(kName);

  for(size_t i = 0; i < kConsumeMessagesSize; ++i)
    info.add_consume_message(kConsumeMessages[i]);

  self_ = hub->CreateComponent(this, info, true);
  DCHECK(self_);
}

HubInputContextManager::~HubInputContextManager() {
  hub_->Detach(this);
  // |self_| will be deleted automatically when detaching from Hub.
}

bool HubInputContextManager::Send(proto::Message* message) {
  Component* source = hub_->GetComponent(message->source());
  DCHECK(source);
  switch (message->type()) {
    case MSG_CREATE_INPUT_CONTEXT:
      return OnMsgCreateInputContext(source, message);
    case MSG_DELETE_INPUT_CONTEXT:
      return OnMsgDeleteInputContext(source, message);
    case MSG_ATTACH_TO_INPUT_CONTEXT:
      return OnMsgAttachToInputContext(source, message);
    case MSG_DETACH_FROM_INPUT_CONTEXT:
      return OnMsgDetachFromInputContext(source, message);
    case MSG_QUERY_INPUT_CONTEXT:
      return OnMsgQueryInputContext(source, message);
    case MSG_FOCUS_INPUT_CONTEXT:
      return OnMsgFocusInputContext(source, message);
    case MSG_BLUR_INPUT_CONTEXT:
      return OnMsgBlurInputContext(source, message);
    case MSG_ACTIVATE_COMPONENT:
      return OnMsgActivateComponent(source, message);
    case MSG_ASSIGN_ACTIVE_CONSUMER:
      return OnMsgAssignActiveConsumer(source, message);
    case MSG_RESIGN_ACTIVE_CONSUMER:
      return OnMsgResignActiveConsumer(source, message);
    case MSG_QUERY_ACTIVE_CONSUMER:
      return OnMsgQueryActiveConsumer(source, message);
    case MSG_REQUEST_CONSUMER:
      return OnMsgRequestConsumer(source, message);
    case MSG_UPDATE_INPUT_CARET:
      return OnMsgUpdateInputCaret(source, message);
    case MSG_QUERY_INPUT_CARET:
      return OnMsgQueryInputCaret(source, message);
    default:
      break;
  }
  DLOG(ERROR) << "Unexpected message:" << GetMessageName(message->type());
  delete message;
  return false;
}

bool HubInputContextManager::OnMsgCreateInputContext(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  // The message sender must wait for the reply message.
  if (!MessageNeedReply(message)) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
  }

  // TODO(suzhe): Support additional payload describing the content type of the
  // input context.

  InputContext* ic = hub_->CreateInputContext(source);
  DCHECK(ic);

  hub_->AttachToInputContext(self_, ic, InputContext::ACTIVE_STICKY, true);

  message->set_icid(ic->id());
  return hub_->ReplyTrue(connector, mptr.release());
}

bool HubInputContextManager::OnMsgDeleteInputContext(
    Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  if (hub_->DeleteInputContext(source, message->icid()))
    return hub_->ReplyTrue(connector, message);
  return hub_->ReplyError(
      connector, message, proto::Error::INVALID_INPUT_CONTEXT);
}

bool HubInputContextManager::OnMsgAttachToInputContext(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  InputContext::AttachState state = ic->GetComponentAttachState(source);

  // We use PASSIVE mode for newly attached components, but we don't want to
  // change the state if the component was already attached.
  if (state == InputContext::NOT_ATTACHED ||
      state == InputContext::PENDING_PASSIVE) {
    state = InputContext::PASSIVE;
  } else if (state == InputContext::PENDING_ACTIVE) {
    state = InputContext::ACTIVE;
  }

  if (hub_->AttachToInputContext(source, ic, state, true))
    return hub_->ReplyTrue(connector, mptr.release());
  return hub_->ReplyError(
      connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
}

bool HubInputContextManager::OnMsgDetachFromInputContext(
    Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (ic && ic->DetachComponent(source))
    return hub_->ReplyTrue(connector, message);
  return hub_->ReplyError(
      connector, message, proto::Error::INVALID_INPUT_CONTEXT);
}

bool HubInputContextManager::OnMsgQueryInputContext(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  // The message sender must wait for the reply message.
  if (!MessageNeedReply(message)) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  ConvertToReplyMessage(message);
  proto::MessagePayload* payload = message->mutable_payload();
  payload->Clear();
  ic->GetInfo(payload->mutable_input_context_info());
  connector->Send(mptr.release());
  return true;
}

bool HubInputContextManager::OnMsgFocusInputContext(
    Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (ic && ic->owner() == source && hub_->FocusInputContext(ic->id()))
    return hub_->ReplyTrue(connector, message);
  return hub_->ReplyError(
      connector, message, proto::Error::INVALID_INPUT_CONTEXT);
}

bool HubInputContextManager::OnMsgBlurInputContext(
    Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (ic && ic->owner() == source && hub_->BlurInputContext(ic->id()))
    return hub_->ReplyTrue(connector, message);
  return hub_->ReplyError(
      connector, message, proto::Error::INVALID_INPUT_CONTEXT);
}

bool HubInputContextManager::OnMsgActivateComponent(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  // If there is no playload, then we activate the source itself for the input
  // context.
  if (!message->has_payload()) {
    InputContext::AttachState state = hub_->RequestAttachToInputContext(
        source, ic, InputContext::ACTIVE, true);
    return hub_->ReplyBoolean(connector, mptr.release(),
                              state != InputContext::NOT_ATTACHED);
  }

  if (!message->payload().uint32_size() && !message->payload().string_size()) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  // If the payload contains one or more uint32 values, then they are ids of the
  // components that need activating. The source component itself won't be
  // activated.
  std::vector<bool> results;
  proto::MessagePayload* payload = message->mutable_payload();
  int size = payload->uint32_size();
  for (int i = 0; i < size; ++i) {
    Component* component = hub_->GetComponent(payload->uint32(i));
    if (!component) {
      results.push_back(false);
      continue;
    }

    InputContext::AttachState state = hub_->RequestAttachToInputContext(
        component, ic, InputContext::ACTIVE, component == source);
    if (state != InputContext::NOT_ATTACHED)
      results.push_back(true);
    else
      results.push_back(false);
  }

  size = payload->string_size();
  for (int i = 0; i < size; ++i) {
    Component* component = hub_->GetComponentByStringID(payload->string(i));
    if (!component) {
      results.push_back(false);
      continue;
    }

    InputContext::AttachState state = hub_->RequestAttachToInputContext(
        component, ic, InputContext::ACTIVE, component == source);
    if (state != InputContext::NOT_ATTACHED)
      results.push_back(true);
    else
      results.push_back(false);
  }

  if (MessageNeedReply(message)) {
    ConvertToReplyMessage(message);
    payload->Clear();
    size = results.size();
    for (int i = 0; i < size; ++i)
      payload->add_boolean(results[i]);

    connector->Send(mptr.release());
  }
  return true;
}

bool HubInputContextManager::OnMsgAssignActiveConsumer(
    Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  if (!message->has_payload() || !message->payload().uint32_size()) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_PAYLOAD);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, message, proto::Error::COMPONENT_NOT_ATTACHED);
  }

  const proto::MessagePayload& payload = message->payload();
  ic->AssignActiveConsumer(
      source, payload.uint32().data(), payload.uint32_size());
  return hub_->ReplyTrue(connector, message);
}

bool HubInputContextManager::OnMsgResignActiveConsumer(
    Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  if (!message->has_payload() || !message->payload().uint32_size()) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_PAYLOAD);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, message, proto::Error::COMPONENT_NOT_ATTACHED);
  }

  const proto::MessagePayload& payload = message->payload();
  ic->ResignActiveConsumer(
      source, payload.uint32().data(), payload.uint32_size());
  return hub_->ReplyTrue(connector, message);
}

bool HubInputContextManager::OnMsgQueryActiveConsumer(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  // The message sender must wait for the reply message.
  if (!MessageNeedReply(message)) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
  }
  if (!message->has_payload() || !message->payload().uint32_size()) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  std::vector<uint32> consumers;
  proto::MessagePayload* payload = message->mutable_payload();
  int size = payload->uint32_size();
  for (int i = 0; i < size; ++i) {
    Component* consumer = ic->GetActiveConsumer(payload->uint32(i));
    consumers.push_back(consumer ? consumer->id() : kComponentBroadcast);
  }

  ConvertToReplyMessage(message);
  payload->Clear();
  for (int i = 0; i < size; ++i)
    payload->add_uint32(consumers[i]);

  connector->Send(mptr.release());
  return true;
}

bool HubInputContextManager::OnMsgRequestConsumer(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic || ic->id() == kInputContextNone) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!message->has_payload() || !message->payload().uint32_size()) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  InputContext::AttachState state = ic->GetComponentAttachState(source);
  if (state == InputContext::NOT_ATTACHED) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::COMPONENT_NOT_ATTACHED);
  }

  std::vector<uint32> already_have_consumers;
  const proto::MessagePayload& payload = message->payload();
  ic->SetMessagesNeedConsumer(source,
                              payload.uint32().data(),
                              payload.uint32_size(),
                              &already_have_consumers);
  if (MessageNeedReply(message)) {
    ConvertToReplyMessage(message);
    proto::MessagePayload* mutable_payload = message->mutable_payload();
    mutable_payload->Clear();
    mutable_payload->add_boolean(true);
    for (size_t i = 0; i < already_have_consumers.size(); ++i)
      mutable_payload->add_uint32(already_have_consumers[i]);
    connector->Send(mptr.release());
  }
  return true;
}

bool HubInputContextManager::OnMsgUpdateInputCaret(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  // TODO(suzhe)
  return false;
}

bool HubInputContextManager::OnMsgQueryInputCaret(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  // TODO(suzhe)
  return false;
}

}  // namespace hub
}  // namespace ipc
