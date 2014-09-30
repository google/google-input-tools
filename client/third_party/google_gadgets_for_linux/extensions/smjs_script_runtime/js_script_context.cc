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

#include "js_script_context.h"

#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/unicode_utils.h>
#include <jscntxt.h>
#include "converter.h"
#include "js_function_slot.h"
#include "js_native_wrapper.h"
#include "js_script_runtime.h"
#include "native_js_wrapper.h"

#ifdef _DEBUG
// #define DEBUG_JS_ROOTS
#endif

#ifdef DEBUG_JS_ROOTS
#include <jscntxt.h>
#include <jsdhash.h>
#endif

namespace ggadget {
namespace smjs {

// OperationCallback is checked after the script executed instructions
// weighted 100 * JS_OPERATION_WEIGHT_BASE (new version), or 100 branches
// (old version, deperecated).
static const uint32 kOperationCallbackMultiply = 100;
#ifdef JS_OPERATION_WEIGHT_BASE
// The accumulated operation weight before OperationCallback is called.
static const uint32 kOperationCallbackWeight =
    kOperationCallbackMultiply * JS_OPERATION_WEIGHT_BASE;
#endif

static const uint64_t kMaxGCInterval = 5000; // 5 seconds.

uint64_t JSScriptContext::last_gc_time_ = 0;
uint64_t JSScriptContext::operation_callback_time_ = 0;
int JSScriptContext::reset_operation_time_timer_ = 0;

JSScriptContext *GetJSScriptContext(JSContext *context) {
  return reinterpret_cast<JSScriptContext *>(JS_GetContextPrivate(context));
}

static JSBool LocaleCompare(JSContext *cx, JSString *s1, JSString *s2,
                            jsval *rval) {
  if (!s1 || !s2)
    return JS_FALSE;

  jschar *chars1 = JS_GetStringChars(s1);
  jschar *chars2 = JS_GetStringChars(s2);
  if (!chars1 || !chars2)
    return JS_FALSE;

  size_t length1 = JS_GetStringLength(s1);
  size_t length2 = JS_GetStringLength(s2);
  std::string locale_s1, locale_s2;
  if (!ConvertUTF16ToLocaleString(chars1, length1, &locale_s1) ||
      !ConvertUTF16ToLocaleString(chars2, length2, &locale_s2)) {
    RaiseException(cx, "Failed to convert strings to locale strings");
    return JS_FALSE;
  }

  *rval = INT_TO_JSVAL(CompareLocaleStrings(locale_s1.c_str(),
                                            locale_s2.c_str()));
  return JS_TRUE;
}

static JSBool LocaleToUnicode(JSContext *cx, char *src, jsval *rval) {
  UTF16String utf16;
  if (ConvertLocaleStringToUTF16(src, &utf16)) {
    JSString *js_str = JS_NewUCStringCopyN(cx, utf16.c_str(), utf16.size());
    if (js_str) {
      *rval = STRING_TO_JSVAL(js_str);
      return JS_TRUE;
    }
  }
  RaiseException(cx, "Failed to convert locale string '%s' to unicode", src);
  return JS_FALSE;
}

static JSLocaleCallbacks gLocaleCallbacks = {
  // SpiderMonkey's upper/lower convertions conform to the Unicode standard.
  NULL, NULL,
  LocaleCompare, LocaleToUnicode,
  NULL
};

// For each native classes registered to each JSScriptContext with
// RegisterClass(), a JSClassWithNativeCtor object will be created.
class JSScriptContext::JSClassWithNativeCtor {
 public:
  JSClassWithNativeCtor(const char *name, Slot *constructor)
      : constructor_(constructor), ref_count_(0) {
    js_class_ = *NativeJSWrapper::GetWrapperJSClass();
    ASSERT(js_class_.addProperty == JS_PropertyStub);
    // Use unique address of addProperty callback to indicate this is a
    // valid JSClassWithNativeCtor structure.
    js_class_.addProperty = TagAddProperty;
    js_class_.name = name;
  }

