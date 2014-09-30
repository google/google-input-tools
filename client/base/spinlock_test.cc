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

//
// Unittest for SpinLock over multi thread and multi process.

#include <windows.h>
#include <psapi.h>
#include "base/commandlineflags.h"
#include "base/scoped_handle_win.h"
#include "base/spinlock.h"
#include "common/hash.h"
#include "common/sharedfile.h"
#include "common/test_util.h"
#include "testing/base/public/gunit.h"

DEFINE_int32(worker_num, 10, "Number of threads");
DEFINE_int32(iters, 1000, "Number of iterations per thread");
DEFINE_bool(worker_process, false,
            "Whether this process is a worker process created by unittest");

static const DWORD kWaitMulliseconds = 5000;
static SpinLock spinlock(LINKER_INITIALIZED);
static const int kArrayLength = 10;
static uint32 values[kArrayLength];
static const wchar_t kTestFile[] = L"spin_lock_file";
static const wchar_t kMappingName[] = L"spin_lock_filemapping";

void Iteration(uint32 id, SpinLock* lock, uint32* value_array) {
  for (int i = 0; i < FLAGS_iters; i++) {
    SpinLockHolder h(lock);
    for (int j = 0; j < kArrayLength; j++) {
      const int index = (j + id) % kArrayLength;
      value_array[index] = HashTo32(value_array[index] + id);
      Sleep(0);
    }
  }
}

DWORD WINAPI WorkerThread(LPVOID param) {
  DWORD tid = GetCurrentThreadId();
  Iteration(tid, &spinlock, values);
  return 0;
}

int WorkerMain() {
  SpinLock* lock;
  ime_goopy::SharedFile shared_file;
  const wstring& filename(ime_goopy::testing::TestUtility::TempFile(kTestFile));
  shared_file.Initialize(filename.c_str(), kMappingName, FALSE, FALSE,
                         sizeof(*lock));
  lock = reinterpret_cast<SpinLock*>(shared_file.GetMappedBase());
  uint32* values = reinterpret_cast<uint32*>(shared_file.GetMappedBase() +
                                             sizeof(*lock));
  DWORD process_id = GetCurrentProcessId();
  Iteration(process_id, lock, values);
  return 0;
}

TEST(SpinLock, SpinLock) {
  scoped_array<HANDLE> threads(new HANDLE[FLAGS_worker_num]);
  for (int i = 0; i < FLAGS_worker_num; ++i) {
    threads[i] = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
  }

  DWORD result = WaitForMultipleObjects(FLAGS_worker_num, threads.get(),
                                        TRUE, kWaitMulliseconds);
  EXPECT_NE(result, WAIT_TIMEOUT);

  for (int i = 0; i < FLAGS_worker_num; ++i) {
    CloseHandle(threads[i]);
  }

  spinlock.Lock();
  for (int i = 1; i < kArrayLength; i++) {
    EXPECT_EQ(values[0], values[i]);
  }
  spinlock.Unlock();
}

TEST(SpinLock, MultiProcess) {
  SpinLock* lock;
  uint32* values;
  const wstring& filename(ime_goopy::testing::TestUtility::TempFile(kTestFile));
  DeleteFileW(filename.c_str());
  ime_goopy::SharedFile shared_file;
  shared_file.Initialize(filename.c_str(), kMappingName, FALSE, TRUE,
                         sizeof(*lock) + sizeof(*values) * kArrayLength);
  lock = new(shared_file.GetMappedBase()) SpinLock();
  values = new(shared_file.GetMappedBase() + sizeof(*lock))
      uint32[kArrayLength];

  scoped_array<HANDLE> worker_process(new HANDLE[FLAGS_worker_num]);
  for (int i = 0; i < FLAGS_worker_num; ++i) {
    STARTUPINFO startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);

    const int kMaxStrLen = 64;
    wchar_t str[kMaxStrLen];
    DWORD size = GetModuleBaseName(GetCurrentProcess(), NULL, str,
                                   kMaxStrLen);
    DCHECK(size);
    wstring command(str, size);
    command += L" --worker_process";
    BOOL ret = CreateProcess(NULL,
                             &*command.begin(),
                             NULL,
                             NULL,
                             TRUE,
                             0,
                             NULL,
                             NULL,
                             &startup,
                             &process);
    DCHECK(ret);
    worker_process[i] = process.hProcess;
    CloseHandle(process.hThread);
  }
  DWORD result = WaitForMultipleObjects(FLAGS_worker_num, worker_process.get(),
                                        TRUE, kWaitMulliseconds);
  EXPECT_NE(result, WAIT_TIMEOUT);
  for (int i = 0; i < FLAGS_worker_num; ++i) {
    CloseHandle(worker_process[i]);
  }

  lock->Lock();
  for (int i = 1; i < kArrayLength; i++) {
    EXPECT_EQ(values[0], values[i]);
  }
  lock->Unlock();
  lock->~SpinLock();
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_worker_process) {
    return WorkerMain();
  }
  return RUN_ALL_TESTS();
}
