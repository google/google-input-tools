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

#include <iostream>
#include <ggadget/scriptable_helper.h>
#include <ggadget/memory_options.h>
#include <ggadget/scriptable_options.h>
#include <unittest/gtest.h>
#include "../js_script_context.h"
#include "../js_script_runtime.h"

using namespace ggadget;
using namespace ggadget::qt;

class Scriptable1 : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x1111111111111111, ScriptableInterface);
  virtual bool IsStrict() const { return false; }
};

class Scriptable2 : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x2222222222222222, ScriptableInterface);
  virtual bool IsStrict() const { return false; }
};

class Scriptable3 : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x3333333333333333, ScriptableInterface);
  virtual bool IsStrict() const { return false; }
};

ScriptableInterface *Constructor() { return new Scriptable3(); }
MemoryOptions data;
ScriptableOptions options(&data, true);
ScriptableOptions *GetData() {  return &options; }
void Print(const char *str) {
  std::cout << str << "\n";
}
void Assert(bool value) {
  ASSERT_TRUE(value);
}
TEST(ShareNativeObject, Test) {
  JSScriptRuntime *runtime = new JSScriptRuntime();
  ScriptContextInterface *ctx1 = runtime->CreateContext();
  ScriptContextInterface *ctx2 = runtime->CreateContext();
  ASSERT_TRUE(ctx1 && ctx2);

  Scriptable1 *global1 = new Scriptable1();
  Scriptable2 *global2 = new Scriptable2();
  ctx1->SetGlobalObject(global1);
  ctx2->SetGlobalObject(global2);

  global1->RegisterProperty("data", NewSlot(GetData), NULL);
//  global2->RegisterProperty("data", NewSlot(GetData), NULL);
  ctx2->AssignFromNative(NULL, "", "data", Variant(&options));
  global1->RegisterMethod("print", NewSlot(Print));
  global2->RegisterMethod("print", NewSlot(Print));
  global1->RegisterMethod("assert", NewSlot(Assert));
  global2->RegisterMethod("assert", NewSlot(Assert));

  ctx1->Execute("data.putValue('name', 'tiger');", NULL, 0);
  ctx1->Execute("assert(data.getValue('name') == 'tiger');", NULL, 0);
  ctx2->Execute("assert(data.getValue('name') == 'tiger');", NULL, 0);

  // build an object in ctx1
  ctx1->Execute("function MyObj() { this.value = 'google'; }", NULL, 0);
  ctx1->Execute("var obj = new MyObj()", NULL, 0);
  ctx1->Execute("data.putValue('obj', obj);", NULL, 0);

  ctx1->Execute("var r = data.getValue('obj');",  NULL, 0);
  ctx1->Execute("assert(r.value == 'google');", NULL, 0);

  ctx2->Execute("var r = data.getValue('obj');",  NULL, 0);
  ctx2->Execute("assert(r.value == 'google');", NULL, 0);

  // change object property from native context
  ctx1->Execute("r.value = 'Beijing';",  NULL, 0);

  ctx1->Execute("assert(r.value == 'Beijing');", NULL, 0);
  ctx2->Execute("assert(r.value == 'Beijing');", NULL, 0);

  // change object property from foreign context
  ctx2->Execute("r.value = 'linux';",  NULL, 0);

  ctx1->Execute("assert(r.value == 'linux');", NULL, 0);
  ctx2->Execute("assert(r.value == 'linux');", NULL, 0);

  // change object property from native context, again
  ctx1->Execute("r.value = 'Beijing';",  NULL, 0);

  ctx1->Execute("assert(r.value == 'Beijing');", NULL, 0);
  ctx2->Execute("assert(r.value == 'Beijing');", NULL, 0);

  ctx1->Destroy();
  ctx2->Destroy();
  delete runtime;
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
