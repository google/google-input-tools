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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_MEMORY_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_MEMORY_H__

#include <ggadget/framework_interface.h>

namespace ggadget {
namespace framework {
namespace linux_system {

class Memory : public MemoryInterface {
 public:
  Memory();
  virtual int64_t GetTotal();
  virtual int64_t GetFree();
  virtual int64_t GetUsed();
  virtual int64_t GetFreePhysical();
  virtual int64_t GetTotalPhysical();
  virtual int64_t GetUsedPhysical();

 private:
  /**
   * Refreshes the memory information.
   */
  void Refresh();

  /**
   * Retrieves the corresponding information from the proc file MemInfo.
   */
  void ReadMemInfoFromProc();

 private:
  enum {
    TOTAL_PHYSICAL,
    FREE_PHYSICAL,
    TOTAL_SWAP,
    FREE_SWAP,
    BUFFERS,
    CACHED,
    SWAP_CACHED,
    MEM_INFO_COUNT
  };

  int64_t mem_info_[MEM_INFO_COUNT];
  time_t timestamp_;

};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_MEMORY_H__
