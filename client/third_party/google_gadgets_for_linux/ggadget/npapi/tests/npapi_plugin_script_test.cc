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

#include <vector>

#include <ggadget/scriptable_interface.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>
#include <ggadget/npapi/npapi_plugin.h>
#include <ggadget/npapi/npapi_plugin_script.h>
#include <ggadget/npapi/npapi_utils.h>

#include <ggadget/tests/init_extensions.h>
#include <ggadget/tests/mocked_timer_main_loop.h>
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::npapi;

class NPAPIPluginScriptTest : public testing::Test {
 protected:
  virtual void SetUp() {
    memset(&mock_class_, 0, sizeof(mock_class_));
    mock_class_.hasMethod = HasMethod;
    mock_class_.invoke = Invoke;
    mock_class_.hasProperty = HasProperty;
    mock_class_.getProperty = GetProperty;
    mock_class_.setProperty = SetProperty;
    mock_npobj_ = CreateNPObject(NULL, &mock_class_);
    EXPECT_EQ(1U, mock_npobj_->referenceCount);

    NPVariant v;
    INT32_TO_NPVARIANT(10, v);
    properties_by_index_.push_back(properties_["integer"] = v);
    BOOLEAN_TO_NPVARIANT(true, v);
    properties_by_index_.push_back(properties_["boolean"] = v);
    NewNPVariantString("test", &v);
    properties_["string"] = v;
    NewNPVariantString("test", &v);
    properties_by_index_.push_back(v);
    OBJECT_TO_NPVARIANT(mock_npobj_, v);
    RetainNPObject(mock_npobj_);
    RetainNPObject(mock_npobj_);
    properties_by_index_.push_back(properties_["object"] = v);
    EXPECT_EQ(3U, mock_npobj_->referenceCount);

    methods_by_index_.push_back(
        methods_["TestBoolean"] = NewSlot(&TestBoolean));
    methods_by_index_.push_back(
        methods_["TestString"] = NewSlot(&TestString));
    methods_by_index_.push_back(
        methods_["TestInteger"] = NewSlot(&TestInteger));
    methods_by_index_.push_back(
        methods_["TestObject"] = NewSlot(&TestObject));
  }

  virtual void TearDown() {
    delete methods_["TestBoolean"];
    delete methods_["TestString"];
    delete methods_["TestInteger"];
    delete methods_["TestObject"];
    EXPECT_EQ(3U, mock_npobj_->referenceCount);
    ReleaseNPVariant(&properties_["string"]);
    ReleaseNPVariant(&properties_["object"]);
    ReleaseNPVariant(&properties_by_index_[2]);
    ReleaseNPVariant(&properties_by_index_[3]);
    methods_.clear();
    methods_by_index_.clear();
    properties_.clear();
    properties_by_index_.clear();
    EXPECT_EQ(1U, mock_npobj_->referenceCount);
    ReleaseNPObject(mock_npobj_);
  }

  // NPClass methods_.
  static bool HasMethod(NPObject *npobj, NPIdentifier id) {
    EXPECT_EQ(mock_npobj_, npobj);
    EXPECT_TRUE(id);
    if (IdentifierIsString(id)) {
      char *name = UTF8FromIdentifier(id);
      bool result = methods_.find(name) != methods_.end();
      MemFree(name);
      return result;
    } else {
      unsigned int index = IntFromIdentifier(id);
      return methods_by_index_.size() > index;
    }
  }

  static bool Invoke(NPObject *npobj, NPIdentifier id, const NPVariant *args,
                     uint32_t argCount, NPVariant *result) {
    EXPECT_EQ(mock_npobj_, npobj);
    EXPECT_TRUE(id);
    Variant argv[argCount];
    for (size_t i = 0; i < argCount; ++i)
      argv[i] = ConvertNPToNative(&args[i]);
    Slot *slot = NULL;
    if (IdentifierIsString(id)) {
      char *name = UTF8FromIdentifier(id);
      if (methods_.find(name) != methods_.end())
        slot = methods_[name];
      MemFree(name);
    } else {
      unsigned int index = IntFromIdentifier(id);
      if (index < methods_by_index_.size())
        slot = methods_by_index_[index];
    }
    if (slot) {
      ResultVariant ret = slot->Call(NULL, argCount, argv);
      ConvertNativeToNP(ret.v(), result);
      return true;
    }
    return false;
  }

