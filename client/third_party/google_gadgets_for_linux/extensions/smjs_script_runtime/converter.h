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

#ifndef EXTENSIONS_SMJS_SCRIPT_RUNTIME_CONVERTER_H__
#define EXTENSIONS_SMJS_SCRIPT_RUNTIME_CONVERTER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/variant.h>
#include "libmozjs_glue.h"

namespace ggadget {
namespace smjs {

class NativeJSWrapper;

/**
 * Converts a @c jsval to a @c Variant of desired type.
 * @param cx JavaScript context.
 * @param owner the related JavaScript object wrapper in which the owner of
 *     the converted value is wrapped.
 * @param prototype providing the desired target type information.
 * @param js_val source @c jsval value.
 * @param[out] native_val result @c Variant value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertJSToNative(JSContext *cx, NativeJSWrapper *owner,
                         const Variant &prototype,
                         jsval js_val, Variant *native_val);

/**
 * Converts a @c jsval to a @c Variant depending source @c jsval type.
 * @param cx JavaScript context.
 * @param js_val source @c jsval value.
 * @param[out] native_val result @c Variant value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertJSToNativeVariant(JSContext *cx, jsval js_val,
                                Variant *native_val);

/**
 * Frees a native value that was created by @c ConvertJSToNative(),
 * if some failed conditions preventing this value from successfully
 * passing to the native code.
 */
void FreeNativeValue(const Variant &native_val);

/**
 * Converts a @c jsval to a @c std::string for printing.
 */
std::string PrintJSValue(JSContext *cx, jsval js_val);

/**
 * Converts JavaScript arguments to native for a native slot.
 */
JSBool ConvertJSArgsToNative(JSContext *cx, NativeJSWrapper *owner,
                             const char *name, Slot *slot,
                             uintN argc, jsval *argv,
                             Variant **params, uintN *expected_argc);

/**
 * Converts a @c Variant to a @c jsval.
 * @param cx JavaScript context.
 * @param native_val source @c Variant value.
 * @param[out] js_val result @c jsval value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertNativeToJS(JSContext* cx,
                         const Variant &native_val,
                         jsval* js_val);

/**
 * Compiles function source into <code>JSFunction *</code>.
 */
JSFunction *CompileFunction(JSContext *cx, const char *script,
                            const char *filename, int lineno);

/**
 * Compile and evaluate a piece of script.
 */
JSBool EvaluateScript(JSContext *cx, JSObject *object, const char *script,
                      const char *filename, int lineno, jsval *rval);

/**
 * Checks if there is pending exception. If there is, convert it into jsval
 * and throws it into the script engine.
 */
JSBool CheckException(JSContext *cx, ScriptableInterface *scriptable);

/**
 * Report an exception into the script engine.
 */
JSBool RaiseException(JSContext *cx, const char *format, ...)
    PRINTF_ATTRIBUTE(2, 3);

} // namespace smjs
} // namespace ggadget

#endif  // EXTENSIONS_SMJS_SCRIPT_RUNTIME_CONVERTER_H__