  static JSBool TagAddProperty(JSContext *cx, JSObject *obj,
                               jsval id, jsval *vp) {
    return JS_PropertyStub(cx, obj, id, vp);
  }

  void Ref() {
    ++ref_count_;
  }

  void Unref() {
    if (--ref_count_ == 0)
      delete this;
  }

  static JSClassWithNativeCtor *CastFrom(JSClass *cls) {
    return cls->addProperty == TagAddProperty ?
           reinterpret_cast<JSClassWithNativeCtor *>(cls) : NULL;
  }

  // This field must be placed first so that JSClass * can be cast to
  // JSClassWithNativeCtor *.
  JSClass js_class_;
  Slot *constructor_;

 private:
  ~JSClassWithNativeCtor() {
    memset(&js_class_, 0, sizeof(js_class_));
    ASSERT(ref_count_ == 0);
    delete constructor_;
    constructor_ = NULL;
  }

  int ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(JSClassWithNativeCtor);
};

JSScriptContext::JSScriptContext(JSScriptRuntime *runtime, JSContext *context)
    : runtime_(runtime),
      context_(context),
      lineno_(0) {
  JS_SetContextPrivate(context_, this);
  JS_SetLocaleCallbacks(context_, &gLocaleCallbacks);
#ifdef HAVE_JS_SetOperationCallback
#ifdef JS_OPERATION_WEIGHT_BASE
  JS_SetOperationCallback(context_, OperationCallback,
                          kOperationCallbackWeight);
#else
  // This func in newer SpiderMonkey in xulrunner 1.9.1 has a different
  // prototype.
  JS_SetOperationCallback(context_, OperationCallback);
#endif
#endif

  JS_SetErrorReporter(context, ReportError);
  // JS_SetOptions(context_, JS_GetOptions(context_) | JSOPTION_STRICT);

  if (!reset_operation_time_timer_) {
    MainLoopInterface *main_loop = GetGlobalMainLoop();
    if (main_loop) {
      reset_operation_time_timer_ =
          main_loop->AddTimeoutWatch(kMaxScriptRunTime / 2,
              new WatchCallbackSlot(NewSlot(OnClearOperationTimeTimer)));
    }
  }
}

JSScriptContext::~JSScriptContext() {
  // Don't report errors during shutdown because the state may be inconsistent.
  JS_SetErrorReporter(context_, NULL);
  // Remove the return value protection reference.
  // See comments in WrapJSObjectToNative() for details.
  JSObject *global_object = JS_GetGlobalObject(context_);
  JS_DeleteProperty(context_, global_object, kGlobalReferenceName);

  // Unregister global classes, to avoid them from being marked (causing crash)
  // if there is live js object after this context is destroyed.
  for (ClassVector::iterator it = registered_classes_.begin();
       it != registered_classes_.end(); ++it) {
    // ClassName.prototype also hold a reference to the class, so deleting
    // the global class will automatically unref.
    JS_DeleteProperty(context_, global_object, (*it)->js_class_.name);
  }

  // Force a GC to make it possible to check if there are leaks.
  JS_GC(context_);

  while (!native_js_wrapper_map_.empty()) {
    NativeJSWrapperMap::iterator it = native_js_wrapper_map_.begin();
    NativeJSWrapper *wrapper = it->second;
#if 0
    // This leak check is not reliable. The remaining objects may be leaks,
    // but can be also native owned objects or constants of native owned
    // objects.
    if (wrapper->scriptable()->GetRefCount() > 1) {
      DLOG("Still referenced by native: jsobj=%p wrapper=%p scriptable=%s"
           " refcount=%d", wrapper->js_object(), wrapper,
           wrapper->name().c_str(), wrapper->scriptable()->GetRefCount());
    }
#endif
    native_js_wrapper_map_.erase(it);
    wrapper->OnContextDestroy();
  }

  while (!js_native_wrapper_map_.empty()) {
    JSNativeWrapperMap::iterator it = js_native_wrapper_map_.begin();
    it->second->OnContextDestroy();
    js_native_wrapper_map_.erase(it);
  }

  JS_DestroyContext(context_);
  context_ = NULL;
}

// As we want to depend on only the public SpiderMonkey APIs, the only
// way to get the current filename and lineno is from the JSErrorReport.
void JSScriptContext::RecordFileAndLine(JSContext *cx, const char *message,
                                        JSErrorReport *report) {
  GGL_UNUSED(message);
  JSScriptContext *context = GetJSScriptContext(cx);
  if (context) {
    context->filename_ = report->filename ? report->filename : "";
    context->lineno_ = report->lineno;
  }
}

void JSScriptContext::GetCurrentFileAndLine(std::string *filename,
                                            int *lineno) {
  filename_.clear();
  lineno_ = 0;
  jsval old_exception;
  JSBool has_old_exception = JS_GetPendingException(context_, &old_exception);
  JSErrorReporter old_reporter = JS_SetErrorReporter(context_,
                                                     RecordFileAndLine);
  // Let the JavaScript engine call RecordFileAndLine.
  JS_ReportWarning(context_, "FAKE");
  JS_SetErrorReporter(context_, old_reporter);
  if (has_old_exception)
    JS_SetPendingException(context_, old_exception);
  else
    JS_ClearPendingException(context_);

  *filename = filename_;
  *lineno = lineno_;
}

void JSScriptContext::GetCurrentFileAndLine(JSContext *context,
                                            std::string *filename,
                                            int *lineno) {
  ASSERT(filename && lineno);
  JSScriptContext *context_wrapper = GetJSScriptContext(context);
  if (context_wrapper) {
    context_wrapper->GetCurrentFileAndLine(filename, lineno);
  } else {
    filename->clear();
    *lineno = 0;
  }
}

NativeJSWrapper *JSScriptContext::WrapNativeObjectToJSInternal(
    JSObject *js_object, NativeJSWrapper *wrapper,
    ScriptableInterface *scriptable) {
  ASSERT(scriptable);
  NativeJSWrapperMap::const_iterator it =
      native_js_wrapper_map_.find(scriptable);
  if (it == native_js_wrapper_map_.end()) {
    if (!js_object) {
      js_object = JS_NewObject(context_, NativeJSWrapper::GetWrapperJSClass(),
                               NULL, NULL);
    }
    if (!js_object)
      return NULL;

    if (!wrapper)
      wrapper = new NativeJSWrapper(context_, js_object, scriptable);
    else
      wrapper->Wrap(scriptable);

    native_js_wrapper_map_[scriptable] = wrapper;
    ASSERT(wrapper->scriptable() == scriptable);
    return wrapper;
  } else {
    ASSERT(!wrapper);
    ASSERT(!js_object);
    return it->second;
  }
}

NativeJSWrapper *JSScriptContext::WrapNativeObjectToJS(
    JSContext *cx, ScriptableInterface *scriptable) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->WrapNativeObjectToJSInternal(NULL, NULL,
                                                         scriptable);
  return NULL;
}

void JSScriptContext::FinalizeNativeJSWrapperInternal(
    NativeJSWrapper *wrapper) {
  native_js_wrapper_map_.erase(wrapper->scriptable());
}

void JSScriptContext::FinalizeNativeJSWrapper(JSContext *cx,
                                              NativeJSWrapper *wrapper) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->FinalizeNativeJSWrapperInternal(wrapper);
}

