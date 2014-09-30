/*
  Copyright 2008 Google Inc.

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

#include <limits.h>
#include <vector>
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "ggadget/main_loop_interface.h"

// This class can be used to test timer related functions.
class MockedTimerMainLoop : public ggadget::MainLoopInterface {
 public:
  MockedTimerMainLoop(uint64_t time_base)
      : run_depth_(0),
        running_(false),
        current_time_(time_base) {
  }

  virtual int AddIOReadWatch(int fd,
                             ggadget::WatchCallbackInterface *callback) {
    ASSERT_M(false, ("IO watches is not supported by MockedTimerMainLoop"));
    return 0;
  }

  virtual int AddIOWriteWatch(int fd,
                              ggadget::WatchCallbackInterface *callback) {
    ASSERT_M(false, ("IO watches is not supported by MockedTimerMainLoop"));
    return 0;
  }

  virtual int AddTimeoutWatch(int interval,
                              ggadget::WatchCallbackInterface *callback) {
    if (interval < 0 || !callback)
      return -1;
    TimerInfo info = { interval, interval, callback };
    timers_.push_back(info);
    int id = static_cast<int>(timers_.size());
    LOG("MockedTimerMainLoop::AddTimeoutWatch: %d id=%d", interval, id);
    return id;
  }

  virtual WatchType GetWatchType(int watch_id) {
    if (watch_id < 1 || watch_id > static_cast<int>(timers_.size()) ||
        timers_[watch_id - 1].interval != -1)
      return INVALID_WATCH;
    return TIMEOUT_WATCH;
  }

  virtual int GetWatchData(int watch_id) {
    if (watch_id < 1 || watch_id > static_cast<int>(timers_.size()))
      return -1;
    return timers_[watch_id - 1].interval;
  }

  virtual void RemoveWatch(int watch_id) {
    if (watch_id >= 1 && watch_id <= static_cast<int>(timers_.size())) {
      TimerInfo *info = &timers_[watch_id - 1];
      if (info->interval == -1)
        return;

      LOG("MockedTimerMainLoop::RemoveTimeoutWatch: id=%d", watch_id);
      info->interval = -1;
      info->callback->OnRemove(this, watch_id);
      info->callback = NULL;
    }
  }

  // This function provided here only to make the program compile.
  // Unittests should use DoIteration() instead.
  virtual void Run() { ASSERT(false); }

  // Do one iteration. Find the nearest timers to present, fire them and
  // advance the current time.
  virtual bool DoIteration(bool may_block) {
    int min_time = INT_MAX;
    for (Timers::iterator it = timers_.begin(); it != timers_.end(); ++it) {
      if (it->interval != -1 && it->remaining < min_time)
        min_time = it->remaining;
    }

    if (min_time == INT_MAX)
      return false;

    AdvanceTime(min_time);
    return true;
  }

  virtual void Quit() { running_ = false; }
  virtual bool IsRunning() const { return running_; }
  virtual uint64_t GetCurrentTime() const { return current_time_; }
  virtual bool IsMainThread() const { return true; }
  virtual void WakeUp() {}

  // Explicitly advance the current time.
  void AdvanceTime(int time) {
    current_time_ += time;
    int size = static_cast<int>(timers_.size());
    for (int i = 0; i < size; i++) {
      TimerInfo *info = &timers_[i];
      if (info->interval != -1) {
        info->remaining -= time;
        if (info->remaining <= 0) {
          LOG("MockedTimerMainLoop fire timer: %d id=%d", info->interval, i);
          if (!info->callback->Call(this, i))
            RemoveWatch(i);
          else
            info->remaining = info->interval;
        }
      }
    }
  }

  struct TimerInfo {
    int interval;
    int remaining;
    ggadget::WatchCallbackInterface *callback;
  };

  int run_depth_;
  bool running_;
  bool quit_flag_;
  uint64_t current_time_;
  typedef std::vector<TimerInfo> Timers;
  Timers timers_;
};
