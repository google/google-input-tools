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

#ifndef GOOPY_BASE_POSIX_PTHREAD_H_
#define GOOPY_BASE_POSIX_PTHREAD_H_

// These functions are already thread-safe on windows

#define strtok_r(_s, _sep, _lasts) \
    (*(_lasts) = strtok((_s), (_sep)))

#define asctime_r(_tm, _buf) \
    (strcpy((_buf), asctime((_tm))), \
    (_buf))

#define ctime_r(_clock, _buf) \
    (strcpy((_buf), ctime((_clock))), \
    (_buf))

#undef gmtime_r
#define gmtime_r(_clock, _result) \
    (*(_result) = *gmtime((_clock)), \
    (_result))

#define localtime_r(_clock, _result) \
    (*(_result) = *localtime((_clock)), \
    (_result))

#define rand_r(_seed) \
    (_seed == _seed? rand() : rand())

#endif  // GOOPY_BASE_POSIX_PTHREAD_H_