JSNativeWrapper *JSScriptContext::WrapJSToNativeInternal(JSObject *obj) {
  ASSERT(obj);
  jsval js_val = OBJECT_TO_JSVAL(obj);
  JSNativeWrapper *wrapper;
  JSNativeWrapperMap::const_iterator it = js_native_wrapper_map_.find(obj);
  if (it == js_native_wrapper_map_.end()) {
    wrapper = new JSNativeWrapper(context_, obj);
    js_native_wrapper_map_[obj] = wrapper;
    return wrapper;
  } else {
    wrapper = it->second;
  }

  // Set the wrapped object as a property of the global object to prevent it
  // from being unexpectedly GC'ed before the native side receives it.
  // This is useful when returning the object to the native side.
  // If this object is passed via native slot parameters, because the objects
  // are also protected by the JS stack, there is no problem of property
  // overwriting.
  // The native side can call Ref() if it wants to hold the wrapper object.
  JS_DefineProperty(context_, JS_GetGlobalObject(context_),
                    kGlobalReferenceName, js_val, NULL, NULL, 0);
  return wrapper;
}

JSNativeWrapper *JSScriptContext::WrapJSToNative(JSContext *cx,
                                                 JSObject *obj) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->WrapJSToNativeInternal(obj);
  return NULL;
}

