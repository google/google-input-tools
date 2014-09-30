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

#include "ipc/hub_impl.h"

#include <google/protobuf/text_format.h>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/hub_command_list_manager.h"
#include "ipc/hub_composition_manager.h"
#include "ipc/hub_hotkey_manager.h"
#include "ipc/hub_input_context_manager.h"
#include "ipc/hub_input_method_manager.h"
#include "ipc/message_util.h"

namespace ipc {
namespace hub {

// Messages that may be produced by the Hub itself.
const uint32 kHubProduceMessages[] = {
  MSG_COMPONENT_CREATED,
  MSG_COMPONENT_DELETED,
  MSG_INPUT_CONTEXT_CREATED,
  MSG_INPUT_CONTEXT_DELETED,
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMPONENT_ACTIVATED,
  MSG_COMPONENT_DEACTIVATED,
  MSG_ACTIVE_CONSUMER_CHANGED,
};
const size_t kHubProduceMessagesSize = arraysize(kHubProduceMessages);

// Messages that can be consumed by the Hub.
const uint32 kHubConsumeMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_QUERY_COMPONENT,
};
const size_t kHubConsumeMessagesSize = arraysize(kHubConsumeMessages);

// Unique string id of the Hub component.
const char kHubStringID[] = "com.google.ime.goopy.ipc.hub";

// Human readable name of the Hub component.
const char kHubName[] = "Goopy IPC Hub";

HubImpl::HubImpl()
    : component_counter_(kComponentDefault),
      input_context_counter_(kInputContextNone),
      focused_input_context_(kInputContextNone),
      hub_component_(NULL),
      hub_input_context_(NULL) {
  // Registers the special component representing Hub itself.
  Attach(this);

  proto::ComponentInfo info;
  info.set_string_id(kHubStringID);
  info.set_name(kHubName);

  for(size_t i = 0; i < kHubProduceMessagesSize; ++i)
    info.add_produce_message(kHubProduceMessages[i]);

  for(size_t i = 0; i < kHubConsumeMessagesSize; ++i)
    info.add_consume_message(kHubConsumeMessages[i]);

  // Creates the special component representing Hub itself.
  hub_component_ = CreateComponent(this, info, true);
  DCHECK(hub_component_);

  // Creates the default input context, which is owned by the special
  // |hub_component_|. All components will be attached to it by default.
  hub_input_context_ = CreateInputContext(hub_component_);
  DCHECK(hub_input_context_);

  // Creates built-in components. They must be created after creating
  // |hub_component_| and |hub_input_context_|.

  input_context_manager_.reset(new HubInputContextManager(this));
  input_method_manager_.reset(new HubInputMethodManager(this));
  hotkey_manager_.reset(new HubHotkeyManager(this));
  command_list_manager_.reset(new HubCommandListManager(this));
  composition_manager_.reset(new HubCompositionManager(this));
}

HubImpl::~HubImpl() {
  // Detaching other connectors for external components. They won't be deleted
  // here as they are not owned by us.
  ConnectorMap::iterator i = connectors_.begin();
  while (i != connectors_.end()) {
    Detach(i->first);
    i = connectors_.begin();
  }
}

void HubImpl::Attach(Connector* connector) {
  if (connector && !connectors_.count(connector)) {
    connectors_.insert(std::make_pair(connector, std::set<uint32>()));
    connector->Attached();
  }
}

void HubImpl::Detach(Connector* connector) {
  if (!connector || !connectors_.count(connector))
    return;

  std::set<uint32> components;
  components.swap(connectors_[connector]);

  // Remove the connector from |connectors_| map first to make sure we won't
  // send any new message to the connector.
  connectors_.erase(connector);

  // Deletes all components owned by this connector.
  std::set<uint32>::const_iterator i = components.begin();
  std::set<uint32>::const_iterator end = components.end();
  for (; i != end; ++i)
    DeleteComponent(connector, *i);

  connector->Detached();
}

bool HubImpl::Dispatch(Connector* connector, proto::Message* message) {
  DCHECK(connector);
  DCHECK(connectors_.count(connector));
  DCHECK(message);
  if (!message)
    return false;

  // Make sure the |message| will be deleted.
  scoped_ptr<proto::Message> mptr(message);

  // The connector must be attached already.
  if (connector != this && !connectors_.count(connector))
    return false;

  uint32 source_id = message->source();

  // Set explicit target input context id.
  if (message->icid() == kInputContextFocused)
    message->set_icid(focused_input_context_);

  // Source component can't be kComponentBroadcast.
  if (source_id == kComponentBroadcast)
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_SOURCE);

