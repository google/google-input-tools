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

#include <jsapi.h>
#include "../js_script_context.h"
#include "../js_script_runtime.h"
#include "ggadget/scriptable_helper.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::smjs;

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

void TestCrossProperties(JSContext *cxn,
                         const char *subsubname, int value) {
  jsval jv;
  ASSERT_TRUE(JS_GetProperty(cxn, JS_GetGlobalObject(cxn), "cross", &jv));
  ASSERT_TRUE(JSVAL_IS_OBJECT(jv));
  JSObject *cross = JSVAL_TO_OBJECT(jv);
  ASSERT_TRUE(JS_GetProperty(cxn, cross, "int_prop", &jv));
  ASSERT_EQ(999, JSVAL_TO_INT(jv));
  ASSERT_TRUE(JS_GetProperty(cxn, cross, "subobj", &jv));
  ASSERT_TRUE(JSVAL_IS_OBJECT(jv));
  JSObject *subobj = JSVAL_TO_OBJECT(jv);
  JSObject *subsubobj = JS_NewObject(cxn, NULL, NULL, NULL);
  jv = OBJECT_TO_JSVAL(subsubobj);
  ASSERT_TRUE(JS_SetProperty(cxn, subobj, subsubname, &jv));
  jv = INT_TO_JSVAL(value);
  ASSERT_TRUE(JS_SetProperty(cxn, subsubobj, "value", &jv));
}

ScriptableInterface *Constructor() { return new Scriptable3(); }

TEST(CrossContext, Test) {
  JSScriptRuntime *runtime = new JSScriptRuntime();
  JSScriptContext *context =
      down_cast<JSScriptContext *>(runtime->CreateContext());
  JSScriptContext *context1 =
      down_cast<JSScriptContext *>(runtime->CreateContext());
  JSScriptContext *context2 =
      down_cast<JSScriptContext *>(runtime->CreateContext());
  JSContext *cx = context->context();
  JSContext *cx1 = context1->context();
  JSContext *cx2 = context2->context();
  ASSERT_TRUE(cx && cx1 && cx2);

  JSObject *global = JS_NewObject(cx, NULL, NULL, NULL);
  JS_SetGlobalObject(cx, global);

  Scriptable1 *native_global1 = new Scriptable1();
  Scriptable2 *native_global2 = new Scriptable2();
  native_global2->Ref();
  context1->SetGlobalObject(native_global1);
  context2->SetGlobalObject(native_global2);

  context1->RegisterClass("class1", NewSlot(Constructor));
  context2->RegisterClass("class2", NewSlot(Constructor));

  JSObject *cross = JS_NewObject(cx1, NULL, NULL, NULL);
  JSObject *subobj = JS_NewObject(cx1, NULL, NULL, NULL);
  jsval jv = OBJECT_TO_JSVAL(cross);
  ASSERT_TRUE(JS_SetProperty(cx, global, "cross", &jv));
  jv = INT_TO_JSVAL(999);
  ASSERT_TRUE(JS_SetProperty(cx, cross, "int_prop", &jv));
  jv = OBJECT_TO_JSVAL(subobj);
  ASSERT_TRUE(JS_SetProperty(cx, cross, "subobj", &jv));
  ASSERT_TRUE(context1->AssignFromContext(NULL, NULL, "cross",
                                          context, NULL, "cross"));
  ASSERT_TRUE(context2->AssignFromContext(NULL, NULL, "cross",
                                          context, NULL, "cross"));

  TestCrossProperties(cx1, "subsubobj1", 1111111);
  TestCrossProperties(cx2, "subsubobj2", 2222222);

  ASSERT_TRUE(JS_GetProperty(cx, subobj, "subsubobj1", &jv));
  ASSERT_TRUE(JSVAL_IS_OBJECT(jv));
  JSObject *subsubobj1 = JSVAL_TO_OBJECT(jv);
  ASSERT_TRUE(JS_GetProperty(cx, subsubobj1, "value", &jv));
  ASSERT_EQ(1111111, JSVAL_TO_INT(jv));
  
  ASSERT_TRUE(JS_GetProperty(cx, subobj, "subsubobj2", &jv));
  ASSERT_TRUE(JSVAL_IS_OBJECT(jv));
  JSObject *subsubobj2 = JSVAL_TO_OBJECT(jv);
  ASSERT_TRUE(JS_GetProperty(cx, subsubobj2, "value", &jv));
  ASSERT_EQ(2222222, JSVAL_TO_INT(jv));

  ASSERT_TRUE(context1->AssignFromContext(
      native_global1, "cross.subobj.subsubobj1", "value",
      context, NULL, "cross.subobj.subsubobj2.value"));
  ASSERT_TRUE(JS_GetProperty(cx, subsubobj1, "value", &jv));
  ASSERT_EQ(2222222, JSVAL_TO_INT(jv));

  context1->Evaluate(NULL, "cross.subobj.subsubobj1.obj = new class1()");
  context2->Evaluate(NULL, "cross.subobj.subsubobj2.obj = new class2()");

  Variant result1 = context->Evaluate(NULL, "cross.subobj.subsubobj1.obj");
  ASSERT_EQ(Variant::TYPE_SCRIPTABLE, result1.type());
  ASSERT_TRUE(VariantValue<Scriptable3 *>()(result1));
  Variant result2 = context->Evaluate(NULL, "cross.subobj.subsubobj2.obj");
  ASSERT_EQ(Variant::TYPE_SCRIPTABLE, result2.type());
  ASSERT_TRUE(VariantValue<Scriptable3 *>()(result2));

  context->CollectGarbage();
  delete native_global1;
  native_global2->Unref();
  context1->Destroy();
  context->CollectGarbage();
  context2->Destroy();
  context->CollectGarbage();

  // The cross context objects should be still available.
  ASSERT_TRUE(JS_GetProperty(cx, subsubobj1, "value", &jv));
  ASSERT_EQ(2222222, JSVAL_TO_INT(jv));

  ASSERT_TRUE(JS_GetProperty(cx, subobj, "subsubobj2", &jv));
  ASSERT_TRUE(JSVAL_IS_OBJECT(jv));
  subsubobj2 = JSVAL_TO_OBJECT(jv);
  ASSERT_TRUE(JS_GetProperty(cx, subsubobj2, "value", &jv));
  ASSERT_EQ(2222222, JSVAL_TO_INT(jv));

  context->Destroy();
  delete runtime;
}

int main(int argc, char *argv[]) {
#ifdef XPCOM_GLUE
  if (!ggadget::libmozjs::LibmozjsGlueStartup()) {
    printf("Failed to load libmozjs.so\n");
    return 1;
  }
#endif
  setlocale(LC_ALL, "");
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
