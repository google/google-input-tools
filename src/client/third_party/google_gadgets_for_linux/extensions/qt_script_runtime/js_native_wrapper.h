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

#ifndef GGADGET_QT_JS_NATIVE_WRAPPER_H__
#define GGADGET_QT_JS_NATIVE_WRAPPER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include "js_script_context.h"

namespace ggadget {
namespace qt {

/**
 * Wraps a JavaScript object into a native @c ScriptableInterface.
 */
class JSNativeWrapper : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x65f4d888b7b749ed, ScriptableInterface);
  JSNativeWrapper(JSScriptContext *context, const QScriptValue& qval);

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

  QScriptValue js_object() { return qval_; }
  JSScriptContext *context() { return context_; }

  static ScriptableInterface* UnwrapJSObject(const QScriptValue& qval);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSNativeWrapper);

  JSScriptContext *context_;
  QScriptValue qval_;
  class JSObjectData : public QObject {
   public:
    JSNativeWrapper *wrapper;
  };
  JSObjectData object_data_;
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_JS_NATIVE_WRAPPER_H__