  uint32 type = message->type();
  proto::Message::ReplyMode reply_mode = message->reply_mode();
  uint32 target_id = message->target();

  // We need to handle MSG_REGISTER_COMPONENT and MSG_DEREGISTER_COMPONENT
  // specially, as they may not be sent from a valid component.
  if (type == MSG_REGISTER_COMPONENT) {
    if (target_id != kComponentDefault) {
      return ReplyError(
          connector, mptr.release(), proto::Error::INVALID_TARGET);
    }
    if (reply_mode != proto::Message::NEED_REPLY) {
      return ReplyError(
          connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
    }
    return RegisterComponents(connector, mptr.release());
  }
  if (type == MSG_DEREGISTER_COMPONENT) {
    if (target_id != kComponentDefault) {
      return ReplyError(
          connector, mptr.release(), proto::Error::INVALID_TARGET);
    }
    if (reply_mode == proto::Message::IS_REPLY) {
      return ReplyError(
          connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
    }
    return DeregisterComponents(connector, mptr.release());
  }

  Component* source = GetComponent(source_id);
  // The source component must belong to the connector.
  if (!source || source->connector() != connector)
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_SOURCE);

  // Validate if the source component has right to produce this message.
  if (!CanComponentProduce(source, message)) {
    return ReplyError(
        connector, mptr.release(), proto::Error::SOURCE_CAN_NOT_PRODUCE);
  }

  // Handle broadcast messages.
  if (target_id == kComponentBroadcast) {
    // A broadcast message may not require a reply message.
    if (reply_mode != proto::Message::NO_REPLY) {
      return ReplyError(
          connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
    }
    return BroadcastMessage(mptr.release());
  }

  // Dispatch a none-broadcast message.
  Component* target = GetComponent(target_id);
  if (!target)
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_TARGET);

  Connector* target_connector = target->connector();

  // The target_connector has been detached.
  if (target_connector != this && !connectors_.count(target_connector))
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_TARGET);

  // Hub can consume any messages.
  if (target_connector != this && !CanComponentConsume(target, message)) {
    return ReplyError(
        connector, mptr.release(), proto::Error::TARGET_CAN_NOT_CONSUME);
  }

  bool result;
  bool has_serial = message->has_serial();
  uint32 serial = message->serial();
  uint32 icid = message->icid();

  result = target_connector->Send(mptr.release());

  if (!result) {
    if (reply_mode != proto::Message::NEED_REPLY)
      return false;

    // The message has been deleted by the target, so we need to create a new
    // one for error reply.
    message = NewMessage(type, target_id, icid);
    message->set_reply_mode(reply_mode);
    message->set_source(source_id);

    // We need to retain original message's serial number.
    if (has_serial)
      message->set_serial(serial);
    return ReplyError(connector, message, proto::Error::SEND_FAILURE);
  }

  return true;
}

// Implementation of Hub::Connector
bool HubImpl::Send(proto::Message* message) {
  Component* source = GetComponent(message->source());

  switch(message->type()) {
    case MSG_ATTACH_TO_INPUT_CONTEXT:
      if (message->reply_mode() == proto::Message::IS_REPLY)
        return OnMsgAttachToInputContextReply(source, message);
      break;
    case MSG_QUERY_COMPONENT:
      return OnMsgQueryComponent(source, message);
    default:
      break;
  }

  return DispatchToActiveConsumer(source, message);
}

// Implementation of InputContext::Delegate
void HubImpl::OnComponentActivated(InputContext* input_context,
                                   Component* component,
                                   const MessageTypeVector& messages) {
  // Don't bother sending the message if the component can't consume it.
  if (component->CanConsume(MSG_COMPONENT_ACTIVATED)) {
    proto::Message* message = NewMessage(
        MSG_COMPONENT_ACTIVATED, component->id(), input_context->id());

    proto::MessagePayload* payload = message->mutable_payload();
    MessageTypeVector::const_iterator i = messages.begin();
    MessageTypeVector::const_iterator end = messages.end();
    for (; i != end; ++i)
      payload->add_uint32(*i);

    Dispatch(this, message);
  }
}

