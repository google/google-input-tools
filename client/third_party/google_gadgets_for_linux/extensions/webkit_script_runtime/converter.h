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

#ifndef EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_CONVERTER_H__
#define EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_CONVERTER_H__
#include <string>
#include <ggadget/common.h>
#include <ggadget/variant.h>
#include "java_script_core.h"

namespace ggadget {
namespace webkit {

class JSScriptContext;

/**
 * Converts a @c JSValueRef to a @c Variant of desired type.
 * @param ctx JavaScript context.
 * @param owner the related JavaScript object wrapper in which the owner of
 *     the converted value is wrapped.
 * @param prototype providing the desired target type information.
 * @param js_val source @c JSValueRef value.
 * @param[out] native_val result @c Variant value.
 * @return true if succeeds.
 */
bool ConvertJSToNative(JSScriptContext *ctx, JSObjectRef owner,
                       const Variant &prototype,
                       JSValueRef js_val, Variant *native_val);

/**
 * Converts a @c JSValueRef to a @c Variant depending on source @c JSValueRef
 * type.
 * @param ctx JavaScript context.
 * @param js_val source @c JSValueRef value.
 * @param[out] native_val result @c Variant value.
 * @return true if succeeds.
 */
bool ConvertJSToNativeVariant(JSScriptContext *ctx, JSValueRef js_val,
                              Variant *native_val);

/**
 * Frees a native value that was created by @c ConvertJSToNative(),
 * if some failed conditions preventing this value from successfully
 * passing to the native code.
 */
void FreeNativeValue(const Variant &native_val);

/**
 * Converts a @c JSValueRef to a @c std::string for printing.
 */
std::string PrintJSValue(JSScriptContext *ctx, JSValueRef js_val);

/**
 * Converts JavaScript arguments to native for a native slot.
 */
bool ConvertJSArgsToNative(JSScriptContext *ctx, JSObjectRef owner,
                           const char *name, Slot *slot,
                           size_t argc, const JSValueRef argv[],
                           Variant **params, size_t *expected_argc,
                           JSValueRef *exception);

/**
 * Converts a @c Variant to a @c JSValueRef.
 * @param ctx JavaScript context.
 * @param native_val source @c Variant value.
 * @param[out] js_val result @c JSValueRef value.
 * @return @c true if succeeds.
 */
bool ConvertNativeToJS(JSScriptContext *ctx, const Variant &native_val,
                       JSValueRef *js_val);

/**
 * Compiles function source into JSObject.
 */
JSObjectRef CompileFunction(JSScriptContext *ctx, const char *script,
                            const char *filename, int lineno,
                            JSValueRef *exception);

/**
 * Creates an exception object from a printf result and stores it into exception.
 */
void RaiseJSException(JSScriptContext *ctx, JSValueRef *exception,
                      const char *format, ...)
    PRINTF_ATTRIBUTE(3, 4);

} // namespace webkit
} // namespace ggadget

#endif  // EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_CONVERTER_H__
