/*
  Copyright 2009 Google Inc.

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

#ifndef EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JS_SCRIPT_CONTEXT_H__
#define EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JS_SCRIPT_CONTEXT_H__

#include <string>
#include <ggadget/scriptable_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/signals.h>
#include "java_script_core.h"

namespace ggadget {
namespace webkit {

class JSScriptRuntime;

/**
 * @c ScriptContext implementation for WebKit JavaScriptCore engine.
 */
class JSScriptContext : public ScriptContextInterface {
 public:
  /**
   * Creates a new JSScriptContext object.
   *
   * If specified js_context is not NULL, then just wrap it. Otherwise, a new
   * JSContextRef will be created.
   */
  JSScriptContext(JSScriptRuntime *runtime, JSContextRef js_context);
  virtual ~JSScriptContext();

  /** @see ScriptContextInterface::Destroy() */
  virtual void Destroy();

  /** @see ScriptContextInterface::Execute() */
  virtual void Execute(const char *script,
                       const char *filename,
                       int lineno);

  /** @see ScriptContextInterface::Compile() */
  virtual Slot *Compile(const char *script,
                        const char *filename,
                        int lineno);

  /** @see ScriptContextInterface::SetGlobalObject() */
  virtual bool SetGlobalObject(ScriptableInterface *global);

  /** @see ScriptContextInterface::RegisterClass() */
  virtual bool RegisterClass(const char *name, Slot *constructor);

  /** @see ScriptContextInterface::AssignFromContext() */
  virtual bool AssignFromContext(ScriptableInterface *dest_object,
                                 const char *dest_object_expr,
                                 const char *dest_property,
                                 ScriptContextInterface *src_context,
                                 ScriptableInterface *src_object,
                                 const char *src_expr);

  /** @see ScriptContextInterface::AssignFromContext() */
  virtual bool AssignFromNative(ScriptableInterface *object,
                                const char *object_expr,
                                const char *property,
                                const Variant &value);

  /** @see ScriptContextInterface::Evaluate() */
  virtual Variant Evaluate(ScriptableInterface *object, const char *expr);

  /** @see ScriptContextInterface::ConnectScriptBlockedFeedback() */
  virtual Connection *ConnectScriptBlockedFeedback(
      ScriptBlockedFeedback *feedback);

  /** @see ScriptContextInterface::CollectGarbage() */
  virtual void CollectGarbage();

  /** @see ScriptContextInterface::GetCurrentFileAndLine() */
  virtual void GetCurrentFileAndLine(std::string *filename, int *lineno);

 public:
  JSScriptRuntime *GetRuntime() const;
  JSContextRef GetContext() const;

  /** Wraps a native Scriptable object into a JavaScript object. */
  JSObjectRef WrapScriptable(ScriptableInterface *scriptable);

  /** Unwraps a wrapped Scriptable object. */
  ScriptableInterface *UnwrapScriptable(JSObjectRef object);

  /** Wraps a JSObjectRef object into a native Scriptable object. */
  ScriptableInterface *WrapJSObject(JSObjectRef object);

  /** Wrap a JSObjectRef function object into a Slot. */
  Slot *WrapJSObjectIntoSlot(const Slot *prototype, JSObjectRef owner,
                             JSObjectRef object);

  /** Unwraps a wrapped JSObjectRef object. */
  JSObjectRef UnwrapJSObject(ScriptableInterface *scriptable);

  /**
   * Unwraps a wrapped javascript function object from a JSFunctionSlot.
   * Returns true if specified slot is a JSFunctionSlot created by this
   * context.
   */
  bool UnwrapJSFunctionSlot(Slot *slot, JSValueRef *js_func);

  /** Checks if a JSObjectRef object is a wrapper of a Scriptable object. */
  bool IsWrapperOfScriptable(JSObjectRef object) const;

  /** Checks if a Scriptable object is a wrapper of JSObjectRef object. */
  bool IsWrapperOfJSObject(ScriptableInterface *scriptable) const;

  /**
   * Checks if there is an pending exception from JavaScript engine.
   * If specified exception is a valid exception, it'll be printed out and false
   * will be returned. Otherwise true will be returned.
   */
  bool CheckJSException(JSValueRef exception);

  /**
   * Checks if there is an pending exception in a Scriptable object.
   * If specified Scriptable object has an pending exception, the exception will
   * be converted into a JavaScript exception and false will be returned.
   * Oterwise true will be returned.
   */
  bool CheckScriptableException(ScriptableInterface *scriptable,
                                JSValueRef *exception);

  /** Checks if a JSValueRef is NaN. */
  bool IsNaN(JSValueRef value) const;

  /** Checks if a JSValueRef is Finite. */
  bool IsFinite(JSValueRef value) const;

  /** Checks if a JSValueRef is a Date object. */
  bool IsDate(JSValueRef value) const;

  /** Checks if a JSValueRef is an Array object. */
  bool IsArray(JSValueRef value) const;

  /** Gets length property of an Array object. */
  unsigned int GetArrayLength(JSObjectRef array) const;

  /**
   * Registers a global function.
   * When calling the callback, PrivateData of the function object is the
   * pointer to this JSScriptContext object.
   */
  void RegisterGlobalFunction(const char *name,
                              JSObjectCallAsFunctionCallback callback);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptContext);

  class Impl;
  Impl *impl_;
};

} // namespace webkit
} // namespace ggadget

#endif  // EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JS_SCRIPT_CONTEXT_H__
