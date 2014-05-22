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

// This is a low level implementation of atomic semantics for reference
// counting.  Please use base/ref_counted.h directly instead.
//
// The implementation includes annotations to avoid some false positives
// when using data race detection tools.

#ifndef GOOPY_BASE_ATOMIC_REF_COUNT_H_
#define GOOPY_BASE_ATOMIC_REF_COUNT_H_
#pragma once

#include "base/atomicops.h"
// Commented by haicsun@google.com to remove third_party dependency
// #include "base/third_party/dynamic_annotations/dynamic_annotations.h"

namespace base {

typedef subtle::Atomic32 AtomicRefCount;

// Increment a reference count by "increment", which must exceed 0.
inline void AtomicRefCountIncN(volatile AtomicRefCount *ptr,
                               AtomicRefCount increment) {
  subtle::NoBarrier_AtomicIncrement(ptr, increment);
}

// Decrement a reference count by "decrement", which must exceed 0,
// and return whether the result is non-zero.
// Insert barriers to ensure that state written before the reference count
// became zero will be visible to a thread that has just made the count zero.
inline bool AtomicRefCountDecN(volatile AtomicRefCount *ptr,
                               AtomicRefCount decrement) {
  // Commented by haicsun@google.com to remove third_party dependency
  // ANNOTATE_HAPPENS_BEFORE(ptr);
  bool res = (subtle::Barrier_AtomicIncrement(ptr, -decrement) != 0);
  // Commented by haicsun@google.com to remove third_party dependency
  //if (!res) {
    //ANNOTATE_HAPPENS_AFTER(ptr);
  //}
  return res;
}

// Increment a reference count by 1.
inline void AtomicRefCountInc(volatile AtomicRefCount *ptr) {
  base::AtomicRefCountIncN(ptr, 1);
}

// Decrement a reference count by 1 and return whether the result is non-zero.
// Insert barriers to ensure that state written before the reference count
// became zero will be visible to a thread that has just made the count zero.
inline bool AtomicRefCountDec(volatile AtomicRefCount *ptr) {
  return base::AtomicRefCountDecN(ptr, 1);
}

// Return whether the reference count is one.  If the reference count is used
// in the conventional way, a refrerence count of 1 implies that the current
// thread owns the reference and no other thread shares it.  This call performs
// the test for a reference count of one, and performs the memory barrier
// needed for the owning thread to act on the object, knowing that it has
// exclusive access to the object.
inline bool AtomicRefCountIsOne(volatile AtomicRefCount *ptr) {
  bool res = (subtle::Acquire_Load(ptr) == 1);
  // Commented by haicsun@google.com to remove third_party dependency
  //if (res) {
    // ANNOTATE_HAPPENS_AFTER(ptr);
  //}
  return res;
}

// Return whether the reference count is zero.  With conventional object
// referencing counting, the object will be destroyed, so the reference count
// should never be zero.  Hence this is generally used for a debug check.
inline bool AtomicRefCountIsZero(volatile AtomicRefCount *ptr) {
  bool res = (subtle::Acquire_Load(ptr) == 0);
  // Commented by haicsun@google.com to remove third_party dependency
  //if (res) {
  //  ANNOTATE_HAPPENS_AFTER(ptr);
  //}
  return res;
}

}  // namespace base

#endif  // GOOPY_BASE_ATOMIC_REF_COUNT_H_
