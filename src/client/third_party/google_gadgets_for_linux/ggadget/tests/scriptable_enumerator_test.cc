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

#include "unittest/gtest.h"
#include "scriptables.h"
#include "ggadget/scriptable_enumerator.h"

class MyItem {
 public:
  MyItem(char data) : data_(data) {
  }

  char GetValue() {
    return data_;
  }

 private:
  char data_;
};

class MyEnumeratable {
 public:
  typedef MyItem ItemType;

  MyEnumeratable(const char *str,
                 bool *flag) : start_(str), str_(str), flag_(flag) {
    ASSERT(str);
  }

  ~MyEnumeratable() {
    if (flag_)
      *flag_ = true;
  }

  void Destroy() {
    delete this;
  }

  bool AtEnd() {
    return *str_ == '\0';
  }

  MyItem *GetItem() {
    return new MyItem(*str_);
  }

  void MoveFirst() {
    str_ = start_;
  }

  void MoveNext() {
    if (*str_)
      ++str_;
  }

  size_t GetCount() {
    return strlen(start_);
  }

 private:
  const char *start_;
  const char *str_;
  bool *flag_;
};

class MyItemWrapper : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x33dff5245c8811dd, ScriptableInterface);

  MyItemWrapper(MyItem *item, int) : data_(item->GetValue()) {
    delete item;
  }

  Variant GetValue() {
    return data_;
  }

 protected:
  virtual void DoClassRegister() {
    RegisterMethod("value", NewSlot(&MyItemWrapper::GetValue));
  }

 private:
  Variant data_;
};

typedef ggadget::ScriptableEnumerator<MyEnumeratable, MyItemWrapper, int,
        UINT64_C(0x09129e0a5c6011dd)> MyScriptableEnumerator;

TEST(ScriptableEnumeratorTest, CreateAndDestroy) {
  bool removed = false;
  BaseScriptable *base = new BaseScriptable(false, true);
  base->Ref();
  MyScriptableEnumerator *enumerator =
      new MyScriptableEnumerator(base, new MyEnumeratable("test", &removed), 0);
  enumerator->Ref();
  enumerator->Unref();
  base->Unref();

  EXPECT_TRUE(removed);
}

char GetItem(MyScriptableEnumerator *e) {
  Variant get_item;
  EXPECT_TRUE(e->GetPropertyInfo("item", &get_item) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(get_item.type() == Variant::TYPE_SLOT);
  // Calls the method "item".
  ResultVariant resulti =
      VariantValue<Slot *>()(get_item)->Call(e, 0, NULL);

  // Retrieves the wrapper.
  MyItemWrapper *wrapper = VariantValue<MyItemWrapper *>()(resulti.v());
  Variant get_value;
  EXPECT_TRUE(wrapper->GetPropertyInfo("value", &get_value) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(get_value.type() == Variant::TYPE_SLOT);
  // Calls the method "value".
  ResultVariant resultv =
      VariantValue<Slot *>()(get_value)->Call(wrapper, 0, NULL);
  // Retrieves the data.
  char data = VariantValue<char>()(resultv.v());
  return data;
}

void MoveFirst(MyScriptableEnumerator *e) {
  Variant move_first;
  EXPECT_TRUE(e->GetPropertyInfo("moveFirst", &move_first) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(move_first.type() == Variant::TYPE_SLOT);
  // Calls the method.
  VariantValue<Slot *>()(move_first)->Call(e, 0, NULL);
}

void MoveNext(MyScriptableEnumerator *e) {
  Variant move_next;
  EXPECT_TRUE(e->GetPropertyInfo("moveNext", &move_next) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(move_next.type() == Variant::TYPE_SLOT);
  // Calls the method.
  VariantValue<Slot *>()(move_next)->Call(e, 0, NULL);
}

bool AtEnd(MyScriptableEnumerator *e) {
  Variant at_end;
  EXPECT_TRUE(e->GetPropertyInfo("atEnd", &at_end) ==
                  ggadget::ScriptableInterface::PROPERTY_METHOD);
  EXPECT_TRUE(at_end.type() == Variant::TYPE_SLOT);
  // Calls the method.
  ResultVariant result =
      VariantValue<Slot *>()(at_end)->Call(e, 0, NULL);
  // Retrieves the wrapper.
  bool data = VariantValue<bool>()(result.v());
  return data;
}

size_t GetCount(MyScriptableEnumerator *e) {
  Variant count;
  EXPECT_TRUE(e->GetPropertyInfo("count", &count) ==
                  ggadget::ScriptableInterface::PROPERTY_NORMAL);
  EXPECT_TRUE(count.type() == Variant::TYPE_INT64);
  // Calls the method.
  ResultVariant result = e->GetProperty("count");
  // Retrieves the wrapper.
  size_t data = VariantValue<uint64_t>()(result.v());
  return data;
}

TEST(ScriptableEnumeratorTest, Enumerate) {
  BaseScriptable *base = new BaseScriptable(false, true);
  base->Ref();
  MyScriptableEnumerator *enumerator =
      new MyScriptableEnumerator(base, new MyEnumeratable( "test", NULL), 0);
  enumerator->Ref();

  EXPECT_EQ(strlen("test"), GetCount(enumerator));

  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('t', GetItem(enumerator));

  MoveNext(enumerator);
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('e', GetItem(enumerator));

  MoveNext(enumerator);
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('s', GetItem(enumerator));

  MoveNext(enumerator);
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('t', GetItem(enumerator));

  MoveNext(enumerator);
  EXPECT_TRUE(AtEnd(enumerator));

  MoveFirst(enumerator);
  EXPECT_FALSE(AtEnd(enumerator));
  EXPECT_EQ('t', GetItem(enumerator));

  enumerator->Unref();
  base->Unref();
}

int main(int argc, char *argv[]) {
  testing::ParseGTestFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
