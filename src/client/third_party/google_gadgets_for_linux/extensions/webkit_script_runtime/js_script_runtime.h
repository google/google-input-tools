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

#ifndef EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JS_SCRIPT_RUNTIME_H__
#define EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JS_SCRIPT_RUNTIME_H__

#include "java_script_core.h"
#include <vector>
#include <utility>
#include <ggadget/script_runtime_interface.h>

namespace ggadget {
namespace webkit {

class JSScriptContext;

/**
 * @c ScriptRuntime implementation for WebKit JavaScriptCore engine.
 */
class JSScriptRuntime : public ScriptRuntimeInterface {
 public:
  JSScriptRuntime();
  virtual ~JSScriptRuntime();

  /** @see ScriptRuntimeInterface::CreateContext() */
  virtual ScriptContextInterface *CreateContext();

  /**
   * Gets the JSClassRef object associated to a specified class definition.
   *
   * If there is no JSClassRef object associated to the class definition, a new
   * JSClassRef object will be created automatically and it will be released
   * when destroying the runtime.
   *
   * @param definition Pointer to a static JSClassDefinition structure.
   * @return associated JSClassRef object.
   */
  JSClassRef GetClassRef(const JSClassDefinition *definition);

  /**
   * Wraps an existing JSContextRef into a JSScriptContext object.
   */
  JSScriptContext *WrapExistingContext(JSContextRef js_context);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptRuntime);

  class Impl;
  Impl *impl_;
};

} // namespace webkit
} // namespace ggadget

#endif  // EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JS_SCRIPT_RUNTIME_H__
