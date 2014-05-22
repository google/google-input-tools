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

#include "perfmon.h"

#include <algorithm>
#include <map>
#include <cmath>
#include <cstring>
#include <ggadget/common.h>
#include <ggadget/light_map.h>
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace framework {
namespace linux_system {

// Internal structure for holding real-time CPU statistics.
// All fields in this structure are measured in units of USER_HZ.
struct CpuStat {
  CpuStat()
      : user(0),
        nice(0),
        system(0),
        idle(0),
        iowait(0),
        hardirq(0),
        softirq(0),
        steal(0),
        uptime(0),
        worktime(0) {}
  int64_t user;
  int64_t nice;
  int64_t system;
  int64_t idle;
  int64_t iowait;
  int64_t hardirq;
  int64_t softirq;
  int64_t steal;
  int64_t uptime;
  int64_t worktime;
};

// the threshold for distinguish different cpu usage value.
static const double kCpuUsageThreshold = 0.001;
// the max length of a line
static const int kMaxLineLength = 1024;
// the time interval for time out watch (milliseconds)
static const int kUpdateInterval = 2000;

// the filename for CPU state in proc system
static const char kProcStatFile[] = "/proc/stat";
// the cpu usage operation
static const char kPerfmonCpuUsage[] = "\\Processor(_Total)\\% Processor Time";
// the cpu state head name
static const char kCpuHeader[] = "cpu";

// the current cpu usage status
static CpuStat current_cpu_status;
// the last cpu usage status
static CpuStat last_cpu_status;

// Gets the current cpu usage
static double GetCurrentCpuUsage() {
  FILE* fp = fopen(kProcStatFile, "rt");
  if (!fp)
    return 0.0;

  char line[kMaxLineLength];

  if (!fgets(line, kMaxLineLength, fp)) {
    fclose(fp);
    return 0.0;
  }

  fclose(fp);

  if (strlen(line) < arraysize(kCpuHeader))
    return 0.0;

  if (!strncmp(line, kCpuHeader, arraysize(kCpuHeader) - 1)) {
    sscanf(line + arraysize(kCpuHeader),
           "%jd %jd %jd %jd %jd %jd %jd",
           &current_cpu_status.user,
           &current_cpu_status.nice,
           &current_cpu_status.system,
           &current_cpu_status.idle,
           &current_cpu_status.iowait,
           &current_cpu_status.hardirq,
           &current_cpu_status.softirq);

    // calculates the cpu total time
    current_cpu_status.uptime =
      current_cpu_status.user + current_cpu_status.nice +
      current_cpu_status.system + current_cpu_status.idle +
      current_cpu_status.iowait + current_cpu_status.hardirq +
      current_cpu_status.softirq;

    // calculates the cpu working time
    current_cpu_status.worktime =
          current_cpu_status.user + current_cpu_status.nice +
          current_cpu_status.system + current_cpu_status.hardirq +
          current_cpu_status.softirq;

    // calculates percentage of cpu usage
    int64_t current_work_time = current_cpu_status.worktime
                                - last_cpu_status.worktime;

    int64_t current_total_time = current_cpu_status.uptime
                                 - last_cpu_status.uptime;

    // update the last cpu status
    last_cpu_status = current_cpu_status;

    return current_total_time
            ? (double) current_work_time / (double) current_total_time
            : 0.0;
  }

  return 0.0;
}

class CpuUsageWatch : public WatchCallbackInterface {
 public:
  CpuUsageWatch()
    : watch_id_(-1),
      current_cpu_usage_(0.0) {
  }

  ~CpuUsageWatch() {
    for (SlotMap::iterator it = slots_.begin(); it != slots_.end(); ++it)
      delete it->second;
    if (watch_id_ >= 0)
      GetGlobalMainLoop()->RemoveWatch(watch_id_);
  }

  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    double last = current_cpu_usage_;
    current_cpu_usage_ = GetCurrentCpuUsage();

    if (std::abs(current_cpu_usage_ - last) >= kCpuUsageThreshold) {
      Variant usage(current_cpu_usage_ * 100.0);
      for (SlotMap::iterator it = slots_.begin(); it != slots_.end(); ++it)
        (*it->second)(kPerfmonCpuUsage, usage);
    }

    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    // In case the main loop is destroyed before the watch.
    watch_id_ = -1;
  }

  void AddCounter(int index, PerfmonInterface::CallbackSlot *slot) {
    SlotMap::iterator it = slots_.find(index);
    if (it != slots_.end())
      delete it->second;

    slots_[index] = slot;

    // Add timeout watch only when there is any counter added.
    if (watch_id_ < 0)
      watch_id_ = GetGlobalMainLoop()->AddTimeoutWatch(kUpdateInterval, this);
  }

  void RemoveCounter(int index) {
    SlotMap::iterator it = slots_.find(index);
    if (it != slots_.end()) {
      delete it->second;
      slots_.erase(it);
    }

    // Remove watch if there is no more counter.
    if (!slots_.size() && watch_id_ >= 0) {
      GetGlobalMainLoop()->RemoveWatch(watch_id_);
      watch_id_ = -1;
    }
  }

  double GetCurrentValue() {
    // If the timeout watch is added, then just return stored value.
    if (watch_id_ >= 0)
      return current_cpu_usage_ * 100.0;

    return GetCurrentCpuUsage() * 100.0;
  }

 private:
  int watch_id_;
  double current_cpu_usage_;

  typedef LightMap<int, PerfmonInterface::CallbackSlot *> SlotMap;
  SlotMap slots_;
};

class Perfmon::Impl {
 public:
  Impl() : counter_index_(0) { }

  int counter_index_;
  CpuUsageWatch cpu_usage_watch_;
};


Perfmon::Perfmon()
  : impl_(new Impl()) {
}

Perfmon::~Perfmon() {
  delete impl_;
}

Variant Perfmon::GetCurrentValue(const char *counter_path) {
  double value = 0;
  if (counter_path && !strcmp(counter_path, kPerfmonCpuUsage)) {
    value = impl_->cpu_usage_watch_.GetCurrentValue();
  }

  return Variant(value);
}

int Perfmon::AddCounter(const char *counter_path, CallbackSlot *slot) {
  if (slot && counter_path && !strcmp(counter_path, kPerfmonCpuUsage)) {
    // In case the counter_index_ is wrapped.
    if (impl_->counter_index_ < 0) impl_->counter_index_ = 0;

    int index = impl_->counter_index_++;
    impl_->cpu_usage_watch_.AddCounter(index, slot);
    return index;
  }

  delete slot;
  return -1;
}

void Perfmon::RemoveCounter(int id) {
  impl_->cpu_usage_watch_.RemoveCounter(id);
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