void HubImpl::OnComponentDeactivated(InputContext* input_context,
                                     Component* component,
                                     const MessageTypeVector& messages) {
  // Don't bother sending the message if the component can't consume it.
  if (component->CanConsume(MSG_COMPONENT_DEACTIVATED)) {
    proto::Message* message = NewMessage(
        MSG_COMPONENT_DEACTIVATED, component->id(), input_context->id());

    proto::MessagePayload* payload = message->mutable_payload();
    MessageTypeVector::const_iterator i = messages.begin();
    MessageTypeVector::const_iterator end = messages.end();
    for (; i != end; ++i)
      payload->add_uint32(*i);

    Dispatch(this, message);
  }
}

void HubImpl::OnComponentDetached(InputContext* input_context,
                                  Component* component,
                                  InputContext::AttachState state) {
  DCHECK(component);
  const uint32 icid = input_context->id();
  component->attached_input_contexts().erase(icid);

  if (!InputContext::IsAttachedState(state))
    return;

  // Nothing needs to be done if the hub itself got detached.
  if (component == hub_component_)
    return;

  // Sends MSG_DETACHED_FROM_INPUT_CONTEXT to the component when necessary.
  if (icid != kInputContextNone &&
      component->CanConsume(MSG_DETACHED_FROM_INPUT_CONTEXT)) {
    Dispatch(this, NewMessage(
        MSG_DETACHED_FROM_INPUT_CONTEXT, component->id(), icid));
  }

  // Broadcasts MSG_COMPONENT_DETACHED when necessary, if the |input_context|
  // is not being destroyed.
  if (input_context->owner() && hub_input_context_ &&
      hub_input_context_->MayConsume(MSG_COMPONENT_DETACHED, false)) {
    proto::Message* message = NewMessage(
        MSG_COMPONENT_DETACHED, kComponentBroadcast, kInputContextNone);
    message->mutable_payload()->add_uint32(icid);
    message->mutable_payload()->add_uint32(component->id());
    BroadcastMessage(message);
  }
}

void HubImpl::OnActiveConsumerChanged(InputContext* input_context,
                                      const MessageTypeVector& messages) {
  // Don't bother broadcasting the message if no component want it.
  if (!input_context->MayConsume(MSG_ACTIVE_CONSUMER_CHANGED, false))
    return;

  proto::Message* message = NewMessage(
      MSG_ACTIVE_CONSUMER_CHANGED, kComponentBroadcast, input_context->id());

  proto::MessagePayload* payload = message->mutable_payload();
  MessageTypeVector::const_iterator i = messages.begin();
  MessageTypeVector::const_iterator end = messages.end();
  for (; i != end; ++i) {
    payload->add_uint32(*i);
    payload->add_boolean(input_context->HasActiveConsumer(*i));
  }

  BroadcastMessage(message);
}

void HubImpl::MaybeDetachComponent(InputContext* input_context,
                                   Component* component) {
  // Components are always attached to the default input context.
  if (input_context != hub_input_context_)
    input_context->DetachComponent(component);
}

void HubImpl::RequestConsumer(InputContext* input_context,
                              const MessageTypeVector& messages,
                              Component* exclude) {
  if (!hub_input_context_ || input_context == hub_input_context_)
    return;

  MessageTypeVector::const_iterator iter = messages.begin();
  MessageTypeVector::const_iterator end = messages.end();
  for (; iter != end; ++iter) {
    uint32 message = *iter;
    if (input_context->MayConsume(message, true))
      continue;

    std::vector<Component*> consumers;
    if (!hub_input_context_->GetAllConsumers(*iter, false, &consumers))
      continue;

    for (size_t i = 0; i < consumers.size(); ++i) {
      Component* consumer = consumers[i];
      if (consumer == exclude || !IsComponentValid(consumer))
        continue;

      if (input_context->GetComponentAttachState(consumer) !=
          InputContext::NOT_ATTACHED)
        continue;

      InputContext::AttachState state = RequestAttachToInputContext(
          consumer, input_context, InputContext::PASSIVE, false);

      if (state != InputContext::NOT_ATTACHED)
        break;
    }
  }
}

