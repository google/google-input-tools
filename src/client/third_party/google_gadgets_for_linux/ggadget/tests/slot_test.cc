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
#include "ggadget/slot.h"
#include "unittest/gtest.h"

#include "slots.h"

using namespace ggadget;

TEST(slot, Slot) {
  TestClass obj;
  Slot *meta_slot = NewSlot(&obj, &TestClass::TestSlotMethod);
  ASSERT_TRUE(meta_slot->HasMetadata());
  ASSERT_EQ(1, meta_slot->GetArgCount());
  ASSERT_EQ(Variant::TYPE_INT64, meta_slot->GetArgTypes()[0]);
  ASSERT_EQ(Variant::TYPE_SLOT, meta_slot->GetReturnType());
  for (int i = 0; i < kNumTestData; i++) {
    Variant param(i);
    Slot *slot = VariantValue<Slot *>()(meta_slot->Call(NULL, 1, &param).v());
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
    delete slot;
  }
  delete meta_slot;
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
