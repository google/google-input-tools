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

#ifndef GGADGET_QT_JS_SCRIPT_RUNTIME_H__
#define GGADGET_QT_JS_SCRIPT_RUNTIME_H__

#include <QtScript/QtScript>
#include <ggadget/script_runtime_interface.h>

namespace ggadget {
namespace qt {

class JSScriptContext;

/**
 * @c ScriptRuntime implementation for Qt4 JavaScript engine.
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
  class Impl;
  Impl *impl_;
};

} // namespace qt
} // namespace ggadget

#endif  // GGADGET_QT_JS_SCRIPT_RUNTIME_H__
