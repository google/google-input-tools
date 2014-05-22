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

// This file defines mutex and locker objects for inter-thread / inter-process
// synchronization use.
//
// The Mutex is a wrapper on either a MutexInternal object for inter-thread
// and inter-process synchronization or a RWLock only for inter-thread
// synchronization but supports reader/writer locks. User can choose to use
// one of them by constructing the Mutex with different constructors
// (See comments of Mutex constructors for more details).
//
// The interface of RWLock based inter-thread Mutex is compatible to google3
// Mutex.
//
// Locking using MutexLock or ReaderMutexLock, WriterMutexLock is preferred
// than calling Mutex::Lock directly since these scoped lock also provide
// IsLocked interface for caller to determine whether the lock is successfully
// acquired.
//
// Usage:
//  For inter-process synchronization.
//    static const char kMutexName[] = "a-mutex";
//    // Creates the mutex object.
//    Mutex mutex(kMutexName);
//
//    if (mutex.LockWithTimeout(kDefaultTimeOut)) {
//      // Do something.
//      mutex.Unlock();
//    }
//
//    {
//      // Lock the mutex.
//      MutexLock lock(&mutex);  // kDefaultTimeOut is used to wait for lock.
//      if (!lock.IsLocked()) {
//        LOG(ERROR) << "Mutex not locked.";
//      } else {
//        // Do something.
//      }
//    }
//    {
//      // Lock the mutex without timeout.
//      MutexLock lock(&mutex, INFINITE);
//      // Do something.
//    }
//
// For inter-thread read/write synchronization.
//   Mutex mutex;
//   mutex.ReaderLock();
//   // Do something.
//   mutex.ReaderUnlock();
//
//   mutex.WriterLock();  // Or just mutex.Lock();
//   // Do something.
//   mutex.WriterUnlock();
//
//   {
//     ReaderMutexLock lock(&mutex, INFINITE);
//     // Do something.
//   }
//
//   {
//     WriterMutexLock lock(&mutex, kDefaultTimeOut);
//     if (!lock.IsLocked()) {
//       LOG(ERROR) << "Mutex not locked.";
//     } else {
//       // Do something.
//     }
//   }

#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#if defined(OS_WINDOWS)
#include <windows.h>
#endif  // OS_WINDOWS

#include "base/logging.h"
#if defined(OS_WINDOWS)
#include "base/mutex-internal-win.h"
#elif defined(__linux__) || defined(OS_MACOSX) || defined(OS_NACL)
#include "base/mutex-internal-posix.h"
#else
#error "Need to implement mutex for this platform"
#endif  // OS_WINDOWS
#include "base/scoped_ptr.h"
#include "base/thread_annotations.h"

static const uint32 kDefaultTimeOut = 5000;  // 5000 ms.

class Mutex {
 public:
  // Constructs a mutex for inter-thread synchronization use only.
  // The constructed Mutex object supports reader/writer lock interfaces.
  Mutex();
  // Same as above in functionality, provided for compatibility with google3
  // Mutex interface:
  // Mutex with static storage class (not on the heap, not on a stack) can
  // use
  //   Mutex mu(base::LINKER_INITIALIZED);
  // to create the Mutex object.
  //
  // Notes on linker initialized behavior:
  //
  // google3 Mutex implementation relies on the fact that the linker will
  // properly initialize the static object memory to zero, which is expected
  // by google3 Mutex implementation, so the Mutex object can be freely used
  // from global initializers without worrying about the order of dynamic
  // initialization (running global constructors before main() starts).
  // This is also why google3 Mutex provides such constructor.
  //
  // In client implementation, the initialization of Mutex object is not
  // trivial (as setting member value to zero in google3 Mutex), but because
  // during dynamic initialization:
  // 1. There are no threads, it's okay that the mutex operations are no-ops.
  // 2. The initialization of mu_ object or rw_lock_ object is unlikely to
  //    happen between a call to Lock() and Unlock(), which will require
  //    a global constructor in one translation unit to call Lock() and
  //    another global constructor in another translation unit to call
  //    Unlock() or the code itself incorrectly depends on the global
  //    initialization order.
  // we are safe to use the Mutex as global static object and to synchronize
  // other global static object construction in the same translation unit
  // even if Lock() and Unlock() are no-ops.
  //
  // The implementation also depends on the fact that linker will zero
  // initialize the mu_ and rw_lock_, so if both mu_ and rw_lock_ are NULL,
  // Lock() will return immediately stating the lock is held and UnLock() is
  // no-op.
  explicit Mutex(base::LinkerInitialized x);

#if defined(OS_WINDOWS)
  // Constructs a mutex for inter-thread or inter-process synchronization use.
  //
  // The Mutex constructed using this constructor currently does not support
  // reader/writer lock interfaces.
  //
  // Use a NULL name for a private Mutex (for inter-thread synchronization use),
  // otherwise it may be shared between processes.  In Vista, it will be shared
  // across Low/Medium/High integrity levels of the current user.
  explicit Mutex(const TCHAR *name);
#endif  // OS_WINDOWS

