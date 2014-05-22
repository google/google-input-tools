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

#include "ipc/hub_input_context.h"

#include "base/logging.h"
#include "ipc/hub_component.h"

namespace ipc {
namespace hub {

InputContext::InputContext(uint32 id, Component* owner, Delegate* delegate)
    : id_(id),
      owner_(owner),
      delegate_(delegate),
      active_hotkey_lists_valid_(false) {
  DCHECK(owner_);
  DCHECK(delegate_);
  // The owner is always attached and never gets detached. And the owner has the
  // highest priority for messages it cares about, so use ACTIVE_STICKY here.
  AttachComponent(owner, ACTIVE_STICKY, true);
}

InputContext::~InputContext() {
  ComponentMap attached_components;
  attached_components.swap(attached_components_);
  active_consumers_.clear();

  const Component* owner = owner_;

  // Sets |owner_| to NULL to let |delegate_| know that this input context is
  // being destroyed.
  owner_ = NULL;

  // We need to inform the delegate that all components have been detached.
  ComponentMap::iterator iter = attached_components.begin();
  ComponentMap::iterator end = attached_components.end();
  for (; iter != end; ++iter) {
    if (iter->first != owner)
      delegate_->OnComponentDetached(this, iter->first, iter->second.state);
  }
}

void InputContext::GetInfo(proto::InputContextInfo* info) const {
  DCHECK(info);
  info->set_id(id_);
  info->set_owner(owner_->id());
  // TODO(suzhe): export information about attached components and active
  // consumers.
}

bool InputContext::AttachComponent(Component* component,
                                   AttachState state,
                                   bool persistent) {
  DCHECK(component);
  if (!component)
    return false;

  if (component == owner_ && state != ACTIVE_STICKY)
    return false;

  AttachState old_state = GetComponentAttachState(component);
  if (state == NOT_ATTACHED)
    return old_state != NOT_ATTACHED ? DetachComponent(component) : false;

  if (IsPendingState(state) && IsAttachedState(old_state))
    return false;

  ComponentState* component_state = &attached_components_[component];
  component_state->state = state;
  component_state->persistent = (component == owner_ ? true : persistent);

  if (IsPendingState(state))
    return true;

  // Activate the component.
  MessageTypeVector messages;
  const proto::ComponentInfo& info = component->info();
  int size = info.consume_message_size();
  for (int i = 0; i < size; ++i) {
    uint32 message = info.consume_message(i);
    // Don't bother if the component already explicitly resigned his active
    // consumer role for the message.
    if (!component_state->resigned_consumer.count(message))
      messages.push_back(message);
  }

  ActivateForMessages(component, messages, state != PASSIVE);
  return true;
}

bool InputContext::DetachComponent(Component* component) {
  DCHECK(component);
  // The owner should never be detached.
  if (!component || component == owner_)
    return false;

  ComponentMap::iterator component_iter = attached_components_.find(component);
  if (component_iter == attached_components_.end())
    return false;

  if (component_iter->second.hotkey_list_set)
    InvalidateActiveHotkeyLists();

  AttachState state = component_iter->second.state;
  attached_components_.erase(component_iter);

  if (IsPendingState(state)) {
    // We might expect this pending component to consume some messages, so if it
    // gets detached, we need to look for new consumers for them.
    const proto::ComponentInfo& info = component->info();
    CheckAndRequestConsumer(info.consume_message().data(),
                            info.consume_message_size(),
                            component);

    // We need to inform the delegate that it has been detached, so that the
    // delegate can do some cleanup task.
    delegate_->OnComponentDetached(this, component, state);
    return true;
  }

  const proto::ComponentInfo& info = component->info();
  MessageTypeVector messages(info.consume_message().begin(),
                             info.consume_message().end());

  DeactivateForMessages(component, messages);
  delegate_->OnComponentDetached(this, component, state);
  return true;
}

bool InputContext::IsComponentActive(Component* component) const {
  DCHECK(component);
  if (!IsComponentReallyAttached(component))
    return false;

  ConsumerMap::const_iterator iter = active_consumers_.begin();
  ConsumerMap::const_iterator end = active_consumers_.end();
  for (; iter != end; ++iter) {
    if (iter->second == component)
      return true;
  }
  return false;
}

bool InputContext::IsComponentRedundant(Component* component) const {
  DCHECK(component);
  if (!IsComponentReallyAttached(component))
    return false;

  const proto::ComponentInfo& info = component->info();
  int size = info.consume_message_size();
  for (int i = 0; i < size; ++i) {
    uint32 message = info.consume_message(i);
    if (GetActiveConsumer(message) == component &&
        MessageNeedConsumer(message, component))
      return false;
  }
  return true;
}

void InputContext::MaybeDetachRedundantComponents() {
  if (!delegate_)
    return;

  std::vector<Component*> components;
  ComponentMap::iterator attached_iter = attached_components_.begin();
  ComponentMap::iterator attached_end = attached_components_.end();
  for (; attached_iter != attached_end; ++attached_iter) {
    if (!attached_iter->second.persistent &&
        IsAttachedState(attached_iter->second.state))
      components.push_back(attached_iter->first);
  }

  std::vector<Component*>::iterator iter = components.begin();
  std::vector<Component*>::iterator end = components.end();
  for (; iter != end; ++iter) {
    if (IsComponentRedundant(*iter))
      delegate_->MaybeDetachComponent(this, *iter);
  }
}

bool InputContext::MayProduce(uint32 message_type, bool include_pending) const {
  ComponentMap::const_iterator iter = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; iter != end; ++iter) {
    if (IsPendingState(iter->second.state) && !include_pending)
      continue;

    if (iter->first->MayProduce(message_type))
      return true;
  }
  return false;
}

bool InputContext::MayConsume(uint32 message_type, bool include_pending) const {
  if (HasActiveConsumer(message_type))
    return true;

  if (!include_pending)
    return false;

  ComponentMap::const_iterator iter = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; iter != end; ++iter) {
    if (IsPendingState(iter->second.state) &&
        iter->first->CanConsume(message_type))
      return true;
  }
  return false;
}

