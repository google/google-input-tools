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

#ifndef GOOPY_IPC_MOCK_MESSAGE_CHANNEL_H_
#define GOOPY_IPC_MOCK_MESSAGE_CHANNEL_H_
#pragma once

#include <queue>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/message_channel.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/simple_message_queue.h"
#include "ipc/thread_message_queue_runner.h"

namespace ipc {

// A mock MessageChannel implementation for testing purpose.
class MockMessageChannel : public MessageChannel,
                           public MessageQueue::Handler,
                           public ThreadMessageQueueRunner::Delegate {
 public:
  MockMessageChannel();
  virtual ~MockMessageChannel();

  bool Init();

  // Sets if the channel is connected or not.
  void SetConnected(bool connected);

  // Sets if Send() method should return true or false. If |enabled| is false
  // then Send() method will just return false.
  void SetSendEnabled(bool enabled);

  // Gets enabled state of Send() method.
  bool GetSendEnabled() const;

  // Posts a message to current channel listener. The message will be delivered
  // to the listener asynchronously from another thread.
  void PostMessageToListener(proto::Message* message);

  // Wait for all messages posted by PostMessageToListener() method are sent to
  // listener.
  void WaitForPostingMessagesToListener();

  // Wait until we receive a message from Send().
  // Returns NULL if no message is received after |timeout| milliseconds.
  proto::Message* WaitMessage(int timeout);

  // Overridden from MessageChannel:
  virtual bool IsConnected() const OVERRIDE;
  virtual bool Send(proto::Message* message) OVERRIDE;
  virtual void SetListener(Listener* listener) OVERRIDE;

 private:
  // Overridden from MessageQueue::Handler:
  virtual void HandleMessage(proto::Message* message, void* user_data) OVERRIDE;

  // Overridden from ThreadMessageQueueRunner::Delegate:
  virtual MessageQueue* CreateMessageQueue() OVERRIDE;
  virtual void DestroyMessageQueue(MessageQueue* queue) OVERRIDE;

  Listener* listener_;

  // Indicates if the channel is connected. It's false by default.
  bool connected_;

  // Indicates if Send() method is enabled. It's true by default.
  bool send_enabled_;

  // A queue to store messages received from Send() method.
  std::queue<proto::Message*> queue_from_send_;

  // A queue for dispatching messages to the listener.
  scoped_ptr<SimpleMessageQueue> queue_to_listener_;

  // Runner thread of |queue_to_listener_|.
  scoped_ptr<ThreadMessageQueueRunner> runner_;

  base::WaitableEvent event_;

  base::WaitableEvent finish_post_event_;

  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(MockMessageChannel);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MOCK_MESSAGE_CHANNEL_H_