  ~Mutex();

  // Blocks until the lock is acquired or 'timeout' is elapsed.
  //  timeout: amount of time (in ms) to wait for the lock. Pass INFINITE to
  //           wait forever.
  //  Returns true iff the mutex was locked.
  bool LockWithTimeout(uint32 timeout);

  // Blocks until the lock is acquired or kDefaultTimeOut is elapsed.
  //
  // *CAVEATS*
  //
  // Note that we use a finite timeout kDefaultTimeOut instead of INFINITE
  // so there's chance when the function returns, the lock is actually not
  // held which may lead to data inconsistency or corruption etc.
  //
  // An assertion failure will be triggered under debug mode if the lock is
  // not held after kDefaultTimeOut is elapsed. In release mode however we
  // just ignore the failure.
  //
  // Since in client environment the host application will be stalled if the
  // input method is waiting for the lock which will result in very bad user
  // experience, we tend to tolerant the inconsistency of runtime states
  // in case there's a dead-lock etc.
  //
  // However when synchronizing persistent data like dictionary etc. in which
  // case data consistency is very important, caller should choose to use
  // LockWithTimeout providing proper timeout and using return value to
  // decide whether lock is properly held.
  void Lock() {
    bool successful = LockWithTimeout(kDefaultTimeOut);
    DCHECK(successful);
  }

  void Unlock();

  // Expected to crash if the Mutex is not held by this thread. Not implemented
  // in client.
  void AssertHeld() const {}

  // Reader/writer lock interfaces, only useful when the Mutex object is
  // constructed via Mutex() or Mutex(base::LinkerInitialized x).

  // Blocks until the lock for read/write operation is acquired or
  // 'timeout' is elapsed.
  //  timeout: amount of time (in ms) to wait for the lock. Pass INFINITE to
  //           wait forever.
  // Returns true iff the mutex is locked.
  inline bool ReaderLock(uint32 timeout);
  inline bool WriterLock(uint32 timeout);

  // Blocks until the lock for read/write operation is acquired or
  // kDefaultTimeOut is elapsed.
  //
  // Triggers assertion failure under debug mode if the lock is not held after
  // kDefaultTimeOut is elapsed.
  //
  // See comments for Lock() for caveats.
  inline void ReaderLock();
  inline void WriterLock();

  inline void ReaderUnlock();
  inline void WriterUnlock();

#if defined (OS_NACL)
  pthread_mutex_t* raw_mutex() {
    return rw_lock_->raw_mutex();
  }
#endif  // OS_NACL

 private:
  scoped_ptr<ime_shared::internal::MutexInternal> mu_;
  scoped_ptr<ime_shared::internal::RWLock> rw_lock_;

  DISALLOW_COPY_AND_ASSIGN(Mutex);
};

// MutexLock(mu) acquires mu when constructed and releases it when destroyed.
// Besides used as a scoped locker, MutexLock also provides IsLocked, Lock, and
// Unlock interfaces to let the caller determine whether the lock is
// successfully acquired and try to acquire the lock again.
class MutexLock {
 public:
  // Locks a mutex.
  //  mutex: the mutex to lock.
  //  timeout: amount of time to wait (in ms), or INFINITE for no timeout.
  MutexLock(Mutex *mutex, uint32 timeout = kDefaultTimeOut) : mutex_(mutex) {
    is_locked_ = mutex_->LockWithTimeout(timeout);
  }

