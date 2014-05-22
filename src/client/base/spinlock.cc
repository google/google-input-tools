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

#include "base/spinlock.h"
#include "common/shellutils.h"

static int adaptive_spin_count = 0;

const LinkerInitialized SpinLock::LINKER_INITIALIZED = LINKER_INITIALIZED;

namespace {
struct SpinLock_InitHelper {
  SpinLock_InitHelper() {
    // On multi-cpu machines, spin for longer before yielding
    // the processor or sleeping.  Reduces idle time significantly.
    if (ime_goopy::ShellUtils::NumCPUs() > 1) {
      adaptive_spin_count = 1000;
    }
  }
};

// Hook into global constructor execution:
// We do not do adaptive spinning before that,
// but nothing lock-intensive should be going on at that time.
static SpinLock_InitHelper init_helper;

// Ported from //depot/google3/base/spinlock_win32-inl.h
static void SpinLockWait(volatile intptr_t* w) {
  if (*w != 0) {
    Sleep(0);
  }
  while (InterlockedCompareExchangePointerAcquire(
             reinterpret_cast<volatile PVOID*>(w),
             reinterpret_cast<PVOID>(1),
             reinterpret_cast<PVOID>(0)) != 0) {
    Sleep(1);
  }
}

static void SpinLockWake(volatile intptr_t* w) {
}

}  // unnamed namespace

void SpinLock::SlowLock() {
  int c = adaptive_spin_count;

  // Spin a few times in the hope that the lock holder releases the lock
  while ((c > 0) && (lockword_ != 0)) {
    c--;
  }

  // This if-statement is for spinlock profiling (contentionz).
  // This code assumes QueryPerformanceCounter is synchronized across
  // processors.
  if (lockword_ == 1) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    int32 now = counter.QuadPart >> PROFILE_TIMESTAMP_SHIFT;
    // Don't loose the lock: make absolutely sure "now" is not zero
    now |= 1;
    // Atomically replace the value of lockword_ with "now" if
    // lockword_ is 1, thereby remembering the first timestamp to
    // be recorded.
    InterlockedCompareExchangePointer(
        reinterpret_cast<volatile PVOID*>(&lockword_),
        reinterpret_cast<PVOID>(static_cast<uintptr_t>(now)),
        reinterpret_cast<PVOID>(1));
    // InterlockedCompareExchangePointer returns:
    //   0: the lock is/was available; nothing stored
    //   1: our timestamp was stored
    //   > 1: an older timestamp is already in lockword_; nothing stored
  }

  SpinLockWait(&lockword_);  // wait until lock acquired; OS specific
}

void SpinLock::SlowUnlock(int64 wait_timestamp) {
  SpinLockWake(&lockword_);  // wake waiter if necessary; OS specific
}
