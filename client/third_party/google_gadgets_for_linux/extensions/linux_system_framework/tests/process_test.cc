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

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <unittest/gtest.h>
#include "../process.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(Process, EnumerateProcesses) {
  Process process;
  ProcessesInterface *processes = process.EnumerateProcesses();
  ASSERT_TRUE(processes != NULL);
  EXPECT_GT(processes->GetCount(), 0);
  LOG("The total count of process: %d", processes->GetCount());
  processes->Destroy();
}

TEST(Process, GetForeground) {
  Process process;
  ProcessInfoInterface *fore_process = process.GetForeground();
  ASSERT_TRUE(fore_process != NULL);
}

TEST(Process, GetInfo) {
  pid_t pid = getpid();
  Process process;
  ProcessInfoInterface *proc_2 = process.GetInfo(pid);
  ASSERT_TRUE(proc_2 != NULL);
  EXPECT_EQ(proc_2->GetProcessId(), pid);
  EXPECT_NE("", proc_2->GetExecutablePath());
  proc_2->Destroy();
  ASSERT_TRUE(process.GetInfo(0) == NULL);
}

// test Process class using the method EnumerateProcesses
TEST(Processes, GetCount1) {
  Process process;
  ProcessesInterface *processes = process.EnumerateProcesses();
  ASSERT_TRUE(processes != NULL);
  EXPECT_GT(processes->GetCount(), 0);
  processes->Destroy();
}

// test Process class using its own constructor
TEST(Processes, GetCount2) {
  Processes* processes = new Processes();
  ASSERT_TRUE(processes != NULL);
  EXPECT_GT(processes->GetCount(), 0);
  processes->Destroy();
}

// test Process class using the method EnumerateProcesses
TEST(Processes, GetItem1) {
  Process process;
  ProcessesInterface *processes = process.EnumerateProcesses();
  ASSERT_TRUE(processes != NULL);
  EXPECT_GT(processes->GetCount(), 0);
  ProcessInfoInterface *item = processes->GetItem(0);
  ASSERT_TRUE(item != NULL);
  EXPECT_TRUE(item->GetProcessId() > 0);
  LOG("The item's process id: %d", item->GetProcessId());
  item->Destroy();
  processes->Destroy();
}

// test Process class using its own constructor
TEST(Processes, GetItem2) {
  Processes* processes = new Processes();
  ASSERT_TRUE(processes != NULL);
  EXPECT_GT(processes->GetCount(), 0);
  ProcessInfoInterface *item = processes->GetItem(0);
  ASSERT_TRUE(item != NULL);
  EXPECT_TRUE(item->GetProcessId() > 0);
  LOG("The item's process id: %d", item->GetProcessId());
  item->Destroy();
  processes->Destroy();
}

// test Process class using its own constructor
TEST(ProcessInfo, GetProcessIdAndGetExecutablePath2) {
  int pid = 255;
  std::string path = "/usr/bin/eclipse";
  ProcessInfo *proc = new ProcessInfo(pid, path);
  ASSERT_TRUE(proc != NULL);
  EXPECT_EQ(proc->GetProcessId(), pid);
  EXPECT_TRUE(proc->GetExecutablePath() == path);
  LOG("The process id: %d", proc->GetProcessId());
  LOG("The executable path: %s", proc->GetExecutablePath().c_str());
  proc->Destroy();
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
