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

#include "js_native_wrapper.h"
#include "converter.h"
#include "js_script_context.h"

namespace ggadget {
namespace qt {
static int i = 0;
JSNativeWrapper::JSNativeWrapper(JSScriptContext *context,
                                 const QScriptValue &qval)
    : context_(context),
      qval_(qval) {
  Ref();
  ASSERT(GetRefCount() == 1);

  object_data_.wrapper = this;
  QScriptValue data = context->engine()->newQObject(&object_data_);

  qval_.setData(data);
  LOG("Create Wrapper: %d", ++i);
}

JSNativeWrapper::~JSNativeWrapper() {
  LOG("Delete Wrapper: %d", --i);
  QScriptValue data = qval_.data();
  ASSERT(data.isQObject());
  qval_.setData(context_->engine()->undefinedValue());
}

void JSNativeWrapper::Ref() const {
  ScriptableHelperDefault::Ref();
  if (GetRefCount() == 2) {
    // There must be a new native reference, let JavaScript know it by adding
    // the object to root.
    // JS_AddNamedRootRT(JS_GetRuntime(js_context_), &js_object_, name_.c_str());
  }
}

void JSNativeWrapper::Unref(bool transient) const {
  if (GetRefCount() == 2) {
    // The last native reference is about to be released, let JavaScript know
    // it by removing the root reference.
    // JS_RemoveRootRT(JS_GetRuntime(js_context_), &js_object_);
  }
  ScriptableHelperDefault::Unref(transient);
}

ScriptableInterface::PropertyType JSNativeWrapper::GetPropertyInfo(
    const char *name, Variant *prototype) {
  GGL_UNUSED(name);
  GGL_UNUSED(prototype);
  return ScriptableInterface::PROPERTY_DYNAMIC;
}

bool JSNativeWrapper::EnumerateProperties(
    EnumeratePropertiesCallback *callback) {
  GGL_UNUSED(callback);
#if 0
  ScopedLogContext log_context(GetJSScriptContext(js_context_));
  ASSERT(callback);
  bool result = true;
  JSIdArray *id_array = JS_Enumerate(js_context_, js_object_);
  if (id_array) {
    for (int i = 0; i < id_array->length; i++) {
      jsid id = id_array->vector[i];
      jsval key = JSVAL_VOID;
      JS_IdToValue(js_context_, id, &key);
      if (JSVAL_IS_STRING(key)) {
        char *name = JS_GetStringBytes(JSVAL_TO_STRING(key));
        if (name &&
            !(*callback)(name, ScriptableInterface::PROPERTY_DYNAMIC,
                         GetProperty(name).v())) {
          result = false;
          break;
        }
      }
      // Otherwise, ignore the property.
    }
  }
  JS_DestroyIdArray(js_context_, id_array);
  delete callback;
#endif
  return false;
}

bool JSNativeWrapper::EnumerateElements(EnumerateElementsCallback *callback) {
  GGL_UNUSED(callback);
#if 0
  ASSERT(callback);
  ScopedLogContext log_context(GetJSScriptContext(js_context_));
  bool result = true;
  JSIdArray *id_array = JS_Enumerate(js_context_, js_object_);
  if (id_array) {
    for (int i = 0; i < id_array->length; i++) {
      jsid id = id_array->vector[i];
      jsval key = JSVAL_VOID;
      JS_IdToValue(js_context_, id, &key);
      if (JSVAL_IS_INT(key)) {
        int index = JSVAL_TO_INT(key);
        if (!(*callback)(index, GetPropertyByIndex(index).v())) {
          result = false;
          break;
        }
      }
      // Otherwise, ignore the property.
    }
  }
  JS_DestroyIdArray(js_context_, id_array);
  delete callback;
  return result;
#endif
  return false;
}

ResultVariant JSNativeWrapper::GetProperty(const char *name) {
  ScopedLogContext log_context(context_);
  Variant result;
  QScriptValue val = qval_.property(name);
  if (!val.isValid() || !ConvertJSToNativeVariant(context_->engine(), val, &result))
    context_->engine()->currentContext()->throwError(
        QString("Failed to convert JS property %1 value to native.")
        .arg(name));
  return ResultVariant(result);
}

bool JSNativeWrapper::SetProperty(const char *name, const Variant &value) {
  ScopedLogContext log_context(context_);
  QScriptValue qval;
  if (!ConvertNativeToJS(context_->engine(), value, &qval)) {
    context_->engine()->currentContext()->throwError(
        QString("Failed to convert native property %1 value(%2) to js val.")
        .arg(name).arg(value.Print().c_str()));
    return false;
  }
  qval_.setProperty(name, qval);
  return true;
}

ResultVariant JSNativeWrapper::GetPropertyByIndex(int index) {
  ScopedLogContext log_context(context_);
  Variant result;
  QScriptValue val = qval_.property(index);
  if (!val.isValid() || !ConvertJSToNativeVariant(context_->engine(), val, &result))
    context_->engine()->currentContext()->throwError(
        QString("Failed to convert JS property %1 value to native.")
        .arg(index));
  return ResultVariant(result);
}

bool JSNativeWrapper::SetPropertyByIndex(int index, const Variant &value) {
  ScopedLogContext log_context(context_);
  QScriptValue qval;
  if (!ConvertNativeToJS(context_->engine(), value, &qval)) {
    context_->engine()->currentContext()->throwError(
        QString("Failed to convert native property %1 value(%2) to js val.")
        .arg(index).arg(value.Print().c_str()));
    return false;
  }
  qval_.setProperty(index, qval);
  return true;
}

ScriptableInterface *JSNativeWrapper::UnwrapJSObject(const QScriptValue &qval) {
  QScriptValue data = qval.data();
  if (data.isQObject()) {
    JSObjectData *obj_data = static_cast<JSObjectData*>(data.toQObject());
    LOG("Reuse jsobj wrapper:%p", obj_data->wrapper);
    return obj_data->wrapper;
  } else {
    return NULL;
  }
}

} // namespace qt
} // namespace ggadget
