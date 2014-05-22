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

#ifndef GOOPY_BASE_AT_EXIT_H_
#define GOOPY_BASE_AT_EXIT_H_
#pragma once

#include <stack>

#include "base/basictypes.h"
#include "base/synchronization/lock.h"

namespace base {

// This class provides a facility similar to the CRT atexit(), except that
// we control when the callbacks are executed. Under Windows for a DLL they
// happen at a really bad time and under the loader lock. This facility is
// mostly used by base::Singleton.
//
// The usage is simple. Early in the main() or WinMain() scope create an
// AtExitManager object on the stack:
// int main(...) {
//    base::AtExitManager exit_manager;
//
// }
// When the exit_manager object goes out of scope, all the registered
// callbacks and singleton destructors will be called.

class AtExitManager {
 public:
  typedef void (*AtExitCallbackType)(void*);

  AtExitManager();

  // The dtor calls all the registered callbacks. Do not try to register more
  // callbacks after this point.
  ~AtExitManager();

  // Registers the specified function to be called at exit. The prototype of
  // the callback function is void func().
  static void RegisterCallback(AtExitCallbackType func, void* param);

  // Calls the functions registered with RegisterCallback in LIFO order. It
  // is possible to register new callbacks after calling this function.
  static void ProcessCallbacksNow();

 protected:
  // This constructor will allow this instance of AtExitManager to be created
  // even if one already exists.  This should only be used for testing!
  // AtExitManagers are kept on a global stack, and it will be removed during
  // destruction.  This allows you to shadow another AtExitManager.
  explicit AtExitManager(bool shadow);

 private:
  struct CallbackAndParam {
    CallbackAndParam(AtExitCallbackType func, void* param)
        : func_(func), param_(param) { }
    AtExitCallbackType func_;
    void* param_;
  };

  base::Lock lock_;
  std::stack<CallbackAndParam> stack_;
  AtExitManager* next_manager_;  // Stack of managers to allow shadowing.

  DISALLOW_COPY_AND_ASSIGN(AtExitManager);
};

#if defined(UNIT_TEST)
class ShadowingAtExitManager : public AtExitManager {
 public:
  ShadowingAtExitManager() : AtExitManager(true) {}
};
#endif  // defined(UNIT_TEST)

}  // namespace base

#endif  // GOOPY_BASE_AT_EXIT_H_
