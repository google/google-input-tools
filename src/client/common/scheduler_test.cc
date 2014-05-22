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

#include "common/scheduler.h"
#include "testing/base/public/gmock.h"
#include <gtest/gunit.h>

using testing::_;
using testing::AllOf;
using testing::Gt;
using testing::Le;
using testing::Return;

namespace ime_goopy {

class MockScheduler : public Scheduler {
 public:
  MockScheduler() : callback_count_(0), callback_result_(true) {}
  MOCK_METHOD3(CreateTimer,
               HANDLE(JobInfo* job_info, DWORD due_time, DWORD period));
  MOCK_METHOD1(DeleteTimer, void(HANDLE timer));

  bool FakeCallback() {
    callback_count_++;
    return callback_result_;
  }
  int callback_count() { return callback_count_; }
  void set_callback_count(int value) { callback_count_ = value; }
  void set_callback_result(bool value) { callback_result_ = value; }

 private:
  int callback_count_;
  bool callback_result_;
  DISALLOW_EVIL_CONSTRUCTORS(MockScheduler);
};

static const uint32 kDefaultInterval = 100;
// Max interval is 300, so the max interval count should be 2, as shown in
// following tests.
static const uint32 kMaxInterval = 300;
static const HANDLE kHandle = reinterpret_cast<HANDLE>(1);

TEST(SchedulerTest, ImmediatelyStart) {
  MockScheduler scheduler;
  scheduler.AddJob(kDefaultInterval,
                   kMaxInterval,
                   0,
                   NewPermanentCallback(&scheduler,
                                        &MockScheduler::FakeCallback));
  EXPECT_CALL(scheduler, CreateTimer(_, 0, kDefaultInterval))
      .WillOnce(Return(kHandle));
  EXPECT_CALL(scheduler, DeleteTimer(kHandle)).Times(1);
  scheduler.Start();
  scheduler.Stop();
}

TEST(SchedulerTest, DelayedStart) {
  MockScheduler scheduler;
  scheduler.AddJob(kDefaultInterval,
                   kMaxInterval,
                   kDefaultInterval,
                   NewPermanentCallback(&scheduler,
                                        &MockScheduler::FakeCallback));
  EXPECT_CALL(
      scheduler,
      CreateTimer(_, AllOf(Gt(0), Le(kDefaultInterval)), kDefaultInterval))
      .WillOnce(Return(kHandle));
  EXPECT_CALL(scheduler, DeleteTimer(kHandle))
      .Times(1);
  scheduler.Start();
  scheduler.Stop();
}

TEST(SchedulerTest, Backoff) {
  MockScheduler scheduler;
  EXPECT_CALL(scheduler, CreateTimer(_, 0, kDefaultInterval))
      .WillOnce(Return(kHandle));
  EXPECT_CALL(scheduler, DeleteTimer(kHandle)).Times(1);
  scheduler.AddJob(kDefaultInterval,
                   kMaxInterval,
                   0,
                   NewPermanentCallback(&scheduler,
                                        &MockScheduler::FakeCallback));
  scheduler.Start();
  Scheduler::JobInfo* job = scheduler.jobs_[0];

  // No backoff.
  scheduler.set_callback_result(true);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(1, scheduler.callback_count());

  // Backoff once.
  scheduler.set_callback_count(0);
  scheduler.set_callback_result(false);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(1, scheduler.callback_count());

  // Skip this call due to backoff.
  scheduler.set_callback_count(0);
  scheduler.set_callback_result(true);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(0, scheduler.callback_count());

  // Process this call after skip once.
  scheduler.set_callback_count(0);
  scheduler.set_callback_result(true);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(1, scheduler.callback_count());

  // After a successful call, there will be no backoff.
  scheduler.set_callback_count(0);
  scheduler.set_callback_result(true);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(1, scheduler.callback_count());

  // Continued backoff.
  scheduler.set_callback_count(0);
  scheduler.set_callback_result(false);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(1, scheduler.callback_count());

  // Skip one call.
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(1, scheduler.callback_count());

  // Processed.
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(2, scheduler.callback_count());

  // Skip two calls.
  Scheduler::TimerCallback(job, TRUE);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(2, scheduler.callback_count());

  // Processed.
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(3, scheduler.callback_count());

  // Due to the max interval, still skip two calls.
  Scheduler::TimerCallback(job, TRUE);
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(3, scheduler.callback_count());

  // Processed.
  Scheduler::TimerCallback(job, TRUE);
  EXPECT_EQ(4, scheduler.callback_count());

  scheduler.Stop();
}
}  // namespace ime_goopy