void JSScriptContext::FinalizeJSNativeWrapperInternal(
    JSNativeWrapper *wrapper) {
  js_native_wrapper_map_.erase(wrapper->js_object());
}

void JSScriptContext::FinalizeJSNativeWrapper(JSContext *cx,
                                              JSNativeWrapper *wrapper) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->FinalizeJSNativeWrapperInternal(wrapper);
}

void JSScriptContext::UnrefJSObjectClass(JSContext *cx, JSObject *object) {
  JSClassWithNativeCtor *c =
      JSClassWithNativeCtor::CastFrom(JS_GET_CLASS(cx, object));
  if (c) c->Unref();
}

void JSScriptContext::Destroy() {
  runtime_->DestroyContext(this);
}

void JSScriptContext::Execute(const char *script,
                              const char *filename,
                              int lineno) {
  jsval rval;
  EvaluateScript(context_, JS_GetGlobalObject(context_), script,
                 filename, lineno, &rval);
}

Slot *JSScriptContext::Compile(const char *script,
                               const char *filename,
                               int lineno) {
  JSFunction *function = CompileFunction(context_, script, filename, lineno);
  if (!function)
    return NULL;

  return new JSFunctionSlot(NULL, context_, NULL,
                            JS_GetFunctionObject(function));
}

static JSBool ReturnSelf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                         jsval *rval) {
  GGL_UNUSED(cx);
  GGL_UNUSED(argc);
  GGL_UNUSED(argv);
  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}

static JSObject *GetClassPrototype(JSContext *cx, const char *class_name) {
  // Add some adapters for JScript.
  jsval ctor;
  if (!JS_GetProperty(cx, JS_GetGlobalObject(cx), class_name, &ctor) ||
      JSVAL_IS_NULL(ctor) || !JSVAL_IS_OBJECT(ctor))
    return NULL;

  jsval proto;
  if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(ctor), "prototype", &proto) ||
      JSVAL_IS_NULL(proto) || !JSVAL_IS_OBJECT(proto))
    return NULL;

  return JSVAL_TO_OBJECT(proto);
}

static void ForceGC(JSContext *cx) {
  JSRuntime *rt = JS_GetRuntime(cx);
  DLOG("Force GC: gcBytes=%"PRIuS" gcLastBytes=%"PRIuS" gcMaxBytes=%"PRIuS
       " gcMaxMallocBytes=%"PRIuS,
       rt->gcBytes, rt->gcLastBytes, rt->gcMaxBytes, rt->gcMaxMallocBytes);
  JS_GC(cx);
  DLOG("Force GC Finished: gcBytes=%"PRIuS" gcLastBytes=%"PRIuS
       " gcMaxBytes=%"PRIuS" gcMaxMallocBytes=%"PRIuS,
       rt->gcBytes, rt->gcLastBytes, rt->gcMaxBytes, rt->gcMaxMallocBytes);
}

