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
#include <ggadget/script_runtime_manager.h>
#include "js_script_runtime.h"

#define Initialize webkit_script_runtime_LTX_Initialize
#define Finalize webkit_script_runtime_LTX_Finalize
#define RegisterScriptRuntimeExtension \
    webkit_script_runtime_LTX_RegisterScriptRuntimeExtension

using ggadget::ScriptRuntimeManager;
using ggadget::webkit::JSScriptRuntime;

static JSScriptRuntime *g_webkit_script_runtime_ = NULL;

extern "C" {
  bool Initialize() {
    LOGI("Initialize webkit_script_runtime extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize webkit_script_runtime extension.");
    delete g_webkit_script_runtime_;
    g_webkit_script_runtime_ = NULL;
  }

  bool RegisterScriptRuntimeExtension(ScriptRuntimeManager *manager) {
    LOGI("Register webkit_script_runtime extension.");
    if (manager) {
      if (!g_webkit_script_runtime_)
        g_webkit_script_runtime_ = new JSScriptRuntime();

      manager->RegisterScriptRuntime("js", g_webkit_script_runtime_);

      // Special handle for gtkwebkit_browser_element.
      manager->RegisterScriptRuntime("webkitjs", g_webkit_script_runtime_);
      return true;
    }
    return false;
  }
}
