#include <ggadget/script_runtime_manager.h>
#include "js_script_runtime.h"

#define Initialize qt_script_runtime_LTX_Initialize
#define Finalize qt_script_runtime_LTX_Finalize
#define RegisterScriptRuntimeExtension \
    qt_script_runtime_LTX_RegisterScriptRuntimeExtension

using ggadget::ScriptRuntimeManager;
using ggadget::qt::JSScriptRuntime;

static JSScriptRuntime *g_script_runtime_ = NULL;

extern "C" {
  bool Initialize() {
    LOGI("Initialize qt_script_runtime extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize qt_script_runtime extension.");
    delete g_script_runtime_;
  }

  bool RegisterScriptRuntimeExtension(ScriptRuntimeManager *manager) {
    LOGI("Register qt_script_runtime extension.");
    if (manager) {
      if (!g_script_runtime_)
        g_script_runtime_ = new JSScriptRuntime();

      manager->RegisterScriptRuntime("js", g_script_runtime_);
      return true;
    }
    return false;
  }
}