static JSBool DoGC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval) {
  GGL_UNUSED(obj);
  GGL_UNUSED(argc);
  GGL_UNUSED(argv);
  GGL_UNUSED(rval);
  ForceGC(cx);
  return JS_TRUE;
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global_object) {
  NativeJSWrapper *wrapper = WrapNativeObjectToJS(context_, global_object);
  JSObject *js_global = wrapper->js_object();
  if (!js_global)
    return false;
  if (!JS_InitStandardClasses(context_, js_global))
    return false;

  // Add some adapters for JScript.
  // JScript programs call Date.getVarDate() to convert a JavaScript Date to
  // a COM VARDATE. We just use Date's where VARDATE's are expected by JScript
  // programs.
  JSObject *date_proto = GetClassPrototype(context_, "Date");
  JS_DefineFunction(context_, date_proto, "getVarDate", &ReturnSelf, 0, 0);
  // For Windows compatibility.
  JS_DefineFunction(context_, js_global, "CollectGarbage", &DoGC, 0, 0);

  return true;
}

JSBool JSScriptContext::ConstructObject(JSContext *cx, JSObject *obj,
                                        uintN argc, jsval *argv, jsval *rval) {
  GGL_UNUSED(rval);
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  ScopedLogContext log_context(context_wrapper);

  JSClassWithNativeCtor *cls =
      reinterpret_cast<JSClassWithNativeCtor *>(JS_GET_CLASS(cx, obj));
  ASSERT(cls);
  // Count the reference from ClassName.prototype.
  cls->Ref();

  // Create a wrapper first which doesn't really wrap a scriptable object,
  // because it is not available before the constructor is called.
  // This wrapper is important if there are any JavaScript callbacks in the
  // constructor argument list.
  NativeJSWrapper *wrapper = new NativeJSWrapper(cx, obj, NULL);
  Variant *params = NULL;
  uintN expected_argc = argc;
  if (!ConvertJSArgsToNative(cx, wrapper, cls->js_class_.name,
                             cls->constructor_, argc, argv,
                             &params, &expected_argc))
    return JS_FALSE;

  ResultVariant return_value = cls->constructor_->Call(NULL, expected_argc,
                                                       params);
  delete [] params;

  ASSERT(return_value.v().type() == Variant::TYPE_SCRIPTABLE);
  ScriptableInterface *scriptable =
      VariantValue<ScriptableInterface *>()(return_value.v());
  if (!scriptable) {
    RaiseException(cx, "Failed to construct native object of class %s",
                   cls->js_class_.name);
    return JS_FALSE;
  }

  context_wrapper->WrapNativeObjectToJSInternal(obj, wrapper, scriptable);
  return JS_TRUE;
}

bool JSScriptContext::RegisterClass(const char *name, Slot *constructor) {
  ASSERT(constructor);
  ASSERT(constructor->GetReturnType() == Variant::TYPE_SCRIPTABLE);
  ASSERT_M(JS_GetGlobalObject(context_), ("Global object should be set first"));

  JSClassWithNativeCtor *cls = new JSClassWithNativeCtor(name, constructor);
  cls->Ref();
  if (!JS_InitClass(context_, JS_GetGlobalObject(context_), NULL,
                    &cls->js_class_, &ConstructObject,
                    constructor->GetArgCount(),
                    NULL, NULL, NULL, NULL)) {
    cls->Unref();
    return false;
  }

  registered_classes_.push_back(cls);
  return true;
}

