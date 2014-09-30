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

#include "ipc/hub_command_list_manager.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/hub_impl.h"
#include "ipc/message_util.h"

namespace {

// Messages can be consumed by this built-in component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_INPUT_CONTEXT_CREATED,
  ipc::MSG_INPUT_CONTEXT_DELETED,
  ipc::MSG_COMPONENT_DETACHED,

  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_UPDATE_COMMANDS,
  ipc::MSG_QUERY_COMMAND_LIST,
};

// Messages can be produced by this built-in component.
const uint32 kProduceMessages[] = {
  ipc::MSG_COMMAND_LIST_CHANGED,
};

const char kStringID[] = "com.google.ime.goopy.ipc.hub.command-list-manager";
const char kName[] = "Goopy IPC Hub Command List Manager";

}  // namespace

namespace ipc {
namespace hub {

HubCommandListManager::HubCommandListManager(HubImpl* hub)
    : self_(NULL),
      hub_(hub) {

  // Register this built-in component.
  hub->Attach(this);

  proto::ComponentInfo info;
  info.set_string_id(kStringID);
  info.set_name(kName);

  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info.add_consume_message(kConsumeMessages[i]);

  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info.add_produce_message(kProduceMessages[i]);

  self_ = hub->CreateComponent(this, info, true);
  DCHECK(self_);
}

HubCommandListManager::~HubCommandListManager() {
  hub_->Detach(this);
  // |self_| will be deleted automatically when detaching from Hub.
}

bool HubCommandListManager::Send(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Component* source = hub_->GetComponent(mptr->source());
  DCHECK(source);
  switch (message->type()) {
    case MSG_INPUT_CONTEXT_CREATED:
      return OnMsgInputContextCreated(mptr.release());
    case MSG_INPUT_CONTEXT_DELETED:
      return OnMsgInputContextDeleted(mptr.release());
    case MSG_COMPONENT_DETACHED:
      return OnMsgComponentDetached(mptr.release());
    case MSG_SET_COMMAND_LIST:
      return OnMsgSetCommandList(source, mptr.release());
    case MSG_UPDATE_COMMANDS:
      return OnMsgUpdateCommands(source, mptr.release());
    case MSG_QUERY_COMMAND_LIST:
      return OnMsgQueryCommandList(source, mptr.release());
    default:
      DLOG(ERROR) << "Unexpected message:" << GetMessageName(mptr->type());
      return false;
  }
}

bool HubCommandListManager::OnMsgInputContextCreated(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  DCHECK(message->has_payload() && message->payload().has_input_context_info());

  const uint32 icid = message->payload().input_context_info().id();
  InputContext* ic = hub_->GetInputContext(icid);
  DCHECK(ic);
  hub_->AttachToInputContext(self_, ic, InputContext::ACTIVE_STICKY, true);
  return true;
}

bool HubCommandListManager::OnMsgInputContextDeleted(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  DCHECK(message->has_payload() && message->payload().uint32_size());

  command_lists_.erase(message->payload().uint32(0));
  return true;
}

bool HubCommandListManager::OnMsgComponentDetached(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  DCHECK(message->has_payload() && message->payload().uint32_size() == 2);

  const uint32 icid = message->payload().uint32(0);
  const uint32 component = message->payload().uint32(1);
  DeleteCommandList(icid, component);
  return true;
}

bool HubCommandListManager::OnMsgSetCommandList(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  const uint32 icid = mptr->icid();
  const uint32 component_id = source->id();
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::COMPONENT_NOT_ATTACHED);
  }
  if (!mptr->has_payload() || !mptr->payload().command_list_size() ||
      !mptr->payload().command_list(0).command_size()) {
    DeleteCommandList(icid, component_id);
    return hub_->ReplyTrue(connector, mptr.release());
  }

  ComponentCommandListMap* map = &command_lists_[icid];
  proto::CommandList* list = &(*map)[component_id];
  list->Swap(mptr->mutable_payload()->mutable_command_list(0));
  SetCommandListOwner(component_id, list);
  BroadcastCommandListChanged(icid, component_id, *map);
  return hub_->ReplyTrue(connector, mptr.release());
}

