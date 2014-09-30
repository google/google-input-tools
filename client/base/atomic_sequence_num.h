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

#ifndef GOOPY_BASE_ATOMIC_SEQUENCE_NUM_H_
#define GOOPY_BASE_ATOMIC_SEQUENCE_NUM_H_
#pragma once

#include "base/atomicops.h"
#include "base/basictypes.h"

namespace base {

class AtomicSequenceNumber {
 public:
  AtomicSequenceNumber() : seq_(0) { }
  explicit AtomicSequenceNumber(base::LinkerInitialized x) { /* seq_ is 0 */ }

  int GetNext() {
    return static_cast<int>(
        base::subtle::NoBarrier_AtomicIncrement(&seq_, 1) - 1);
  }

 private:
  base::subtle::Atomic32 seq_;
  DISALLOW_COPY_AND_ASSIGN(AtomicSequenceNumber);
};

}  // namespace base

#endif  // GOOPY_BASE_ATOMIC_SEQUENCE_NUM_H_
