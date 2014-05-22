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

#define UNIT_TEST
#include "base/at_exit.h"
#include "base/logging.h"

#include "testing/base/public/gunit.h"

namespace {

int g_test_counter_1 = 0;
int g_test_counter_2 = 0;

void IncrementTestCounter1(void* unused) {
  ++g_test_counter_1;
}

void IncrementTestCounter2(void* unused) {
  ++g_test_counter_2;
}

void ZeroTestCounters() {
  g_test_counter_1 = 0;
  g_test_counter_2 = 0;
}

void ExpectCounter1IsZero(void* unused) {
  EXPECT_EQ(0, g_test_counter_1);
}

void ExpectParamIsNull(void* param) {
  EXPECT_EQ(static_cast<void*>(NULL), param);
}

void ExpectParamIsCounter(void* param) {
  EXPECT_EQ(&g_test_counter_1, param);
}

}  // namespace

class AtExitTest : public testing::Test {
 private:
  // Don't test the global AtExitManager, because asking it to process its
  // AtExit callbacks can ruin the global state that other tests may depend on.
  base::ShadowingAtExitManager exit_manager_;
};

TEST_F(AtExitTest, Basic) {
  ZeroTestCounters();
  base::AtExitManager::RegisterCallback(&IncrementTestCounter1, NULL);
  base::AtExitManager::RegisterCallback(&IncrementTestCounter2, NULL);
  base::AtExitManager::RegisterCallback(&IncrementTestCounter1, NULL);

  EXPECT_EQ(0, g_test_counter_1);
  EXPECT_EQ(0, g_test_counter_2);
  base::AtExitManager::ProcessCallbacksNow();
  EXPECT_EQ(2, g_test_counter_1);
  EXPECT_EQ(1, g_test_counter_2);
}

TEST_F(AtExitTest, LIFOOrder) {
  ZeroTestCounters();
  base::AtExitManager::RegisterCallback(&IncrementTestCounter1, NULL);
  base::AtExitManager::RegisterCallback(&ExpectCounter1IsZero, NULL);
  base::AtExitManager::RegisterCallback(&IncrementTestCounter2, NULL);

  EXPECT_EQ(0, g_test_counter_1);
  EXPECT_EQ(0, g_test_counter_2);
  base::AtExitManager::ProcessCallbacksNow();
  EXPECT_EQ(1, g_test_counter_1);
  EXPECT_EQ(1, g_test_counter_2);
}

TEST_F(AtExitTest, Param) {
  base::AtExitManager::RegisterCallback(&ExpectParamIsNull, NULL);
  base::AtExitManager::RegisterCallback(&ExpectParamIsCounter,
                                        &g_test_counter_1);
  base::AtExitManager::ProcessCallbacksNow();
}