bool HubCommandListManager::OnMsgUpdateCommands(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  const uint32 icid = mptr->icid();
  const uint32 component_id = source->id();
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::COMPONENT_NOT_ATTACHED);
  }
  if (!mptr->has_payload() || !mptr->payload().command_list_size() ||
      !mptr->payload().command_list(0).command_size()) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  CommandListMap::iterator ic_iter = command_lists_.find(icid);
  if (ic_iter == command_lists_.end())
    return hub_->ReplyFalse(connector, mptr.release());

  ComponentCommandListMap::iterator comp_iter =
      ic_iter->second.find(component_id);
  if (comp_iter == ic_iter->second.end())
    return hub_->ReplyFalse(connector, mptr.release());

  bool result = false;
  proto::CommandList* command_list =
      mptr->mutable_payload()->mutable_command_list(0);

  for (int i = 0; i < command_list->command_size(); ++i) {
    if (UpdateCommand(command_list->mutable_command(i), &comp_iter->second))
      result = true;
  }

  if (result)
    BroadcastCommandListChanged(icid, component_id, ic_iter->second);

  return hub_->ReplyBoolean(connector, mptr.release(), result);
}

bool HubCommandListManager::OnMsgQueryCommandList(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  if (mptr->reply_mode() != proto::Message::NEED_REPLY) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
  }

  const uint32 icid = mptr->icid();
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  ConvertToReplyMessage(mptr.get());
  proto::MessagePayload* payload = mptr->mutable_payload();
  payload->Clear();

  CommandListMap::iterator ic_iter = command_lists_.find(icid);
  if (ic_iter != command_lists_.end()) {
    for (ComponentCommandListMap::iterator comp_iter = ic_iter->second.begin();
         comp_iter != ic_iter->second.end(); ++comp_iter) {
      payload->add_command_list()->CopyFrom(comp_iter->second);
    }
  }
  connector->Send(mptr.release());
  return true;
}

void HubCommandListManager::DeleteCommandList(uint32 icid, uint32 component) {
  CommandListMap::iterator ic_iter = command_lists_.find(icid);
  if (ic_iter == command_lists_.end())
    return;

  ComponentCommandListMap::iterator comp_iter = ic_iter->second.find(component);
  if (comp_iter == ic_iter->second.end())
    return;

  comp_iter->second.clear_command();
  BroadcastCommandListChanged(icid, component, ic_iter->second);
  ic_iter->second.erase(comp_iter);
}

void HubCommandListManager::BroadcastCommandListChanged(
    uint32 icid,
    uint32 changed_component,
    const ComponentCommandListMap& command_lists) {
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic || !ic->MayConsume(MSG_COMMAND_LIST_CHANGED, false))
    return;

  proto::Message* message = new proto::Message();
  message->set_type(MSG_COMMAND_LIST_CHANGED);
  message->set_source(self_->id());
  message->set_target(kComponentBroadcast);
  message->set_icid(icid);

  proto::MessagePayload* payload = message->mutable_payload();
  for (ComponentCommandListMap::const_iterator i = command_lists.begin();
       i != command_lists.end(); ++i) {
    payload->add_command_list()->CopyFrom(i->second);
    payload->add_boolean(i->first == changed_component);
  }
  hub_->Dispatch(this, message);
}

// static
void HubCommandListManager::SetCommandListOwner(uint32 owner,
                                                proto::CommandList* commands) {
  commands->set_owner(owner);
  for (int i = 0; i < commands->command_size(); ++i) {
    proto::Command* command = commands->mutable_command(i);
    if (command->has_sub_commands())
      SetCommandListOwner(owner, command->mutable_sub_commands());
  }
}

// static
bool HubCommandListManager::UpdateCommand(proto::Command* new_command,
                                          proto::CommandList* commands) {
  for (int i = 0; i < commands->command_size(); ++i) {
    proto::Command* command = commands->mutable_command(i);

    if (command->id() != new_command->id())
      continue;

    command->Swap(new_command);

    // Updates the owner of sub command lists.
    if (command->has_sub_commands())
      SetCommandListOwner(commands->owner(), command->mutable_sub_commands());
    return true;
  }

  // Search recursively.
  for (int i = 0; i < commands->command_size(); ++i) {
    proto::Command* command = commands->mutable_command(i);
    if (!command->has_sub_commands())
      continue;

    if (UpdateCommand(new_command, command->mutable_sub_commands()))
      return true;
  }

  return false;
}

}  // namespace hub
}  // namespace ipc