  static bool HasProperty(NPObject *npobj, NPIdentifier id) {
    EXPECT_EQ(mock_npobj_, npobj);
    EXPECT_TRUE(id);
    if (IdentifierIsString(id)) {
      char *name = UTF8FromIdentifier(id);
      bool result = properties_.find(name) != properties_.end();
      MemFree(name);
      return result;
    } else {
      size_t index = static_cast<size_t>(IntFromIdentifier(id));
      return properties_by_index_.size() > index;
    }
  }

  static bool GetProperty(NPObject *npobj, NPIdentifier id, NPVariant *result) {
    EXPECT_EQ(mock_npobj_, npobj);
    EXPECT_TRUE(id);
    bool ret = false;
    if (IdentifierIsString(id)) {
      char *name = UTF8FromIdentifier(id);
      if (properties_.find(name) != properties_.end())
        *result = properties_[name];
      MemFree(name);
      ret = true;
    } else {
      unsigned int index = IntFromIdentifier(id);
      if (index < properties_by_index_.size()) {
        if (result)
          *result = properties_by_index_[index];
        ret = true;
      }
    }
    if (ret) {
      // Do convertion to ensure string and object values are properly
      // allocated or referenced.
      ResultVariant v(ConvertNPToNative(result));
      ConvertNativeToNP(v.v(), result);
    }
    return ret;
  }

  static bool SetProperty(NPObject *npobj, NPIdentifier id,
                          const NPVariant *value) {
    EXPECT_EQ(mock_npobj_, npobj);
    EXPECT_TRUE(id);
    if (HasProperty(npobj, id)) {
      if (IdentifierIsString(id)) {
        char *name = UTF8FromIdentifier(id);
        ReleaseNPVariant(&properties_[name]);
        properties_[name] = *value;
        MemFree(name);
        return true;
      } else {
        unsigned int index = IntFromIdentifier(id);
        ReleaseNPVariant(&properties_by_index_[index]);
        properties_by_index_[index] = *value;
        return true;
      }
    }
    return false;
  }

  static bool TestBoolean(bool boolean) { return boolean; }
  static const char *TestString(const char *string) { return string; }
  static int TestInteger(int integer) { return integer; }
  static ScriptableInterface *TestObject(ScriptableInterface *object) {
    return object;
  }

  // Mocked NPObject.
  static NPClass mock_class_;
  static NPObject *mock_npobj_;
  static std::map<std::string, NPVariant> properties_;
  static std::vector<NPVariant> properties_by_index_;
  static std::map<std::string, Slot *> methods_;
  static std::vector<Slot *> methods_by_index_;
};

NPClass NPAPIPluginScriptTest::mock_class_;
NPObject *NPAPIPluginScriptTest::mock_npobj_ = NULL;
std::map<std::string, NPVariant> NPAPIPluginScriptTest::properties_;
std::vector<NPVariant> NPAPIPluginScriptTest::properties_by_index_;
std::map<std::string, Slot *> NPAPIPluginScriptTest::methods_;
std::vector<Slot *> NPAPIPluginScriptTest::methods_by_index_;

