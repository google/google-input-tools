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

// Core interfaces and definitions used by by low-level //base interfaces such
// as SpinLock.

#ifndef BASE_SCHEDULING_LOW_LEVEL_SUPPORT_H_
#define BASE_SCHEDULING_LOW_LEVEL_SUPPORT_H_

// CAUTION: These headers must be carefully curated to avoid circular
// dependencies within //base.
#include "base/macros.h"

class SpinLock;  // To allow use of SchedulingGuard.
class GoogleOnceInitSchedulingHelper;  // To allow use of SchedulingGuard.

namespace base {
namespace scheduling {

// Used to describe how a thread may be scheduled.  Typically associated with
// the declaration of a resource supporting synchronized access.
//
// SCHEDULE_COOPERATIVE_AND_KERNEL:
// Specifies that when waiting, a cooperative thread (e.g. a Fiber) may
// reschedule (using base::scheduling semantics); allowing other cooperative
// threads to proceed.
//
// SCHEDULE_KERNEL_ONLY: (Also described as "non-cooperative")
// Specifies that no cooperative scheduling semantics may be used, even if the
// current thread is itself cooperatively scheduled.  This means that
// cooperative threads will NOT allow other cooperative threads to execute in
// their place while waiting for a resource of this type.  Host operating system
// semantics (e.g. a futex) may still be used.
//
// When optional, clients should strongly prefer SCHEDULE_COOPERATIVE_AND_KERNEL
// by default.  SCHEDULE_KERNEL_ONLY should only be used for resources on which
// base::scheduling (e.g. the implementation of a Scheduler) may depend.
//
// NOTE: Cooperative resources may not be nested below non-cooperative ones.
// This means that it is invalid to to acquire a SCHEDULE_COOPERATIVE_AND_KERNEL
// resource if a SCHEDULE_KERNEL_ONLY resource is already held.
enum SchedulingMode {
  SCHEDULE_KERNEL_ONLY = 0,         // Allow scheduling only the host OS.
  SCHEDULE_COOPERATIVE_AND_KERNEL,  // Also allow cooperative scheduling.
};

// SchedulingGuard
// Provides guard semantics that may be used to disable cooperative rescheduling
// of the calling thread within specific program blocks.  This is used to
// protect resources (e.g. low-level SpinLocks or Domain code) that cooperative
// scheduling depends on.
//
// Domain implementations capable of rescheduling in reaction to involuntary
// kernel thread actions (e.g blocking due to a pagefault or syscall) must
// guarantee that an annotated thread is not allowed to (cooperatively)
// reschedule until the annotated region is complete.
//
// It is an error to attempt to use a cooperatively scheduled resource (e.g.
// Mutex) within a rescheduling-disabled region.
//
// All methods are async-signal safe.
class SchedulingGuard {
 public:
  // Returns true iff the calling thread may be cooperatively rescheduled.
  static bool ReschedulingIsAllowed();

 private:
  // Disable cooperative rescheduling of the calling thread.  It may still
  // initiate scheduling operations (e.g. wake-ups), however, it may not itself
  // reschedule.  Nestable.  The returned result is opaque, clients should not
  // attempt to interpret it.
  // REQUIRES: Result must be passed to a pairing EnableScheduling().
  static bool DisableRescheduling();

  // Marks the end of a rescheduling disabled region, previously started by
  // DisableRescheduling().
  // REQUIRES: Pairs with innermost call (and result) of DisableRescheduling().
  static void EnableRescheduling(bool disable_result);

  // A scoped helper for {Disable, Enable}Rescheduling().
  // REQUIRES: destructor must run in same thread as constructor.
  struct ScopedDisable {
    ScopedDisable() { disabled = SchedulingGuard::DisableRescheduling(); }
    ~ScopedDisable() { SchedulingGuard::EnableRescheduling(disabled); }

    bool disabled;
  };

  // Access to SchedulingGuard is explicitly white-listed.
  friend class Downcalls;
  friend class Domain;
  friend class DowncallsTestlets;
  friend class DomainThreadDonator;
  friend class Scheduler;
  friend class ::GoogleOnceInitSchedulingHelper;
  friend class ::SpinLock;

  DISALLOW_COPY_AND_ASSIGN(SchedulingGuard);
};

//------------------------------------------------------------------------------
// End of public interfaces.
//------------------------------------------------------------------------------

inline bool SchedulingGuard::ReschedulingIsAllowed() {
  return false;
}

inline bool SchedulingGuard::DisableRescheduling() {
  return false;
}

inline void SchedulingGuard::EnableRescheduling(bool disable_result) {
  return;
}

}  // namespace scheduling
}  // namespace base

#endif  // BASE_SCHEDULING_LOW_LEVEL_SUPPORT_H_