uint32 HubImpl::AllocateComponentID() {
  uint32 current_id = component_counter_;
  while (components_.count(component_counter_)) {
    ++component_counter_;
    if (current_id == component_counter_) {
      // NOTREACHED();
      return current_id;
    }
  }
  return component_counter_++;
}

uint32 HubImpl::AllocateInputContextID() {
  uint32 current_id = input_context_counter_;
  while (input_contexts_.count(input_context_counter_)) {
    ++input_context_counter_;
    if (current_id == input_context_counter_) {
      // NOTREACHED();
      return current_id;
    }
  }
  return input_context_counter_++;
}

bool HubImpl::RegisterComponents(Connector* connector,
                                 proto::Message* message) {
  DCHECK(hub_input_context_);

  // Delete the message automatically.
  scoped_ptr<proto::Message> mptr(message);
  if (!message->has_payload())
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_PAYLOAD);

  proto::MessagePayload* payload = message->mutable_payload();
  int size = payload->component_info_size();
  if (!size)
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_PAYLOAD);

  std::vector<uint32> ids;
  std::vector<Component*> components;
  for (int i = 0; i < size; ++i) {
    proto::ComponentInfo* info = payload->mutable_component_info(i);
    Component* component = CreateComponent(connector, *info, false);
    uint32 id = component ? component->id() : kComponentDefault;
    info->set_id(id);
    ids.push_back(id);
    components.push_back(component);
  }

  // Reuse the message object as reply message.
  ConvertToReplyMessage(message);

  if (!connector->Send(mptr.release())) {
    // Delete newly created component if we failed to send back the reply
    // message.
    for (int i = 0; i < size; ++i) {
      uint32 id = ids[i];
      if (id != kComponentDefault)
        DeleteComponent(connector, id);
    }
    return false;
  }

  // In order to make sure that external components receives no other messages
  // before receiving the reply message of MSG_REGISTER_COMPONENT, we need to
  // attach them to the default input context here rather than in
  // CreateComponent().
  for (int i = 0; i < size; ++i) {
    if (components[i]) {
      AttachToInputContext(components[i], hub_input_context_,
                           InputContext::PASSIVE, true);
    }
  }

  return true;
}

Component* HubImpl::CreateComponent(Connector* connector,
                                    const proto::ComponentInfo& info,
                                    bool built_in) {
  // A component must have a unique string id.
  if (!info.has_string_id())
    return NULL;

  ComponentStringIDMap::iterator string_id_iter =
      components_by_string_id_.find(info.string_id());
  // The string id must be unique.
  if (string_id_iter != components_by_string_id_.end())
    return NULL;

  // TODO(suzhe): Validate info's content.

  uint32 id = AllocateComponentID();
  Component* component = new Component(id, connector, info);

  bool broadcast = false;
  if (hub_input_context_)
    broadcast = hub_input_context_->MayConsume(MSG_COMPONENT_CREATED, false);

  components_[id] = component;
  components_by_string_id_[info.string_id()] = component;
  connectors_[connector].insert(id);

  if (broadcast) {
    proto::Message* message = NewMessage(
        MSG_COMPONENT_CREATED, kComponentBroadcast, kInputContextNone);
    message->mutable_payload()->add_component_info()->CopyFrom(
        component->info());
    BroadcastMessage(message, id);
  }

  if (hub_input_context_ && built_in) {
    AttachToInputContext(component, hub_input_context_,
                         InputContext::ACTIVE_STICKY, true);
  }

  return component;
}

bool HubImpl::DeregisterComponents(Connector* connector,
                                   proto::Message* message) {
  // Delete the message automatically.
  scoped_ptr<proto::Message> mptr(message);
  bool need_reply = (message->reply_mode() == proto::Message::NEED_REPLY);

  if (!message->has_payload())
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_PAYLOAD);

  proto::MessagePayload* payload = message->mutable_payload();
  int size = payload->uint32_size();
  if (!size)
    return ReplyError(connector, mptr.release(), proto::Error::INVALID_PAYLOAD);

  std::vector<bool> results;
  for (int i = 0; i < size; ++i)
    results.push_back(DeleteComponent(connector, payload->uint32(i)));

  // Only send a reply message if the sender want it.
  if (need_reply) {
    // Reuse the message object as reply message.
    ConvertToReplyMessage(message);
    payload->Clear();
    for (int i = 0; i < size; ++i)
      payload->add_boolean(results[i]);

    connector->Send(mptr.release());
  }
  return true;
}