bool JSScriptContext::AssignFromContext(ScriptableInterface *dest_object,
                                        const char *dest_object_expr,
                                        const char *dest_property,
                                        ScriptContextInterface *src_context,
                                        ScriptableInterface *src_object,
                                        const char *src_expr) {
  ASSERT(src_context);
  ASSERT(dest_property);
  AutoLocalRootScope scope(context_);

  jsval dest_val;
  if (!EvaluateToJSVal(dest_object, dest_object_expr, &dest_val) ||
      !JSVAL_IS_OBJECT(dest_val) || JSVAL_IS_NULL(dest_val)) {
    DLOG("Expression %s doesn't evaluate to a non-null object",
         dest_object_expr);
    return false;
  }
  JSObject *dest_js_object = JSVAL_TO_OBJECT(dest_val);

  jsval src_val;
  JSScriptContext *src_js_context = down_cast<JSScriptContext *>(src_context);
  AutoLocalRootScope scope1(src_js_context->context_);
  if (!src_js_context->EvaluateToJSVal(src_object, src_expr, &src_val))
    return false;

  return JS_SetProperty(context_, dest_js_object, dest_property, &src_val);
}

bool JSScriptContext::AssignFromNative(ScriptableInterface *object,
                                       const char *object_expr,
                                       const char *property,
                                       const Variant &value) {
  ASSERT(property);
  AutoLocalRootScope scope(context_);

  jsval dest_val;
  if (!EvaluateToJSVal(object, object_expr, &dest_val) ||
      !JSVAL_IS_OBJECT(dest_val) || JSVAL_IS_NULL(dest_val)) {
    DLOG("Expression %s doesn't evaluate to a non-null object", object_expr);
    return false;
  }
  JSObject *js_object = JSVAL_TO_OBJECT(dest_val);

  jsval src_val;
  if (!ConvertNativeToJS(context_, value, &src_val))
    return false;
  return JS_SetProperty(context_, js_object, property, &src_val);
}

Variant JSScriptContext::Evaluate(ScriptableInterface *object,
                                  const char *expr) {
  Variant result;
  jsval js_val;
  if (EvaluateToJSVal(object, expr, &js_val)) {
    ConvertJSToNativeVariant(context_, js_val, &result);
    // Just left result in void state on any error.
  }
  return result;
}

JSBool JSScriptContext::EvaluateToJSVal(ScriptableInterface *object,
                                        const char *expr, jsval *result) {
  *result = JSVAL_VOID;
  JSObject *js_object;
  if (object) {
    NativeJSWrapperMap::const_iterator it = native_js_wrapper_map_.find(object);
    if (it == native_js_wrapper_map_.end()) {
      DLOG("Object %p hasn't a wrapper in JS", object);
      return JS_FALSE;
    }
    js_object = it->second->js_object();
  } else {
    js_object = JS_GetGlobalObject(context_);
  }

  if (expr && *expr) {
    if (!EvaluateScript(context_, js_object, expr, expr, 1, result)) {
      DLOG("Failed to evaluate dest_object_expr %s against JSObject %p",
           expr, js_object);
      return JS_FALSE;
    }
  } else {
    *result = OBJECT_TO_JSVAL(js_object);
  }
  return JS_TRUE;
}

void JSScriptContext::ReportError(JSContext *cx, const char *message,
                                  JSErrorReport *report) {
  GGL_UNUSED(cx);
  LOGE("%s:%d: %s", report->filename, report->lineno, message);
}

void JSScriptContext::MaybeGC(JSContext *cx) {
  MainLoopInterface *main_loop = GetGlobalMainLoop();
  uint64_t now = main_loop ? main_loop->GetCurrentTime() : 0;

  // Trigger GC on certain conditions.
  JSRuntime *rt = cx->runtime;
  uint32 bytes = rt->gcBytes;
  uint32 last_bytes = rt->gcLastBytes;
  if ((bytes > 8192 && bytes / 4 > last_bytes) ||
      now - last_gc_time_ > kMaxGCInterval) {
#if 0
    DLOG("GC Triggered: gcBytes=%u gcLastBytes=%u gcMaxBytes=%u "
         "gcMaxMallocBytes=%u", bytes, last_bytes, rt->gcMaxBytes,
         rt->gcMaxMallocBytes);
#endif
    JS_GC(cx);
    last_gc_time_ = now;
#if 0
    DLOG("GC Finished: gcBytes=%u gcLastBytes=%u gcMaxBytes=%u "
         "gcMaxMallocBytes=%u", rt->gcBytes, rt->gcLastBytes, rt->gcMaxBytes,
         rt->gcMaxMallocBytes);
#endif
  }
}

