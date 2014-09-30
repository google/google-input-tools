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

#include <time.h>

#include "common/scheduler.h"
#include "base/logging.h"

namespace ime_goopy {

Scheduler::Scheduler() : running_(false) {
}

Scheduler::~Scheduler() {
}

void Scheduler::AddJob(uint32 default_interval,
                       uint32 max_interval,
                       uint32 delay_start,
                       JobCallback* callback) {
  assert(default_interval);
  assert(max_interval);
  assert(callback);
  assert(!running_);

  JobInfo* info = new JobInfo;
  ZeroMemory(info, sizeof(*info));
  info->default_interval = default_interval;
  info->max_interval = max_interval;
  info->delay_start = delay_start;
  info->callback = callback;

  jobs_.push_back(info);
}

bool Scheduler::Start() {
  assert(!running_);
  srand(static_cast<unsigned int>(time(NULL)));
  running_ = true;
  bool all_success = true;
  for (int i = 0; i < jobs_.size(); i++) {
    uint32 delay = 0;
    if (jobs_[i]->delay_start)
      delay = MulDiv(rand(), jobs_[i]->delay_start, RAND_MAX);
    jobs_[i]->timer = CreateTimer(jobs_[i], delay, jobs_[i]->default_interval);
    if (!jobs_[i]->timer) all_success = false;
  }
  return all_success;
}

void Scheduler::Stop() {
  assert(running_);
  running_ = false;
  for (int i = 0; i < jobs_.size(); i++) {
    if (!jobs_[i]->timer) continue;
    DeleteTimer(jobs_[i]->timer);
    jobs_[i]->timer = NULL;
    jobs_[i]->skip_count = 0;
    jobs_[i]->backoff_count = 0;
    delete jobs_[i]->callback;
    delete jobs_[i];
  }
  jobs_.clear();
}

void Scheduler::TimerCallback(void* param, BOOLEAN timer_or_wait) {
  assert(param);
  JobInfo* job = reinterpret_cast<JobInfo*>(param);
  if (job->running) return;
  if (job->skip_count) {
    job->skip_count--;
    DLOG(INFO) << "Backoff, " << job->skip_count << " times left...";
    return;
  }
  job->running = true;
  CoInitialize(NULL);
  bool success = job->callback->Run();
  CoUninitialize();
  job->running = false;
  if (success) {
    job->backoff_count = 0;
  } else {
    uint32 new_backoff_count =
      (job->backoff_count == 0) ? 1 : job->backoff_count * 2;
    if (new_backoff_count * job->default_interval < job->max_interval)
      job->backoff_count = new_backoff_count;
    job->skip_count = job->backoff_count;
  }
  // Reduce memory usage.
  SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
}

HANDLE Scheduler::CreateTimer(JobInfo* job_info, DWORD due_time, DWORD period) {
  HANDLE new_timer = NULL;
  bool success = CreateTimerQueueTimer(&new_timer,
                                       NULL,
                                       &Scheduler::TimerCallback,
                                       reinterpret_cast<void*>(job_info),
                                       due_time * 1000,
                                       period * 1000,
                                       WT_EXECUTELONGFUNCTION);
  if (!success) return NULL;
  return new_timer;
}

void Scheduler::DeleteTimer(HANDLE timer) {
  // Wait for the callback to complete before return.
  DeleteTimerQueueTimer(NULL, timer, INVALID_HANDLE_VALUE);
}

}  // namespace ime_sync
