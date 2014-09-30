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

#ifndef EXTENSIONS_SMJS_SCRIPT_RUNTIME_JS_SCRIPT_RUNTIME_H__
#define EXTENSIONS_SMJS_SCRIPT_RUNTIME_JS_SCRIPT_RUNTIME_H__

#include <ggadget/script_runtime_interface.h>
#include "libmozjs_glue.h"

namespace ggadget {
namespace smjs {

class JSScriptContext;

/**
 * @c ScriptRuntime implementation for SpiderMonkey JavaScript engine.
 */
class JSScriptRuntime : public ScriptRuntimeInterface {
 public:
  JSScriptRuntime();
  virtual ~JSScriptRuntime();

  /** @see ScriptRuntimeInterface::CreateContext() */
  virtual ScriptContextInterface *CreateContext();

  void DestroyContext(JSScriptContext *context);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptRuntime);
  JSRuntime *runtime_;
};

// The maximum execution time of a piece of script (10 seconds).
const uint64_t kMaxScriptRunTime = 10000;

} // namespace smjs
} // namespace ggadget

#endif  // EXTENSIONS_SMJS_SCRIPT_RUNTIME_JS_SCRIPT_RUNTIME_H__