JSBool JSScriptContext::OperationCallback(JSContext *cx) {
  MaybeGC(cx);

  JSScriptContext *this_p = GetJSScriptContext(cx);
  if (!this_p)
    return JS_TRUE;

  MainLoopInterface *main_loop = GetGlobalMainLoop();
  if (!main_loop)
    return JS_TRUE;

  uint64_t now = main_loop->GetCurrentTime();
  // Check for long time script operation that may blocks UI.
  if (operation_callback_time_ == 0) {
    // The current script operation is just started. Start timing now.
    operation_callback_time_ = now;
    return JS_TRUE;
  }

  static uint64_t last_time = 0;
  if (last_time != 0 &&
      (now < last_time || now - last_time > kMaxScriptRunTime / 10)) {
    DLOG("Time changed, reset blocked-script timer.");
    // There must be some time change. Reset the timer.
    last_time = now;
    operation_callback_time_ = now;
    return JS_TRUE;
  }
  last_time = now;

  if (now > operation_callback_time_ + kMaxScriptRunTime) {
    static bool handling_script_blocked_signal = false;
    if (handling_script_blocked_signal) {
      // This may occur if some events (e.g. timer) caused again script
      // blocked before the last ScriptBlockedFeedback (which may displayed
      // a confirm dialog) returns. Just cancel the script.
      return JS_FALSE;
    }

    std::string filename;
    int lineno;
    GetCurrentFileAndLine(cx, &filename, &lineno);
    DLOG("Script runs too long at %s:%d, ask user whether to break",
         filename.c_str(), lineno);
    handling_script_blocked_signal = true;
    if (!this_p->script_blocked_signal_.HasActiveConnections() ||
        this_p->script_blocked_signal_(filename.c_str(), lineno)) {
      handling_script_blocked_signal = false;
      DLOG("Reset script timer");
      operation_callback_time_ = main_loop->GetCurrentTime();
      return JS_TRUE;
    }
    handling_script_blocked_signal = false;
    // Cancel the current script.
    return JS_FALSE;
  }
  return JS_TRUE;
}

bool JSScriptContext::OnClearOperationTimeTimer(int watch_id) {
  ASSERT(watch_id == reset_operation_time_timer_);
  operation_callback_time_ = 0;
  return true;
}

Connection *JSScriptContext::ConnectScriptBlockedFeedback(
    ScriptBlockedFeedback *feedback) {
  return script_blocked_signal_.Connect(feedback);
}

void JSScriptContext::CollectGarbage() {
  ForceGC(context_);
}

#ifdef DEBUG_JS_ROOTS
// This struct is private since JS170. Must defined same as JS structure.
struct MyJSGCRootHashEntry {
  JSDHashEntryHdr hdr;
  void *root;
  const char *name;
};

JS_STATIC_DLL_CALLBACK(JSDHashOperator) PrintRoot(JSDHashTable *table,
                                                  JSDHashEntryHdr *hdr,
                                                  uint32 number, void *arg) {
  MyJSGCRootHashEntry *rhe = reinterpret_cast<MyJSGCRootHashEntry *>(hdr);
  jsval *rp = reinterpret_cast<jsval *>(rhe->root);
  DLOG("%d: name=%s address=%p value=%p",
       number, rhe->name, rp, JSVAL_TO_OBJECT(*rp));
  return JS_DHASH_NEXT;
}

void DebugRoot(JSContext *cx) {
  DLOG("============== Roots ================");
  JSRuntime *rt = JS_GetRuntime(cx);
  JS_DHashTableEnumerate(&rt->gcRootsHash, PrintRoot, cx);
  DLOG("=========== End of Roots ============");
}
#else
void DebugRoot(JSContext *) {
}
#endif

} // namespace smjs
} // namespace ggadget
