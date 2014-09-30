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

#ifndef GOOPY_COMMON_SCOPED_LOCK_H_
#define GOOPY_COMMON_SCOPED_LOCK_H_

#include <atlbase.h>
#include <atlcom.h>
#include "common/mutex.h"

namespace ime_goopy {
template <class T> class ScopedLock {
 public:
  explicit ScopedLock(T* obj) : obj_(obj) {
    DCHECK(obj);
    obj_->Lock();
  }
  ~ScopedLock() {
    obj_->Unlock();
  }
 private:
  T* obj_;
};
}  // namespace ime_goopy

#endif  // GOOPY_COMMON_SCOPED_LOCK_H_
