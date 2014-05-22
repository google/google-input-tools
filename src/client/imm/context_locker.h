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

#ifndef GOOPY_IMM_CONTEXT_LOCKER_H__
#define GOOPY_IMM_CONTEXT_LOCKER_H__

#include "base/basictypes.h"
#include "base/logging.h"
#include "imm/immdev.h"

namespace ime_goopy {
namespace imm {

class WindowsImmLockPolicy {
 public:
  inline static LPINPUTCONTEXT ImmLockIMC(HIMC himc) {
    return ::ImmLockIMC(himc);
  }

  inline static BOOL ImmUnlockIMC(HIMC himc) {
    return ::ImmUnlockIMC(himc);
  }

  inline static LPVOID ImmLockIMCC(HIMCC himcc) {
    return ::ImmLockIMCC(himcc);
  }

  inline static BOOL ImmUnlockIMCC(HIMCC himcc) {
    return ::ImmUnlockIMCC(himcc);
  }

  inline static HIMCC ImmCreateIMCC(DWORD size) {
    return ::ImmCreateIMCC(size);
  }

  inline static HIMCC ImmReSizeIMCC(HIMCC himcc, DWORD size) {
    return ::ImmReSizeIMCC(himcc, size);
  }
};

// Lock input context and provide smart pointer access.
template <class T, class ImmLockPolicy = WindowsImmLockPolicy>
class HIMCLockerT {
 public:
  explicit HIMCLockerT(HIMC himc) : himc_(NULL), pointer_(NULL) {
    pointer_ = reinterpret_cast<T*>(ImmLockPolicy::ImmLockIMC(himc));
    if (!pointer_) return;
    himc_ = himc;
  }

  ~HIMCLockerT() {
    if (himc_) ImmLockPolicy::ImmUnlockIMC(himc_);
    pointer_ = NULL;
  }

  T* get() const {
    return pointer_;
  }

  T* operator->() const  {
    assert(pointer_);
    return pointer_;
  }

  bool operator==(T* p) const {
    return pointer_ == p;
  }

  bool operator!=(T* p) const {
    return pointer_ != p;
  }

  bool operator!() const {
    return pointer_ == NULL;
  }

 private:
  HIMC himc_;
  T* pointer_;
  DISALLOW_EVIL_CONSTRUCTORS(HIMCLockerT);
};

// HIMCCLocker is a helper class to lock input context component in IMM. This
// class behave like a smart pointer, you can access the member of the HIMCC
// using "->" operator.
template <class T, class ImmLockPolicy = WindowsImmLockPolicy>
class HIMCCLockerT {
 public:
  explicit HIMCCLockerT(HIMCC himcc) : himcc_(himcc), pointer_(NULL) {
    pointer_ = reinterpret_cast<T*>(ImmLockPolicy::ImmLockIMCC(himcc_));
    if (!pointer_) himcc_ = NULL;
  }

  // This construtor can make sure the component has specific size.
  HIMCCLockerT(HIMCC* himcc, int size) : himcc_(NULL), pointer_(NULL) {
    Prepare(himcc, size);
  }

  explicit HIMCCLockerT(HIMCC* himcc) : himcc_(NULL), pointer_(NULL) {
    Prepare(himcc, sizeof(T));
  }

  ~HIMCCLockerT() {
    if (himcc_) ImmLockPolicy::ImmUnlockIMCC(himcc_);
    pointer_ = NULL;
  }

  T* get() const {
    return pointer_;
  }

  T* operator->() const  {
    assert(pointer_);
    return pointer_;
  }

  T& operator[](int i) const {
    assert(pointer_);
    return pointer_[i];
  }

  bool operator==(T* p) const {
    return pointer_ == p;
  }

  bool operator!=(T* p) const {
    return pointer_ == p;
  }

  bool operator!() const {
    return pointer_ == NULL;
  }

 protected:
  T* pointer_;

 private:
  void Prepare(HIMCC* himcc, int size) {
    // Prepare himcc.
    if (!*himcc) {
      *himcc = ImmLockPolicy::ImmCreateIMCC(size);
    } else {
      *himcc = ImmLockPolicy::ImmReSizeIMCC(*himcc, size);
    }
    if (!*himcc) return;

    // Lock it.
    pointer_ = reinterpret_cast<T*>(ImmLockPolicy::ImmLockIMCC(*himcc));
    if (!pointer_) return;

    himcc_ = *himcc;
  }

  HIMCC himcc_;
  DISALLOW_EVIL_CONSTRUCTORS(HIMCCLockerT);
};
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_CONTEXT_LOCKER_H__

