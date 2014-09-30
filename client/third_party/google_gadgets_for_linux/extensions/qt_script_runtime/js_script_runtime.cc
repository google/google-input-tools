/*
  Copyright 2007 Google Inc.

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
#include <ggadget/signals.h>

#include "js_script_runtime.h"
#include "js_script_context.h"

namespace ggadget {
namespace qt {

class JSScriptRuntime::Impl {
 public:
  Impl() {
  }

  ~Impl() {
  }
};

JSScriptRuntime::JSScriptRuntime()
    : impl_(new Impl) {
  InitScriptContextData();
}

JSScriptRuntime::~JSScriptRuntime() {
  delete impl_;
}

ScriptContextInterface *JSScriptRuntime::CreateContext() {
  LOG("CreateContext");
  return new JSScriptContext();
}

void JSScriptRuntime::DestroyContext(JSScriptContext *context) {
  LOG("DestroyContext");
  delete context;
}

} // namespace qt
} // namespace ggadget
