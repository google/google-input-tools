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

#ifndef EXTENSIONS_SMJS_SCRIPT_RUNTIME_JS_NATIVE_WRAPPER_H__
#define EXTENSIONS_SMJS_SCRIPT_RUNTIME_JS_NATIVE_WRAPPER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include "libmozjs_glue.h"

namespace ggadget {
namespace smjs {

/**
 * Wraps a JavaScript object into a native @c ScriptableInterface.
 *
 * In case that the native side needs a Variant value and JS gives a function,
 * the function will be wrapped into JSNativeWrapper instead of being converted
 * to a native Slot, to ease memory management.
 */
class JSNativeWrapper : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x65f4d888b7b749ed, ScriptableInterface);
  JSNativeWrapper(JSContext *js_context, JSObject *js_object);

 protected:
  virtual ~JSNativeWrapper();

 public:
  virtual void Ref() const;
  virtual void Unref(bool transient = false) const;
  virtual PropertyType GetPropertyInfo(const char *name,
                                       Variant *prototype);
  virtual ResultVariant GetProperty(const char *name);
  virtual bool SetProperty(const char *name, const Variant &value);
  virtual ResultVariant GetPropertyByIndex(int index);
  virtual bool SetPropertyByIndex(int index, const Variant &value);
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback);
  virtual bool EnumerateElements(EnumerateElementsCallback *callback);

  JSContext *js_context() { return js_context_; }
  JSObject *js_object() { return js_object_; }

  void OnContextDestroy();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSNativeWrapper);
  static void FinalizeTracker(JSContext *cx, JSObject *obj);
  bool CheckContext() const;

  static JSClass js_reference_tracker_class_;
  JSContext *js_context_;
  JSObject *js_object_;
  std::string name_;
  Slot *call_self_slot_;
};

} // namespace smjs
} // namespace ggadget

#endif // EXTENSIONS_SMJS_SCRIPT_RUNTIME_JS_NATIVE_WRAPPER_H__
