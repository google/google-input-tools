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

#include "ggadget/script_context_interface.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/scriptable_interface.h"
#include "../js_script_context.h"

using namespace ggadget;

class GlobalObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject() {
  }
  virtual bool IsStrict() const { return false; }
};

static GlobalObject *global;

// Called by the initialization code in js_shell.cc.
// Used to compile a standalone js_shell.
bool InitCustomObjects(ScriptContextInterface *context) {
  global = new GlobalObject();
  context->SetGlobalObject(global);
  return true;
}

void DestroyCustomObjects(ScriptContextInterface *context) {
  delete global;
}
