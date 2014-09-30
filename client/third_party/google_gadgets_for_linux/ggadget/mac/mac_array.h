/*
  Copyright 2013 Google Inc.

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

#ifndef GGADGET_MAC_MAC_ARRAY_H_
#define GGADGET_MAC_MAC_ARRAY_H_

#include <CoreFoundation/CoreFoundation.h>

#include "ggadget/mac/scoped_cftyperef.h"

namespace ggadget {
namespace mac {

// A warpper of CFArrayRef. This wrapper makes you easy to access elements in
// CFArrayRef with array-like syntax. Unlike ScopedCFTypeRef class, this class
// uses RETAIN as the default ownership policy.
template<typename T>
class MacArray {
 public:
  typedef ScopedCFTypeRef<CFArrayRef> ScopedArrayRef;
  explicit MacArray(
      CFArrayRef array,
      ScopedArrayRef::OwnershipPolicy policy = ScopedArrayRef::RETAIN)
      : array_(array, policy) {}

  T operator[](size_t index) {
    return static_cast<T>(CFArrayGetValueAtIndex(array_.get(), index));
  }

  CFIndex size() const {
    return CFArrayGetCount(array_.get());
  }

  CFArrayRef get() const {
    return array_.get();
  }
 private:
  ScopedArrayRef array_;
};

} // namespace mac
} // namespace ggadget


#endif // GGADGET_MAC_MAC_ARRAY_H_
