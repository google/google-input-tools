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

#include "ipc/mock_message_channel.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/time.h"
#include "ipc/test_util.h"

namespace ipc {

MockMessageChannel::MockMessageChannel()
    : listener_(NULL),
      connected_(false),
      send_enabled_(true),
      event_(false, false),
      finish_post_event_(false, false) {
}

MockMessageChannel::~MockMessageChannel() {
  SetListener(NULL);
  base::AutoLock auto_lock(lock_);
  while (!queue_from_send_.empty()) {
    delete queue_from_send_.front();
    queue_from_send_.pop();
  }
}

bool MockMessageChannel::Init() {
  DCHECK(!runner_.get());
  runner_.reset(new ThreadMessageQueueRunner(this));
  runner_->Run();
  return runner_->IsRunning();
}

void MockMessageChannel::SetConnected(bool connected) {
  const bool old_connected = connected_;
  {
    base::AutoLock auto_lock(lock_);
    connected_ = connected;
  }
  if (old_connected != connected && listener_) {
    if (connected)
      listener_->OnMessageChannelConnected(this);
    else
      listener_->OnMessageChannelClosed(this);
  }
}

void MockMessageChannel::SetSendEnabled(bool enabled) {
  base::AutoLock auto_lock(lock_);
  send_enabled_ = enabled;
}

bool MockMessageChannel::GetSendEnabled() const {
  base::AutoLock auto_lock(lock_);
  return send_enabled_;
}

void MockMessageChannel::PostMessageToListener(proto::Message* message) {
  DCHECK(message);

  base::AutoLock auto_lock(lock_);
  DCHECK(listener_);
  DCHECK(queue_to_listener_.get());
  finish_post_event_.Reset();
  queue_to_listener_->Post(message, NULL);
}

void MockMessageChannel::WaitForPostingMessagesToListener() {
  if (!queue_to_listener_->PendingCount())
    return;
  finish_post_event_.Wait();
  DCHECK(!queue_to_listener_->PendingCount());
  return;
}

proto::Message* MockMessageChannel::WaitMessage(int timeout) {
  if (!WaitOnMessageQueue(timeout, &queue_from_send_, &event_, &lock_))
    return NULL;

  proto::Message* message = NULL;
  {
    base::AutoLock auto_lock(lock_);
    message = queue_from_send_.front();
    queue_from_send_.pop();
  }
  return message;
}

bool MockMessageChannel::IsConnected() const {
  base::AutoLock auto_lock(lock_);
  return connected_;
}

bool MockMessageChannel::Send(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  base::AutoLock auto_lock(lock_);
  if (!send_enabled_ || !connected_)
    return false;
  queue_from_send_.push(mptr.release());
  event_.Signal();
  return true;
}

void MockMessageChannel::SetListener(Listener* listener) {
  Listener* old_listener;
  {
    base::AutoLock auto_lock(lock_);
    old_listener = listener_;
    listener_ = listener;
  }
  if (old_listener)
    old_listener->OnDetachedFromMessageChannel(this);
  if (listener)
    listener->OnAttachedToMessageChannel(this);
}

void MockMessageChannel::HandleMessage(proto::Message* message,
                                       void* user_data) {
  DCHECK(listener_);
  listener_->OnMessageReceived(this, message);
  if (!queue_to_listener_->PendingCount())
    finish_post_event_.Signal();
}

MessageQueue* MockMessageChannel::CreateMessageQueue() {
  DCHECK(!queue_to_listener_.get());
  queue_to_listener_.reset(new SimpleMessageQueue(this));
  return queue_to_listener_.get();
}

void MockMessageChannel::DestroyMessageQueue(MessageQueue* queue) {
  DCHECK_EQ(queue_to_listener_.get(), queue);
  queue_to_listener_.reset();
}

}  // namespace ipc
