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

#ifndef GOOPY_IPC_SIMPLE_MESSAGE_QUEUE_H_
#define GOOPY_IPC_SIMPLE_MESSAGE_QUEUE_H_
#pragma once

#include <queue>
#include <utility>

#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "ipc/message_queue.h"

namespace ipc {

// A simple MessageQueue implementation. It's mainly for testing purpose, and
// can only be run by a ThreadMessageQueueRunner.
class SimpleMessageQueue : public MessageQueue {
 public:
  explicit SimpleMessageQueue(Handler* handler);
  virtual ~SimpleMessageQueue();

  // Overridden from MessageQueue:
  virtual bool Post(proto::Message* message, void* user_data) OVERRIDE;
  virtual bool DoMessage(int* timeout) OVERRIDE;
  virtual bool DoMessageNonexclusive(int* timeout) OVERRIDE;
  virtual void Quit() OVERRIDE;
  virtual boolean InCurrentThread() OVERRIDE;

  // Returns the number of pending messages.
  size_t PendingCount() const;

 private:
  Handler* handler_;

  // A queue to store pending messages.
  std::queue<std::pair<proto::Message*, void*> > queue_;

  // Signaled when a message is posted to the queue.
  base::WaitableEvent event_;

  // Id of the thread creating this message queue.
  base::PlatformThreadId thread_id_;

  mutable base::Lock lock_;

  // Indicates how many levels that DoMessage() has been called recursively.
  int recursive_level_;

  DISALLOW_COPY_AND_ASSIGN(SimpleMessageQueue);
};

}  // namespace ipc

#endif  // GOOPY_IPC_SIMPLE_MESSAGE_QUEUE_H_
