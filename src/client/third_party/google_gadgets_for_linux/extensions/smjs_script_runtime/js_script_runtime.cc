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

#include "js_script_runtime.h"

#include <unistd.h>
#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include "js_script_context.h"

namespace ggadget {
namespace smjs {

static const uint32 kDefaultContextSize = 32 * 1024 * 1024;
static const uint32 kDefaultStackTrunkSize = 4096;

#ifdef HAVE_JS_TriggerAllOperationCallbacks

static void *TriggerOperationCallbacksThread(void *data) {
  JSRuntime **runtime_ptr = static_cast<JSRuntime **>(data);
  while (true) {
    // Hazard zone. The main thread should avoid to destroy the runtime in this
    // zone. Copy *runtime_ptr to stack to avoid use of locks.
    JSRuntime *runtime = *runtime_ptr;
    if (!runtime)
      break;
    JS_TriggerAllOperationCallbacks(runtime);
    // End of hazard zone.

    sleep(kMaxScriptRunTime / 2000);
  }
  delete runtime_ptr;
  return NULL;
}
#endif

JSScriptRuntime::JSScriptRuntime()
    : runtime_(JS_NewRuntime(kDefaultContextSize)) {
  ASSERT(runtime_);
  // Use the similar policy as Mozilla Gecko that unconstrains the runtime's
  // threshold on nominal heap size, to avoid triggering GC too often.
  JS_SetGCParameter(runtime_, JSGC_MAX_BYTES, 0xffffffff);

#ifdef HAVE_JS_TriggerAllOperationCallbacks
  JSRuntime **runtime_ptr = new JSRuntime *;
  *runtime_ptr = runtime_;
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_t thread;
  if (pthread_create(&thread, &thread_attr,
                     TriggerOperationCallbacksThread, runtime_ptr) == 0) {
    DLOG("Started TriggerAllOperationCallbacks thread.");
    JS_SetRuntimePrivate(runtime_, runtime_ptr);
  } else {
    LOGE("Failed to start TriggerAllOperationCallbacks thread.");
    delete runtime_ptr;
  }
  pthread_attr_destroy(&thread_attr);
#endif
}

JSScriptRuntime::~JSScriptRuntime() {
#ifdef HAVE_JS_TriggerAllOperationCallbacks
  JSRuntime **runtime_ptr = static_cast<JSRuntime **>(
      JS_GetRuntimePrivate(runtime_));
  if (runtime_ptr) {
    // Let the TriggerAllOperationCallback thread safely quit if it happens not
    // sleeping.
    *runtime_ptr = NULL;
    // Use sleep to avoid the runtime from being destroyed in the hazard zone.
    // This can avoid use of locks.
    usleep(10000); // 10ms is enough for the thread to exit the hazard zone.
  }
#endif
  JS_DestroyRuntime(runtime_);
}

ScriptContextInterface *JSScriptRuntime::CreateContext() {
  JSContext *context = JS_NewContext(runtime_, kDefaultStackTrunkSize);
  ASSERT(context);
  if (!context)
    return NULL;
  JSScriptContext *result = new JSScriptContext(this, context);
  return result;
}

void JSScriptRuntime::DestroyContext(JSScriptContext *context) {
  delete context;
}

} // namespace smjs
} // namespace ggadget