TEST_F(NPAPIPluginScriptTest, CallNPPluginObject) {
  // Mocked NPPluginObject.
  ScriptableNPObject mock_npplugin_object(mock_npobj_);
  mock_npplugin_object.Ref();
  EXPECT_EQ(4U, mock_npobj_->referenceCount);

  // Test hasProperty and getProperty.
  Variant prototype;
  EXPECT_EQ(ScriptableInterface::PROPERTY_DYNAMIC,
            mock_npplugin_object.GetPropertyInfo("integer", &prototype));
  Variant value = mock_npplugin_object.GetProperty("integer").v();
  EXPECT_EQ(Variant::TYPE_INT64, value.type());
  EXPECT_EQ(10, VariantValue<int>()(value));

  EXPECT_EQ(ScriptableInterface::PROPERTY_DYNAMIC,
            mock_npplugin_object.GetPropertyInfo("boolean", &prototype));
  value = mock_npplugin_object.GetProperty("boolean").v();
  EXPECT_EQ(Variant::TYPE_BOOL, value.type());
  EXPECT_EQ(true, VariantValue<bool>()(value));

  EXPECT_EQ(ScriptableInterface::PROPERTY_DYNAMIC,
            mock_npplugin_object.GetPropertyInfo("string", &prototype));
  value = mock_npplugin_object.GetProperty("string").v();
  EXPECT_EQ(Variant::TYPE_STRING, value.type());
  EXPECT_EQ(0, strncmp(VariantValue<const char *>()(value), "test", 4));

  EXPECT_EQ(ScriptableInterface::PROPERTY_DYNAMIC,
            mock_npplugin_object.GetPropertyInfo("object", &prototype));
  ResultVariant result = mock_npplugin_object.GetProperty("object");
  value = result.v();
  EXPECT_EQ(Variant::TYPE_SCRIPTABLE, value.type());
  EXPECT_EQ(mock_npobj_, VariantValue<ScriptableNPObject *>()(value)->UnWrap());

  // Test setProperty.
  EXPECT_TRUE(mock_npplugin_object.SetProperty("integer", Variant(20)));
  value = mock_npplugin_object.GetProperty("integer").v();
  EXPECT_EQ(Variant::TYPE_INT64, value.type());
  EXPECT_EQ(20, VariantValue<int>()(value));

  // Test hasMethod and invoke.
  Slot *slot;
  result = mock_npplugin_object.GetProperty("TestBoolean");
  value = result.v();
  EXPECT_EQ(Variant::TYPE_SCRIPTABLE, value.type());
  ScriptableInterface *script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(Variant::TYPE_SLOT, value.type());
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  Variant param = Variant(true);
  Variant ret = slot->Call(NULL, 1, &param).v();
  EXPECT_EQ(Variant::TYPE_BOOL, ret.type());
  EXPECT_EQ(true, VariantValue<bool>()(ret));

  result = mock_npplugin_object.GetProperty("TestString");
  value = result.v();
  EXPECT_EQ(Variant::TYPE_SCRIPTABLE, value.type());
  script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(Variant::TYPE_SLOT, value.type());
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  param = Variant("test");
  ret = slot->Call(NULL, 1, &param).v();
  EXPECT_EQ(Variant::TYPE_STRING, ret.type());
  EXPECT_EQ(0, strncmp(VariantValue<const char *>()(ret), "test", 4));

  result = mock_npplugin_object.GetProperty("TestInteger");
  value = result.v();
  EXPECT_EQ(Variant::TYPE_SCRIPTABLE, value.type());
  script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(Variant::TYPE_SLOT, value.type());
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  param = Variant(50);
  ret = slot->Call(NULL, 1, &param).v();
  EXPECT_EQ(Variant::TYPE_INT64, ret.type());
  EXPECT_EQ(50, VariantValue<int>()(ret));

  result = mock_npplugin_object.GetProperty("TestObject");
  value = result.v();
  EXPECT_EQ(Variant::TYPE_SCRIPTABLE, value.type());
  script = VariantValue<ScriptableInterface *>()(value);
  value = script->GetProperty("").v();
  EXPECT_EQ(Variant::TYPE_SLOT, value.type());
  slot = VariantValue<Slot *>()(value);
  EXPECT_TRUE(slot);
  param = Variant(&mock_npplugin_object);
  result = slot->Call(NULL, 1, &param);
  ret = result.v();
  EXPECT_EQ(Variant::TYPE_SCRIPTABLE, ret.type());
  EXPECT_EQ(mock_npobj_, VariantValue<ScriptableNPObject *>()(ret)->UnWrap());

  mock_npplugin_object.Unref(true);
}

MockedTimerMainLoop main_loop(0);

int main(int argc, char **argv) {
  ggadget::SetGlobalMainLoop(&main_loop);
  testing::ParseGTestFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
