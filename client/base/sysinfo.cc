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

#include "base/sysinfo.h"

#if defined(OS_WINDOWS)
#include <windows.h>
#elif defined(OS_MACOSX) || defined(__ANDROID__) || defined(OS_NACL)
#include <pthread.h>
#else
#error "Unsupported platform."
#endif  // OS_WINDOWS

pid_t GetTID() {
#if defined(OS_WINDOWS)
  return GetCurrentThreadId();
#elif defined(OS_MACOSX)
  return pthread_mach_thread_np(pthread_self());
#elif defined(__ANDROID__)
  return gettid();
#elif defined(OS_NACL)
  return reinterpret_cast<pid_t>(pthread_self());
#endif  // OS_WINDOWS
}
