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

#include <cmath>
#include <QtCore/QtDebug>
#include <QtCore/QDateTime>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_holder.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>
#include "converter.h"
#include "js_function_slot.h"
#include "js_native_wrapper.h"
#include "js_script_context.h"
#include "json.h"

#if 1
#undef DLOG
#define DLOG  true ? (void) 0 : LOG
#endif

namespace ggadget {
namespace qt {

static bool ConvertJSToNativeVoid(const QScriptValue &qval,
                                  Variant *val) {
  GGL_UNUSED(qval);
  *val = Variant();
  return true;
}

static bool ConvertJSToNativeBool(const QScriptValue &qval,
                                  Variant *val) {
  *val = Variant(qval.toBoolean());
  return true;
}

static bool ConvertJSToNativeInt(const QScriptValue &qval,
                                 Variant *val) {
  *val = Variant(static_cast<int64_t>(round(qval.toNumber())));
  return true;
}

static bool ConvertJSToNativeDouble(const QScriptValue &qval,
                                    Variant *val) {
  *val = Variant(static_cast<double>(qval.toNumber()));
  return true;
}

static bool ConvertJSToNativeString(const QScriptValue &qval,
                                    Variant *val) {
  if (qval.isNull())
    *val = Variant(static_cast<const char*>(NULL));
  else
    *val = Variant(qval.toString().toUtf8().data());
  return true;
}

static bool ConvertJSToNativeUTF16String(const QScriptValue &qval,
                                         Variant *val) {
  if (qval.isNull()) {
    *val = Variant(static_cast<const UTF16Char *>(NULL));
  } else {
    std::string str = qval.toString().toUtf8().data();
    UTF16String utf16_text;
    ConvertStringUTF8ToUTF16(str.c_str(), str.length(), &utf16_text);
    *val = Variant(utf16_text);
  }
  return true;
}

static bool ConvertJSToScriptable(QScriptEngine *e,
                                  const QScriptValue &qval,
                                  Variant *val) {
  // 2 kinds of js objects.
  //  - real js object
  //  - wrapper of native object
  ScriptableInterface *obj = GetNativeObject(qval);
  if (obj == NULL)
    obj = GetEngineContext(e)->WrapJSObject(qval);
  ASSERT(obj);
  *val = Variant(obj);
  return true;
}

static bool ConvertJSToSlot(QScriptEngine *e,
                            const Variant &prototype,
                            const QScriptValue &qval,
                            Variant *val) {
  JSFunctionSlot *slot = NULL;
  if (qval.isString()) {
    slot = new JSFunctionSlot(VariantValue<Slot*>()(prototype), e,
                              qval.toString().toUtf8().data(), NULL, 0);
  } else if (qval.isFunction()) {
    slot = new JSFunctionSlot(VariantValue<Slot*>()(prototype), e, qval);
  } else if (!qval.isNull()) {
    DLOG("ConvertJSToSlot failed:%s", qval.toString().toUtf8().data());
    return false;
  }
  *val = Variant(slot);
  return true;
}

static bool ConvertJSToNativeDate(const QScriptValue &qval, Variant *val) {
  QDateTime t = qval.toDateTime();
  uint64_t time_in_msec = t.toTime_t();
  time_in_msec *= 1000;
  time_in_msec += t.time().msec();
  *val = Variant(Date(time_in_msec));
  return true;
}

static bool ConvertJSToJSON(const QScriptValue &qval, Variant *val) {
  std::string json;
  JSONEncode(NULL, qval, &json);
  *val = Variant(JSONString(json));
  return true;
}

bool ConvertJSToNativeVariant(QScriptEngine *e, const QScriptValue &qval,
                              Variant *val) {
  if (qval.isNull() || !qval.isValid() || qval.isUndefined())
    return ConvertJSToNativeVoid(qval, val);
  if (qval.isBoolean())
    return ConvertJSToNativeBool(qval, val);
  // Don't try to convert the object to native Date, because JavaScript Date
  // is mutable, and sometimes the script may want to read it back and
  // change it. We only convert to native Date if the native side explicitly
  // requires a Date.
  // if (qval.isDate())
  //   return ConvertJSToNativeDate(qval, val);
  if (qval.isNumber())
    return ConvertJSToNativeDouble(qval, val);
  if (qval.isString())
    return ConvertJSToNativeString(qval, val);
 /* if (qval.isFunction()) {
    DLOG("Function not supported");
    ASSERT(0);
  } */
  if (qval.isQObject()) {
    DLOG("QObject not supported");
    ASSERT(0);
  }
  if (qval.isQMetaObject()){
    DLOG("QMetaObject not supported");
    ASSERT(0);
  }
  if (qval.isArray()) {
    DLOG("Array not supported");
    ASSERT(0);
  }
  if (qval.isObject())
    return ConvertJSToScriptable(e, qval, val);
  return false;
}

bool ConvertJSToNative(QScriptEngine *e, const Variant &prototype,
                       const QScriptValue &qval, Variant *val) {
  switch (prototype.type()) {
    case Variant::TYPE_VOID:
      return ConvertJSToNativeVoid(qval, val);
    case Variant::TYPE_BOOL:
      return ConvertJSToNativeBool(qval, val);
    case Variant::TYPE_INT64:
      return ConvertJSToNativeInt(qval, val);
    case Variant::TYPE_DOUBLE:
      return ConvertJSToNativeDouble(qval, val);
    case Variant::TYPE_STRING:
      return ConvertJSToNativeString(qval, val);
    case Variant::TYPE_JSON:
      return ConvertJSToJSON(qval, val);
    case Variant::TYPE_UTF16STRING:
      return ConvertJSToNativeUTF16String(qval, val);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertJSToScriptable(e, qval, val);
    case Variant::TYPE_SLOT:
      return ConvertJSToSlot(e, prototype, qval, val);
    case Variant::TYPE_DATE:
      return ConvertJSToNativeDate(qval, val);
    case Variant::TYPE_VARIANT:
      return ConvertJSToNativeVariant(e, qval, val);
   default:
      DLOG("ConvertJSToNative failed");
      return false;
  }
}

void FreeNativeValue(const Variant &native_val) {
  // Delete the JSFunctionSlot object that was created by ConvertJSToNative().
  if (native_val.type() == Variant::TYPE_SLOT)
    delete VariantValue<Slot *>()(native_val);
}

bool ConvertJSArgsToNative(QScriptContext *ctx, Slot *slot,
                           int *expected_argc, Variant **argv) {
  *argv = NULL;
  int argc = ctx->argumentCount();
  *expected_argc = argc;
  const Variant::Type *arg_types = NULL;
  const Variant *default_args = NULL;

  if (slot->HasMetadata()) {
    arg_types = slot->GetArgTypes();
    *expected_argc = slot->GetArgCount();
    if (*expected_argc == INT_MAX) {
      // Simply converts each arguments to native.
      *argv = new Variant[argc];
      *expected_argc = argc;
      int arg_type_idx = 0;
      for (int i = 0; i < argc; ++i) {
        bool result = false;
        if (arg_types && arg_types[arg_type_idx] != Variant::TYPE_VOID) {
          result = ConvertJSToNative(ctx->engine(),
                                     Variant(arg_types[arg_type_idx]),
                                     ctx->argument(i), &(*argv)[i]);
          ++arg_type_idx;
        } else {
          result = ConvertJSToNativeVariant(ctx->engine(), ctx->argument(i),
                                            &(*argv)[i]);
        }
        if (!result) {
          for (int j = 0; j < i; j++)
            FreeNativeValue((*argv)[j]);
          delete [] *argv;
          *argv = NULL;
          ctx->throwError(QString("Failed to convert argument %1 to native")
                          .arg(i));
          return false;
        }
      }
      return true;
    }

    default_args = slot->GetDefaultArgs();
    if (argc != *expected_argc) {
      int min_argc = *expected_argc;
      if (min_argc > 0 && default_args && argc < *expected_argc) {
        for (int i = min_argc - 1; i >= 0; i--) {
          if (default_args[i].type() != Variant::TYPE_VOID)
            min_argc--;
          else
            break;
        }
      }
      if (argc > *expected_argc || argc < min_argc) {
        ctx->throwError(
            QString("Wrong number of arguments: at least %1, actual:%2")
            .arg(min_argc).arg(argc));
        return false;
      }
    }
  }

  if (*expected_argc > 0) {
    *argv = new Variant[*expected_argc];
    // Fill up trailing default argument values.
    for (int i = argc; i < *expected_argc; i++) {
      (*argv)[i] = default_args[i];
    }

    for (int i = 0; i < argc; i++) {
      bool result;
      if (arg_types) {
        result = ConvertJSToNative(ctx->engine(), Variant(arg_types[i]),
                                   ctx->argument(i), &(*argv)[i]);
      } else {
        result = ConvertJSToNativeVariant(ctx->engine(), ctx->argument(i),
                                          &(*argv)[i]);
      }
      if (!result) {
        for (int j = 0; j < i; j++)
          FreeNativeValue((*argv)[j]);
        delete [] *argv;
        *argv = NULL;
        ctx->throwError(QString("Failed to convert argument %1 to native")
                        .arg(i));
        return false;
      }
    }
  }
  return true;
}

static bool ConvertNativeToJSVoid(QScriptEngine *engine,
                                  const Variant &val, QScriptValue *qval) {
  GGL_UNUSED(engine);
  GGL_UNUSED(val);
  *qval = QScriptValue();
  return true;
}

static bool ConvertNativeToJSBool(QScriptEngine *engine,
                                  const Variant &val, QScriptValue *qval) {
  *qval = QScriptValue(engine, VariantValue<bool>()(val));
  return true;
}

static bool ConvertNativeINT64ToJSNumber(QScriptEngine *engine,
                                         const Variant &val,
                                         QScriptValue *qval) {
  double value = VariantValue<int64_t>()(val);
  *qval = QScriptValue(engine, value);
  return true;
}

static bool ConvertNativeToJSNumber(QScriptEngine *engine,
                                    const Variant &val, QScriptValue *qval) {
  double value = VariantValue<double>()(val);
  *qval = QScriptValue(engine, value);
  return true;
}

static bool ConvertNativeToJSString(QScriptEngine *engine,
                                    const Variant &val, QScriptValue *qval) {
  const char *value = VariantValue<const char *>()(val);
  if (value)
    *qval = QScriptValue(engine, QString::fromUtf8(value));
  else
    *qval = engine->nullValue();
  return true;
}

static bool ConvertNativeUTF16ToJSString(QScriptEngine *engine,
                                         const Variant &val, QScriptValue *qval) {
  const UTF16Char *value = VariantValue<const UTF16Char *>()(val);
  if (!value) {
    *qval = engine->nullValue();
  } else {
    std::string str;
    ConvertStringUTF16ToUTF8(value, &str);
    *qval = QScriptValue(engine, QString::fromUtf8(str.c_str()));
  }
  return true;
}

static bool ConvertNativeArrayToJS(QScriptEngine *engine,
                                   ScriptableArray *array, QScriptValue *qval) {
  ScriptableHolder<ScriptableArray> array_holder(array);
  size_t length = array->GetCount();
  *qval = engine->newArray(length);
  if (!qval->isValid()) return false;

  for (size_t i = 0; i < length; i++) {
    QScriptValue item;
    if (ConvertNativeToJS(engine, array->GetItem(i), &item)) {
      qval->setProperty(i, item);
    }
  }
  return true;
}
static bool ConvertNativeToJSObject(QScriptEngine *engine,
                                    const Variant &val, QScriptValue *qval) {
  ScriptableInterface *scriptable = VariantValue<ScriptableInterface *>()(val);
  if (!scriptable) {
    DLOG("ConvertNativeToJSObject: scriptable is null");
    *qval = engine->nullValue();
    return true;
  }
  JSScriptContext *ctx = GetEngineContext(engine);
  if (scriptable->IsInstanceOf(ScriptableArray::CLASS_ID)) {
    return ConvertNativeArrayToJS(engine,
                                  down_cast<ScriptableArray*>(scriptable),
                                  qval);
  }
  *qval = ctx->GetScriptValueOfNativeObject(scriptable);
  return true;
}

static bool ConvertNativeToJSDate(QScriptEngine *engine,
                                  const Variant &val, QScriptValue *qval) {
  Date date = VariantValue<Date>()(val);
  *qval = engine->newDate(date.value);
  return true;
}

static bool ConvertNativeToJSFunction(QScriptEngine *engine,
                                      const Variant &val, QScriptValue *qval) {
  GGL_UNUSED(engine);
  GGL_UNUSED(val);
  GGL_UNUSED(qval);
  // To be compatible with the Windows version, we don't support returning
  // native Slots to JavaScript.
  DLOG("ConvertNativeToJSFunction");
  ASSERT(0);
  return true;
}

static bool ConvertJSONToJS(QScriptEngine *engine,
                            const Variant &val, QScriptValue *qval) {
  JSONString json_str = VariantValue<JSONString>()(val);
  return JSONDecode(engine, json_str.value.c_str(), qval);
}

bool ConvertNativeToJS(QScriptEngine *engine,
                       const Variant &val, QScriptValue *qval) {
  switch (val.type()) {
    case Variant::TYPE_VOID:
      return ConvertNativeToJSVoid(engine, val, qval);
    case Variant::TYPE_BOOL:
      return ConvertNativeToJSBool(engine, val, qval);
    case Variant::TYPE_INT64:
      return ConvertNativeINT64ToJSNumber(engine, val, qval);
    case Variant::TYPE_DOUBLE:
      return ConvertNativeToJSNumber(engine, val, qval);
    case Variant::TYPE_STRING:
      return ConvertNativeToJSString(engine, val, qval);
    case Variant::TYPE_JSON:
      return ConvertJSONToJS(engine, val, qval);
    case Variant::TYPE_UTF16STRING:
      return ConvertNativeUTF16ToJSString(engine, val, qval);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertNativeToJSObject(engine, val, qval);
    case Variant::TYPE_SLOT:
      return ConvertNativeToJSFunction(engine, val, qval);
    case Variant::TYPE_DATE:
      return ConvertNativeToJSDate(engine, val, qval);
    case Variant::TYPE_VARIANT:
      // Normally there is no real value of this type, so convert it to void.
      return ConvertNativeToJSVoid(engine, val, qval);
    default:
      return false;
  }
}

} // namespace qt
} // namespace ggadget