bool HubImpl::DeleteComponent(Connector* connector, uint32 id) {
  Component* component = GetComponent(id);
  if (!component)
    return false;

  // A connector can only delete its own components.
  if (component->connector() != connector)
    return false;

  // Remove the component from |components_| map first, so that we won't send
  // any additional message to it.
  components_.erase(id);
  components_by_string_id_.erase(component->info().string_id());

  ConnectorMap::iterator connector_iter = connectors_.find(connector);
  if (connector_iter != connectors_.end())
    connector_iter->second.erase(id);

  // Detach the component from the default input context first.
  if (hub_input_context_) {
    if (component == hub_component_)
      DeleteInputContext(component, kInputContextNone);
    else
      hub_input_context_->DetachComponent(component);
  }

  // Detach the component from all attached input contexts and delete all input
  // contexts owned by it.
  const std::set<uint32>& input_contexts = component->attached_input_contexts();
  std::set<uint32>::const_iterator i = input_contexts.begin();
  while (i != input_contexts.end()) {
    InputContext* ic = GetInputContext(*i);
    DCHECK(ic);
    if (ic->owner() == component)
      DeleteInputContext(component, *i);
    else
      ic->DetachComponent(component);

    i = input_contexts.begin();
  }

  if (id == kComponentDefault)
    hub_component_ = NULL;

  delete component;

  if (hub_input_context_ &&
      hub_input_context_->MayConsume(MSG_COMPONENT_DELETED, false)) {
    proto::Message* message = NewMessage(
        MSG_COMPONENT_DELETED, kComponentBroadcast, kInputContextNone);
    message->mutable_payload()->add_uint32(id);
    BroadcastMessage(message);
  }
  return true;
}

InputContext* HubImpl::CreateInputContext(Component* owner) {
  uint32 icid = AllocateInputContextID();
  InputContext* ic = new InputContext(icid, owner, this);
  input_contexts_[icid] = ic;
  owner->attached_input_contexts().insert(icid);

  if (icid != kInputContextNone &&
      hub_input_context_->MayConsume(MSG_INPUT_CONTEXT_CREATED, false)) {
    proto::Message* message = NewMessage(
        MSG_INPUT_CONTEXT_CREATED, kComponentBroadcast, kInputContextNone);
    ic->GetInfo(message->mutable_payload()->mutable_input_context_info());
    BroadcastMessage(message);
  }
  return ic;
}

bool HubImpl::DeleteInputContext(Component* owner, uint32 icid) {
  InputContextMap::iterator i = input_contexts_.find(icid);
  if (i == input_contexts_.end())
    return false;

  InputContext* ic = i->second;
  if (ic->owner() != owner)
    return false;

  input_contexts_.erase(i);
  owner->attached_input_contexts().erase(icid);

  if (icid == focused_input_context_)
    focused_input_context_ = kInputContextNone;

  if (icid == kInputContextNone)
    hub_input_context_ = NULL;

  delete ic;

  if (hub_input_context_ &&
      hub_input_context_->MayConsume(MSG_INPUT_CONTEXT_DELETED, false)) {
    proto::Message* message = NewMessage(
        MSG_INPUT_CONTEXT_DELETED, kComponentBroadcast, kInputContextNone);
    message->mutable_payload()->add_uint32(icid);
    BroadcastMessage(message);
  }
  return true;
}

