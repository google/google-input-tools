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

#include "base/posix/sys/mman.h"

#include "base/port.h"
#include "base/logging.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
  int mlock(const void *addr, size_t len) {
    // This would probably work fine if needed, but so far it ain't.
    return VirtualLock(const_cast<LPVOID>(addr), len) ? 0 : -1;
  }

  int munlock(const void *addr, size_t len) {
    // This would probably work fine if needed, but so far it ain't.
    return VirtualUnlock(const_cast<LPVOID>(addr), len) ? 0 : -1;
  }

  void* mmap(void *start, size_t length, int prot, int flags, int fd,
             off_t offset) {
    DCHECK_EQ(fd, -1) << "mmap() doesn't yet support mapping of files";
    DCHECK_EQ(offset, 0) << "mmap() doesn't yet support mapping of files "
                            "at non-zero offsets";
    if (fd != -1 || offset != 0) {
      return MAP_FAILED;
    }
    const int ok_flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if (0 != (flags & ~ok_flags)) {
      DLOG(FATAL) << "unsupported flags in mmap() " << hex << flags;
      return MAP_FAILED;
    }
    DWORD protect = PAGE_NOACCESS;
    if (prot == (PROT_WRITE|PROT_READ)) {
      protect = PAGE_READWRITE;
    } else if (prot == PROT_READ) {
      protect = PAGE_READONLY;
    } else if (prot == (PROT_READ|PROT_EXEC)) {
      protect = PAGE_EXECUTE_READ;
    } else if (prot == (PROT_READ|PROT_EXEC|PROT_WRITE)) {
      protect = PAGE_EXECUTE_READWRITE;
    } else if (prot == PROT_EXEC) {
      protect = PAGE_EXECUTE;
    } else if (prot == PROT_NONE) {
      protect = PAGE_NOACCESS;
    } else {
      DLOG(FATAL) << "unsupported protection in mmap() " << hex << prot;
      return MAP_FAILED;
    }
    // Memory allocated by VirtualAlloc is automatically initialized to zero,
    // unless MEM_RESET is specified, and we never specify MEM_RESET.
    void *result = VirtualAlloc(start, length, MEM_RESERVE|MEM_COMMIT, protect);
    return 0 == result ? MAP_FAILED : result;
  }

  int munmap(void* start, size_t length) {
    // Queries size of consecutive memory pages allocated by a call to
    // VirtualAlloc before.
    MEMORY_BASIC_INFORMATION info = { 0 };
    SIZE_T info_size = VirtualQuery(start, &info, sizeof(info));
    if (info_size == 0) {
      return -1;
    }
    if (info.AllocationBase != start ||
        info.RegionSize != length ||
        info.State != MEM_COMMIT) {
      return -1;
    }
    return VirtualFree(start, 0, MEM_RELEASE) ? 0 : -1;
  }

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
