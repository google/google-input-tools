/*
  Copyright 2008 Google Inc.

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

#include <stdio.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/framework_interface.h>
#include <ggadget/variant.h>
#include "ggadget/tests/native_main_loop.h"
#include <unittest/gtest.h>
#include "../perfmon.cc"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

static const int kInvalidWatchId = -1;

static const Variant kInvalidCpuUsage(0.0);

static NativeMainLoop g_main_loop;

// the dummy function call lost for test
static void MockFunctionCallSlot(const char *name, const Variant &id) {
  printf("%s\n", name);
}

// Normal test.
TEST(Perfmon, AddCounter_Success) {
  Perfmon perfmon;
  int watch_id = perfmon.AddCounter(kPerfmonCpuUsage,
                                    NewSlot(MockFunctionCallSlot));
  EXPECT_GE(watch_id, 0);
}

// Failure test for AddCounter when the input count path is NULL
TEST(Perfmon, AddCounter_Failure_NullCounterPath) {
  Perfmon perfmon;
  int watch_id = perfmon.AddCounter(NULL, NewSlot(MockFunctionCallSlot));
  EXPECT_EQ(kInvalidWatchId, watch_id);
}

// Failure test for AddCounter when the input call back slot is NULL
TEST(Perfmon, AddCounter_Failure_NullCallBackSlot) {
  Perfmon perfmon;
  int watch_id = perfmon.AddCounter(kPerfmonCpuUsage, NULL);
  EXPECT_EQ(kInvalidWatchId, watch_id);
}

// Failure test for AddCounter when the input counter path is empty.
TEST(Perfmon, AddCounter_Failure_EmptyCounterPath) {
  Perfmon perfmon;
  int watch_id = perfmon.AddCounter("", NewSlot(MockFunctionCallSlot));
  EXPECT_EQ(kInvalidWatchId, watch_id);
}

// Failure test for AddCounter when the input counter path is invalid.
TEST(Perfmon, AddCounter_Failure_InvalidCounterPath) {
  Perfmon perfmon;
  int watch_id = perfmon.AddCounter("MOMERY", NewSlot(MockFunctionCallSlot));
  EXPECT_EQ(kInvalidWatchId, watch_id);
}

// Accuracy test for GetCurrentValue.
TEST(Perfmon, GetCurrentValue_Accuray) {
  Perfmon perfmon;
  Variant value;
  for (int i = 0; i < 10; ++i) {
    value = perfmon.GetCurrentValue(kPerfmonCpuUsage);
    EXPECT_EQ(Variant::TYPE_DOUBLE, value.type());
    double result = 0.0;
    value.ConvertToDouble(&result);
    EXPECT_GE(result, 0.0);
    EXPECT_LE(result, 100.0);
    LOG("The current CPU usage: %lf\n", result);
  }
}

// Failure test for GetCurrentValue when the input counter path is NULL.
TEST(Perfmon, GetCurrentValue_Failure_NullCounterPath) {
  Perfmon perfmon;
  Variant value = perfmon.GetCurrentValue(NULL);
  EXPECT_EQ(kInvalidCpuUsage, value);
}

// Failure test for GetCurrentValue when the input counter path is empty.
TEST(Perfmon, GetCurrentValue_Failure_EmptyCounterPath) {
  Perfmon perfmon;
  Variant value = perfmon.GetCurrentValue("");
  EXPECT_EQ(kInvalidCpuUsage, value);
}

// Failure test for GetCurrentValue when the input counter path is invalid.
TEST(Perfmon, GetCurrentValue_Failure_InvalidCounterPath) {
  Perfmon perfmon;
  Variant value = perfmon.GetCurrentValue("MEM");
  EXPECT_EQ(kInvalidCpuUsage, value);
}

int main(int argc, char *argv[]) {
  testing::ParseGTestFlags(&argc, argv);
  SetGlobalMainLoop(&g_main_loop);
  return RUN_ALL_TESTS();
}