bool InputContext::AssignActiveConsumer(Component* component,
                                        const uint32* messages,
                                        size_t size) {
  DCHECK(component);
  ComponentMap::iterator iter = attached_components_.find(component);
  if (iter == attached_components_.end() || IsPendingState(iter->second.state))
    return false;

  MessageTypeVector valid_messages;
  for (size_t i = 0; i < size; ++i) {
    uint32 message = messages[i];
    if (component->CanConsume(message)) {
      iter->second.resigned_consumer.erase(message);
      valid_messages.push_back(message);
    }
  }

  ActivateForMessages(component, valid_messages, true);
  return true;
}

bool InputContext::ResignActiveConsumer(Component* component,
                                        const uint32* messages,
                                        size_t size) {
  DCHECK(component);
  ComponentMap::iterator iter = attached_components_.find(component);
  if (iter == attached_components_.end() || IsPendingState(iter->second.state))
    return false;

  bool persistent = iter->second.persistent;
  MessageTypeVector valid_messages;
  for (size_t i = 0; i < size; ++i) {
    uint32 message = messages[i];
    if (component->CanConsume(message)) {
      iter->second.resigned_consumer.insert(message);
      valid_messages.push_back(message);
    }
  }

  DeactivateForMessages(component, valid_messages);
  if (!persistent && IsComponentRedundant(component))
    delegate_->MaybeDetachComponent(this, component);
  return true;
}

void InputContext::SetMessagesNeedConsumer(
    Component* component,
    const uint32* messages,
    size_t size,
    std::vector<uint32>* already_have_consumers) {
  DCHECK(messages);
  ComponentMap::iterator iter = attached_components_.find(component);
  if (iter == attached_components_.end())
    return;

  MessageTypeSet* need_consumer = &(iter->second.need_consumer);
  const bool first_time = !need_consumer->size();
  need_consumer->clear();

  for (size_t i = 0; i < size; ++i) {
    const uint32 message = messages[i];
    if (component->MayProduce(message))
      need_consumer->insert(message);
  }

  MessageTypeVector missing_consumers;
  MessageTypeSet::iterator msg_iter = need_consumer->begin();
  MessageTypeSet::iterator msg_end = need_consumer->end();
  for (; msg_iter != msg_end; ++msg_iter) {
    const uint32 message = *msg_iter;
    if (!MayConsume(message, true))
      missing_consumers.push_back(message);
    else if (already_have_consumers)
      already_have_consumers->push_back(message);
  }

  if (missing_consumers.size())
    delegate_->RequestConsumer(this, missing_consumers, component);
  if (!first_time)
    MaybeDetachRedundantComponents();
}

