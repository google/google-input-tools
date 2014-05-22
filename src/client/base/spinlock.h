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
// Fast spinlocks (at least on x86, a lock/unlock pair is approximately
// half the cost of a Mutex because the unlock just does a store instead
// of a compare-and-swap which is expensive).

// SpinLock is async signal safe.
// If used within a signal handler, all lock holders
// should block the signal even outside the signal handler.

#ifndef GOOPY_BASE_SPINLOCK_H_
#define GOOPY_BASE_SPINLOCK_H_

#include "base/basictypes.h"

enum LinkerInitialized { LINKER_INITIALIZED };

class SpinLock {
 public:
  SpinLock() : lockword_(0) { }

  // Special constructor for use with static SpinLock objects.  E.g.,
  //
  //    static SpinLock lock(base::LINKER_INITIALIZED);
  //
  // When intialized using this constructor, we depend on the fact
  // that the linker has already initialized the memory appropriately.
  // A SpinLock constructed like this can be freely used from global
  // initializers without worrying about the order in which global
  // initializers run.
  explicit SpinLock(LinkerInitialized /*x*/) {
    // Does nothing; lockword_ is already initialized
  }

  // Acquire this SpinLock.
  inline void Lock() {
    if (InterlockedCompareExchangePointerAcquire(
            reinterpret_cast<volatile PVOID*>(&lockword_),
            reinterpret_cast<PVOID>(1),
            reinterpret_cast<PVOID>(0)) != 0) {
      SlowLock();
    }
  }

  // Acquire this SpinLock and return true if the acquisition can be
  // done without blocking, else return false.  If this SpinLock is
  // free at the time of the call, TryLock will return true with high
  // probability.
  inline bool TryLock() {
    return InterlockedCompareExchangePointerAcquire(
        reinterpret_cast<volatile PVOID*>(&lockword_),
        reinterpret_cast<PVOID>(1),
        reinterpret_cast<PVOID>(0)) == 0;
  }

  // Release this SpinLock, which must be held by the calling thread.
  inline void Unlock() {
    int64 wait_timestamp = static_cast<uint64>(lockword_);
    lockword_ = 0;
    if (wait_timestamp != 1) {
      // Collect contentionz profile info, and speed the wakeup of any waiter.
      // The lockword_ value indicates when the waiter started waiting.
      SlowUnlock(wait_timestamp);
    }
  }

  // Report if we think the lock can be held by this thread.
  // When the lock is truly held by the invoking thread
  // we will always return true.
  // Indended to be used as CHECK(lock.IsHeld());
  inline bool IsHeld() const {
    return lockword_ != 0;
  }

  // The timestamp for contentionz lock profiling must fit into 31 bits.
  // as lockword_ is 32 bits and we loose an additional low-order bit due
  // to the statement "now |= 1" in SlowLock().
  // To select 31 bits from the 64-bit cycle counter, we shift right by
  // PROFILE_TIMESTAMP_SHIFT = 7.
  // Using these 31 bits, we reduce granularity of time measurement to
  // 256 cycles, and will loose track of wait time for waits greater than
  // 109 seconds on a 5 GHz machine, longer for faster clock cycles.
  // Waits this long should be very rare.
  enum { PROFILE_TIMESTAMP_SHIFT = 7 };

  static const LinkerInitialized LINKER_INITIALIZED;  // backwards compat
 private:
  // Lock-state:  0 means unlocked; 1 means locked with no waiters; values
  // greater than 1 indicate locked with waiters, where the value is the time
  // the first waiter started waiting and is used for contention profiling.
  volatile intptr_t lockword_;

  void SlowLock();
  void SlowUnlock(int64 wait_timestamp);

  DISALLOW_COPY_AND_ASSIGN(SpinLock);
};

// Corresponding locker object that arranges to acquire a spinlock for
// the duration of a C++ scope.
class SpinLockHolder {
 private:
  SpinLock* lock_;
 public:
  inline explicit SpinLockHolder(SpinLock* l) : lock_(l) {
    l->Lock();
  }
  inline ~SpinLockHolder() { lock_->Unlock(); }
};
// Catch bug where variable name is omitted, e.g. SpinLockHolder (&lock);
#define SpinLockHolder(x) COMPILE_ASSERT(0, spin_lock_decl_missing_var_name)

#endif  // GOOPY_BASE_SPINLOCK_H_
