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

#ifndef GOOPY_IPC_HUB_HOST_H_
#define GOOPY_IPC_HUB_HOST_H_
#pragma once

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "ipc/hub_impl.h"
#include "ipc/message_queue.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/thread_message_queue_runner.h"

namespace ipc {

// HubHost is thread-safe Hub implementation, operations on the hub will be
// serialized and handled in one thread
class HubHost : public Hub,
                public ThreadMessageQueueRunner::Delegate,
                public MessageQueue::Handler {
 public:
  HubHost();
  virtual ~HubHost();

  // Methods of interface Hub
  virtual void Attach(Hub::Connector* connector) OVERRIDE;
  virtual void Detach(Hub::Connector* connector) OVERRIDE;
  virtual bool Dispatch(Hub::Connector* connector,
                        proto::Message* message) OVERRIDE;

  // Start a thread message queue runner to run the hub
  void Run();

  // Quit runner
  void Quit();

 private:
  // Methods of ThreadMessageQueueRunner::Delegate
  virtual MessageQueue* CreateMessageQueue() OVERRIDE;
  virtual void DestroyMessageQueue(MessageQueue* queue) OVERRIDE;
  virtual void RunnerThreadStarted() OVERRIDE;
  virtual void RunnerThreadTerminated() OVERRIDE;

  // Methods of MessageQueue::Handler
  virtual void HandleMessage(proto::Message* message, void* user_data) OVERRIDE;

  // Message queue runner will serialize incoming messages in message queue
  // and dispatch them one by one in one thread
  scoped_ptr<ThreadMessageQueueRunner> message_queue_runner_;

  // The message queue created for message_queue_runner_
  scoped_ptr<MessageQueue> message_queue_;

  // Non-thread-safe hub
  scoped_ptr<hub::HubImpl> hub_impl_;

  // Runner_lock_ protects message_queue_runner_'s thread-safety
  base::Lock runner_lock_;

  DISALLOW_COPY_AND_ASSIGN(HubHost);
};

}  // namespace ipc

#endif  // GOOPY_IPC_HUB_HOST_H_
