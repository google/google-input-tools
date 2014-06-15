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

// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/cpu.h"
#include "base/logging.h"

#include "testing/base/public/gunit.h"

// Tests whether we can run extended instructions represented by the CPU
// information. This test actually executes some extended instructions (such as
// MMX, SSE, etc.) supported by the CPU and sees we can run them without
// "undefined instruction" exceptions. That is, this test succeeds when this
// test finishes without a crash.
TEST(CPU, RunExtendedInstructions) {
#if defined(ARCH_CPU_X86_FAMILY)
  // Retrieve the CPU information.
  base::CPU cpu;

#if defined(OS_WIN)
  ASSERT_TRUE(cpu.has_mmx());

  // Execute an MMX instruction.
  __asm emms;

  if (cpu.has_sse()) {
    // Execute an SSE instruction.
    __asm xorps xmm0, xmm0;
  }

  if (cpu.has_sse2()) {
    // Execute an SSE 2 instruction.
    __asm psrldq xmm0, 0;
  }

  if (cpu.has_sse3()) {
    // Execute an SSE 3 instruction.
    __asm addsubpd xmm0, xmm0;
  }

  if (cpu.has_ssse3()) {
    // Execute a Supplimental SSE 3 instruction.
    __asm psignb xmm0, xmm0;
  }

  if (cpu.has_sse41()) {
    // Execute an SSE 4.1 instruction.
    __asm pmuldq xmm0, xmm0;
  }

  if (cpu.has_sse42()) {
    // Execute an SSE 4.2 instruction.
    __asm crc32 eax, eax;
  }
#elif defined(OS_POSIX) && defined(__x86_64__)
  ASSERT_TRUE(cpu.has_mmx());

  // Execute an MMX instruction.
  __asm__ __volatile__("emms\n" : : : "mm0");

  if (cpu.has_sse()) {
    // Execute an SSE instruction.
    __asm__ __volatile__("xorps %%xmm0, %%xmm0\n" : : : "xmm0");
  }

  if (cpu.has_sse2()) {
    // Execute an SSE 2 instruction.
    __asm__ __volatile__("psrldq $0, %%xmm0\n" : : : "xmm0");
  }

  if (cpu.has_sse3()) {
    // Execute an SSE 3 instruction.
    __asm__ __volatile__("addsubpd %%xmm0, %%xmm0\n" : : : "xmm0");
  }

  if (cpu.has_ssse3()) {
    // Execute a Supplimental SSE 3 instruction.
    __asm__ __volatile__("psignb %%xmm0, %%xmm0\n" : : : "xmm0");
  }

  if (cpu.has_sse41()) {
    // Execute an SSE 4.1 instruction.
    __asm__ __volatile__("pmuldq %%xmm0, %%xmm0\n" : : : "xmm0");
  }

  if (cpu.has_sse42()) {
    // Execute an SSE 4.2 instruction.
    __asm__ __volatile__("crc32 %%eax, %%eax\n" : : : "eax");
  }
#endif
#endif
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  logging::InitLogging("", logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
      logging::DONT_LOCK_LOG_FILE, logging::APPEND_TO_OLD_LOG_FILE);

  return RUN_ALL_TESTS();
}
