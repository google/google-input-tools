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

// This file is used for debugging assertion support.  The Lock class
// is functionally a wrapper around the LockImpl class, so the only
// real intelligence in the class is in the debugging logic.

#if !defined(NDEBUG)

#include "base/synchronization/lock.h"
#include "base/logging.h"

namespace base {

Lock::Lock() : lock_() {
  owned_by_thread_ = false;
  owning_thread_id_ = static_cast<PlatformThreadId>(0);
}

void Lock::AssertAcquired() const {
  DCHECK(owned_by_thread_);
  DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
}

void Lock::CheckHeldAndUnmark() {
  DCHECK(owned_by_thread_);
  DCHECK_EQ(owning_thread_id_, PlatformThread::CurrentId());
  owned_by_thread_ = false;
  owning_thread_id_ = static_cast<PlatformThreadId>(0);
}

void Lock::CheckUnheldAndMark() {
  DCHECK(!owned_by_thread_);
  owned_by_thread_ = true;
  owning_thread_id_ = PlatformThread::CurrentId();
}

}  // namespace base

#endif  // NDEBUG
