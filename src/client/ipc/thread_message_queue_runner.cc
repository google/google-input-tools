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

#include "ipc/thread_message_queue_runner.h"

#include "base/logging.h"
#include "ipc/message_queue.h"

namespace ipc {

////////////////////////////////////////////////////////////////////////////////
// ThreadMessageQueueRunner::Impl implementation.
////////////////////////////////////////////////////////////////////////////////
class ThreadMessageQueueRunner::Impl : public base::PlatformThread::Delegate {
 public:
  explicit Impl(ThreadMessageQueueRunner::Delegate* delegate);
  virtual ~Impl();

  void Run();
  void Quit();
  bool IsRunning() const;
  base::PlatformThreadId GetThreadId() const;

 private:
  // Overridden from base::PlatformThread::Delegate.
  virtual void ThreadMain() OVERRIDE;

  // Our owner.
  ThreadMessageQueueRunner::Delegate* delegate_;

  // The MessageQueue object run by us, but it's owned by |delegate_|.
  MessageQueue* message_queue_;

  // Handle of the dedicated thread.
  base::PlatformThreadHandle thread_handle_;

  // Id of the thread running |message_queue_|.
  base::PlatformThreadId thread_id_;

  // Signaled when the dedicated thread starts to run.
  base::WaitableEvent event_;

  mutable base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

ThreadMessageQueueRunner::Impl::Impl(
    ThreadMessageQueueRunner::Delegate* delegate)
    : delegate_(delegate),
      message_queue_(NULL),
      thread_handle_(base::kNullThreadHandle),
      thread_id_(base::kInvalidThreadId),
      event_(false, false) {
  DCHECK(delegate_);
}

ThreadMessageQueueRunner::Impl::~Impl() {
  Quit();
}

void ThreadMessageQueueRunner::Impl::Run() {
  {
    base::AutoLock auto_lock(lock_);
    if (thread_handle_ != base::kNullThreadHandle)
      return;

    // |message_queue_| should be created from the runner thread.
    const bool success = base::PlatformThread::Create(0, this, &thread_handle_);
    DCHECK(success);
    DCHECK(thread_handle_ != base::kNullThreadHandle);
  }
  event_.Wait();  // Wait for the thread to complete initialization.
}

void ThreadMessageQueueRunner::Impl::Quit() {
  base::PlatformThreadHandle handle = base::kNullThreadHandle;
  {
    base::AutoLock auto_lock(lock_);
    if (thread_handle_ == base::kNullThreadHandle)
      return;

    handle = thread_handle_;

    // Reset thread_handle_ first to prevent Quit() from being called again.
    thread_handle_ = base::kNullThreadHandle;

    // It will quit all recursive calls to |message_queue_->DoMessage()| and
    // |message_queue_->DoMessageNonexclusive()|.
    if (message_queue_)
      message_queue_->Quit();
  }

  base::PlatformThread::Join(handle);

  {
    base::AutoLock auto_lock(lock_);
    if (!message_queue_)
      return;

    // Our thread was already killed by the system before calling Quit()!
    // So we need to do cleanup here.
    delegate_->DestroyMessageQueue(message_queue_);
    message_queue_ = NULL;
    thread_id_ = base::kInvalidThreadId;
  }
  delegate_->RunnerThreadTerminated();
}

bool ThreadMessageQueueRunner::Impl::IsRunning() const {
  base::AutoLock auto_lock(lock_);
  return message_queue_ != NULL;
}

base::PlatformThreadId ThreadMessageQueueRunner::Impl::GetThreadId() const {
  base::AutoLock auto_lock(lock_);
  return thread_id_;
}

void ThreadMessageQueueRunner::Impl::ThreadMain() {
  {
    base::AutoLock auto_lock(lock_);
    DCHECK(!message_queue_);
    message_queue_ = delegate_->CreateMessageQueue();
    DCHECK(message_queue_);
    thread_id_ = base::PlatformThread::CurrentId();
  }
  delegate_->RunnerThreadStarted();
  event_.Signal();

  while (message_queue_->DoMessageNonexclusive(NULL))
    continue;

  {
    base::AutoLock auto_lock(lock_);
    delegate_->DestroyMessageQueue(message_queue_);
    message_queue_ = NULL;
    thread_id_ = base::kInvalidThreadId;
  }
  delegate_->RunnerThreadTerminated();
}


////////////////////////////////////////////////////////////////////////////////
// ThreadMessageQueueRunner implementation.
////////////////////////////////////////////////////////////////////////////////
ThreadMessageQueueRunner::ThreadMessageQueueRunner(Delegate* delegate)
    : impl_(new Impl(delegate)) {
}

ThreadMessageQueueRunner::~ThreadMessageQueueRunner() {
}

void ThreadMessageQueueRunner::Run() {
  impl_->Run();
}

void ThreadMessageQueueRunner::Quit() {
  impl_->Quit();
}

bool ThreadMessageQueueRunner::IsRunning() const {
  return impl_->IsRunning();
}

base::PlatformThreadId ThreadMessageQueueRunner::GetThreadId() const {
  return impl_->GetThreadId();
}

}  // namespace ipc
