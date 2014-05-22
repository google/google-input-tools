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
//
// Unfortunately <time.h> is different on Windows and Posix. Hence
// google3 .cc files which include <time.h> will get the wrong thing.
// This file wraps the Posix <time.h>.  Sometimes an extra effort is
// required to get this sucker included.

#ifndef GOOPY_BASE_POSIX_CONFLICT_TIME_H_
#define GOOPY_BASE_POSIX_CONFLICT_TIME_H_

#include <assert.h>
#include <pthread.h>
#include <time.h>

struct timespec {
  time_t  tv_sec;   // seconds
  long    tv_nsec;  // nanoseconds
};

#endif  // GOOPY_BASE_POSIX_CONFLICT_TIME_H_