size_t InputContext::GetAllConsumers(uint32 message_type,
                                     bool include_pending,
                                     std::vector<Component*>* consumers) const {
  DCHECK(consumers);
  consumers->clear();

  // Always returns the active consumer as the first consumer.
  Component* active_consumer = GetActiveConsumer(message_type);
  if (active_consumer)
    consumers->push_back(active_consumer);

  ComponentMap::const_iterator iter = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; iter != end; ++iter) {
    if (IsPendingState(iter->second.state) && !include_pending)
      continue;

    Component* component = iter->first;
    if (component == active_consumer || !component->CanConsume(message_type) ||
        iter->second.resigned_consumer.count(message_type))
      continue;

    consumers->push_back(component);
  }
  return consumers->size();
}

size_t InputContext::GetAllConsumersID(uint32 message_type,
                                       bool include_pending,
                                       std::vector<uint32>* ids) const {
  DCHECK(ids);
  ids->clear();

  std::vector<Component*> consumers;
  if (GetAllConsumers(message_type, include_pending, &consumers)) {
    std::vector<Component*>::iterator i = consumers.begin();
    std::vector<Component*>::iterator end = consumers.end();
    for (; i != end; ++i)
      ids->push_back((*i)->id());
  }
  return ids->size();
}

size_t InputContext::GetAllMessagesNeedConsumer(
    bool include_pending, std::vector<uint32>* messages) const {
  DCHECK(messages);
  MessageTypeSet produce_messages;

  // Collects all messages may be produced by attached components.
  ComponentMap::const_iterator iter = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; iter != end; ++iter) {
    if (IsPendingState(iter->second.state) && !include_pending)
      continue;

    // Only care about messages that need consumers.
    const MessageTypeSet& need_consumer = iter->second.need_consumer;
    produce_messages.insert(need_consumer.begin(), need_consumer.end());
  }

  messages->clear();
  MessageTypeSet::iterator message_iter = produce_messages.begin();
  MessageTypeSet::iterator message_end = produce_messages.end();
  for (; message_iter != message_end; ++message_iter) {
    if (!MayConsume(*message_iter, include_pending))
      messages->push_back(*message_iter);
  }
  return messages->size();
}

const HotkeyList* InputContext::GetComponentActiveHotkeyList(
    Component* component) const {
  ComponentMap::const_iterator i = attached_components_.find(component);
  if (i != attached_components_.end() && i->second.hotkey_list_set)
    return component->GetHotkeyList(i->second.hotkey_list_id);
  return NULL;
}

void InputContext::SetComponentActiveHotkeyList(
    Component* component, uint32 id) {
  ComponentMap::iterator i = attached_components_.find(component);
  if (i == attached_components_.end())
    return;

  if (i->second.hotkey_list_set && i->second.hotkey_list_id == id)
    return;

  i->second.hotkey_list_id = id;
  i->second.hotkey_list_set = true;
  InvalidateActiveHotkeyLists();
}

void InputContext::UnsetComponentActiveHotkeyList(Component* component) {
  ComponentMap::iterator i = attached_components_.find(component);
  if (i != attached_components_.end() && i->second.hotkey_list_set) {
    i->second.hotkey_list_set = false;
    InvalidateActiveHotkeyLists();
  }
}

void InputContext::ComponentHotkeyListUpdated(Component* component, uint32 id) {
  ComponentMap::iterator i = attached_components_.find(component);
  if (i != attached_components_.end() && i->second.hotkey_list_set &&
      i->second.hotkey_list_id == id)
    InvalidateActiveHotkeyLists();
}

void InputContext::ComponentHotkeyListRemoved(Component* component, uint32 id) {
  ComponentMap::iterator i = attached_components_.find(component);
  if (i != attached_components_.end() && i->second.hotkey_list_set &&
      i->second.hotkey_list_id == id) {
    i->second.hotkey_list_set = false;
    InvalidateActiveHotkeyLists();
  }
}

Component* InputContext::FindConsumer(uint32 message_type,
                                      Component* exclude) const {
  class Result {
   public:
    Result() : consumer_(NULL), state_(NOT_ATTACHED), active_(false) { }

    void Update(Component* c, AttachState s, bool a) {
      if (consumer_ && (state_ > s || (state_ == s && active_ > a)))
        return;

      consumer_ = c;
      state_ = s;
      active_ = a;
    }

    Component* consumer_;
    AttachState state_;
    bool active_;
  };

  Result result;
  ComponentMap::const_iterator iter = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; iter != end; ++iter) {
    Component* component = iter->first;
    if (IsPendingState(iter->second.state) || component == exclude ||
        !component->CanConsume(message_type) ||
        iter->second.resigned_consumer.count(message_type))
      continue;

    result.Update(component, iter->second.state, IsComponentActive(component));
  }

  return result.consumer_;
}

