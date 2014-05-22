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

#include "ipc/mock_component_host.h"

#include "base/logging.h"
#include "base/time.h"
#include "ipc/component.h"
#include "ipc/message_util.h"
#include "ipc/test_util.h"

namespace ipc {

MockComponentHost::MockComponentHost()
    : component_(NULL),
      outgoing_event_(false, false),
      serial_count_(0),
      pause_count_(0) {
}

MockComponentHost::~MockComponentHost() {
  if (component_)
    RemoveComponent(component_);

  while (!outgoing_messages_.empty()) {
    delete outgoing_messages_.front();
    outgoing_messages_.pop();
  }
}

bool MockComponentHost::AddComponent(ipc::Component* component) {
  {
    base::AutoLock auto_lock(component_lock_);
    DCHECK(component);
    if (component_)
      return false;
    if (!InitInfo(component))
      return false;

    component_ = component;
    component_->DidAddToHost(this);
    // Assign the component an arbitrary id other than kComponentDefault.
    info_.set_id(kMockComponentId);
  }
  // Release component lock here to allow component sending messages in
  // OnRegistered.
  component_->Registered(kMockComponentId);
  return true;
}

bool MockComponentHost::RemoveComponent(ipc::Component* component) {
  base::AutoLock auto_lock(component_lock_);
  DCHECK(component);
  if (component_ != component)
    return false;
  component_->Deregistered();
  component_->DidRemoveFromHost();
  component_ = NULL;
  info_.Clear();
  produce_messages_.clear();
  consume_messages_.clear();
  return true;
}

bool MockComponentHost::Send(Component* component,
                             proto::Message* message,
                             uint32* serial) {
  base::AutoLock auto_lock(message_lock_);
  DCHECK(message);
  DCHECK_EQ(component_, component);
  SendUnlocked(message, serial);
  return true;
}

bool MockComponentHost::SendWithReply(Component* component,
                                      proto::Message* message,
                                      int timeout,
                                      proto::Message** reply) {
  base::AutoLock auto_lock(message_lock_);
  DCHECK(message);
  DCHECK_EQ(component_, component);
  const bool need_reply = MessageNeedReply(message);
  uint32 serial = 0;
  SendUnlocked(message, &serial);
  if (!reply)
    return true;
  *reply = NULL;
  if (!need_reply)
    return true;
  if (!next_reply_message_.get())
    return false;
  *reply = next_reply_message_.release();
  return true;
}

void MockComponentHost::PauseMessageHandling(Component* component) {
  base::AutoLock auto_lock(component_lock_);
  DCHECK_EQ(component_, component);
  pause_count_++;
}

void MockComponentHost::ResumeMessageHandling(Component* component) {
  base::AutoLock auto_lock(component_lock_);
  DCHECK_EQ(component_, component);
  DCHECK_GT(pause_count_, 0);
  pause_count_--;
}

bool MockComponentHost::WaitOutgoingMessage(int timeout) {
  return WaitOnMessageQueue(
      timeout, &outgoing_messages_, &outgoing_event_, &message_lock_);
}

proto::Message* MockComponentHost::PopOutgoingMessage() {
  base::AutoLock auto_lock(message_lock_);
  if (outgoing_messages_.empty())
    return NULL;
  proto::Message* message = outgoing_messages_.front();
  outgoing_messages_.pop();
  return message;
}

void MockComponentHost::SetNextReplyMessage(proto::Message* message) {
  base::AutoLock auto_lock(message_lock_);
  next_reply_message_.reset(message);
}

bool MockComponentHost::HandleMessage(proto::Message* message) {
  DCHECK(message);
  scoped_ptr<proto::Message> mptr(message);
  {
    base::AutoLock auto_lock(component_lock_);
    DCHECK(component_);
    if (!consume_messages_.count(mptr->type()))
      return false;
  }
  component_->Handle(mptr.release());
  return true;
}

bool MockComponentHost::MayProduce(uint32 message_type) {
  base::AutoLock auto_lock(component_lock_);
  return produce_messages_.count(message_type);
}

bool MockComponentHost::CanConsume(uint32 message_type) {
  base::AutoLock auto_lock(component_lock_);
  return consume_messages_.count(message_type);
}

bool MockComponentHost::IsMessageHandlingPaused() {
  base::AutoLock auto_lock(component_lock_);
  return pause_count_ > 0;
}

bool MockComponentHost::InitInfo(Component* component) {
  info_.Clear();
  produce_messages_.clear();
  consume_messages_.clear();

  component->GetInfo(&info_);
  if (info_.string_id().empty()) {
    info_.Clear();
    return false;
  }

  for (int i = 0; i < info_.produce_message_size(); ++i)
    produce_messages_.insert(info_.produce_message(i));

  for (int i = 0; i < info_.consume_message_size(); ++i)
    consume_messages_.insert(info_.consume_message(i));

  return true;
}

void MockComponentHost::SendUnlocked(proto::Message* message, uint32* serial) {
  message_lock_.AssertAcquired();
  outgoing_messages_.push(message);
  if (!MessageIsReply(message))
    message->set_serial(++serial_count_);
  if (serial)
    *serial = message->serial();
  outgoing_event_.Signal();
}

}  // namespace ipc
