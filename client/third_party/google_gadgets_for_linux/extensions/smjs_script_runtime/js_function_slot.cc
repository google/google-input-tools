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

#include "js_function_slot.h"

#include <ggadget/scoped_ptr.h>
#include <ggadget/string_utils.h>
#include "converter.h"
#include "js_native_wrapper.h"
#include "js_script_context.h"
#include "native_js_wrapper.h"

namespace ggadget {
namespace smjs {

JSFunctionSlot::JSFunctionSlot(const Slot *prototype,
                               JSContext *context,
                               NativeJSWrapper *owner,
                               JSObject *function)
    : prototype_(prototype),
      context_(context),
      owner_(owner),
      function_(function),
      death_flag_ptr_(NULL) {
  ASSERT(function &&
         JS_TypeOfValue(context, OBJECT_TO_JSVAL(function)) == JSTYPE_FUNCTION);

  int lineno;
  JSScriptContext::GetCurrentFileAndLine(context, &function_info_, &lineno);
  StringAppendPrintf(&function_info_, ":%d", lineno);

  // Because the function may have a indirect reference to the owner through
  // the closure, we can't simply add the function to root, otherwise there
  // may be circular references if the native object's ownership is shared:
  //     native object =C++=> this slot =C++=> js function =JS=>
  //     closure =JS=> js wrapper object(owner) =C++=> native object.
  // This circle prevents the wrapper object and the function from being GC'ed.
  // Expose the circle to the JS engine by letting the owner manage this slot.
  if (owner) {
    owner->AddJSFunctionSlot(this);
  } else {
    // Otherwise, it's safe to add this function to root.
    JS_AddNamedRootRT(JS_GetRuntime(context),
                      &function_, function_info_.c_str());
  }
}

JSFunctionSlot::~JSFunctionSlot() {
  // Set *death_flag_ to true to let Call() know this slot is to be deleted.
  if (death_flag_ptr_)
    *death_flag_ptr_ = true;

  if (function_) {
    if (owner_)
      owner_->RemoveJSFunctionSlot(this);
    else
      JS_RemoveRootRT(JS_GetRuntime(context_), &function_);
  }
}

ResultVariant JSFunctionSlot::Call(ScriptableInterface *object, int argc,
                                   const Variant argv[]) const {
  Variant return_value(GetReturnType());

  if (!function_) {
    // Don't raise exception because the context_ may be invalid now.
    LOG("Finalized JavaScript function %s still be called",
        function_info_.c_str());
    return ResultVariant(return_value);
  }

  ScopedLogContext log_context(GetJSScriptContext(context_));
  if (JS_IsExceptionPending(context_))
    return ResultVariant(return_value);

  scoped_array<jsval> js_args;
  {
    AutoLocalRootScope local_root_scope(context_);
    if (!local_root_scope.good())
      return ResultVariant(return_value);

    if (argc > 0) {
      js_args.reset(new jsval[argc]);
      for (int i = 0; i < argc; i++) {
        if (!ConvertNativeToJS(context_, argv[i], &js_args[i])) {
          RaiseException(context_,
              "Failed to convert argument %d(%s) of function(%s) to jsval",
              i, argv[i].Print().c_str(), function_info_.c_str());
          return ResultVariant(return_value);
        }
      }
    }
  }

  bool death_flag = false;
  bool *death_flag_ptr = &death_flag;
  if (!death_flag_ptr_) {
    // Let the destructor inform us when this object is to be deleted.
    death_flag_ptr_ = death_flag_ptr;
  } else {
    // There must be some upper stack frame containing Call() call of the same
    // object. We just use the outer most death_flag_.
    death_flag_ptr = death_flag_ptr_;
  }

  JSObject *this_object = NULL;
  if (object && object->IsInstanceOf(JSNativeWrapper::CLASS_ID))
    this_object = down_cast<JSNativeWrapper *>(object)->js_object();

  jsval rval;
  JSBool ret = JS_CallFunctionValue(context_, this_object,
                                    OBJECT_TO_JSVAL(function_),
                                    argc, js_args.get(), &rval);
  if (!*death_flag_ptr) {
    if (death_flag_ptr == &death_flag)
      death_flag_ptr_ = NULL;

    if (context_) {
      if (ret) {
        if (!ConvertJSToNative(context_, NULL, return_value, rval,
                               &return_value)) {
          RaiseException(context_,
              "Failed to convert JS function(%s) return value(%s) to native",
              function_info_.c_str(), PrintJSValue(context_, rval).c_str());
        } else {
          // Must first hold return_value in a ResultValue, to prevent the
          // result from being deleted during GC.
          ResultVariant result(return_value);
          // Normal GC triggering doesn't work well if only little JS code is
          // executed but many native objects are referenced by dead JS
          // objects. Call MaybeGC to ensure GC is not starved.
          JSScriptContext::MaybeGC(context_);
          return result;
        }
      } else {
        JS_ReportPendingException(context_);
      }
    }
  }

  return ResultVariant(return_value);
}

void JSFunctionSlot::Mark() {
  if (function_)
    JS_MarkGCThing(context_, function_, "JSFunctionSlot", NULL);
}

void JSFunctionSlot::Finalize() {
  context_ = NULL;
  function_ = NULL;
  owner_ = NULL;
}

} // namespace smjs
} // namespace ggadget
