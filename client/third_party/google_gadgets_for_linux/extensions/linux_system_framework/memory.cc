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

#include <cstdlib>
#include <string>
#include <cstdio>
#include <sys/time.h>
#include <ggadget/string_utils.h>
#include "memory.h"

namespace ggadget {
namespace framework {
namespace linux_system {

// Represents the time interval for refreshing the memory info in seconds.
static time_t kTimeInterval = 2;

// Represents the memory info file name.
static const char* kMemInfoFile = "/proc/meminfo";

// Represents the keys for memory info in proc file system.
static const char* kKeysInMemInfo[] = {
  "MemTotal", "MemFree", "SwapTotal",
  "SwapFree", "Buffers", "Cached", "SwapCached"
};

Memory::Memory() {
  timestamp_ = 0;
}

int64_t Memory::GetTotal() {
  Refresh();
  return mem_info_[TOTAL_PHYSICAL] + mem_info_[TOTAL_SWAP];
}

int64_t Memory::GetFree() {
  Refresh();
  return mem_info_[FREE_PHYSICAL] + mem_info_[BUFFERS] + mem_info_[CACHED] +
         mem_info_[SWAP_CACHED] + mem_info_[FREE_SWAP];
}

int64_t Memory::GetUsed() {
  return GetTotal() - GetFree();
}

int64_t Memory::GetFreePhysical() {
  Refresh();

  // here: free physical memory = free + buffer + cache + swap_cache
  return mem_info_[FREE_PHYSICAL] + mem_info_[BUFFERS] + mem_info_[CACHED] +
         mem_info_[SWAP_CACHED];
}

int64_t Memory::GetTotalPhysical() {
  Refresh();
  return mem_info_[TOTAL_PHYSICAL];
}

int64_t Memory::GetUsedPhysical() {
  return GetTotalPhysical() - GetFreePhysical();
}

void Memory::Refresh() {
  timeval tv;

  // if time interval is less than 2 seconds, just return
  if (!gettimeofday(&tv, NULL) && tv.tv_sec - timestamp_ <= kTimeInterval)
    return;

  ReadMemInfoFromProc();

  // update the time stamp
  timestamp_ = tv.tv_sec;
}

void Memory::ReadMemInfoFromProc() {
  FILE* fp = fopen(kMemInfoFile, "r");
  if (!fp)
    return;

  char line[1001]; // big enough to hold the line
  std::string key, value;

  // search the MemInfo file and get the expected memory value
  while (fgets(line, sizeof(line), fp)) {
    if (!SplitString(line, ":", &key, &value))
      continue;

    key = TrimString(key);
    value = TrimString(value);

    for (int i = 0; i < MEM_INFO_COUNT; ++i) {
      if (key == kKeysInMemInfo[i]) {
        mem_info_[i] = strtoll(value.c_str(), NULL, 10) * 1024;
        break;
      }
    }
  }

  fclose(fp);
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
