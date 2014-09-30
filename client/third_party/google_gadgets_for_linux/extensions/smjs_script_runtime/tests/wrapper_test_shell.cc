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

#include "ggadget/tests/scriptables.h"
#include "../js_script_context.h"

using namespace ggadget;
using namespace ggadget::smjs;

class GlobalObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject()
      // "scriptable" is native owned and strict.
      // "scriptable2" is native owned and not strict.
      : test_scriptable1(true, true),
        test_scriptable2(true, false, true) {
    RegisterConstant("scriptable", &test_scriptable1);
    RegisterConstant("scriptable2", &test_scriptable2);
    // For testing name overriding.
    RegisterConstant("s1", &test_scriptable1);
    RegisterProperty("s2", NewSlot(this, &GlobalObject::GetS2), NULL);
    RegisterMethod("globalMethod", NewSlot(this, &GlobalObject::GlobalMethod));
  }
  virtual bool IsStrict() const { return false; }

  ScriptableInterface *ConstructScriptable() {
    // Return shared ownership object.
    return test_scriptable2.NewObject(false, true, true);
  }

  ScriptableInterface *GetS2() { return &test_scriptable2; }

  std::string GlobalMethod() { return "hello world"; }

  BaseScriptable test_scriptable1;
  ExtScriptable test_scriptable2;
};

static GlobalObject *global;

static ScriptableInterface *NullCtor() {
  return NULL;
}

// Called by the initialization code in js_shell.cc.
JSBool InitCustomObjects(JSScriptContext *context) {
  global = new GlobalObject();
  context->SetGlobalObject(global);
  context->RegisterClass("TestScriptable",
                         NewSlot(global, &GlobalObject::ConstructScriptable));
  context->RegisterClass("TestNullCtor", NewSlot(NullCtor));
  return JS_TRUE;
}

void DestroyCustomObjects(JSScriptContext *context) {
  delete global;
}
