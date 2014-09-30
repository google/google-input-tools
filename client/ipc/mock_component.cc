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

#include "ipc/mock_component.h"

#include "base/logging.h"
#include "base/time.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace ipc {

MockComponent::MockComponent(const std::string& string_id)
    : string_id_(string_id),
      incoming_event_(false, false),
      thread_id_(base::kInvalidThreadId),
      handle_count_(0) {
}

MockComponent::~MockComponent() {
  while (!outgoing_.empty()) {
    delete outgoing_.front().message;
    outgoing_.pop();
  }
  while (!incoming_.empty()) {
    delete incoming_.front();
    incoming_.pop();
  }
}

void MockComponent::GetInfo(proto::ComponentInfo* info) {
  info->set_string_id(string_id_);
  thread_id_ = base::PlatformThread::CurrentId();
}

void MockComponent::Handle(proto::Message* message) {
  ASSERT_EQ(id(), message->target());
  ASSERT_EQ(thread_id_, base::PlatformThread::CurrentId());
  base::AutoLock auto_lock(lock_);
  handle_count_++;
  incoming_.push(new proto::Message(*message));
  incoming_event_.Signal();
  ProcessOutgoingMessagesUnlocked();
  handle_count_--;
  ReplyTrue(message);
}

void MockComponent::OnRegistered() {
  ASSERT_NE(kComponentDefault, id());
  ASSERT_EQ(thread_id_, base::PlatformThread::CurrentId());
  base::AutoLock auto_lock(lock_);
  incoming_.push(NULL);
  incoming_event_.Signal();
  ProcessOutgoingMessagesUnlocked();
}

void MockComponent::OnDeregistered() {
  ASSERT_EQ(kComponentDefault, id());
  base::AutoLock auto_lock(lock_);
  incoming_.push(NULL);
  incoming_event_.Signal();
  ProcessOutgoingMessagesUnlocked();
}

void MockComponent::AddOutgoingMessage(proto::Message* message,
                                       bool expected_result,
                                       uint32 timeout) {
  return AddOutgoingMessageWithMode(message, false, expected_result, timeout);
}

void MockComponent::AddOutgoingMessageWithMode(
    proto::Message* message,
    bool non_recursive,
    bool expected_result,
    uint32 timeout) {
  base::AutoLock auto_lock(lock_);
  OutgoingMessage msg;
  msg.message = message;
  msg.expected_result = expected_result;
  msg.non_recursive = non_recursive;
  msg.timeout = timeout;
  outgoing_.push(msg);
}

bool MockComponent::WaitIncomingMessage(int timeout) {
  return WaitOnMessageQueue(timeout, &incoming_, &incoming_event_, &lock_);
}

proto::Message* MockComponent::PopIncomingMessage() {
  base::AutoLock auto_lock(lock_);
  if (incoming_.empty())
    return NULL;
  proto::Message* message = incoming_.front();
  incoming_.pop();
  return message;
}

void MockComponent::ProcessOutgoingMessagesUnlocked() {
  while (!outgoing_.empty()) {
    OutgoingMessage msg = outgoing_.front();
    outgoing_.pop();

    const uint32 type = msg.message->type();
    const bool need_reply =
        (msg.message->reply_mode() == proto::Message::NEED_REPLY);

    proto::Message* reply = NULL;
    bool result = false;
    {
      base::AutoUnlock auto_unlock(lock_);
      if (!need_reply) {
        result = Send(msg.message, NULL);
      } else if (msg.non_recursive) {
        result = SendWithReplyNonRecursive(msg.message, msg.timeout, &reply);
      } else {
        result = SendWithReply(msg.message, msg.timeout, &reply);
      }
    }

    ASSERT_EQ(msg.expected_result, result);
    if (result && need_reply) {
      ASSERT_TRUE(reply);
      ASSERT_EQ(type, reply->type());
      ASSERT_EQ(proto::Message::IS_REPLY, reply->reply_mode());
      incoming_.push(reply);
    } else {
      ASSERT_FALSE(reply);
    }
  }
}

}  // namespace ipc
