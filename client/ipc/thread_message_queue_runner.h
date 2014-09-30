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

#ifndef GOOPY_IPC_THREAD_MESSAGE_QUEUE_RUNNER_H_
#define GOOPY_IPC_THREAD_MESSAGE_QUEUE_RUNNER_H_
#pragma once

#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/cancellation_flag.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"

namespace ipc {

class MessageQueue;

// A class to run a MessageQueue in a dedicated thread.
class ThreadMessageQueueRunner {
 public:
  // An interface that should be implemented by the class creating and owning a
  // ThreadMessageQueueRunner.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Creates a MessageQueue. The newly created MessageQueue is owned by the
    // delegate. This method will be called from the dedicated thread, and
    // should only be called once.
    virtual MessageQueue* CreateMessageQueue() = 0;

    // Destroys the MessageQueue created by CreateMessageQueue() method.
    // This method will be called from the dedicated thread, and should only be
    // called once. This method will be called from the main thread if the
    // dedicated thread was already killed before calling Quit().
    virtual void DestroyMessageQueue(MessageQueue* queue) = 0;

    // Called when the runner thread is being started. It'll be called from
    // the runner thread just before running the message queue.
    virtual void RunnerThreadStarted() {}

    // Called when the runner thread is being terminated. It'll be called from
    // the runner thread just before exiting. This method will be called from
    // the main thread if the dedicated thread is already killed before calling
    // Quit().
    virtual void RunnerThreadTerminated() {}
  };

  explicit ThreadMessageQueueRunner(Delegate* delegate);

  ~ThreadMessageQueueRunner();

  // Creates the runner thread and start to run it. It does nothing if the
  // runner thread is already running.
  void Run();

  // Quits the runner thread. Nothing happens if the thread is not created
  // yet. It'll block until the thread is quitted successfully. It must be
  // called from a thread other than the runner thread.
  void Quit();

  // Checks if the runner thread is running.
  bool IsRunning() const;

  // Returns id of the dedicated thread.
  base::PlatformThreadId GetThreadId() const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(ThreadMessageQueueRunner);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_QUEUE_H_
