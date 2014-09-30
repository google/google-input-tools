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
//
// One of a suite of files emulating Posix for MSVC; see ../README.txt

#ifndef I18N_INPUT_ENGINE_STUBS_POSIX_SYS_MMAN_H_
#define I18N_INPUT_ENGINE_STUBS_POSIX_SYS_MMAN_H_

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#define PROT_NONE  0x0

#define MAP_FAILED ((void*) -1)  // NOLINT

#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_TYPE        0x0f
#define MAP_FIXED       0x10
#define MAP_FILE        0
#define MAP_ANON        0x20
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS   MAP_ANON
#endif
#define MAP_GROWSDOWN   0x0100
#define MAP_DENYWRITE   0x0800
#define MAP_EXECUTABLE  0x1000
#define MAP_LOCKED      0x2000
#define MAP_NORESERVE   0x4000

int mlock(const void *addr, size_t len);
int munlock(const void *addr, size_t len);

void* mmap(void* start, size_t length, int prot, int flags, int fd,
           off_t offset);

// The length must be multiples of page size.
int munmap(void* start, size_t length);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // I18N_INPUT_ENGINE_STUBS_POSIX_SYS_MMAN_H_