bool HubImpl::AttachToInputContext(Component* component,
                                   InputContext* input_context,
                                   InputContext::AttachState state,
                                   bool persistent) {
  DCHECK(IsComponentValid(component));
  if(!input_context->AttachComponent(component, state, persistent))
    return false;

  const uint32 icid = input_context->id();
  component->attached_input_contexts().insert(icid);
  if (!InputContext::IsAttachedState(state))
    return true;

  // Sends MSG_INPUT_CONTEXT_GOT_FOCUS to the component when necessary, so
  // that it will not need to query the input context's focus status
  // explicitly.
  if (icid == focused_input_context_ && icid != kInputContextNone &&
      component->CanConsume(MSG_INPUT_CONTEXT_GOT_FOCUS)) {
    component->connector()->Send(
        NewMessage(MSG_INPUT_CONTEXT_GOT_FOCUS, component->id(), icid));
  }

  // Broadcasts MSG_COMPONENT_ATTACHED if necessary.
  if (hub_input_context_->MayConsume(MSG_COMPONENT_ATTACHED, false)) {
    proto::Message* message = NewMessage(
        MSG_COMPONENT_ATTACHED, kComponentBroadcast, kInputContextNone);
    message->mutable_payload()->add_uint32(icid);
    message->mutable_payload()->add_uint32(component->id());
    BroadcastMessage(message);
  }
  return true;
}

bool HubImpl::FocusInputContext(uint32 icid) {
  if (focused_input_context_ == icid)
    return true;

  if (icid == kInputContextFocused)
    return false;

  BlurInputContext(focused_input_context_);
  focused_input_context_ = icid;

  if (icid == kInputContextNone)
    return true;

  proto::Message* message =
      NewMessage(MSG_INPUT_CONTEXT_GOT_FOCUS, kComponentBroadcast, icid);
  return BroadcastMessage(message);
}

bool HubImpl::BlurInputContext(uint32 icid) {
  if (icid == kInputContextFocused)
    icid = focused_input_context_;

  if (focused_input_context_ != icid)
    return true;

  focused_input_context_ = kInputContextNone;

  if (icid == kInputContextNone)
    return true;

  proto::Message* message =
      NewMessage(MSG_INPUT_CONTEXT_LOST_FOCUS, kComponentBroadcast, icid);
  return BroadcastMessage(message);
}

bool HubImpl::OnMsgAttachToInputContextReply(Component* source,
                                             proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);

  DCHECK(message->reply_mode() == proto::Message::IS_REPLY);
  if (message->reply_mode() != proto::Message::IS_REPLY)
    return false;

  InputContext* ic = GetInputContext(message->icid());
  if (!ic)
    return false;

  InputContext::AttachState state = ic->GetComponentAttachState(source);
  // The component should already be attached with a pending state.
  if (!InputContext::IsPendingState(state))
    return false;

  // We only attach the component if the reply message contains exactly one
  // boolean payload and its value is true.
  if (!message->has_payload() || !message->payload().boolean_size() ||
      !message->payload().boolean(0) || message->payload().has_error()) {
    ic->DetachComponent(source);
    return false;
  }

  if (state == InputContext::PENDING_PASSIVE)
    state = InputContext::PASSIVE;
  else if (state == InputContext::PENDING_ACTIVE)
    state = InputContext::ACTIVE;

  return AttachToInputContext(source, ic, state, false);
}

bool HubImpl::OnMsgQueryComponent(Component* source, proto::Message* message) {
  Connector* connector = source->connector();

  // This message makes no sense without a reply.
  if (message->reply_mode() != proto::Message::NEED_REPLY)
    return ReplyError(connector, message, proto::Error::INVALID_REPLY_MODE);

  proto::MessagePayload* payload = message->mutable_payload();
  int size = payload->component_info_size();
  if (!size) {
    // Return all components.
    payload->Clear();
    ComponentMap::const_iterator iter = components_.begin();
    ComponentMap::const_iterator end = components_.end();
    for (; iter != end; ++iter)
      payload->add_component_info()->CopyFrom(iter->second->info());
  } else {
    ComponentMap matched_components;

    for (int i = 0; i < size; ++i) {
      const proto::ComponentInfo& query = payload->component_info(i);
      if (query.has_id()) {
        ComponentMap::const_iterator iter = components_.find(query.id());
        if (iter != components_.end() && iter->second->MatchInfoTemplate(query))
          matched_components[iter->second->id()] = iter->second;
      } else if (query.has_string_id()) {
        ComponentStringIDMap::const_iterator iter =
            components_by_string_id_.find(query.string_id());
        if (iter != components_by_string_id_.end() &&
            iter->second->MatchInfoTemplate(query)) {
          matched_components[iter->second->id()] = iter->second;
        }
      } else {
        ComponentMap::const_iterator iter = components_.begin();
        ComponentMap::const_iterator end = components_.end();
        for (; iter != end; ++iter) {
          if (iter->second->MatchInfoTemplate(query))
            matched_components[iter->second->id()] = iter->second;
        }
      }
    }

    payload->Clear();
    ComponentMap::const_iterator iter = matched_components.begin();
    ComponentMap::const_iterator end = matched_components.end();
    for (; iter != end; ++iter)
      payload->add_component_info()->CopyFrom(iter->second->info());
  }

  if (payload->component_info_size()) {
    ConvertToReplyMessage(message);
  } else {
    ConvertToErrorReplyMessage(
        message, proto::Error::COMPONENT_NOT_FOUND, NULL);
  }

  connector->Send(message);
  return true;
}

