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
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>

#include "native_js_wrapper.h"
#include "converter.h"
#include "js_function_slot.h"
#include "js_script_context.h"

#ifdef _DEBUG
// Uncomment the following to debug wrapper memory.
// #define DEBUG_JS_WRAPPER_MEMORY
// #define DEBUG_JS_ROOTS
// #define DEBUG_FORCE_GC
#endif

namespace ggadget {
namespace smjs {

// This JSClass is used to create wrapper JSObjects.
JSClass NativeJSWrapper::wrapper_js_class_ = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE | JSCLASS_NEW_ENUMERATE | JSCLASS_NEW_RESOLVE,
  JS_PropertyStub, JS_PropertyStub,
  GetWrapperPropertyDefault, SetWrapperPropertyDefault,
  reinterpret_cast<JSEnumerateOp>(EnumerateWrapper),
  reinterpret_cast<JSResolveOp>(ResolveWrapperProperty),
  JS_ConvertStub, FinalizeWrapper,
  NULL, NULL, CallWrapperSelf, NULL,
  NULL, NULL, MarkWrapper, NULL,
};

NativeJSWrapper::NativeJSWrapper(JSContext *js_context,
                                 JSObject *js_object,
                                 ScriptableInterface *scriptable)
    : js_context_(js_context),
      js_object_(js_object),
      scriptable_(NULL),
      on_reference_change_connection_(NULL) {
  ASSERT(js_object);

  // Store this wrapper into the JSObject's private slot.
  JS_SetPrivate(js_context, js_object_, this);

  if (scriptable)
    Wrap(scriptable);
}

NativeJSWrapper::~NativeJSWrapper() {
  if (scriptable_) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("Delete: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
         js_context_, js_object_, this, name_.c_str(),
         scriptable_->GetRefCount());
#endif
    DetachJS(false);
  }

  if (js_context_)
    JS_SetPrivate(js_context_, js_object_, NULL);
}

void NativeJSWrapper::Wrap(ScriptableInterface *scriptable) {
  ASSERT(scriptable && !scriptable_);
  scriptable_ = scriptable;

  JSClass *cls = JS_GET_CLASS(js_context_, js_object_);
  ASSERT(cls);
#ifdef _DEBUG
  name_ = StringPrintf("[object %s %p CLASS_ID=%jx]",
                       cls->name, scriptable, scriptable->GetClassId());
#else
  name_ = StringPrintf("[object %s CLASS_ID=%jx]",
                       cls->name, scriptable->GetClassId());
#endif

  if (scriptable->GetRefCount() > 0) {
    // There must be at least one native reference, let JavaScript know it
    // by adding the object to root.
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("AddRoot: cx=%p jsobjaddr=%p jsobj=%p wrapper=%p scriptable=%s",
         js_context_, &js_object_, js_object_, this, name_.c_str());
#endif
    JS_AddNamedRootRT(JS_GetRuntime(js_context_), &js_object_, name_.c_str());
    DebugRoot(js_context_);
  }
  scriptable->Ref();
  on_reference_change_connection_ = scriptable->ConnectOnReferenceChange(
      NewSlot(this, &NativeJSWrapper::OnReferenceChange));

#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("Wrap: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
       js_context_, js_object_, this, name_.c_str(), scriptable->GetRefCount());
#ifdef DEBUG_FORCE_GC
  // This GC forces many hidden memory allocation errors to expose.
  DLOG("ForceGC");
  JS_GC(js_context_);
#endif
#endif
}

JSBool NativeJSWrapper::Unwrap(JSContext *cx, JSObject *obj,
                               ScriptableInterface **scriptable) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper) {
    *scriptable = wrapper->scriptable_;
    return JS_TRUE;
  } else {
    return JS_FALSE;
  }
}

// Get the NativeJSWrapper from a JS wrapped ScriptableInterface object.
// The NativeJSWrapper pointer is stored in the object's private slot.
NativeJSWrapper *NativeJSWrapper::GetWrapperFromJS(JSContext *cx,
                                                   JSObject *js_object) {
  if (js_object) {
    JSClass *cls = JS_GET_CLASS(cx, js_object);
    if (cls && cls->getProperty == wrapper_js_class_.getProperty &&
        cls->setProperty == wrapper_js_class_.setProperty) {
      ASSERT(cls->resolve == wrapper_js_class_.resolve &&
             cls->finalize == wrapper_js_class_.finalize);

      NativeJSWrapper *wrapper =
          reinterpret_cast<NativeJSWrapper *>(JS_GetPrivate(cx, js_object));
      if (!wrapper)
        // This is the prototype object created by JS_InitClass();
        return NULL;

      ASSERT(wrapper && wrapper->js_object_ == js_object);
      return wrapper;
    }
  }

  // The JSObject is not a JS wrapped ScriptableInterface object.
  return NULL;
}

