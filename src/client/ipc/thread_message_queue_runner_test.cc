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

#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/time.h"
#include "ipc/simple_message_queue.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace {

class ThreadMessageQueueRunnerTest
    : public ::testing::Test,
      public ipc::ThreadMessageQueueRunner::Delegate,
      public ipc::MessageQueue::Handler {
 protected:
  ThreadMessageQueueRunnerTest()
      : thread_id_(base::PlatformThread::CurrentId()),
        runner_started_(false),
        runner_terminated_(false),
        recursive_(false),
        recursive_timeout_(0),
        message_count_(0),
        recursive_count_(0),
        timeout_event_(true, false) {
  }

  virtual ~ThreadMessageQueueRunnerTest() {
  }

  virtual void SetUp() {
    runner_.reset(new ipc::ThreadMessageQueueRunner(this));
  }

  virtual void TearDown() {
    runner_->Quit();
    ASSERT_FALSE(runner_->IsRunning());
    runner_.reset();
  }

  // Overridden from ipc::ThreadMessageQueueRunner::Delegate:
  virtual ipc::MessageQueue* CreateMessageQueue() OVERRIDE {
    queue_.reset(new ipc::SimpleMessageQueue(this));
    return queue_.get();
  }

  virtual void DestroyMessageQueue(ipc::MessageQueue* queue) OVERRIDE {
    ASSERT_EQ(queue_.get(), queue);
    ASSERT_EQ(0, queue_->PendingCount());
    queue_.reset();
  }

  virtual void RunnerThreadStarted() OVERRIDE {
    runner_started_ = true;
    ASSERT_NE(base::kInvalidThreadId, runner_->GetThreadId());
    ASSERT_NE(thread_id_, runner_->GetThreadId());
  }

  virtual void RunnerThreadTerminated() OVERRIDE {
    runner_terminated_ = true;
    ASSERT_EQ(base::kInvalidThreadId, runner_->GetThreadId());
  }

  // Overridden from ipc::MessageQueue::Handler:
  virtual void HandleMessage(ipc::proto::Message* message,
                             void* data) OVERRIDE {
    ASSERT_TRUE(message);
    ASSERT_TRUE(data);
    delete message;

    message_count_++;
    if (recursive_) {
      int timeout = recursive_timeout_;
      bool success = queue_->DoMessage(timeout > 0 ? &timeout : NULL);
      if (recursive_timeout_ > 0)
        ASSERT_TRUE(timeout <= recursive_timeout_);

      if (success)
        recursive_count_++;
      else if (timeout == 0)
        timeout_event_.Signal();
    }
  }

  scoped_ptr<ipc::SimpleMessageQueue> queue_;
  scoped_ptr<ipc::ThreadMessageQueueRunner> runner_;

  base::PlatformThreadId thread_id_;
  bool runner_started_;
  bool runner_terminated_;
  bool recursive_;
  int recursive_timeout_;
  int message_count_;
  int recursive_count_;

  base::WaitableEvent timeout_event_;
};

TEST_F(ThreadMessageQueueRunnerTest, NormalDispatch) {
  runner_->Run();
  ASSERT_TRUE(queue_.get());
  ASSERT_TRUE(runner_->IsRunning());
  ASSERT_TRUE(runner_started_);

  for (int i = 0; i < 100; ++i)
    ASSERT_TRUE(queue_->Post(new ipc::proto::Message(), this));

  runner_->Quit();
  ASSERT_TRUE(runner_terminated_);
  ASSERT_FALSE(runner_->IsRunning());
  ASSERT_EQ(100, message_count_);
}

TEST_F(ThreadMessageQueueRunnerTest, RecursiveNoTimeout) {
  recursive_ = true;
  recursive_timeout_ = -1;

  runner_->Run();
  ASSERT_TRUE(queue_.get());
  ASSERT_TRUE(runner_->IsRunning());
  ASSERT_TRUE(runner_started_);

  for (int i = 0; i < 100; ++i)
    ASSERT_TRUE(queue_->Post(new ipc::proto::Message(), this));

  runner_->Quit();
  ASSERT_TRUE(runner_terminated_);
  ASSERT_FALSE(runner_->IsRunning());
  ASSERT_EQ(100, message_count_);
  ASSERT_EQ(99, recursive_count_);
}

TEST_F(ThreadMessageQueueRunnerTest, RecursiveWithTimeout) {
  recursive_ = true;
  recursive_timeout_ = 20;

  runner_->Run();
  ASSERT_TRUE(queue_.get());
  ASSERT_TRUE(runner_->IsRunning());
  ASSERT_TRUE(runner_started_);

  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(queue_->Post(new ipc::proto::Message(), this));
    ASSERT_FALSE(
        timeout_event_.TimedWait(base::TimeDelta::FromMilliseconds(5)));
    ASSERT_TRUE(
        timeout_event_.TimedWait(base::TimeDelta::FromMilliseconds(20)));
    timeout_event_.Reset();
  }

  runner_->Quit();
  ASSERT_TRUE(runner_terminated_);
  ASSERT_FALSE(runner_->IsRunning());
  ASSERT_EQ(10, message_count_);
  ASSERT_EQ(0, recursive_count_);
}

TEST_F(ThreadMessageQueueRunnerTest, Restart) {
  runner_->Run();
  ASSERT_TRUE(queue_.get());
  ASSERT_TRUE(runner_->IsRunning());
  ASSERT_TRUE(runner_started_);

  ASSERT_TRUE(queue_->Post(new ipc::proto::Message(), this));

  runner_->Quit();
  ASSERT_FALSE(queue_.get());
  ASSERT_TRUE(runner_terminated_);
  ASSERT_FALSE(runner_->IsRunning());
  ASSERT_EQ(1, message_count_);

  runner_started_ = false;
  runner_terminated_ = false;
  message_count_ = 0;

  runner_->Run();
  ASSERT_TRUE(queue_.get());
  ASSERT_TRUE(runner_->IsRunning());
  ASSERT_TRUE(runner_started_);

  ASSERT_TRUE(queue_->Post(new ipc::proto::Message(), this));

  runner_->Quit();
  ASSERT_FALSE(queue_.get());
  ASSERT_TRUE(runner_terminated_);
  ASSERT_FALSE(runner_->IsRunning());
  ASSERT_EQ(1, message_count_);
}

}  // namespace
