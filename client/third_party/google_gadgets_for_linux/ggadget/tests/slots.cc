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

#include "ggadget/slot.h"
#include "slots.h"

using namespace ggadget;

// Hold the result of test functions/methods.
std::string result;

Slot *TestClass::TestSlotMethod(int i) {
  switch (i) {
    case 0: return NewSlot(&TestVoidFunction0);
    case 1: return NewSlot(&TestVoidFunction9);
    case 2: return NewSlot(&TestBoolFunction0);
    case 3: return NewSlot(&TestBoolFunction9);
    case 4: return NewSlot(this, &TestClass::TestVoidMethod0);
    case 5: return NewSlot(this, &TestClass::TestBoolMethod0);
    case 6: return NewSlot(this, &TestClass::TestVoidMethod2);
    case 7: return NewSlot(this, &TestClass::TestDoubleMethod2);
    case 8: return NewSlot(this, &TestClass::TestVoidMethod9);
    case 9: return NewSlot(this, &TestClass::TestBoolMethod9);
    case 10: return NewSlot((TestClass0 *)this, &TestClass0::TestVoidMethod2);
    case 11: return NewFunctorSlot<void>
                                  (TestVoidFunctor0());
    case 12: return NewFunctorSlot<void,
                                   int, bool, const char *,
                                   const std::string &, std::string, char,
                                   unsigned char, short,
                                   std::vector<int> *>
                                  (TestVoidFunctor9());
    case 13: return NewFunctorSlot<bool>
                                  (TestBoolFunctor0());
    case 14: return NewFunctorSlot<bool,
                                   int, bool, const char *,
                                   const std::string &, std::string, char,
                                   unsigned char, short,
                                   const std::vector<int> *>
                                  (TestBoolFunctor9());
    case 15: return NewSlot(&TestVariant);
    case 16: return NewSlot(this, &TestClass::TestDoubleMethod2, 1.234);
    case 17: return NewSlot(this, &TestClass::TestVoidMethod2, 0x1234UL);
    default: return NULL;
  }
}

std::string str_b("bbb");
std::string str_c("ccc");
std::string str_e("eee");
std::string str_f("fff");

class VectorHolder {
 public:
  VectorHolder(int i) {
    v.push_back(i);
  }
  std::vector<int> v;
};

VectorHolder vector_holder1(999);
VectorHolder vector_holder2(888);

TestData testdata[] = {
  { 0, Variant::TYPE_VOID, { }, { }, Variant(), "TestVoidFunction0" },
  { 9, Variant::TYPE_VOID, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_ANY,
                           },
                           { Variant(1),
                             Variant(true),
                             Variant("a"),
                             Variant(str_b),
                             Variant(str_c),
                             Variant('x'),
                             Variant('y'),
                             Variant(100),
                             Variant(&vector_holder1.v),
                           },
    Variant(), "TestVoidFunction9: 1 1 a bbb ccc x y 100 999" },
  { 0, Variant::TYPE_BOOL, { }, { }, Variant(false), "TestBoolFunction0" },
  { 9, Variant::TYPE_BOOL, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_CONST_ANY,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("d"),
                             Variant(str_e),
                             Variant(str_f),
                             Variant('X'),
                             Variant('Y'),
                             Variant(-222),
                             Variant(&vector_holder2.v),
                           },
    Variant(true), "TestBoolFunction9: 100 0 d eee fff X Y -222 888" },
  { 0, Variant::TYPE_VOID, { }, { }, Variant(), "TestVoidMethod0" },
  { 0, Variant::TYPE_BOOL, { }, { }, Variant(true), "TestBoolMethod0" },
  { 2, Variant::TYPE_VOID, { Variant::TYPE_INT64, Variant::TYPE_INT64 },
                           { Variant('a'), Variant(0xffffffffUL) },
    Variant(), "TestVoidMethod2: a ffffffff" },
  { 2, Variant::TYPE_DOUBLE, { Variant::TYPE_INT64, Variant::TYPE_DOUBLE },
                             { Variant(-999), Variant(-3.14) },
    Variant(2.0), "TestDoubleMethod2: -999 -3.140" },
  { 9, Variant::TYPE_VOID, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_ANY,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("a"),
                             Variant(str_b),
                             Variant(str_c),
                             Variant('x'),
                             Variant('y'),
                             Variant(999),
                             Variant(&vector_holder1.v),
                           },
    Variant(), "TestVoidMethod9: 100 0 a bbb ccc x y 999 999" },
  { 9, Variant::TYPE_BOOL, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_CONST_ANY,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("d"),
                             Variant(str_e),
                             Variant(str_f),
                             Variant('X'),
                             Variant('Y'),
                             Variant(222),
                             Variant(&vector_holder2.v),
                           },
    Variant(false), "TestBoolMethod9: 100 0 d eee fff X Y 222 888" },
  { 2, Variant::TYPE_VOID, { Variant::TYPE_INT64, Variant::TYPE_INT64 },
                           { Variant('a'), Variant(0xffffffffUL) },
    Variant(), "TestVoidMethod2: a ffffffff" },
  { 0, Variant::TYPE_VOID, { }, { }, Variant(), "TestVoidFunctor0" },
  { 9, Variant::TYPE_VOID, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_ANY,
                           },
                           { Variant(1),
                             Variant(true),
                             Variant("a"),
                             Variant(str_b),
                             Variant(str_c),
                             Variant('x'),
                             Variant('y'),
                             Variant(100),
                             Variant(&vector_holder1.v),
                           },
    Variant(), "TestVoidFunctor9: 1 1 a bbb ccc x y 100 999" },
  { 0, Variant::TYPE_BOOL, { }, { }, Variant(false), "TestBoolFunctor0" },
  { 9, Variant::TYPE_BOOL, { Variant::TYPE_INT64,
                             Variant::TYPE_BOOL,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_STRING,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_INT64,
                             Variant::TYPE_CONST_ANY,
                           },
                           { Variant(100),
                             Variant(false),
                             Variant("d"),
                             Variant(str_e),
                             Variant(str_f),
                             Variant('X'),
                             Variant('Y'),
                             Variant(-222),
                             Variant(&vector_holder2.v),
                           },
    Variant(true), "TestBoolFunctor9: 100 0 d eee fff X Y -222 888" },
  { 1, Variant::TYPE_VARIANT, { Variant::TYPE_VARIANT },
    { Variant(Variant::TYPE_VARIANT) }, Variant(Variant::TYPE_VARIANT),
    "VARIANT" },
  { 1, Variant::TYPE_DOUBLE, { Variant::TYPE_INT64 }, { Variant(-999) },
    Variant(2.0), "TestDoubleMethod2: -999 1.234" },
  { 1, Variant::TYPE_VOID, { Variant::TYPE_INT64 }, { Variant('a') },
    Variant(), "TestVoidMethod2: a 1234" }
};

const int kNumTestData = arraysize(testdata);
