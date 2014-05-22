/*
  Copyright 2011 Google Inc.

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

#include <set>
#include "ggadget/string_utils.h"
#include "unittest/gtest.h"
#include "scriptables.h"

using namespace ggadget;

// Structure of expected property information.
struct PropertyInfo {
  const char *name;
  ScriptableInterface::PropertyType type;
  Variant prototype;
};

static bool SlotPrototypeEquals(Slot *s1, Slot *s2) {
  if (s1 == s2) return true;
  if (!s1 || !s2) return false;
  if (s1->GetArgCount() != s2->GetArgCount()) return false;
  for (int i = 0; i < s1->GetArgCount(); i++) {
    if (s1->GetArgTypes()[i] != s2->GetArgTypes()[i])
      return false;
  }
  return true;
}

static void CheckProperty(ScriptableInterface *scriptable,
                          const PropertyInfo &info) {
  printf("CheckProperty: %s\n", info.name);
  Variant prototype;
  ScriptableInterface::PropertyType type =
      scriptable->GetPropertyInfo(info.name, &prototype);
  ASSERT_EQ(info.type, type);
  if (info.prototype.type() == Variant::TYPE_SLOT) {
    ASSERT_TRUE(SlotPrototypeEquals(VariantValue<Slot *>()(info.prototype),
                                    VariantValue<Slot *>()(prototype)));
  } else {
    ASSERT_EQ(info.prototype, prototype);
  }
  ASSERT_EQ(info.type, scriptable->GetPropertyInfo(info.name, NULL));
}

static void CheckFalseProperty(ScriptableInterface *scriptable,
                               const char *name) {
  ASSERT_EQ(ScriptableInterface::PROPERTY_NOT_EXIST,
            scriptable->GetPropertyInfo(name, NULL));
}

static void CheckPropertyInfo(bool register_class) {
  BaseScriptable *scriptable = new BaseScriptable(true, register_class);
  ASSERT_STREQ("", g_buffer.c_str());

  // Expected property information for BaseScriptable.
  PropertyInfo property_info[] = {
    { "ClearBuffer", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable, &BaseScriptable::ClearBuffer)) },
    { "MethodDouble2", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable, &BaseScriptable::MethodDouble2)) },
    { "DoubleProperty", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_DOUBLE) },
    { "BufferReadOnly", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "Buffer", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "JSON", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_JSON) },
    { "my_ondelete", ScriptableInterface::PROPERTY_NORMAL,
      Variant(new SignalSlot(&scriptable->my_ondelete_signal_)) },
    { "EnumSimple", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_INT64) },
    { "EnumString", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "VariantProperty", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_VARIANT) },
  };

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    CheckProperty(scriptable, property_info[i]);
  }
  CheckFalseProperty(scriptable, "not_exist");

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    if (property_info[i].prototype.type() == Variant::TYPE_SLOT)
      delete VariantValue<Slot *>()(property_info[i].prototype);
  }

  delete scriptable;
  EXPECT_STREQ("Destruct\n", g_buffer.c_str());
}

TEST(ScriptableHelperTest, TestPropertyInfo) {
  CheckPropertyInfo(true);
  CheckPropertyInfo(false);
}

static void TestOnRefChange(int ref, int change) {
  AppendBuffer(StringPrintf("TestRefChange(%d,%d)\n", ref, change).c_str());
}

static void TestOnDeleteAsEventSink() {
  AppendBuffer("TestOnDeleteAsEventSink\n");
}

static void CheckOnDelete(bool register_class) {
  BaseScriptable *scriptable = new BaseScriptable(true, register_class);
  ASSERT_STREQ("", g_buffer.c_str());
  ASSERT_TRUE(scriptable->ConnectOnReferenceChange(NewSlot(TestOnRefChange)));
  scriptable->SetProperty("my_ondelete",
                          Variant(NewSlot(TestOnDeleteAsEventSink)));
  delete scriptable;
  EXPECT_STREQ("TestOnDeleteAsEventSink\nDestruct\n"
               "TestRefChange(1,-1)\nTestRefChange(0,0)\n",
               g_buffer.c_str());
}

TEST(ScriptableHelperTest, TestOnDelete) {
  CheckOnDelete(true);
  CheckOnDelete(false);
}

static void CheckPropertyAndMethod(bool register_class) {
  BaseScriptable *scriptable = new BaseScriptable(true, register_class);
  ASSERT_STREQ("", g_buffer.c_str());
  ASSERT_EQ(Variant(""), scriptable->GetProperty("BufferReadOnly").v());
  AppendBuffer("TestBuffer\n");
  ASSERT_FALSE(scriptable->SetProperty("BufferReadOnly",
                                       Variant("Buffer\n")));
  ASSERT_EQ(Variant("TestBuffer\n"),
            scriptable->GetProperty("BufferReadOnly").v());
  g_buffer.clear();

  ASSERT_EQ(Variant(0.0), scriptable->GetProperty("DoubleProperty").v());
  ASSERT_STREQ("GetDoubleProperty()=0.000\n", g_buffer.c_str());
  g_buffer.clear();
  ASSERT_TRUE(scriptable->SetProperty("DoubleProperty", Variant(3.25)));
  ASSERT_STREQ("SetDoubleProperty(3.250)\n", g_buffer.c_str());
  g_buffer.clear();
  ASSERT_EQ(Variant(3.25), scriptable->GetProperty("DoubleProperty").v());
  ASSERT_STREQ("GetDoubleProperty()=3.250\n", g_buffer.c_str());

  Variant result1(scriptable->GetProperty("ClearBuffer").v());
  ASSERT_EQ(result1.type(), Variant::TYPE_SLOT);
  ASSERT_EQ(Variant(),
            VariantValue<Slot *>()(result1)->Call(scriptable, 0, NULL).v());
  ASSERT_STREQ("", g_buffer.c_str());

  ASSERT_EQ(Variant(BaseScriptable::VALUE_0),
            scriptable->GetProperty("EnumSimple").v());
  ASSERT_TRUE(scriptable->SetProperty("EnumSimple",
                                      Variant(BaseScriptable::VALUE_2)));
  ASSERT_EQ(Variant(BaseScriptable::VALUE_2),
            scriptable->GetProperty("EnumSimple").v());

  ASSERT_EQ(Variant("VALUE_2"), scriptable->GetProperty("EnumString").v());
  ASSERT_TRUE(scriptable->SetProperty("EnumString", Variant("VALUE_0")));
  ASSERT_EQ(Variant(BaseScriptable::VALUE_0),
            scriptable->GetProperty("EnumSimple").v());
  ASSERT_EQ(Variant("VALUE_0"), scriptable->GetProperty("EnumString").v());
  ASSERT_TRUE(scriptable->SetProperty("EnumString", Variant("VALUE_INVALID")));
  ASSERT_EQ(Variant(BaseScriptable::VALUE_0),
            scriptable->GetProperty("EnumSimple").v());
  ASSERT_EQ(Variant("VALUE_0"), scriptable->GetProperty("EnumString").v());

  ASSERT_EQ(Variant(0), scriptable->GetProperty("VariantProperty").v());
  ASSERT_TRUE(scriptable->SetProperty("VariantProperty", Variant(1234)));
  ASSERT_EQ(Variant(1234), scriptable->GetProperty("VariantProperty").v());
  delete scriptable;
}

TEST(ScriptableHelperTest, TestPropertyAndMethod) {
  CheckPropertyAndMethod(true);
  CheckPropertyAndMethod(false);
}

static void CheckConstant(const char *name,
                          ScriptableInterface *scriptable,
                          Variant value) {
  printf("CheckConstant: %s\n", name);
  Variant prototype;
  ASSERT_EQ(ScriptableInterface::PROPERTY_CONSTANT,
            scriptable->GetPropertyInfo(name, &prototype));
  ASSERT_EQ(value, prototype);
}

static void CheckConstants(bool register_class) {
  BaseScriptable *scriptable = new BaseScriptable(true, register_class);
  CheckConstant("Fixed", scriptable, Variant(123456789));
  for (int i = 0; i < 10; i++) {
    char name[20];
    sprintf(name, "ICONSTANT%d", i);
    CheckConstant(name, scriptable, Variant(i));
    sprintf(name, "SCONSTANT%d", i);
    CheckConstant(name, scriptable, Variant(name));
  }
  delete scriptable;
}

TEST(ScriptableHelperTest, TestConstants) {
  CheckConstants(true);
  CheckConstants(false);
}

static void CheckExtPropertyInfo(bool register_class) {
  ExtScriptable *scriptable = new ExtScriptable(true, true, register_class);
  BaseScriptable *scriptable1 = scriptable;
  ASSERT_STREQ("", g_buffer.c_str());

  // Expected property information for BaseScriptable.
  PropertyInfo property_info[] = {
    // These are inherited from BaseScriptable.
    { "ClearBuffer", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable1, &BaseScriptable::ClearBuffer)) },
    { "MethodDouble2", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable1, &BaseScriptable::MethodDouble2)) },
    { "DoubleProperty", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_DOUBLE) },
    { "BufferReadOnly", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "Buffer", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "JSON", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_JSON) },
    { "my_ondelete", ScriptableInterface::PROPERTY_NORMAL,
      Variant(new SignalSlot(&scriptable->my_ondelete_signal_)) },
    { "EnumSimple", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_INT64) },
    { "EnumString", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "VariantProperty", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_VARIANT) },

    // The following are defined in ExtScriptable itself.
    { "ObjectMethod", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable, &ExtScriptable::ObjectMethod)) },
    { "onlunch", ScriptableInterface::PROPERTY_NORMAL,
      Variant(new SignalSlot(
          &(ExtScriptable::GetInner(scriptable)->onlunch_signal_))) },
    { "onsupper", ScriptableInterface::PROPERTY_NORMAL,
      Variant(new SignalSlot(
          &(ExtScriptable::GetInner(scriptable)->onsupper_signal_))) },
    { "time", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "OverrideSelf", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_SCRIPTABLE) },
    { "SignalResult", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_STRING) },
    { "NewObject", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlotWithDefaultArgs(
          NewSlot(scriptable, &ExtScriptable::NewObject),
          kNewObjectDefaultArgs)) },
    { "ReleaseObject", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlotWithDefaultArgs(
          NewSlot(scriptable, &ExtScriptable::ReleaseObject),
          kReleaseObjectDefaultArgs)) },
    { "NativeOwned", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_BOOL) },
    { "ConcatArray", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable, &ExtScriptable::ConcatArray)) },
    { "SetCallback", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable, &ExtScriptable::SetCallback)) },
    { "CallCallback", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(scriptable, &ExtScriptable::CallCallback)) },
    { "oncomplex", ScriptableInterface::PROPERTY_NORMAL,
      Variant(new SignalSlot(
          &(ExtScriptable::GetInner(scriptable)->complex_signal_))) },
    { "FireComplexSignal", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(ExtScriptable::GetInner(scriptable),
                      &ExtScriptable::Inner::FireComplexSignal)) },
    { "ComplexSignalData", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_SCRIPTABLE) },

    // The following are defined in the prototype.
    { "PrototypeMethod", ScriptableInterface::PROPERTY_METHOD,
      Variant(NewSlot(Prototype::GetInstance(), &Prototype::Method)) },
    { "PrototypeSelf", ScriptableInterface::PROPERTY_NORMAL,
      Variant(Variant::TYPE_SCRIPTABLE) },
    { "ontest", ScriptableInterface::PROPERTY_NORMAL,
      Variant(new SignalSlot(&Prototype::GetInstance()->ontest_signal_)) },
    // Prototype's OverrideSelf is overriden by ExtScriptable's OverrideSelf.
  };

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    CheckProperty(scriptable, property_info[i]);
  }

  // Const is defined in prototype.
  CheckConstant("Const", scriptable, Variant(987654321));

  for (int i = 0; i < static_cast<int>(arraysize(property_info)); i++) {
    if (property_info[i].prototype.type() == Variant::TYPE_SLOT)
      delete VariantValue<Slot *>()(property_info[i].prototype);
  }

  delete scriptable;
  EXPECT_STREQ("Destruct\n", g_buffer.c_str());
}

TEST(ScriptableHelperTest, TestExtPropertyInfo) {
  CheckExtPropertyInfo(true);
  CheckExtPropertyInfo(false);
}

TEST(ScriptableHelperTest, TestArray) {
  ExtScriptable *scriptable = new ExtScriptable(true, true, false);
  for (int i = 0; i < ExtScriptable::kArraySize; i++)
    ASSERT_TRUE(scriptable->SetPropertyByIndex(i, Variant(i * 2)));
  for (int i = 0; i < ExtScriptable::kArraySize; i++)
    ASSERT_EQ(Variant(i * 2 + 10000), scriptable->GetPropertyByIndex(i).v());

  int invalid_id = ExtScriptable::kArraySize;
  ASSERT_FALSE(scriptable->SetPropertyByIndex(invalid_id, Variant(100)));
  ASSERT_EQ(Variant(), scriptable->GetPropertyByIndex(invalid_id).v());
  delete scriptable;
}

TEST(ScriptableHelperTest, TestDynamicProperty) {
  ExtScriptable *scriptable = new ExtScriptable(true, true, false);
  char name[20];
  char value[20];
  const int num_tests = 10;

  for (int i = 0; i < num_tests; i++) {
    snprintf(name, sizeof(name), "d%d", i);
    snprintf(value, sizeof(value), "v%dv", i * 2);
    ASSERT_EQ(ScriptableInterface::PROPERTY_DYNAMIC,
              scriptable->GetPropertyInfo(name, NULL));
    ASSERT_TRUE(scriptable->SetProperty(name, Variant(value)));
  }
  for (int i = 0; i < num_tests; i++) {
    snprintf(name, sizeof(name), "d%d", i);
    snprintf(value, sizeof(value), "Value:v%dv", i * 2);
    ASSERT_EQ(Variant(value), scriptable->GetProperty(name).v());
  }

  // Test dynamic signal.
  Slot0<void> *slot = NewSlot(reinterpret_cast<void (*)()>(NULL));
  Variant prototype;
  ASSERT_EQ(ScriptableInterface::PROPERTY_DYNAMIC,
            scriptable->GetPropertyInfo("s", &prototype));
  ASSERT_EQ(Variant::TYPE_SLOT, prototype.type());
  ASSERT_TRUE(scriptable->SetProperty("s", Variant(slot)));
  ResultVariant result = scriptable->GetProperty("s");
  ASSERT_EQ(Variant::TYPE_SLOT, result.v().type());
  ASSERT_EQ(slot, VariantValue<Slot *>()(result.v()));

  ASSERT_EQ(ScriptableInterface::PROPERTY_NOT_EXIST,
            scriptable->GetPropertyInfo("not_supported", NULL));
  ASSERT_EQ(Variant::TYPE_VOID,
            scriptable->GetProperty("not_supported").v().type());
  delete scriptable;
}

class NameChecker {
 public:
  NameChecker(ScriptableInterface *scriptable, std::set<std::string> *names)
      : scriptable_(scriptable), names_(names) { }
  bool Check(const char *name, ScriptableInterface::PropertyType type,
             const Variant &value) const {
    LOG("Expect name: %s", name);
    EXPECT_EQ(scriptable_->GetProperty(name).v(), value);
    EXPECT_EQ(scriptable_->GetPropertyInfo(name, NULL), type);
    EXPECT_EQ(1U, names_->erase(name));
    return true;
  }

  ScriptableInterface *scriptable_;
  std::set<std::string> *names_;
};

static void CheckEnumerateProperties(bool register_class) {
  ExtScriptable *scriptable = new ExtScriptable(true, true, register_class);
  static const char *property_names[] = {
    "Buffer", "BufferReadOnly", "CallCallback", "ConcatArray", "Const",
    "ReleaseObject", "DoubleProperty", "EnumSimple", "EnumString",
    "Fixed", "ICONSTANT0", "ICONSTANT1", "ICONSTANT2", "ICONSTANT3",
    "ICONSTANT4", "ICONSTANT5", "ICONSTANT6", "ICONSTANT7", "ICONSTANT8",
    "ICONSTANT9", "JSON", "NewObject", "OverrideSelf", "PrototypeMethod",
    "PrototypeSelf", "SCONSTANT0", "SCONSTANT1", "SCONSTANT2", "SCONSTANT3",
    "SCONSTANT4", "SCONSTANT5", "SCONSTANT6", "SCONSTANT7", "SCONSTANT8",
    "SCONSTANT9", "SetCallback", "SignalResult", "NativeOwned", "ObjectMethod",
    "MethodDouble2", "ClearBuffer", "VALUE_0", "VALUE_1", "VALUE_2",
    "VariantProperty", "length", "my_ondelete", "onlunch", "onsupper",
    "ontest", "time", "oncomplex", "FireComplexSignal", "FireDynamicSignal",
    "ComplexSignalData", "IntProperty", ""
  };
  std::set<std::string> expected;
  for (size_t i = 0; i < arraysize(property_names); ++i)
    expected.insert(property_names[i]);
  NameChecker checker(scriptable, &expected);
  scriptable->EnumerateProperties(NewSlot(&checker, &NameChecker::Check));
  ASSERT_TRUE(expected.empty());
}

TEST(ScriptableHelperTest, TestEnumerateProperties) {
  CheckEnumerateProperties(true);
  CheckEnumerateProperties(false);
}

// We need a new scriptable class to prevent interferring from other tests.
class RemovePropertyScriptable : public BaseScriptable {
 public:
  DEFINE_CLASS_ID(0x44fac5eaf67b408b, BaseScriptable);

  explicit RemovePropertyScriptable(bool register_class)
      : BaseScriptable(true, register_class) {
  }
};

TEST(ScriptableHelperTest, TestRemoveProperty) {
  RemovePropertyScriptable *scriptable = new RemovePropertyScriptable(false);

  ASSERT_TRUE(scriptable->RemoveProperty("ClearBuffer"));
  ASSERT_EQ(ScriptableInterface::PROPERTY_NOT_EXIST,
            scriptable->GetPropertyInfo("ClearBuffer", NULL));
  ASSERT_TRUE(scriptable->RemoveProperty("DoubleProperty"));
  ASSERT_EQ(ScriptableInterface::PROPERTY_NOT_EXIST,
            scriptable->GetPropertyInfo("DoubleProperty", NULL));
  ASSERT_TRUE(scriptable->RemoveProperty("my_ondelete"));
  ASSERT_EQ(ScriptableInterface::PROPERTY_NOT_EXIST,
            scriptable->GetPropertyInfo("my_ondelete", NULL));
  ASSERT_FALSE(scriptable->RemoveProperty("not_exist"));

  delete scriptable;
  scriptable = new RemovePropertyScriptable(true);
  ASSERT_FALSE(scriptable->RemoveProperty("ClearBuffer"));
  ASSERT_EQ(ScriptableInterface::PROPERTY_METHOD,
            scriptable->GetPropertyInfo("ClearBuffer", NULL));
  delete scriptable;
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
