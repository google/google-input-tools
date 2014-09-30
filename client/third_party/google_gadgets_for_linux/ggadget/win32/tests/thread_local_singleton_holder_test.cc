/* Copyright 2011 Google Inc.

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

#include <windows.h>
#include "ggadget/win32/thread_local_singleton_holder.h"
#include "unittest/gtest.h"

namespace {

void BaseTest() {
  ASSERT_FALSE(ggadget::win32::ThreadLocalSingletonHolder<DWORD>::GetValue());

  DWORD value;
  ASSERT_TRUE(ggadget::win32::ThreadLocalSingletonHolder<DWORD>::SetValue(&value));

  ASSERT_EQ(
      &value, ggadget::win32::ThreadLocalSingletonHolder<DWORD>::GetValue());
}

DWORD WINAPI TestThreadProc(void *parameter) {
  BaseTest();
  return 0;
}

} // namespace

TEST(ThreadLocalSingletonHolderTest, BaseTest) {
  BaseTest();
  DWORD thread_id;
  HANDLE thread_handle =
      CreateThread(NULL, 0, TestThreadProc, NULL, 0, &thread_id);
  ASSERT_NE(INVALID_HANDLE_VALUE, thread_handle);
  ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(thread_handle, INFINITE));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
