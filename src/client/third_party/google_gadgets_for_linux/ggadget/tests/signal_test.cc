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
#include "ggadget/signals.h"
#include "unittest/gtest.h"

#include "slots.h"

using namespace ggadget;

typedef Signal0<void> Signal0Void;
typedef Signal0<bool> Signal0Bool;
typedef Signal9<void, int, bool, const char *, const char *, std::string,
    char, unsigned char, short, std::vector<int> *> Signal9Void;
typedef Signal9<bool, int, bool, const char *, const char *, std::string,
    char, unsigned char, short, const std::vector<int> *> Signal9Bool;
typedef Signal2<void, char, unsigned long> Signal2Void;
typedef Signal2<double, int, double> Signal2Double;
typedef Signal1<Slot *, int> MetaSignal;

typedef Signal9<void, long, bool, std::string, std::string, const char *,
    int, unsigned short, int, std::vector<int> *> Signal9VoidCompatible1;
typedef Signal9<void, long, bool, std::string, std::string, const char *,
    int, unsigned short, int, const std::vector<int> *> Signal9VoidCompatible2;
typedef Signal1<Variant, Variant> SignalVariant;
typedef Signal1<double, int> Signal1Double;
typedef Signal1<void, char> Signal1Void;

static void CheckSlot(int i, Slot *slot) {
  ASSERT_TRUE(slot->HasMetadata());
  ASSERT_EQ(testdata[i].argc, slot->GetArgCount());
  ASSERT_EQ(testdata[i].return_type, slot->GetReturnType());
  for (int j = 0; j < testdata[i].argc; j++)
    ASSERT_EQ(testdata[i].arg_types[j], slot->GetArgTypes()[j]);
  ResultVariant call_result =
      slot->Call(NULL, testdata[i].argc, testdata[i].args);
  ASSERT_EQ(testdata[i].return_value, call_result.v());
  printf("%d: '%s' '%s'\n", i, result.c_str(), testdata[i].result);
  ASSERT_STREQ(testdata[i].result, result.c_str());
}

TEST(signal, SignalBasics) {
  TestClass obj;
  MetaSignal meta_signal;
  Connection *connection = meta_signal.Connect(
      NewSlot(&obj, &TestClass::TestSlotMethod));
  ASSERT_TRUE(connection != NULL);
  ASSERT_EQ(1, meta_signal.GetArgCount());
  ASSERT_EQ(Variant::TYPE_INT64, meta_signal.GetArgTypes()[0]);
  ASSERT_EQ(Variant::TYPE_SLOT, meta_signal.GetReturnType());
  ASSERT_EQ(1U, meta_signal.GetConnectionCount());

  // Initially unblocked.
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    CheckSlot(i, temp_slot);
    delete temp_slot;
  }

  connection->Reconnect(NewSlot(&obj, &TestClass::TestSlotMethod));
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    CheckSlot(i, temp_slot);
    delete temp_slot;
  }

  // Disconnect the connection.
  connection->Disconnect();
  for (int i = 0; i < kNumTestData; i++) {
    Slot *temp_slot = meta_signal(i);
    ASSERT_TRUE(temp_slot == NULL);
  }
  ASSERT_EQ(0U, meta_signal.GetConnectionCount());
}

TEST(signal, SignalConnectNullSlot) {
  TestClass obj;
  MetaSignal meta_signal;
  Connection *connection = meta_signal.Connect(NULL);
  ASSERT_TRUE(connection != NULL);
  ASSERT_TRUE(connection->slot() == NULL);

  connection->Reconnect(NewSlot(&obj, &TestClass::TestSlotMethod));
}

TEST(signal, SignalSlotCompatibility) {
  TestClass obj;
  MetaSignal meta_signal;
  meta_signal.Connect(NewSlot(&obj, &TestClass::TestSlotMethod));

  Signal0Void signal0, signal4, signal11;
  Signal0Bool signal2, signal5, signal13;
  Signal9Void signal1, signal8, signal12;
  Signal9Bool signal3, signal9, signal14;
  Signal2Void signal6, signal10;
  Signal2Double signal7;
  Signal9VoidCompatible1 signal9_compatible1;
  Signal9VoidCompatible2 signal9_compatible2;
  SignalVariant signal15;
  Signal1Double signal16;
  Signal1Void signal17;

  Signal *signals[] = { &signal0, &signal1, &signal2, &signal3, &signal4,
                        &signal5, &signal6, &signal7, &signal8, &signal9,
                        &signal10, &signal11, &signal12, &signal13, &signal14,
                        &signal15, &signal16, &signal17 };

  for (int i = 0; i < kNumTestData; i++)
    ASSERT_TRUE(signals[i]->ConnectGeneral(meta_signal(i)) != NULL);

  // Compatible.
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(0)) != NULL);
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(4)) != NULL);
  // Signal returning void is compatible with slot returning any type.
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(2)) != NULL);
  // Special compatible by variant type automatic conversion.
  ASSERT_TRUE(signal9_compatible1.ConnectGeneral(meta_signal(1)) != NULL);
  ASSERT_TRUE(signal9_compatible1.ConnectGeneral(meta_signal(8)) != NULL);
  ASSERT_TRUE(signal9_compatible2.ConnectGeneral(meta_signal(3)) != NULL);
  ASSERT_TRUE(signal9_compatible2.ConnectGeneral(meta_signal(9)) != NULL);

  // Incompatible.
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(1)) == NULL);
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(7)) == NULL);
  ASSERT_TRUE(signal0.ConnectGeneral(meta_signal(9)) == NULL);
  ASSERT_TRUE(signal2.ConnectGeneral(meta_signal(0)) == NULL);
  ASSERT_TRUE(signal9_compatible1.ConnectGeneral(meta_signal(0)) == NULL);
  ASSERT_TRUE(signal9_compatible1.ConnectGeneral(meta_signal(2)) == NULL);
  ASSERT_TRUE(signal9_compatible1.ConnectGeneral(meta_signal(6)) == NULL);
  ASSERT_TRUE(signal9_compatible1.ConnectGeneral(meta_signal(7)) == NULL);
  ASSERT_TRUE(signal9_compatible2.ConnectGeneral(meta_signal(0)) == NULL);
  ASSERT_TRUE(signal9_compatible2.ConnectGeneral(meta_signal(2)) == NULL);
  ASSERT_TRUE(signal9_compatible2.ConnectGeneral(meta_signal(6)) == NULL);
  ASSERT_TRUE(signal9_compatible2.ConnectGeneral(meta_signal(7)) == NULL);
  ASSERT_TRUE(signal9.ConnectGeneral(meta_signal(8)) == NULL);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
