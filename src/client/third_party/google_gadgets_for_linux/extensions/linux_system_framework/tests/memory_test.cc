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

#include <stdio.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <unittest/gtest.h>
#include "../memory.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(Memory, GetTotal) {
  Memory memory;
  int64_t total = memory.GetTotal();
  EXPECT_GT(total, 0);
  LOG("total memory: %jd", total);
}

TEST(Memory, GetFree) {
  Memory memory;
  int64_t free = memory.GetFree();
  int64_t total = memory.GetFree();
  EXPECT_GT(free, 0);
  EXPECT_GE(total, free);
  LOG("free memory: %jd", free);
}

TEST(Memory, GetUsed) {
  Memory memory;
  int64_t free = memory.GetFree();
  int64_t total = memory.GetTotal();
  int64_t used = memory.GetUsed();
  EXPECT_GT(used, 0);
  EXPECT_GE(total, used);
  EXPECT_EQ(total, used + free);
  LOG("used memory: %jd", used);
}

TEST(Memory, GetFreePhysical) {
  Memory memory;
  int64_t free_physical = memory.GetFreePhysical();
  EXPECT_GT(free_physical, 0);
  LOG("free physical memory: %jd", free_physical);
}

TEST(Memory, GetTotalPhysical) {
  Memory memory;
  int64_t total_physical = memory.GetTotalPhysical();
  int64_t free_physical = memory.GetFreePhysical();
  EXPECT_GT(total_physical, 0);
  EXPECT_GE(total_physical, free_physical);
  LOG("total physical memory: %jd", total_physical);
}

TEST(Memory, GetUsedPhysical) {
  Memory memory;
  int64_t total_physical = memory.GetTotalPhysical();
  int64_t free_physical = memory.GetFreePhysical();
  int64_t used_physical = memory.GetUsedPhysical();
  EXPECT_GT(total_physical, 0);
  EXPECT_GE(total_physical, free_physical);
  EXPECT_EQ(total_physical, free_physical + used_physical);
  LOG("used physical memory: %jd", used_physical);
}


int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
