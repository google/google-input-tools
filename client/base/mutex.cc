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

#include "base/mutex.h"

Mutex::Mutex()
    : rw_lock_(new ime_shared::internal::RWLock) {
  DCHECK(rw_lock_.get() != NULL);
}

Mutex::Mutex(base::LinkerInitialized x)
    : rw_lock_(new ime_shared::internal::RWLock) {
  DCHECK(rw_lock_.get() != NULL);
}

#if defined(OS_WINDOWS)
Mutex::Mutex(const TCHAR *name)
    : mu_(new ime_shared::internal::MutexInternal(name)) {
  DCHECK(mu_.get() != NULL);
}
#endif  // OS_WINDOWS

Mutex::~Mutex() {
}

void Mutex::Unlock() {
  if (mu_.get()) {
    mu_->Unlock();
  } else if (rw_lock_.get()) {
    rw_lock_->UnLock();
  }
}

bool Mutex::LockWithTimeout(uint32 timeout) {
  if (mu_.get()) {
    return mu_->Lock(timeout);
  } else if (rw_lock_.get()) {
    return rw_lock_->WriteLock(timeout);
  }
  // This happens during dynamic initialization, it's safe to return true.
  // See comments for Mutex(base::LinkerInitialized x) in mutex.h for more
  // details.
  return true;
}

ReaderMutexLock::ReaderMutexLock(Mutex *mu, uint32 timeout)
    : mu_(mu), is_locked_(false) {
  DCHECK(mu_ != NULL);
  if (mu_)
    is_locked_ = mu_->ReaderLock(timeout);
}

ReaderMutexLock::~ReaderMutexLock() {
  if (is_locked_)
    mu_->ReaderUnlock();
}

WriterMutexLock::WriterMutexLock(Mutex *mu, uint32 timeout)
    : mu_(mu), is_locked_(false) {
  DCHECK(mu_ != NULL);
  if (mu_)
    is_locked_ = mu_->WriterLock(timeout);
}

WriterMutexLock::~WriterMutexLock() {
  if (is_locked_)
    mu_->WriterUnlock();
}