  // Destructor unlocks the mutex if necessary.
  ~MutexLock() {
    Unlock();
  }

  // Acquires the mutex lock.
  bool Lock(uint32 timeout) {
    is_locked_ = mutex_->LockWithTimeout(timeout);
    return is_locked_;
  }

  // Tests whether the mutex was locked.
  bool IsLocked() const {
    return is_locked_;
  }

  // Explicitly unlocks the mutex.
  void Unlock() {
    if (is_locked_) {
      mutex_->Unlock();
      is_locked_ = false;
    }
  }

 private:
  Mutex *const mutex_;
  bool is_locked_;

  DISALLOW_COPY_AND_ASSIGN(MutexLock);
};
// TODO: Remove reference of MutexLocker in client code.
typedef MutexLock MutexLocker;

class ReaderMutexLock {
 public:
  ReaderMutexLock(Mutex *mu, uint32 timeout = kDefaultTimeOut);
  ~ReaderMutexLock();

  bool IsLocked() { return is_locked_; }

 private:
  Mutex *mu_;
  bool is_locked_;

  DISALLOW_COPY_AND_ASSIGN(ReaderMutexLock);
};

class WriterMutexLock {
 public:
  WriterMutexLock(Mutex *mu, uint32 timeout = kDefaultTimeOut);
  ~WriterMutexLock();

  bool IsLocked() { return is_locked_; }

 private:
  Mutex *mu_;
  bool is_locked_;

  DISALLOW_COPY_AND_ASSIGN(WriterMutexLock);
};

inline void Mutex::ReaderLock() {
  DCHECK(mu_.get() == NULL)
      << "Inter-process reader/writer lock is not supported.";
  if (rw_lock_.get()) {
    bool successful = rw_lock_->ReadLock(kDefaultTimeOut);
    DCHECK(successful);
  }
}

inline bool Mutex::ReaderLock(uint32 timeout) {
  DCHECK(mu_.get() == NULL)
      << "Inter-process reader/writer lock is not supported.";
  if (rw_lock_.get()) {
    return rw_lock_->ReadLock(timeout);
  }
  // This happens during dynamic initialization, it's safe to return true.
  // See comments for Mutex(base::LinkerInitialized x) for more details.
  return true;
}

inline void Mutex::ReaderUnlock() {
  DCHECK(mu_.get() == NULL)
      << "Inter-process reader/writer lock is not supported.";
  if (rw_lock_.get()) rw_lock_->UnLock();
}

inline void Mutex::WriterLock() {
  DCHECK(mu_.get() == NULL)
      << "Inter-process reader/writer lock is not supported.";
  if (rw_lock_.get()) {
    bool successful = rw_lock_->WriteLock(kDefaultTimeOut);
    DCHECK(successful);
  }
}

inline bool Mutex::WriterLock(uint32 timeout) {
  DCHECK(mu_.get() == NULL)
      << "Inter-process reader/writer lock is not supported.";
  if (rw_lock_.get()) {
    return rw_lock_->WriteLock(timeout);
  }
  // This happens during dynamic initialization, it's safe to return true.
  // See comments for Mutex(base::LinkerInitialized x) for more details.
  return true;
}

inline void Mutex::WriterUnlock() {
  DCHECK(mu_.get() == NULL)
      << "Inter-process reader/writer lock is not supported.";
  if (rw_lock_.get()) rw_lock_->UnLock();
}

#if defined(OS_NACL)
class Condition {
 public:
  Condition() {
    pthread_cond_init(&cond_, NULL);
  }
  ~Condition() {
    pthread_cond_destroy(&cond_);
  }
  int signal() {
    return pthread_cond_signal(&cond_);
  }
  int broadcast() {
    return pthread_cond_broadcast(&cond_);
  }
  void wait(Mutex *mutex) {
    pthread_cond_wait(&cond_, mutex->raw_mutex());
  }

 private:
  pthread_cond_t cond_;
  DISALLOW_COPY_AND_ASSIGN(Condition);
};
#endif  // OS_NACL

#endif  // BASE_MUTEX_H_
