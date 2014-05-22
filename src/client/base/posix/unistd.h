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

// One of a suite of files emulating POSIX for MSVC; see README.txt
//
// Lots more stuff than really belongs in unistd.h has been shoveled into this
// file. Some of it is held here temporarily, and some of it is just here
// because we want it lots of places, and unistd.h is included in lots of
// places.

#ifndef GOOPY_BASE_POSIX_UNISTD_H_
#define GOOPY_BASE_POSIX_UNISTD_H_

#include <process.h>

#if _MSC_VER < 1600
#define ECONNREFUSED WSAECONNREFUSED
#define EWOULDBLOCK  WSAEWOULDBLOCK
#define EINPROGRESS  WSAEINPROGRESS
#define EDESTADDRREQ WSAEDESTADDRREQ
#endif  // _MSC_VER < 1600

#define _execl  execl
#define _execlp execlp
#define _execle execle
#define _execv  execv
#define _execvp execvp

#ifdef __cplusplus
extern "C" {
#endif

  size_t getpagesize();

  // Windows actually has these functions too, but with the underscore
  // prefix just to make porting difficult, or something.
  #define stat _stat
  #define fstat _fstat
  #define lstat _stat

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // GOOPY_BASE_POSIX_UNISTD_H_
