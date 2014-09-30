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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "ggadget/dbus/dbus_result_receiver.h"
#include "ggadget/logger.h"
#include "ggadget/slot.h"
#include "ggadget/scriptable_array.h"
#include "ggadget/string_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::dbus;

TEST(DBusResultReceiver, SingleResultReceiver) {
  DBusBooleanReceiver bool_receiver;
  DBusIntReceiver int_receiver;
  DBusStringReceiver string_receiver;
  DBusDoubleReceiver double_receiver;

  ASSERT_TRUE(bool_receiver.Callback(0, Variant(true)));
  ASSERT_TRUE(bool_receiver.GetValue());
  ASSERT_TRUE(bool_receiver.Callback(0, Variant(false)));
  ASSERT_FALSE(bool_receiver.GetValue());
  ASSERT_FALSE(bool_receiver.Callback(1, Variant(false)));
  ASSERT_FALSE(bool_receiver.Callback(0, Variant(32)));

  ASSERT_TRUE(int_receiver.Callback(0, Variant(123)));
  ASSERT_EQ(123, int_receiver.GetValue());
  ASSERT_TRUE(string_receiver.Callback(0, Variant("Hello")));
  ASSERT_STREQ("Hello", string_receiver.GetValue().c_str());

  ASSERT_TRUE(double_receiver.Callback(0, Variant(123.456)));
  ASSERT_EQ(123.456, double_receiver.GetValue());

  DBusProxy::ResultCallback *slot = double_receiver.NewSlot();
  ASSERT_TRUE(slot != NULL);
  ASSERT_TRUE((*slot)(0, Variant(456.123)));
  ASSERT_EQ(456.123, double_receiver.GetValue());
  delete slot;
}

TEST(DBusResultReceiver, ArrayResultReceiver) {
  StringVector result;
  DBusStringArrayReceiver result_receiver(&result);
  static const char *values[] = {
    "Hello",
    "World",
    "Foo",
    NULL
  };

  ScriptableArray *array = ScriptableArray::Create(values);
  array->Ref();
  DBusProxy::ResultCallback *slot = result_receiver.NewSlot();

  ASSERT_TRUE((*slot)(0, Variant(array)));

  for (size_t i = 0; values[i]; ++i)
    ASSERT_STREQ(values[i], result[i].c_str());

  delete slot;
  array->Unref();
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
