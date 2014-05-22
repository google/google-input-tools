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

#include "../js_script_context.h"
#include "../js_script_runtime.h"
#include "ggadget/scriptable_helper.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::webkit;

class DataObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x1111111111111111, ScriptableInterface);
  DataObject() : int_property_(0) {
    RegisterProperty("intProperty",
                     NewSlot(this, &DataObject::GetIntProperty),
                     NewSlot(this, &DataObject::SetIntProperty));
    RegisterProperty("stringProperty",
                     NewSlot(this, &DataObject::GetStringProperty),
                     NewSlot(this, &DataObject::SetStringProperty));
    RegisterProperty("varProperty",
                     NewSlot(this, &DataObject::GetVarProperty),
                     NewSlot(this, &DataObject::SetVarProperty));
  }

  int GetIntProperty() const { return int_property_; }
  void SetIntProperty(int prop) { int_property_ = prop; }
  std::string GetStringProperty() const { return string_property_; }
  void SetStringProperty(const std::string &str) { string_property_ = str; }
  Variant GetVarProperty() const { return var_property_.v(); }
  void SetVarProperty(const Variant &v) { var_property_ = ResultVariant(v); }

  int int_property_;
  std::string string_property_;
  ResultVariant var_property_;
};

class GlobalObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject() {
    RegisterConstant("data", &data_);
  }

  virtual bool IsStrict() const { return false; }

  DataObject data_;
};

static const char kScript1[] = "\
var callee = { \n\
  strVal : \"Hello\", \n\
  intVal : 123, \n\
  getStrVal : function() { return this.strVal; }, \n\
  setStrVal : function(str) { this.strVal = str; }, \n\
  incIntVal : function() { this.intVal++; }, \n\
  getIntVal : function() { return this.intVal; } \n\
};\n\
data.varProperty = callee;";

static const char kScript2[] = "\
var callee = data.varProperty;\n\
data.stringProperty = callee.getStrVal();\n\
callee.setStrVal(\" world\");\n\
data.stringProperty = data.stringProperty + callee.strVal;\n\
data.intProperty = callee.getIntVal();\n\
callee.incIntVal();\n\
data.intProperty = data.intProperty + callee.intVal;\n\
data.varProperty = null;";

TEST(CrossContextTest, CrossContextTest) {
  JSScriptRuntime *runtime = new JSScriptRuntime();
  ScriptContextInterface *context1 = runtime->CreateContext();
  ScriptContextInterface *context2 = runtime->CreateContext();
  GlobalObject *global = new GlobalObject();

  context1->SetGlobalObject(global);
  context2->SetGlobalObject(global);

  context1->Execute(kScript1, "file1", 1);
  context2->Execute(kScript2, "file2", 1);

  ASSERT_STREQ("Hello world", global->data_.string_property_.c_str());
  ASSERT_EQ(247, global->data_.int_property_);
  ASSERT_EQ(Variant::TYPE_VOID, global->data_.var_property_.v().type());
  context2->Destroy();
  context1->Destroy();
  delete runtime;
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
