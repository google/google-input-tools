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

#include "base/mutex.h"
#include <gtest/gtest.h>

static const int kReaderNum = 100;
static const int kWriterNum = 100;

static int shared_count = 0;
static int num_reader = 0;

static Mutex mutex1(base::LINKER_INITIALIZED);
static Mutex mutex2(base::LINKER_INITIALIZED);

DWORD WINAPI Read(LPVOID param) {
  // Wait a moment for writers to be created.
  Sleep(100);

  ReaderMutexLock lock(&mutex1);
  EXPECT_TRUE(lock.IsLocked());

  int old_shared_count = shared_count;

  {
    WriterMutexLock tmp(&mutex2);
    EXPECT_TRUE(tmp.IsLocked());
    num_reader++;
  }
  int current_num_reader;
  // This is a dead loop if readers cannot access concurrently which is not
  // expected. (Reader vs Reader)
  do {
    ReaderMutexLock tmp(&mutex2);
    EXPECT_TRUE(tmp.IsLocked());
    current_num_reader = num_reader;
  } while (current_num_reader == 1);

  Sleep(500);

  // Writer should not access during our reading. (Reader vs Writer)
  EXPECT_TRUE(old_shared_count == shared_count);
  return 0;
}

DWORD WINAPI Write(LPVOID param) {
  WriterMutexLock lock(&mutex1);
  EXPECT_TRUE(lock.IsLocked());

  int old_count = shared_count;
  shared_count++;

  Sleep(500);

  // Other writers should not access during our writing (Writer vs Writer);
  EXPECT_TRUE(old_count + 1 == shared_count);
  return 0;
}

TEST(TestMutex, TestRWLock) {
  // Create reader and writer.
  HANDLE thread_array[kReaderNum + kWriterNum];
  int i = 0, j = 0;
  for (; i < kReaderNum; i++)
    thread_array[i] = CreateThread(NULL, 0, Read, NULL, 0, NULL);
  for (; j < kWriterNum; j++)
    thread_array[i + j] = CreateThread(NULL, 0, Write, NULL, 0, NULL);
  WaitForMultipleObjects(kReaderNum + kWriterNum, thread_array, true, INFINITE);
  for (size_t i = 0; i < kReaderNum + kWriterNum; ++i) {
    CloseHandle(thread_array[i]);
  }
}
