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
// DO NOT include this file directly. Use public interface in mutex.h instead.

#ifndef I18N_INPUT_ENGINE_STUBS_GOOGLE3_BASE_MUTEX_INTERNAL_WIN_H_  // NOLINT
#define I18N_INPUT_ENGINE_STUBS_GOOGLE3_BASE_MUTEX_INTERNAL_WIN_H_  // NOLINT

#include <windows.h>

#include "base/macros.h"

namespace ime_shared {
namespace internal {

class MutexInternal {
 public:
  explicit MutexInternal(const TCHAR *name);

  // Releases the mutex.
  ~MutexInternal();

  // Locks the mutex.
  //  timeout: amount of time (in ms) to wait for the lock.  Pass INFINITE to
  //           wait forever.
  //  returns true iff the mutex was locked.
  bool Lock(uint32 timeout);

  // Unlocks the mutex.
  void Unlock();

 private:
  HANDLE mutex_;

  DISALLOW_COPY_AND_ASSIGN(MutexInternal);
};

// Reader-writer lock. It's not for inter-process synchronization, but for
// multi-threads within one process.
class RWLock {
 public:
  RWLock();
  ~RWLock();

  bool ReadLock(uint32 timeout);
  bool WriteLock(uint32 timeout);
  void UnLock();

 private:
  enum LockState {
    LOCK_NONE,
    LOCK_READING,
    LOCK_WRITING,
  } state_;
  int read_count_;
  HANDLE unlock_event_;
  HANDLE access_mutex_;
  CRITICAL_SECTION critical_mutex_;
};

}  // namespace internal
}  // namespace ime_shared

#endif  // I18N_INPUT_ENGINE_STUBS_GOOGLE3_BASE_MUTEX_INTERNAL_WIN_H_  // NOLINT
