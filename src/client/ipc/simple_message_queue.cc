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

#include "ipc/simple_message_queue.h"

#include <algorithm>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/time.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

SimpleMessageQueue::SimpleMessageQueue(Handler* handler)
    : handler_(handler),
      event_(false, false),
      thread_id_(base::PlatformThread::CurrentId()),
      recursive_level_(0) {
  DCHECK(handler_);
}

SimpleMessageQueue::~SimpleMessageQueue() {
  base::AutoLock auto_lock(lock_);
  DCHECK_EQ(0, recursive_level_);
  // Delete all pending messages to avoid memory leak.
  while (!queue_.empty()) {
    delete queue_.front().first;
    queue_.pop();
  }
}

bool SimpleMessageQueue::Post(proto::Message* message, void* user_data) {
  scoped_ptr<proto::Message> mptr(message);
  {
    base::AutoLock auto_lock(lock_);
    // Do not allow to push more messages after calling Quit().
    if (!queue_.empty() && queue_.back().first == NULL)
      return false;
    queue_.push(std::make_pair(mptr.release(), user_data));
  }
  event_.Signal();
  return true;
}

bool SimpleMessageQueue::DoMessage(int* timeout) {
  DCHECK_EQ(thread_id_, base::PlatformThread::CurrentId());

  const base::TimeTicks start_time = base::TimeTicks::Now();
  const int64 total_timeout = (timeout ? *timeout : -1);
  int64 remained_timeout = total_timeout;

  base::AutoLock auto_lock(lock_);
  ++recursive_level_;
  if (total_timeout != 0) {
    while (queue_.empty()) {
      base::AutoUnlock auto_unlock(lock_);
      if (total_timeout > 0) {
        event_.TimedWait(base::TimeDelta::FromMilliseconds(remained_timeout));
        remained_timeout = total_timeout -
            (base::TimeTicks::Now() - start_time).InMilliseconds();
        if (remained_timeout <= 0)
          break;
      } else {
        event_.Wait();
      }
    }
  }

  if (timeout && total_timeout > 0)
    *timeout = std::max(static_cast<int64>(0), remained_timeout);

  proto::Message* message = NULL;
  void* user_data = NULL;
  if (!queue_.empty()) {
    message = queue_.front().first;
    user_data = queue_.front().second;

    // Do not pop NULL message if we are in a recursive call to make sure all
    // outer calls can quit correctly.
    if (message || recursive_level_ == 1)
      queue_.pop();
  }

  if (message) {
    base::AutoUnlock auto_unlock(lock_);
    handler_->HandleMessage(message, user_data);
  }

  --recursive_level_;
  return message != NULL;
}

bool SimpleMessageQueue::DoMessageNonexclusive(int* timeout) {
  // Disallow this function to be called recursively.
  DCHECK(!recursive_level_);
  return DoMessage(timeout);
}

void SimpleMessageQueue::Quit() {
  Post(NULL, NULL);
}

boolean SimpleMessageQueue::InCurrentThread() {
  return thread_id_ == base::PlatformThread::CurrentId();
}

size_t SimpleMessageQueue::PendingCount() const {
  base::AutoLock auto_lock(lock_);
  return queue_.size();
}

}  // namespace ipc