bool HubImpl::DispatchToActiveConsumer(Component* source,
                                       proto::Message* message) {
  Connector* connector = source->connector();

  // A reply message should always have explicit target set.
  if (message->reply_mode() == proto::Message::IS_REPLY)
    return ReplyError(connector, message, proto::Error::INVALID_TARGET);

  InputContext* ic = GetInputContext(message->icid());
  if (!ic)
    return ReplyError(connector, message, proto::Error::INVALID_INPUT_CONTEXT);

  Component* consumer = ic->GetActiveConsumer(message->type());
  if (!consumer)
    return ReplyError(connector, message, proto::Error::NO_ACTIVE_CONSUMER);

  // Re-dispatch the message with explicit target.
  message->set_target(consumer->id());
  message->set_icid(ic->id());
  return Dispatch(connector, message);
}

bool HubImpl::BroadcastMessage(proto::Message* message,
                               uint32 exclude_component) {
  DCHECK(message->reply_mode() == proto::Message::NO_REPLY);

  // Make sure the |message| will be deleted.
  scoped_ptr<proto::Message> mptr(message);

  // A broadcast message may not require a reply message.
  if (message->reply_mode() != proto::Message::NO_REPLY)
    return false;

  // Broadcast the message
  InputContext* input_context = GetInputContext(message->icid());
  if (!input_context)
    return false;

  std::vector<uint32> consumers;
  if (!input_context->GetAllConsumersID(message->type(), false, &consumers))
    return true;

  uint32 source_id = message->source();
  std::vector<uint32>::iterator i = consumers.begin();
  std::vector<uint32>::iterator end = consumers.end();
  for (; i != end; ++i) {
    if (*i == source_id || *i == exclude_component)
      continue;

    // We need to lookup each consumer one by one, in case they got removed
    // during broadcasting.
    Component* consumer = GetComponent(*i);
    if (!consumer)
      continue;

    Connector* consumer_connector = consumer->connector();
    // The consumer_connector has been detached.
    if (consumer_connector != this && !connectors_.count(consumer_connector))
      continue;

    proto::Message* copy_message = new proto::Message(*message);
    copy_message->set_target(consumer->id());
    consumer_connector->Send(copy_message);
  }

  return true;
}

InputContext::AttachState HubImpl::RequestAttachToInputContext(
    Component* component,
    InputContext* input_context,
    InputContext::AttachState state,
    bool allow_implicit_attach) {
  DCHECK(state == InputContext::PASSIVE || state == InputContext::ACTIVE);
  bool implicit_attach = !component->CanConsume(MSG_ATTACH_TO_INPUT_CONTEXT);
  InputContext::AttachState old_state =
      input_context->GetComponentAttachState(component);
  bool persistent = input_context->IsComponentPersistent(component);

  // If the component can consume MSG_ATTACH_TO_INPUT_CONTEXT, then we need to
  // ask it to attach to the input context explicitly, otherwise we can only
  // attach the component if |allow_implicit_attach| is true.
  // If the component has already been attached, then we don't need to send the
  // request anymore.
  if (!InputContext::IsAttachedState(old_state)) {
    if (implicit_attach && !allow_implicit_attach)
      return InputContext::NOT_ATTACHED;

    if (!implicit_attach) {
      state = (state == InputContext::ACTIVE ? InputContext::PENDING_ACTIVE :
               InputContext::PENDING_PASSIVE);
    }
  }

  if (!AttachToInputContext(component, input_context, state, persistent))
    return InputContext::NOT_ATTACHED;

  // Send out MSG_ATTACH_TO_INPUT_CONTEXT. The component will be attached when
  // we receive the reply message. It may happen synchronously.
  // If the component was already in pending state, we don't need to send this
  // message again.
  if (!implicit_attach && old_state == InputContext::NOT_ATTACHED) {
    proto::Message* message = NewMessage(
        MSG_ATTACH_TO_INPUT_CONTEXT, component->id(), input_context->id());
    message->set_reply_mode(proto::Message::NEED_REPLY);
    input_context->GetInfo(
        message->mutable_payload()->mutable_input_context_info());
    // TODO(suzhe): Send content type related information.
    if (!Dispatch(this, message)) {
      input_context->DetachComponent(component);
      return InputContext::NOT_ATTACHED;
    }
  }

  return state;
}