JSBool NativeJSWrapper::CheckNotDeleted() {
  if (!js_context_) {
    LOG("The context of a JS wrapped native object has already been "
        "destroyed.");
    return JS_FALSE;
  }
  if (!scriptable_) {
    RaiseException(js_context_, "Native object has been deleted");
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::CallWrapperSelf(JSContext *cx, JSObject *obj,
                                        uintN argc, jsval *argv,
                                        jsval *rval) {
  GGL_UNUSED(obj);
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  // In this case, the real self object being called is at argv[-2].
  JSObject *self_object = JSVAL_TO_OBJECT(argv[-2]);
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, self_object);
  ScopedLogContext log_context(GetJSScriptContext(cx));
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->CallSelf(argc, argv, rval));
}

JSBool NativeJSWrapper::CallWrapperMethod(JSContext *cx, JSObject *obj,
                                          uintN argc, jsval *argv,
                                          jsval *rval) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  ScopedLogContext log_context(GetJSScriptContext(cx));
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->CallMethod(argc, argv, rval));
}

JSBool NativeJSWrapper::WrapperDefaultToString(JSContext *cx, JSObject *obj,
                                               uintN argc, jsval *argv,
                                               jsval *rval) {
  GGL_UNUSED(argc);
  GGL_UNUSED(argv);
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  ScopedLogContext log_context(GetJSScriptContext(cx));
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->DefaultToString(rval));
}

JSBool NativeJSWrapper::GetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  // Don't check exception here to let exception handling succeed.
  ScopedLogContext log_context(GetJSScriptContext(cx));
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyDefault(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  // Don't check exception here to let exception handling succeed.
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  ScopedLogContext log_context(GetJSScriptContext(cx));
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyDefault(id, *vp));
}

JSBool NativeJSWrapper::GetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  ScopedLogContext log_context(GetJSScriptContext(cx));
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyByName(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  ScopedLogContext log_context(GetJSScriptContext(cx));
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyByName(id, *vp));
}

JSBool NativeJSWrapper::EnumerateWrapper(JSContext *cx, JSObject *obj,
                                         JSIterateOp enum_op,
                                         jsval *statep, jsid *idp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  ScopedLogContext log_context(GetJSScriptContext(cx));
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         // Don't CheckNotDeleted() when enum_op == JSENUMERATE_DESTROY
         // because we need this to clean up the resources allocated during
         // enumeration. NOTE: this may occur during GC.
         ((enum_op == JSENUMERATE_DESTROY || wrapper->CheckNotDeleted()) &&
          wrapper->Enumerate(enum_op, statep, idp));
}

JSBool NativeJSWrapper::ResolveWrapperProperty(JSContext *cx, JSObject *obj,
                                               jsval id, uintN flags,
                                               JSObject **objp) {
  ScopedLogContext log_context(GetJSScriptContext(cx));
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->ResolveProperty(id, flags, objp));
}

void NativeJSWrapper::FinalizeWrapper(JSContext *cx, JSObject *obj) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("Finalize: cx=%p jsobj=%p wrapper=%p scriptable=%s",
         cx, obj, wrapper, wrapper->name_.c_str());
#endif

    if (wrapper->scriptable_) {
      // The current context may be different from wrapper's context during
      // GC collecting. Use the wrapper's context instead.
      JSScriptContext::FinalizeNativeJSWrapper(wrapper->js_context_, wrapper);
    }

    for (JSFunctionSlots::iterator it = wrapper->js_function_slots_.begin();
         it != wrapper->js_function_slots_.end(); ++it)
      (*it)->Finalize();
    delete wrapper;
  }

  JSScriptContext::UnrefJSObjectClass(cx, obj);
}

uint32 NativeJSWrapper::MarkWrapper(JSContext *cx, JSObject *obj, void *arg) {
  GGL_UNUSED(arg);
  // The current context may be different from wrapper's context during
  // GC marking.
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper && wrapper->scriptable_)
    wrapper->Mark();
  return 0;
}

void NativeJSWrapper::DetachJS(bool caused_by_native) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("DetachJS: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
       js_context_, js_object_, this, name_.c_str(),
       scriptable_->GetRefCount());
