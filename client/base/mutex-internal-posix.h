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
// Implements MutexInternal and RWLock using pthread.
//
// As pthread library on different platforms support rwlock and timed lock in
// different degrees, we fall back from timed lock to non-timed lock if timed
// lock is not supported, and fall back from rwlock to mutex lock if rwlock
// is not supported.

#ifndef BASE_MUTEX_INTERNAL_POSIX_H_
#define BASE_MUTEX_INTERNAL_POSIX_H_

#include <pthread.h>
#include <time.h>

#if defined(OS_MACOSX)
#define PTHREAD_HAVE_RWLOCK
#define PTHREAD_HAVE_LOCK
#elif defined(__ANDROID__) || defined(OS_NACL)
#define PTHREAD_HAVE_LOCK
#endif  // OS_MACOSX

namespace ime_shared {
namespace internal {

#if defined(PTHREAD_HAVE_TIMED_LOCK)
static void GetTimeoutTimespec(uint32 timeout_ms, struct timespec* ts) {
  static const uint32 kSecToMSecMultiplier = 1000;
  static const uint32 kMSecToNSecMultiplier = 1000000;
  static const uint32 kSecToNSecMultiplier = 1000000000;
  clock_gettime(CLOCK_REALTIME, ts);
  ts->tv_sec += timeout_ms / kSecToMSecMultiplier;
  ts->tv_nsec += (timeout_ms % kSecToMSecMultiplier) * kMSecToNSecMultiplier;
  if (ts->tv_nsec >= kSecToNSecMultiplier) {
    ts->tv_nsec -= kSecToNSecMultiplier;
    ++ts->tv_sec;
  }
}
#endif  // PTHREAD_HAVE_TIMED_LOCK

#if defined(PTHREAD_HAVE_LOCK) || defined(PTHREAD_HAVE_TIMED_LOCK)
class MutexInternal {
 public:
  MutexInternal() {
    pthread_mutex_init(&lock_, NULL);
  }

  ~MutexInternal() {
    pthread_mutex_destroy(&lock_);
  }

  bool Lock(uint32 timeout) {
#if defined(PTHREAD_HAVE_TIMED_LOCK)
    struct timespec ts = { 0 };
    GetTimeoutTimespec(timeout, &ts);
    return pthread_mutex_timedlock(&lock_, &ts) == 0;
#else  // PTHREAD_HAVE_LOCK
    return pthread_mutex_lock(&lock_) == 0;
#endif  // PTHREAD_HAVE_TIMED_LOCK
  }

  void Unlock() {
    pthread_mutex_unlock(&lock_);
  }

 private:
  pthread_mutex_t lock_;
};
#else  // !(PTHREAD_HAVE_LOCK || PTHREAD_HAVE_TIMED_LOCK)
#error "No pthread lock can be used to implement MutexInternal."
#endif  // PTHREAD_HAVE_LOCK || PTHREAD_HAVE_TIMED_LOCK

class RWLock {
#if defined(PTHREAD_HAVE_RWLOCK)

 public:
  RWLock() {
    pthread_rwlock_init(&lock_, NULL);
  }

  ~RWLock() {
    pthread_rwlock_destroy(&lock_);
  }

  bool ReadLock(uint32 timeout) {
    return pthread_rwlock_rdlock(&lock_) == 0;
  }

  bool WriteLock(uint32 timeout) {
    return pthread_rwlock_wrlock(&lock_) == 0;
  }

  void UnLock() {
    pthread_rwlock_unlock(&lock_);
  }

 private:
  pthread_rwlock_t lock_;

#elif defined(PTHREAD_HAVE_LOCK) || defined(PTHREAD_HAVE_TIMED_LOCK)

 public:
  RWLock() {
    pthread_mutex_init(&lock_, NULL);
  }

  ~RWLock() {
    pthread_mutex_destroy(&lock_);
  }

  bool ReadLock(uint32 timeout) {
    return WriteLock(timeout);
  }

  bool WriteLock(uint32 timeout) {
#if defined(PTHREAD_HAVE_TIMED_LOCK)
    struct timespec ts = { 0 };
    GetTimeoutTimespec(timeout, &ts);
    return pthread_mutex_timedlock(&lock_, &ts) == 0;
#else  // PTHREAD_HAVE_LOCK
    return pthread_mutex_lock(&lock_) == 0;
#endif  // PTHREAD_HAVE_TIMED_LOCK
  }

  void UnLock() {
    pthread_mutex_unlock(&lock_);
  }

#if defined(OS_NACL)
  pthread_mutex_t* raw_mutex() {
    return &lock_;
  }
#endif  // OS_NACL

 private:
  pthread_mutex_t lock_;
#else  // !(PTHREAD_HAVE_LOCK || PTHREAD_HAVE_TIMED_LOCK)
#error "No pthread lock can be used to implement RWLock."
#endif  // PTHREAD_HAVE_RWLOCK
};

}  // namespace internal
}  // namespace ime_shared
#endif  // BASE_MUTEX_INTERNAL_POSIX_H_
