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

#ifndef GOOPY_COMMON_SCHEDULER_H_
#define GOOPY_COMMON_SCHEDULER_H_

#include <vector>

#include "base/basictypes.h"
#include "base/callback.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
// Scheduler is a timer, call registered callback at a given interval.
// Besides a simple timer, Scheduler added the following featues:
//   1. Backoff when the registered callback returns false.
//   2. Delayed start the timer, to reduce server traffic peak.
class Scheduler {
 public:
  typedef ResultCallback<bool> JobCallback;

  Scheduler();
  virtual ~Scheduler();

  // All interval values are in seconds.
  void AddJob(uint32 default_interval,
              uint32 max_interval,
              uint32 delay_start,
              JobCallback* callback);
  bool Start();
  // Stop will wait for all callback to complete before return. When you can't
  // wait, for example, when the system is shutting down and the daemon is
  // still running, you can ignore calling Stop. The timer will be terminated
  // anyway by system, and allocated memory will be collected too. After Stop,
  // all added job will be cleared.
  void Stop();

 protected:
  struct JobInfo {
    uint32 default_interval;
    uint32 max_interval;
    uint32 delay_start;
    JobCallback* callback;
    uint32 skip_count;
    uint32 backoff_count;
    HANDLE timer;
    bool running;
  };
  virtual HANDLE CreateTimer(JobInfo* job_info, DWORD due_time, DWORD period);
  virtual void DeleteTimer(HANDLE timer);

 private:
  static void CALLBACK TimerCallback(void* param, BOOLEAN timer_or_wait);

  vector<JobInfo*> jobs_;
  bool running_;
  FRIEND_TEST(SchedulerTest, Backoff);
  DISALLOW_EVIL_CONSTRUCTORS(Scheduler);
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_SCHEDULER_H_