#endif

  on_reference_change_connection_->Disconnect();
  scriptable_->Unref(caused_by_native);
  scriptable_ = NULL;

  if (js_context_) {
    JS_RemoveRootRT(JS_GetRuntime(js_context_), &js_object_);
    DebugRoot(js_context_);
  }
}

void NativeJSWrapper::OnContextDestroy() {
  DetachJS(false);
  while (!js_function_slots_.empty()) {
    JSFunctionSlots::iterator it = js_function_slots_.begin();
    (*it)->Finalize();
    js_function_slots_.erase(it);
  }
  JS_SetPrivate(js_context_, js_object_, NULL);
  js_context_ = NULL;
}

void NativeJSWrapper::OnReferenceChange(int ref_count, int change) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("OnReferenceChange(%d,%d): cx=%p jsobj=%p wrapper=%p scriptable=%s",
       ref_count, change, js_context_, js_object_, this, name_.c_str());
#endif

  if (change == 0) {
    // Remove the wrapper mapping from the context, but leave this wrapper
    // alive to accept mistaken JavaScript calls gracefully.
    JSScriptContext::FinalizeNativeJSWrapper(js_context_, this);

    // As the native side is deleting the object, now the script side can also
    // delete it if there is no other active references.
    DetachJS(true);

#ifdef DEBUG_JS_WRAPPER_MEMORY
#ifdef DEBUG_FORCE_GC
    // This GC forces many hidden memory allocation errors to expose.
    DLOG("ForceGC");
    JS_GC(js_context_);
#endif
#endif
  } else {
    ASSERT(change == 1 || change == -1);
    if (change == 1 && ref_count == 1) {
      // There must be at least one native reference, let JavaScript know it
      // by adding the object to root.
#ifdef DEBUG_JS_WRAPPER_MEMORY
      DLOG("AddRoot: cx=%p jsobjaddr=%p jsobj=%p wrapper=%p scriptable=%s",
           js_context_, &js_object_, js_object_, this, name_.c_str());
#endif
      JS_AddNamedRootRT(JS_GetRuntime(js_context_), &js_object_, name_.c_str());
      DebugRoot(js_context_);
    } else if (change == -1 && ref_count == 2) {
      // The last native reference is about to be released, let JavaScript know
      // it by removing the root reference.
#ifdef DEBUG_JS_WRAPPER_MEMORY
      DLOG("RemoveRoot: cx=%p jsobjaddr=%p jsobj=%p wrapper=%p scriptable=%s",
           js_context_, &js_object_, js_object_, this, name_.c_str());
#endif
      JS_RemoveRootRT(JS_GetRuntime(js_context_), &js_object_);
      DebugRoot(js_context_);
    }
  }
}

JSBool NativeJSWrapper::CallSelf(uintN argc, jsval *argv, jsval *rval) {
  ASSERT(scriptable_);

  Variant prototype;
  // Get the default method for this object.
  if (scriptable_->GetPropertyInfo("", &prototype) !=
      ScriptableInterface::PROPERTY_METHOD) {
    RaiseException(js_context_, "Object can't be called as a function");
    return JS_FALSE;
  }

  if (!CheckException(js_context_, scriptable_))
    return JS_FALSE;

  return CallNativeSlot("DEFAULT", VariantValue<Slot *>()(prototype),
                        argc, argv, rval);
}

JSBool NativeJSWrapper::CallMethod(uintN argc, jsval *argv, jsval *rval) {
  ASSERT(scriptable_);
  // According to JS stack structure, argv[-2] is the current function object.
  jsval func_val = argv[-2];
  // Get the method slot from the reserved slot.
  jsval val;
  if (!JS_GetReservedSlot(js_context_, JSVAL_TO_OBJECT(func_val), 0, &val) ||
      !JSVAL_IS_INT(val))
    return JS_FALSE;

  const char *name = JS_GetFunctionName(JS_ValueToFunction(js_context_,
                                                           func_val));
  return CallNativeSlot(name, reinterpret_cast<Slot *>(JSVAL_TO_PRIVATE(val)),
                        argc, argv, rval);
}

