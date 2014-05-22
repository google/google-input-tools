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

#ifndef GGADGET_QT_CONVERTER_H__
#define GGADGET_QT_CONVERTER_H__

#include <QtScript/QScriptValue>
#include <QtScript/QScriptContext>
#include <ggadget/variant.h>

namespace ggadget {
namespace qt {

/**
 * Converts a @c QScriptValue to a @c Variant of desired type.
 * @param engine JavaScript engine.
 * @param prototype providing the desired target type information.
 * @param qval source @c QScriptValue value.
 * @param[out] val result @c Variant value.
 * @return @c true if succeeds.
 */
bool ConvertJSToNative(QScriptEngine *engine,
                       const Variant &prototype,
                       const QScriptValue &qval,
                       Variant *val);

/**
 * Converts a @c QScriptValue to a @c Variant based on source @c qval type.
 * @param engine JavaScript engine.
 * @param qval source @c QScriptValue value.
 * @param[out] val result @c Variant value.
 * @return @c true if succeeds.
 */
bool ConvertJSToNativeVariant(QScriptEngine *e, const QScriptValue &qval,
                              Variant *val);
/**
 * Frees a native value that was created by @c ConvertJSToNative(),
 * if some failed conditions preventing this value from successfully
 * passing to the native code.
 */
void FreeNativeValue(const Variant &val);

/**
 * Converts JavaScript arguments to native for a native slot.
 */
bool ConvertJSArgsToNative(QScriptContext *ctx, Slot *slot,
                           int *expected_argc, Variant **argv);

/**
 * Converts a @c Variant to a @c jsval.
 * @param engine JavaScript engine.
 * @param val source @c Variant value.
 * @param[out] qval result @c QScriptValue.
 * @return @c true if succeeds.
 */
bool ConvertNativeToJS(QScriptEngine *engine, const Variant &val,
                       QScriptValue *qval);

} // namespace qt
} // namespace ggadget

#endif  // GGADGET_QT_CONVERTER_H__
