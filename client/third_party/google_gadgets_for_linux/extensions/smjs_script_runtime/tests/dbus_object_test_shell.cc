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

#include <ggadget/logger.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/extension_manager.h>
#include <ggadget/tests/native_main_loop.h>
#include <ggadget/tests/init_extensions.h>
#include "../js_script_context.h"

using namespace ggadget;
using namespace ggadget::smjs;

class GlobalObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject() {
  }
  virtual bool IsStrict() const { return false; }
};

static GlobalObject *global;
static NativeMainLoop main_loop;

// Called by the initialization code in js_shell.cc.
// Used to compile a standalone js_shell.
JSBool InitCustomObjects(JSScriptContext *context) {
  SetGlobalMainLoop(&main_loop);
  global = new GlobalObject();
  context->SetGlobalObject(global);

  static const char *kExtensions[] = {
    "dbus_script_class/dbus-script-class",
    "libxml2_xml_parser/libxml2-xml-parser"
  };

  int argc = 0;
  const char **argv = NULL;
  INIT_EXTENSIONS(argc, argv, kExtensions);

  ScriptExtensionRegister ext_register(context, NULL);
  ExtensionManager::GetGlobalExtensionManager()->RegisterLoadedExtensions(
      &ext_register);

  return JS_TRUE;
}

void DestroyCustomObjects(JSScriptContext *context) {
  delete global;
}
