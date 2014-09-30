// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GGADGET_MAC_SCOPED_CFTYPEREF_H_
#define GGADGET_MAC_SCOPED_CFTYPEREF_H_

#include <CoreFoundation/CoreFoundation.h>

namespace ggadget {
namespace mac {

// Annotate a function indicating the caller must examine the return value.
// Use like:
//   int foo() WARN_UNUSED_RESULT;
#if defined(COMPILER_GCC)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

// ScopedCFTypeRef<> is patterned after scoped_ptr<>, but maintains ownership
// of a CoreFoundation object: any object that can be represented as a
// CFTypeRef.  Style deviations here are solely for compatibility with
// scoped_ptr<>'s interface, with which everyone is already familiar.
//
// By default, ScopedCFTypeRef<> takes ownership of an object (in the
// constructor or in reset()) by taking over the caller's existing ownership
// claim.  The caller must own the object it gives to ScopedCFTypeRef<>, and
// relinquishes an ownership claim to that object.  ScopedCFTypeRef<> does not
// call CFRetain(). This behavior is parameterized by the |OwnershipPolicy|
// enum. If the value |RETAIN| is passed (in the constructor or in reset()),
// then ScopedCFTypeRef<> will call CFRetain() on the object, and the initial
// ownership is not changed.

template<typename CFT>
class ScopedCFTypeRef {
 public:
  // Defines the ownership policy for a scoped object.
  enum OwnershipPolicy {
    // The scoped object takes ownership of an object by taking over an existing
    // ownership claim.
    ASSUME,

    // The scoped object will retain the the object and any initial ownership is
    // not changed.
    RETAIN
  };

  explicit ScopedCFTypeRef(
      CFT object = NULL,
      OwnershipPolicy policy = ASSUME)
      : object_(object) {
    if (object_ && policy == RETAIN)
      CFRetain(object_);
  }

  ScopedCFTypeRef(const ScopedCFTypeRef<CFT>& that)
      : object_(that.object_) {
    if (object_)
      CFRetain(object_);
  }

  ~ScopedCFTypeRef() {
    if (object_)
      CFRelease(object_);
  }

  ScopedCFTypeRef& operator=(const ScopedCFTypeRef<CFT>& that) {
    reset(that.get(), RETAIN);
    return *this;
  }

  void reset(CFT object = NULL, OwnershipPolicy policy = ASSUME) {
    if (object_ == object)
      return;
    if (object && policy == RETAIN)
      CFRetain(object);
    if (object_)
      CFRelease(object_);
    object_ = object;
  }

  bool operator==(CFT that) const {
    return object_ == that;
  }

  bool operator!=(CFT that) const {
    return object_ != that;
  }

  operator CFT() const {
    return object_;
  }

  CFT get() const {
    return object_;
  }

  void swap(ScopedCFTypeRef& that) {
    CFT temp = that.object_;
    that.object_ = object_;
    object_ = temp;
  }

  // ScopedCFTypeRef<>::release() is like scoped_ptr<>::release.  It is NOT
  // a wrapper for CFRelease().  To force a ScopedCFTypeRef<> object to call
  // CFRelease(), use ScopedCFTypeRef<>::reset().
  CFT release() WARN_UNUSED_RESULT {
    CFT temp = object_;
    object_ = NULL;
    return temp;
  }

 private:
  CFT object_;
};

}  // namespace mac
}  // namespace ggadget

#endif  // GGADGET_MAC_SCOPED_CFTYPEREF_H_
