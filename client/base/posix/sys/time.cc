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
// Copyright (c) 2012 and onward, Google Inc, all rights reserved
// Author: Eric Fredricksen
// One of a suite of files emulating POSIX for MSVC; see ../README.txt

#include "i18n/input/engine/stubs/posix/sys/time.h"

#include <assert.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
  int gettimeofday(struct timeval *tv, struct timezone *tz) {
    assert(tz == NULL);
    assert(tv);
    const DWORD now = GetTickCount();
    tv->tv_sec = now / 1000;
    tv->tv_usec = (now % 1000) * 1000;
    return 0;
  }
#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