bool HubImpl::ReplyError(Connector* connector,
                         proto::Message* message,
                         proto::Error::Code error_code) {
#if !defined(NDEBUG)
  std::string text;
  PrintMessageToString(*message, &text, false);
  DLOG(WARNING) << "Error when processing message: "
                << proto::Error::Code_Name(error_code) << ":\n"
                << text;
#endif
  // Don't bother sending a reply error message if a reply message is not
  // required.
  if (message->reply_mode() != proto::Message::NEED_REPLY ||
      !connectors_.count(connector)) {
    delete message;
    return false;
  }

  // TODO(suzhe): Error message.
  ConvertToErrorReplyMessage(message, error_code, NULL);
  connector->Send(message);

  // Always return true to prevent the hub from trying to reply another error
  // message again.
  return true;
}

bool HubImpl::ReplyBoolean(
    Connector* connector, proto::Message* message, bool value) {
  // Don't bother sending a reply ok message if a reply message is not
  // required.
  if (message->reply_mode() != proto::Message::NEED_REPLY ||
      !connectors_.count(connector)) {
    delete message;
    return true;
  }

  ConvertToBooleanReplyMessage(message, value);
  connector->Send(message);

  // Always return true to prevent the hub from trying to reply another error
  // message again.
  return true;
}

bool HubImpl::CheckMsgNeedReply(Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  if (message->reply_mode() != proto::Message::NEED_REPLY) {
    // Though calling ReplyError() here will just delete the message, this
    // method might print some logging message to help debug.
    ReplyError(connector, message, proto::Error::INVALID_REPLY_MODE);
    return false;
  }
  return true;
}

bool HubImpl::CheckMsgInputContext(Component* source, proto::Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = GetInputContext(message->icid());
  if (!ic) {
    ReplyError(connector, message, proto::Error::INVALID_INPUT_CONTEXT);
    return false;
  }
  return true;
}

bool HubImpl::CheckMsgInputContextAndSourceAttached(Component* source,
                                                    proto::Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = GetInputContext(message->icid());
  if (!ic) {
    ReplyError(connector, message, proto::Error::INVALID_INPUT_CONTEXT);
    return false;
  }
  if (!ic->IsComponentReallyAttached(source)) {
    ReplyError(connector, message, proto::Error::COMPONENT_NOT_ATTACHED);
    return false;
  }
  return true;
}

// static
proto::Message* HubImpl::NewMessage(uint32 type, uint32 target, uint32 icid) {
  proto::Message* message = new proto::Message();
  message->set_type(type);
  message->set_reply_mode(proto::Message::NO_REPLY);
  message->set_source(kComponentDefault);
  message->set_target(target);
  message->set_icid(icid);
  return message;
}

// static
bool HubImpl::CanComponentProduce(const Component* component,
                                  const proto::Message* message) {
  switch(message->reply_mode()) {
    case proto::Message::IS_REPLY:
      return component->CanConsume(message->type());
    default:
      return component->MayProduce(message->type());
  }
}

// static
bool HubImpl::CanComponentConsume(const Component* component,
                                  const proto::Message* message) {
  switch(message->reply_mode()) {
    case proto::Message::IS_REPLY:
      return component->MayProduce(message->type());
    default:
      return component->CanConsume(message->type());
  }
}

}  // namespace hub
}  // namespace ipc
