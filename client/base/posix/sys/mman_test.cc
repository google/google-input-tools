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
// Copyright 2012 Google Inc. All Rights Reserved.
// Author: zyb@google.com (Yuanbo Zhang)

#include "i18n/input/engine/stubs/posix/sys/mman.h"

#include <windows.h>
#include "base/macros.h"
#include "testing/base/public/gunit.h"

TEST(MmapTest, TestMapAndUnmap) {
  SYSTEM_INFO sysinfo = { 0 };
  GetSystemInfo(&sysinfo);
  printf("Page size: %d\n", sysinfo.dwPageSize);

  static const int kRequestSize[] = {
    sysinfo.dwPageSize - 1,
    sysinfo.dwPageSize,
    sysinfo.dwPageSize + 1,
    2 * sysinfo.dwPageSize - 1,
    2 * sysinfo.dwPageSize,
    2 * sysinfo.dwPageSize + 1,
  };
  static const int kRoundedSize[] = {
    sysinfo.dwPageSize,
    sysinfo.dwPageSize,
    2 * sysinfo.dwPageSize,
    2 * sysinfo.dwPageSize,
    2 * sysinfo.dwPageSize,
    3 * sysinfo.dwPageSize,
  };
  for (int i = 0; i < arraysize(kRequestSize); ++i) {
    void* addr = mmap(NULL, kRequestSize[i], PROT_READ, MAP_PRIVATE, -1, 0);
    EXPECT_EQ(0, munmap(addr, kRoundedSize[i])) << i;
  }

  for (int i = 0; i < arraysize(kRequestSize); ++i) {
    void* addr = mmap(NULL, kRequestSize[i] + sysinfo.dwPageSize,
                      PROT_READ, MAP_PRIVATE, -1, 0);
    EXPECT_EQ(-1, munmap(addr, kRoundedSize[i])) << i;
  }
}