void InputContext::ActivateForMessages(Component* component,
                                       const MessageTypeVector& messages,
                                       bool active) {
  std::map<Component*, MessageTypeVector> deactivated_components;
  MessageTypeVector activated_messages;
  MessageTypeVector::const_iterator i = messages.begin();
  MessageTypeVector::const_iterator end = messages.end();
  for (; i != end; ++i) {
    uint32 message = *i;
    Component* old = GetActiveConsumer(message);
    if (old == component || (!active && old))
      continue;

    if (old && GetComponentAttachState(old) == ACTIVE_STICKY)
      continue;

    active_consumers_[message] = component;
    activated_messages.push_back(message);
    if (old)
      deactivated_components[old].push_back(message);
  }

  if (activated_messages.size()) {
    delegate_->OnComponentActivated(this, component, activated_messages);

    std::map<Component*, MessageTypeVector>::iterator iter =
        deactivated_components.begin();
    std::map<Component*, MessageTypeVector>::iterator end =
        deactivated_components.end();
    for (; iter != end; ++iter) {
      component = iter->first;
      delegate_->OnComponentDeactivated(this, component, iter->second);
      if (IsComponentRedundant(component) && !IsComponentPersistent(component))
        delegate_->MaybeDetachComponent(this, iter->first);
    }

    delegate_->OnActiveConsumerChanged(this, activated_messages);
  }
}

void InputContext::DeactivateForMessages(Component* component,
                                         const MessageTypeVector& messages) {
  MessageTypeVector deactivated_messages;
  MessageTypeVector::const_iterator msg_iter = messages.begin();
  MessageTypeVector::const_iterator msg_end = messages.end();
  for (; msg_iter != msg_end; ++msg_iter) {
    uint32 message = *msg_iter;
    ConsumerMap::iterator iter = active_consumers_.find(message);
    if (iter != active_consumers_.end() && iter->second == component) {
      active_consumers_.erase(iter);
      deactivated_messages.push_back(message);
    }
  }

  // Find alternative consumers for messages originally consumed by the
  // component.
  MessageTypeVector messages_need_consumer;
  std::map<Component*, MessageTypeVector> activated_components;
  msg_iter = deactivated_messages.begin();
  msg_end = deactivated_messages.end();
  for (; msg_iter != msg_end; ++msg_iter) {
    uint32 message = *msg_iter;
    Component* consumer = FindConsumer(message, component);
    if (consumer) {
      active_consumers_[message] = consumer;
      activated_components[consumer].push_back(message);
    } else if (MessageNeedConsumer(message, component)) {
      messages_need_consumer.push_back(message);
    }
  }

  if (deactivated_messages.size()) {
    std::map<Component*, MessageTypeVector>::iterator iter =
        activated_components.begin();
    std::map<Component*, MessageTypeVector>::iterator end =
        activated_components.end();
    for (; iter != end; ++iter)
      delegate_->OnComponentActivated(this, iter->first, iter->second);

    delegate_->OnComponentDeactivated(this, component, deactivated_messages);

    delegate_->OnActiveConsumerChanged(this, deactivated_messages);

    if (messages_need_consumer.size())
      delegate_->RequestConsumer(this, messages_need_consumer, component);
  }
}

void InputContext::CheckAndRequestConsumer(const uint32* messages,
                                           int size,
                                           Component* exclude) {
  if (!delegate_)
    return;

  MessageTypeVector need_consumer;
  for (int i = 0; i < size; ++i) {
    uint32 message = messages[i];
    if (MessageNeedConsumer(message, exclude) && !GetActiveConsumer(message))
      need_consumer.push_back(message);
  }

  delegate_->RequestConsumer(this, need_consumer, exclude);
}

bool InputContext::MessageNeedConsumer(uint32 message,
                                       Component* exclude) const {
  ComponentMap::const_iterator iter = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; iter != end; ++iter) {
    if (iter->first != exclude && iter->second.need_consumer.count(message))
      return true;
  }
  return false;
}

void InputContext::InvalidateActiveHotkeyLists() {
  active_hotkey_lists_valid_ = false;
  active_hotkey_lists_.clear();
}

void InputContext::InitializeActiveHotkeyLists() {
  active_hotkey_lists_.clear();

  ComponentMap::const_iterator i = attached_components_.begin();
  ComponentMap::const_iterator end = attached_components_.end();
  for (; i != end; ++i) {
    const HotkeyList* hotkey_list = GetComponentActiveHotkeyList(i->first);
    if (hotkey_list)
      active_hotkey_lists_.push_back(hotkey_list);
  }
  active_hotkey_lists_valid_ = true;
}

}  // namespace hub
}  // namespace ipc
