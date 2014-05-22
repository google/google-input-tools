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

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/mutex-internal-win.h"
#include "base/security_utils_win.h"

namespace ime_shared {
namespace internal {

MutexInternal::MutexInternal(const TCHAR *name) {
  mutex_ = CreateMutex(NULL, FALSE, name);
  if (name != NULL) {
    // Sets the integrity level of the mutex object to low so the mutex is
    // accessible from IE process in protect mode.
    ime_goopy::SetHandleLowIntegrity(mutex_, SE_KERNEL_OBJECT);
  }
  DCHECK(mutex_ != NULL);
}

MutexInternal::~MutexInternal() {
  if (mutex_ != NULL) {
    CloseHandle(mutex_);
  }
}

bool MutexInternal::Lock(uint32 timeout) {
  if (mutex_ != NULL) {
    DWORD wait_result = WaitForSingleObject(mutex_, timeout);
    if (wait_result == WAIT_OBJECT_0) {
      return true;
    } else if (wait_result == WAIT_ABANDONED) {
      // The MSDN documentation is unclear on whether the mutex is owned by
      // this thread in this case.  I wrote this little program to determine
      // the truth:
      //
      // #include <windows.h>
      // #include <assert.h>
      // #include <conio.h>
      // #include <stdio.h>
      // #include <stdlib.h>
      // #include <tchar.h>
      //
      // #define MUTEX_NAME _T("{4B96ACA9-19E1-40c0-B540-7B8220FF68AB}")
      //
      // int main() {
      //   HANDLE mutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
      //   DWORD ret;
      //   assert(mutex != NULL);
      //   puts("Waiting");
      //   ret = WaitForSingleObject(mutex, INFINITE);
      //   if (ret == WAIT_ABANDONED) {
      //     puts("WAIT_ABANDONED");
      //   } else if (ret == WAIT_OBJECT_0) {
      //     puts("WAIT_OBJECT_0");
      //   }
      //   while (!kbhit())
      //     ;
      //   return 0;
      // }
      //
      // Since the program does not release the mutex when it exits, any
      // thread waiting on the mutex will get WAIT_ABANDONED returned to
      // it.
      //
      // By repeatedly running two instances of this, you can see that the
      // thread *does* get ownership of the mutex when WaitForSingleObject
      // returns WAIT_ABANDONED (tested on XP Pro SP2, 2000 Pro (Japanese
      // edition).
      //
      // This is consistent with the MSDN documentation, but inconsistent
      // with the database example they provide.  We concluded that their
      // example is faulty.
      return true;
    }
  }
  return false;
}

void MutexInternal::Unlock() {
  if (mutex_ != NULL) {
    ReleaseMutex(mutex_);
  }
}

RWLock::RWLock()
    : state_(LOCK_NONE), read_count_(0),
      unlock_event_(CreateEvent(NULL, TRUE, FALSE, NULL)),
      access_mutex_(CreateMutex(NULL, FALSE, NULL)) {
  InitializeCriticalSection(&critical_mutex_);
}

RWLock::~RWLock() {
  DeleteCriticalSection(&critical_mutex_);
  if (access_mutex_)
    CloseHandle(access_mutex_);
  if (unlock_event_)
    CloseHandle(unlock_event_);
}

bool RWLock::ReadLock(uint32 timeout) {
  DWORD result = WaitForSingleObject(access_mutex_, timeout);
  if (result != WAIT_OBJECT_0)
    return false;

  if (state_ == LOCK_WRITING) {
    result = WaitForSingleObject(unlock_event_, timeout);
    if (result != WAIT_OBJECT_0) {
      ReleaseMutex(access_mutex_);
      return false;
    }
  }

  EnterCriticalSection(&critical_mutex_);
  state_ = LOCK_READING;
  read_count_ += 1;
  ResetEvent(unlock_event_);
  LeaveCriticalSection(&critical_mutex_);

  ReleaseMutex(access_mutex_);
  return true;
}

bool RWLock::WriteLock(uint32 timeout) {
  DWORD result = WaitForSingleObject(access_mutex_, timeout);
  if (result != WAIT_OBJECT_0)
    return false;

  if (state_ != LOCK_NONE) {
    result = WaitForSingleObject(unlock_event_, timeout);
    if (result != WAIT_OBJECT_0) {
      ReleaseMutex(access_mutex_);
      return false;
    }
  }

  EnterCriticalSection(&critical_mutex_);
  state_ = LOCK_WRITING;
  ResetEvent(unlock_event_);
  LeaveCriticalSection(&critical_mutex_);

  ReleaseMutex(access_mutex_);
  return true;
}

void RWLock::UnLock() {
  EnterCriticalSection(&critical_mutex_);
  if (state_ == LOCK_WRITING ||
      (state_ == LOCK_READING && --read_count_ == 0)) {
    state_ = LOCK_NONE;
    SetEvent(unlock_event_);
  }
  LeaveCriticalSection(&critical_mutex_);
}

}  // namespace internal
}  // namespace ime_shared