JSBool NativeJSWrapper::CallNativeSlot(const char *name, Slot *slot,
                                       uintN argc, jsval *argv, jsval *rval) {
  ASSERT(scriptable_);

  Variant *params = NULL;
  uintN expected_argc = argc;
  if (!ConvertJSArgsToNative(js_context_, this, name, slot, argc, argv,
                             &params, &expected_argc))
    return JS_FALSE;

  ResultVariant return_value = slot->Call(scriptable_, expected_argc, params);
  delete [] params;
  params = NULL;

  if (!CheckException(js_context_, scriptable_))
    return JS_FALSE;

  JSBool result = ConvertNativeToJS(js_context_, return_value.v(), rval);
  if (!result)
    RaiseException(js_context_,
                   "Failed to convert native function result(%s) to jsval",
                   return_value.v().Print().c_str());
  return result;
}

JSBool NativeJSWrapper::DefaultToString(jsval *rval) {
  return ConvertNativeToJS(js_context_, Variant(name_), rval);
}

JSBool NativeJSWrapper::GetPropertyDefault(jsval id, jsval *vp) {
  if (JSVAL_IS_INT(id))
    // The script wants to get the property by an array index.
    return GetPropertyByIndex(id, vp);

  // Use the default JavaScript logic.
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyDefault(jsval id, jsval js_val) {
  ASSERT(scriptable_);
  if (JSVAL_IS_INT(id))
    // The script wants to set the property by an array index.
    return SetPropertyByIndex(id, js_val);

  if (scriptable_->IsStrict()) {
    // The scriptable object don't allow the script engine to assign to
    // unregistered properties.
    RaiseException(js_context_,
                   "The native object doesn't support setting property %s.",
                   PrintJSValue(js_context_, id).c_str());
    return JS_FALSE;
  }
  // Use the default JavaScript logic.
  return JS_TRUE;
}

JSBool NativeJSWrapper::GetPropertyByIndex(jsval id, jsval *vp) {
  ASSERT(scriptable_);
  if (!JSVAL_IS_INT(id))
    // Should not occur.
    return JS_FALSE;

  int int_id = JSVAL_TO_INT(id);
  ResultVariant return_value = scriptable_->GetPropertyByIndex(int_id);
  if (!ConvertNativeToJS(js_context_, return_value.v(), vp)) {
    RaiseException(js_context_,
                   "Failed to convert native property [%d] value(%s) to jsval.",
                   int_id, return_value.v().Print().c_str());
    return JS_FALSE;
  }

  return CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::SetPropertyByIndex(jsval id, jsval js_val) {
  ASSERT(scriptable_);
  if (!JSVAL_IS_INT(id))
    // Should not occur.
    return JS_FALSE;

  int int_id = JSVAL_TO_INT(id);
  // Use the original value as the prototype, so that we can convert the
  // JS value to proper native type.
  Variant prototype = scriptable_->GetPropertyByIndex(int_id).v();
  if (!CheckException(js_context_, scriptable_))
    return JS_FALSE;

  // If there is no original value, don't set it.
  if (prototype.type() == Variant::TYPE_VOID) {
    if (scriptable_->IsStrict()) {
      // The scriptable object don't allow the script engine to assign to
      // unregistered properties.
      RaiseException(js_context_,
                     "The native object doesn't support setting property [%d].",
                     int_id);
      return JS_FALSE;
    }
    // Use the default JavaScript logic.
    return JS_TRUE;
  }

  Variant value;
  if (!ConvertJSToNative(js_context_, this, prototype, js_val, &value)) {
    RaiseException(js_context_,
                   "Failed to convert JS property [%d] value(%s) to native.",
                   int_id, PrintJSValue(js_context_, js_val).c_str());
    return JS_FALSE;
  }

  if (!scriptable_->SetPropertyByIndex(int_id, value)) {
    RaiseException(js_context_,
                   "Failed to set native property [%d] (may be readonly).",
                   int_id);
    FreeNativeValue(value);
    return JS_FALSE;
  }

  return CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::GetPropertyByName(jsval id, jsval *vp) {
  ASSERT(scriptable_);
  if (!JSVAL_IS_STRING(id))
    // Should not occur
    return JS_FALSE;

  JSString *idstr = JSVAL_TO_STRING(id);
  if (!idstr)
    return JS_FALSE;

  const jschar *utf16_name = JS_GetStringChars(idstr);
  size_t name_length = JS_GetStringLength(idstr);
  UTF16ToUTF8Converter utf8_name(utf16_name, name_length);
  ResultVariant return_value = scriptable_->GetProperty(utf8_name.get());
  if (!CheckException(js_context_, scriptable_))
    return JS_FALSE;

  if (return_value.v().type() == Variant::TYPE_VOID) {
    // This must be a dynamic property which is no more available.
    // Remove the property and fallback to the default handler.
    jsval r;
    JS_DeleteUCProperty2(js_context_, js_object_, utf16_name, name_length, &r);
    return GetPropertyDefault(id, vp);
  }

  if (!ConvertNativeToJS(js_context_, return_value.v(), vp)) {
    RaiseException(js_context_,
                   "Failed to convert native property %s value(%s) to jsval",
                   utf8_name.get(), return_value.v().Print().c_str());
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyByName(jsval id, jsval js_val) {
  ASSERT(scriptable_);
  if (!JSVAL_IS_STRING(id))
    // Should not occur
    return JS_FALSE;

  JSString *idstr = JSVAL_TO_STRING(id);
  if (!idstr)
    return JS_FALSE;

  const jschar *utf16_name = JS_GetStringChars(idstr);
  size_t name_length = JS_GetStringLength(idstr);
  UTF16ToUTF8Converter utf8_name(utf16_name, name_length);
  Variant prototype;
  if (scriptable_->GetPropertyInfo(utf8_name.get(), &prototype) ==
      ScriptableInterface::PROPERTY_NOT_EXIST) {
    // This must be a dynamic property which is no more available.
    // Remove the property and fallback to the default handler.
    jsval r;
    JS_DeleteUCProperty2(js_context_, js_object_, utf16_name, name_length, &r);
    return SetPropertyDefault(id, js_val);
  }
  if (!CheckException(js_context_, scriptable_))
    return JS_FALSE;

  Variant value;
  if (!ConvertJSToNative(js_context_, this, prototype, js_val, &value)) {
    RaiseException(js_context_,
                   "Failed to convert JS property %s value(%s) to native.",
                   utf8_name.get(), PrintJSValue(js_context_, js_val).c_str());
    return JS_FALSE;
  }

  if (!scriptable_->SetProperty(utf8_name.get(), value)) {
    RaiseException(js_context_,
                   "Failed to set native property %s (may be readonly).",
                   utf8_name.get());
    FreeNativeValue(value);
    return JS_FALSE;
  }

  return CheckException(js_context_, scriptable_);
}

class NameCollector {
 public:
  NameCollector(std::vector<std::string> *names) : names_(names) { }
  bool Collect(const char *name, ScriptableInterface::PropertyType type,
               const Variant &value) {
    GGL_UNUSED(type);
    GGL_UNUSED(value);
    names_->push_back(name);
    return true;
  }
  std::vector<std::string> *names_;
};

JSBool NativeJSWrapper::Enumerate(JSIterateOp enum_op,
                                  jsval *statep, jsid *idp) {
  if (!scriptable_->IsEnumeratable()) {
    *statep = JSVAL_NULL;
    if (idp)
      *idp = JS_ValueToId(js_context_, INT_TO_JSVAL(0), idp);
    return JS_TRUE;
  }

  ScopedLogContext log_context(GetJSScriptContext(js_context_));
  std::vector<std::string> *properties;
  switch (enum_op) {
    case JSENUMERATE_INIT: {
      properties = new std::vector<std::string>;
      NameCollector collector(properties);
      scriptable_->EnumerateProperties(NewSlot(&collector,
                                               &NameCollector::Collect));
      *statep = PRIVATE_TO_JSVAL(properties);
      if (idp)
        JS_ValueToId(js_context_, INT_TO_JSVAL(properties->size()), idp);
      break;
    }
    case JSENUMERATE_NEXT:
      properties = reinterpret_cast<std::vector<std::string> *>(
          JSVAL_TO_PRIVATE(*statep));
      if (!properties->empty()) {
        const char *name = properties->begin()->c_str();
        jsval idval = STRING_TO_JSVAL(JS_NewStringCopyZ(js_context_, name));
        JS_ValueToId(js_context_, idval, idp);
        properties->erase(properties->begin());
        break;
      }
      // Fall Through!
    case JSENUMERATE_DESTROY:
      properties = reinterpret_cast<std::vector<std::string> *>(
          JSVAL_TO_PRIVATE(*statep));
      delete properties;
      *statep = JSVAL_NULL;
      break;
    default:
      return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::ResolveProperty(jsval id, uintN flags,
                                        JSObject **objp) {
  ASSERT(scriptable_);
  ASSERT(objp);
  *objp = NULL;

  if (!JSVAL_IS_STRING(id))
    return JS_TRUE;

  JSString *idstr = JSVAL_TO_STRING(id);
  if (!idstr)
    return JS_FALSE;

  const jschar *utf16_name = JS_GetStringChars(idstr);
  size_t name_length = JS_GetStringLength(idstr);
  UTF16ToUTF8Converter utf8_name(utf16_name, name_length);

  // The JS program defines a new symbol. This has higher priority than the
  // properties of the global scriptable object.
  if (flags & JSRESOLVE_DECLARING)
    return JS_TRUE;

  Variant prototype;
  ScriptableInterface::PropertyType type =
      scriptable_->GetPropertyInfo(utf8_name.get(), &prototype);
  if (type == ScriptableInterface::PROPERTY_NOT_EXIST) {
    if (strcmp(utf8_name.get(), "toString") == 0) {
      // Define a default toString() operator to ease debugging.
      JS_DefineUCFunction(js_context_, js_object_, utf16_name, name_length,
                          WrapperDefaultToString, 0, 0);
      *objp = js_object_;
    } else if (strcmp(utf8_name.get(), "__NATIVE_CLASS_ID__") == 0) {
      // Register __NATIVE_CLASS_ID__ property for JS debugging.
      jsval js_val;
      ConvertNativeToJS(js_context_,
                        Variant(StringPrintf("%jx", scriptable_->GetClassId())),
                        &js_val);
      JS_DefineUCProperty(js_context_, js_object_, utf16_name, name_length,
                          js_val, JS_PropertyStub, JS_PropertyStub,
                          JSPROP_READONLY | JSPROP_PERMANENT);
      *objp = js_object_;
    }

    // This property is not supported by the Scriptable, use default logic.
    return JS_TRUE;
  }

  if (!CheckException(js_context_, scriptable_))
    return JS_FALSE;

  if (type == ScriptableInterface::PROPERTY_METHOD) {
    // Define a Javascript function.
    Slot *slot = VariantValue<Slot *>()(prototype);
    JSFunction *function = JS_DefineUCFunction(js_context_, js_object_,
                                               utf16_name, name_length,
                                               CallWrapperMethod,
                                               slot->GetArgCount(), 0);
    if (!function)
      return JS_FALSE;

    JSObject *func_object = JS_GetFunctionObject(function);
    if (!func_object)
      return JS_FALSE;
    // Put the slot pointer into the first reserved slot of the
    // function object.  A function object has 2 reserved slots.
    if (!JS_SetReservedSlot(js_context_, func_object,
                            0, PRIVATE_TO_JSVAL(slot)))
      return JS_FALSE;

    *objp = js_object_;
    return JS_TRUE;
  }

  // Define a JavaScript property.
  jsval js_val = JSVAL_VOID;
  *objp = js_object_;
  if (type == ScriptableInterface::PROPERTY_CONSTANT) {
    if (!ConvertNativeToJS(js_context_, prototype, &js_val)) {
      RaiseException(js_context_, "Failed to convert init value(%s) to jsval",
                     prototype.Print().c_str());
      return JS_FALSE;
    }
    // This property is a constant, register a property with initial value.
    // Then the JavaScript engine will handle it.
    return JS_DefineUCProperty(js_context_, js_object_, utf16_name, name_length,
                               js_val, JS_PropertyStub, JS_PropertyStub,
                               JSPROP_READONLY | JSPROP_PERMANENT);
  }

  uintN property_attrs = 0;
  if (type == ScriptableInterface::PROPERTY_NORMAL) {
    // Don't set JSPROP_SHARED for slot properties, because we want the JS
    // engine to cache the value so that the script can read it back without
    // native code intervention. This is a required feature of the 5.8 API.
    if (prototype.type() != Variant::TYPE_SLOT)
      property_attrs |= JSPROP_SHARED;
    property_attrs |= JSPROP_PERMANENT;
  } else {
    property_attrs |= JSPROP_SHARED;
  }
  return JS_DefineUCProperty(js_context_, js_object_,  utf16_name, name_length,
                             js_val,
                             GetWrapperPropertyByName,
                             SetWrapperPropertyByName,
                             property_attrs);
}

void NativeJSWrapper::AddJSFunctionSlot(JSFunctionSlot *slot) {
  js_function_slots_.insert(slot);
}

void NativeJSWrapper::RemoveJSFunctionSlot(JSFunctionSlot *slot) {
  js_function_slots_.erase(slot);
}

void NativeJSWrapper::Mark() {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("Mark: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
       js_context_, js_object_, this, name_.c_str(),
       scriptable_->GetRefCount());
#endif
  for (JSFunctionSlots::const_iterator it = js_function_slots_.begin();
       it != js_function_slots_.end(); ++it)
    (*it)->Mark();
}

} // namespace smjs
} // namespace ggadget
