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

#include "converter.h"

#include <jsobj.h>
#include <jscntxt.h>
#include <jsfun.h>
#include <jsnum.h>
#include <cmath>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_holder.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/js/jscript_massager.h>
#include "js_function_slot.h"
#include "js_native_wrapper.h"
#include "js_script_context.h"
#include "json.h"
#include "native_js_wrapper.h"

using namespace ggadget::js;

namespace ggadget {
namespace smjs {

static JSBool ConvertJSToNativeVoid(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  GGL_UNUSED(cx);
  GGL_UNUSED(js_val);
  *native_val = Variant();
  return JS_TRUE;
}

static JSBool ConvertJSToNativeBool(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  if (JSVAL_IS_STRING(js_val)) {
    JSString *js_string = JSVAL_TO_STRING(js_val);
    char *bytes = JS_GetStringBytes(js_string);
    if (!bytes)
      return JS_FALSE;
    // Convert "" or "false" to bool value false.
    *native_val = Variant(*bytes && strcasecmp(bytes, "false") != 0);
    return JS_TRUE;
  }

  JSBool value;
  if (!JS_ValueToBoolean(cx, js_val, &value))
    return JS_FALSE;

  *native_val = Variant(static_cast<bool>(value));
  return JS_TRUE;
}

static JSBool ConvertJSToNativeInt(JSContext *cx, jsval js_val,
                                   Variant *native_val) {
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(0);
    return JS_TRUE;
  }

  JSBool result = JS_FALSE;
  if (JSVAL_IS_INT(js_val)) {
    int32 int_val;
    result = JS_ValueToECMAInt32(cx, js_val, &int_val);
    if (result)
      *native_val = Variant(int_val);
  } else {
    jsdouble double_val = 0;
    result = JS_ValueToNumber(cx, js_val, &double_val);
    if (result) {
      // If double_val is NaN, it may because js_val is NaN, or js_val is a
      // string containing non-numeric chars. Both case are invalid for int.
      if (!JSDOUBLE_IS_NaN(double_val))
        *native_val = Variant(static_cast<int64_t>(round(double_val)));
      else
        result = JS_FALSE;
    }
  }
  return result;
}

static JSBool ConvertJSToNativeDouble(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(0.0);
    return JS_TRUE;
  }

  jsdouble double_val = 0;
  JSBool result = JS_ValueToNumber(cx, js_val, &double_val);
  if (result) {
    if (JSVAL_IS_DOUBLE(js_val) || !JSDOUBLE_IS_NaN(double_val))
      // If double_val is NaN, it may because js_val is NaN, or js_val is a
      // string containing non-numeric chars. The former case is acceptable.
      *native_val = Variant(static_cast<double>(double_val));
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertJSToNativeString(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  if (JSVAL_IS_NULL(js_val)) {
    *native_val = Variant(static_cast<const char *>(NULL));
    return JS_TRUE;
  }
  if (JSVAL_IS_VOID(js_val)) {
    // Default value of a string is "";
    *native_val = Variant("");
    return JS_TRUE;
  }
  if (JSVAL_IS_OBJECT(js_val)) {
    // Here allows asssigning ScriptableBinaryData to a native string, because
    // Windows version also allows it.
    ScriptableInterface *scriptable;
    if (NativeJSWrapper::Unwrap(cx, JSVAL_TO_OBJECT(js_val), &scriptable) &&
        scriptable->IsInstanceOf(ScriptableBinaryData::CLASS_ID)) {
      ScriptableBinaryData *data =
          down_cast<ScriptableBinaryData *>(scriptable);
      *native_val = Variant(data->data());
      DLOG("Convert binary data to string: length=%zu", data->data().size());
      return JS_TRUE;
    }
  }

  // Protects the result from JS_ValueToString from GC.
  AutoLocalRootScope local_root_scope(cx);
  if (!local_root_scope.good())
    return JS_FALSE;

  JSString *js_string = JS_ValueToString(cx, js_val);
  if (js_string) {
    jschar *chars = JS_GetStringChars(js_string);
    if (chars) {
      std::string utf8_string;
      // Don't cast chars to UTF16Char *, to let the compiler check if they
      // are compatible.
      ConvertStringUTF16ToUTF8(chars, JS_GetStringLength(js_string),
                               &utf8_string);
      *native_val = Variant(utf8_string);
      return JS_TRUE;
    }
  }
  return JS_FALSE;
}

static JSBool ConvertJSToNativeUTF16String(JSContext *cx, jsval js_val,
                                           Variant *native_val) {
  static const UTF16Char kEmptyUTF16String[] = { 0 };
  if (JSVAL_IS_NULL(js_val)) {
    *native_val = Variant(static_cast<const UTF16Char *>(NULL));
    return JS_TRUE;
  }
  if (JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(kEmptyUTF16String);
    return JS_TRUE;
  }

  // Protects the result from JS_ValueToString from GC.
  AutoLocalRootScope local_root_scope(cx);
  if (!local_root_scope.good())
    return JS_FALSE;

  JSString *js_string = JS_ValueToString(cx, js_val);
  if (js_string) {
    jschar *chars = JS_GetStringChars(js_string);
    if (chars) {
      // Don't cast chars to UTF16Char *, to let the compiler check if they
      // are compatible.
      *native_val = Variant(UTF16String(chars, JS_GetStringLength(js_string)));
      return JS_TRUE;
    }
  }
  return JS_FALSE;
}

static JSBool ConvertJSToScriptable(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  JSBool result = JS_TRUE;
  ScriptableInterface *scriptable;
  if (JSVAL_IS_VOID(js_val) || JSVAL_IS_NULL(js_val) ||
      (JSVAL_IS_INT(js_val) && JSVAL_TO_INT(js_val) == 0)) {
    scriptable = NULL;
  } else if (JSVAL_IS_OBJECT(js_val)) {
    JSObject *object = JSVAL_TO_OBJECT(js_val);
    // This object may be a JS wrapped native object.
    // If it is not, NativeJSWrapper::Unwrap simply fails.
    if (!NativeJSWrapper::Unwrap(cx, object, &scriptable)) {
      // NativeJSWrapper::Unwrap failed, this object is an origin JS object.
      scriptable = JSScriptContext::WrapJSToNative(cx, object);
    }
  } else {
    result = JS_FALSE;
  }

  if (result)
    *native_val = Variant(scriptable);
  return result;
}

static JSBool ConvertJSToSlot(JSContext *cx, NativeJSWrapper *owner,
                              const Variant &prototype,
                              jsval js_val, Variant *native_val) {
  JSBool result = JS_TRUE;
  JSObject *function_object;
  if (JSVAL_IS_VOID(js_val) || JSVAL_IS_NULL(js_val) ||
      (JSVAL_IS_INT(js_val) && JSVAL_TO_INT(js_val) == 0)) {
    function_object = NULL;
  } else if (JSVAL_IS_STRING(js_val)) {
    // Protects the result of CompileFunction from GC.
    AutoLocalRootScope local_root_scope(cx);
    if (!local_root_scope.good())
      return JS_FALSE;

    JSString *script_source = JSVAL_TO_STRING(js_val);
    jschar *script_chars = JS_GetStringChars(script_source);
    if (!script_chars)
      return JS_FALSE;

    std::string filename;
    int lineno;
    JSScriptContext::GetCurrentFileAndLine(cx, &filename, &lineno);
    JSFunction *function = CompileFunction(cx,
        UTF16ToUTF8Converter(script_chars,
                             JS_GetStringLength(script_source)).get(),
        filename.c_str(), lineno);

    if (!function)
      result = JS_FALSE;
    function_object = JS_GetFunctionObject(function);
  } else {
    // If js_val is a function, JS_ValueToFunction() will succeed,
    // Otherwise, JS_ValueToFunction() will raise an error.
    if (!JS_ValueToFunction(cx, js_val))
      result = JS_FALSE;
    function_object = JSVAL_TO_OBJECT(js_val);
  }

  if (result) {
    JSFunctionSlot *slot = NULL;
    if (function_object) {
      slot = new JSFunctionSlot(VariantValue<Slot *>()(prototype),
                                cx, owner, function_object);
    }
    *native_val = Variant(slot);
  }
  return result;
}

static JSBool ConvertJSToNativeDate(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  if (JSVAL_IS_VOID(js_val) || JSVAL_IS_NULL(js_val)) {
    // Special rule to keep compatibile with Windows version.
    *native_val = Variant(Date(0));
    return JS_TRUE;
  }

  if (JSVAL_IS_OBJECT(js_val)) {
    JSObject *obj = JSVAL_TO_OBJECT(js_val);
    ASSERT(obj);
    JSClass *cls = JS_GET_CLASS(cx, obj);
    if (!cls || strcmp("Date", cls->name) != 0)
      return JS_FALSE;

    if (!JS_CallFunctionName(cx, obj, "getTime", 0, NULL, &js_val))
      return JS_FALSE;
    // Now js_val is the result of Date.getTime().
  }

  Variant int_val(0);
  // Omit the return value. TODO: Make clear the exact coversion rule.
  ConvertJSToNativeInt(cx, js_val, &int_val);
  *native_val = Variant(Date(VariantValue<uint64_t>()(int_val)));
  return JS_TRUE;
}

static JSBool ConvertJSToJSON(JSContext *cx, jsval js_val,
                              Variant *native_val) {
  std::string json;
  JSONEncode(cx, js_val, &json);
  *native_val = Variant(JSONString(json));
  return JS_TRUE;
}

JSBool ConvertJSToNativeVariant(JSContext *cx, jsval js_val,
                                Variant *native_val) {
  if (JSVAL_IS_VOID(js_val) || JSVAL_IS_NULL(js_val))
    return ConvertJSToNativeVoid(cx, js_val, native_val);
  if (JSVAL_IS_BOOLEAN(js_val))
    return ConvertJSToNativeBool(cx, js_val, native_val);
  if (JSVAL_IS_INT(js_val))
    return ConvertJSToNativeInt(cx, js_val, native_val);
  if (JSVAL_IS_DOUBLE(js_val))
    return ConvertJSToNativeDouble(cx, js_val, native_val);
  if (JSVAL_IS_STRING(js_val))
    return ConvertJSToNativeString(cx, js_val, native_val);
  if (JSVAL_IS_OBJECT(js_val)) {
    // Don't try to convert the object to native Date, because JavaScript Date
    // is mutable, and sometimes the script may want to read it back and
    // change it. We only convert to native Date if the native side explicitly
    // requires a Date.
    // if (ConvertJSToNativeDate(cx, js_val, native_val))
    //   return JS_TRUE;
    // JS function is also wrapped into JSNativeWrapper instead of being
    // converted to a JSFunctionSlot, to ease memory management.
    return ConvertJSToScriptable(cx, js_val, native_val);
  }
  return JS_FALSE;
}

JSBool ConvertJSToNative(JSContext *cx, NativeJSWrapper *owner,
                         const Variant &prototype,
                         jsval js_val, Variant *native_val) {
  switch (prototype.type()) {
    case Variant::TYPE_VOID:
      return ConvertJSToNativeVoid(cx, js_val, native_val);
    case Variant::TYPE_BOOL:
      return ConvertJSToNativeBool(cx, js_val, native_val);
    case Variant::TYPE_INT64:
      return ConvertJSToNativeInt(cx, js_val, native_val);
    case Variant::TYPE_DOUBLE:
      return ConvertJSToNativeDouble(cx, js_val, native_val);
    case Variant::TYPE_STRING:
      return ConvertJSToNativeString(cx, js_val, native_val);
    case Variant::TYPE_JSON:
      return ConvertJSToJSON(cx, js_val, native_val);
    case Variant::TYPE_UTF16STRING:
      return ConvertJSToNativeUTF16String(cx, js_val, native_val);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertJSToScriptable(cx, js_val, native_val);
    case Variant::TYPE_SLOT:
      return ConvertJSToSlot(cx, owner, prototype, js_val, native_val);
    case Variant::TYPE_DATE:
      return ConvertJSToNativeDate(cx, js_val, native_val);
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
      return JS_FALSE;
    case Variant::TYPE_VARIANT:
      return ConvertJSToNativeVariant(cx, js_val, native_val);
    default:
      return JS_FALSE;
  }
}

void FreeNativeValue(const Variant &native_val) {
  // Delete the JSFunctionSlot object that was created by ConvertJSToNative().
  if (native_val.type() == Variant::TYPE_SLOT)
    delete VariantValue<Slot *>()(native_val);
}

std::string PrintJSValue(JSContext *cx, jsval js_val) {
  switch (JS_TypeOfValue(cx, js_val)) {
    case JSTYPE_STRING: {
      Variant v;
      ConvertJSToNativeString(cx, js_val, &v);
      return VariantValue<std::string>()(v);
    }
    case JSTYPE_OBJECT: {
      std::string json;
      JSONEncode(cx, js_val, &json);
      return json;
    }
    default: {
      JSString *str = JS_ValueToString(cx, js_val);
      if (str) {
        jschar *utf16 = JS_GetStringChars(str);
        size_t length = JS_GetStringLength(str);
        std::string utf8;
        ConvertStringUTF16ToUTF8(utf16, length, &utf8);
        return utf8;
      }
      return "##ERROR##";
    }
  }
}

JSBool ConvertJSArgsToNative(JSContext *cx, NativeJSWrapper *owner,
                             const char *name, Slot *slot,
                             uintN argc, jsval *argv,
                             Variant **params, uintN *expected_argc) {
  *params = NULL;
  const Variant::Type *arg_types = NULL;
  *expected_argc = argc;
  const Variant *default_args = NULL;
  if (slot->HasMetadata()) {
    arg_types = slot->GetArgTypes();
    *expected_argc = static_cast<uintN>(slot->GetArgCount());
    if (*expected_argc == INT_MAX) {
      // Simply converts each arguments to native.
      *params = new Variant[argc];
      *expected_argc = argc;
      uintN arg_type_idx = 0;
      for (uintN i = 0; i < argc; i++) {
        JSBool result = false;
        if (arg_types && arg_types[arg_type_idx] != Variant::TYPE_VOID) {
          result = ConvertJSToNative(cx, owner,
                                     Variant(arg_types[arg_type_idx]),
                                     argv[i], &(*params)[i]);
          ++arg_type_idx;
        } else {
          result = ConvertJSToNativeVariant(cx, argv[i], &(*params)[i]);
        }
        if (!result) {
          for (uintN j = 0; j < i; j++)
            FreeNativeValue((*params)[j]);
          delete [] *params;
          *params = NULL;
          RaiseException(cx,
                         "Failed to convert argument %d(%s) of function(%s) to"
                         " native", i, PrintJSValue(cx, argv[i]).c_str(), name);
          return JS_FALSE;
        }
      }
      return JS_TRUE;
    }
    default_args = slot->GetDefaultArgs();
    if (argc != *expected_argc) {
      uintN min_argc = *expected_argc;
      if (min_argc > 0 && default_args && argc < *expected_argc) {
        for (int i = static_cast<int>(min_argc) - 1; i >= 0; i--) {
          if (default_args[i].type() != Variant::TYPE_VOID)
            min_argc--;
          else
            break;
        }
      }

      if (argc > *expected_argc || argc < min_argc) {
        // Argc mismatch.
        RaiseException(cx, "Wrong number of arguments for function(%s): %u "
                       "(expected: %u, at least: %u)",
                       name, argc, *expected_argc, min_argc);
        return JS_FALSE;
      }
    }
  }

  if (*expected_argc > 0) {
    *params = new Variant[*expected_argc];
    // Fill up trailing default argument values.
    for (uintN i = argc; i < *expected_argc; i++) {
      ASSERT(default_args);  // Otherwise already returned JS_FALSE.
      (*params)[i] = default_args[i];
    }

    for (uintN i = 0; i < argc; i++) {
      if (default_args && default_args[i].type() != Variant::TYPE_VOID &&
          JSVAL_IS_VOID(argv[i])) {
        // Use the default value.
        (*params)[i] = default_args[i];
      } else {
        JSBool result;
        if (arg_types) {
          result = ConvertJSToNative(cx, owner,
                                     Variant(arg_types[i]), argv[i],
                                     &(*params)[i]);
        } else {
          result = ConvertJSToNativeVariant(cx, argv[i], &(*params)[i]);
        }
        if (!result) {
          for (uintN j = 0; j < i; j++)
            FreeNativeValue((*params)[j]);
          delete [] *params;
          *params = NULL;
          RaiseException(cx,
                         "Failed to convert argument %d(%s) of function(%s) to"
                         " native", i, PrintJSValue(cx, argv[i]).c_str(), name);
          return JS_FALSE;
        }
      }
    }
  }
  return JS_TRUE;
}

static JSBool ConvertNativeToJSVoid(JSContext *cx,
                                    const Variant &native_val,
                                    jsval *js_val) {
  GGL_UNUSED(cx);
  GGL_UNUSED(native_val);
  *js_val = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool ConvertNativeToJSBool(JSContext *cx,
                                    const Variant &native_val,
                                    jsval *js_val) {
  GGL_UNUSED(cx);
  *js_val = BOOLEAN_TO_JSVAL(VariantValue<bool>()(native_val));
  return JS_TRUE;
}

static JSBool ConvertNativeToJSInt(JSContext *cx,
                                   const Variant &native_val,
                                   jsval *js_val) {
  int64_t value = VariantValue<int64_t>()(native_val);
  if (value >= JSVAL_INT_MIN && value <= JSVAL_INT_MAX) {
    *js_val = INT_TO_JSVAL(static_cast<int32>(value));
    return JS_TRUE;
  } else {
    jsdouble *pdouble = JS_NewDouble(cx, static_cast<jsdouble>(value));
    if (pdouble) {
      *js_val = DOUBLE_TO_JSVAL(pdouble);
      return JS_TRUE;
    } else {
      return JS_FALSE;
    }
  }
}

static JSBool ConvertNativeToJSDouble(JSContext *cx,
                                      const Variant &native_val,
                                      jsval *js_val) {
  jsdouble *pdouble = JS_NewDouble(cx, VariantValue<double>()(native_val));
  if (pdouble) {
    *js_val = DOUBLE_TO_JSVAL(pdouble);
    return JS_TRUE;
  } else {
    return JS_FALSE;
  }
}

static JSBool ConvertNativeToJSString(JSContext *cx,
                                      const Variant &native_val,
                                      jsval *js_val) {
  JSBool result = JS_TRUE;
  if (!VariantValue<const char *>()(native_val)) {
    *js_val = JSVAL_NULL;
  } else {
    std::string source = VariantValue<std::string>()(native_val);
    size_t source_size = source.size();

    // Though JSAPI doesn't require the string to be NULL-terminated, we found
    // that all JSAPI created strings are NULL-terminated, so we do the same.
    jschar *utf16_buffer =
        (jschar *)JS_malloc(cx, (source_size + 1) * sizeof(jschar));
    if (!utf16_buffer)
      return JS_FALSE;

    size_t dest_size = 0;
    // Don't cast utf16_string.c_str() to jschar *, to let the compiler check
    // if they are compatible.
    if (ConvertStringUTF8ToUTF16Buffer(source, utf16_buffer, source_size,
                                       &dest_size) != source_size) {
      DLOG("Convert non-UTF8 string data to fake UTF16 length=%zu",
           source_size);
      dest_size = (source_size + 1) / 2;
      // Failed to convert to utf16, the source may contain arbitrary binary
      // data. This is mainly for compatibility of XMLHttpRequest.responseBody
      // to Microsoft, that combines each two bytes into one 16 bit word.
      for (size_t i = 0; i < source_size; i += 2) {
        utf16_buffer[i / 2] = static_cast<jschar>(
            static_cast<unsigned char>(source[i]) |
            (static_cast<unsigned char>(source[i + 1]) << 8));
      }
      if (source_size % 2)
        utf16_buffer[dest_size - 1] = source[source_size - 1];
    }
    utf16_buffer[dest_size] = 0;

    // Shrink the buffer size if the required dest size is far smaller than
    // allocated.
    if (dest_size + 16 < source_size) {
      utf16_buffer = (jschar *)JS_realloc(cx, utf16_buffer,
                                          (dest_size + 1) * sizeof(jschar));
    }

    // Javascript adopts utf16_buffer.
    JSString *js_string = JS_NewUCString(cx, utf16_buffer, dest_size);
    if (js_string)
      *js_val = STRING_TO_JSVAL(js_string);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertNativeUTF16ToJSString(JSContext *cx,
                                           const Variant &native_val,
                                           jsval *js_val) {
  JSBool result = JS_TRUE;
  const UTF16Char *char_ptr = VariantValue<const UTF16Char *>()(native_val);
  if (!char_ptr) {
    *js_val = JSVAL_NULL;
  } else {
    // Don't cast utf16_string.c_str() to jschar *, to let the compiler check
    // if they are compatible.
    JSString *js_string = JS_NewUCStringCopyZ(cx, char_ptr);
    if (js_string)
      *js_val = STRING_TO_JSVAL(js_string);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ReturnSelf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                         jsval *rval) {
  GGL_UNUSED(cx);
  GGL_UNUSED(argc);
  GGL_UNUSED(argv);
  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}

static JSBool GetCollectionItem(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *rval) {
  if (argc >= 1 && JSVAL_IS_INT(argv[0]))
    return JS_GetElement(cx, obj, JSVAL_TO_INT(argv[0]), rval);
  return JSVAL_VOID;
}

static JSBool ConvertNativeArrayToJS(JSContext *cx, ScriptableArray *array,
                                     jsval *js_val) {
  // To make sure that the array will be destroyed correctly.
  ScriptableHolder<ScriptableArray> array_holder(array);
  size_t length = array->GetCount();
  if (length > JSVAL_INT_MAX)
    return JS_FALSE;

  JSObject *js_array = JS_NewArrayObject(cx, 0, NULL);
  if (!js_array)
    return JS_FALSE;

  for (size_t i = 0; i < length; i++) {
    jsval item;
    if (ConvertNativeToJS(cx, array->GetItem(i), &item))
      JS_SetElement(cx, js_array, static_cast<jsint>(i), &item);
  }

  // We return a JavaScript array where a VBArray is expected in original
  // JScript program. JScript program calls toArray() to convert a VBArray to
  // a JavaScript array. We just let toArray() return the array itself.
  JS_DefineFunction(cx, js_array, "toArray", &ReturnSelf, 0, 0);

  // We also return a JavaScript array where a JScript Collection is expected.
  // We should add count and item() properties for it.
  JS_DefineProperty(cx, js_array, "count", INT_TO_JSVAL(length), NULL, NULL,
                    JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineFunction(cx, js_array, "item", &GetCollectionItem, 1, 0);

  *js_val = OBJECT_TO_JSVAL(js_array);
  return JS_TRUE;
}

static JSBool ConvertNativeToJSObject(JSContext *cx,
                                      const Variant &native_val,
                                      jsval *js_val) {
  JSBool result = JS_TRUE;
  ScriptableInterface *scriptable =
      VariantValue<ScriptableInterface *>()(native_val);
  if (!scriptable) {
    *js_val = JSVAL_NULL;
  } else if (scriptable->IsInstanceOf(ScriptableArray::CLASS_ID)) {
    result = ConvertNativeArrayToJS(cx,
                                    down_cast<ScriptableArray *>(scriptable),
                                    js_val);
  } else if (scriptable->IsInstanceOf(JSNativeWrapper::CLASS_ID)) {
    // TODO: Create new wrapper or JSFunction if crossing JS runtimes.
    *js_val = OBJECT_TO_JSVAL(
        down_cast<JSNativeWrapper *>(scriptable)->js_object());
  } else {
    NativeJSWrapper *wrapper = JSScriptContext::WrapNativeObjectToJS(
        cx, scriptable);
    if (wrapper) {
      JSObject *js_object = wrapper->js_object();
      if (js_object)
        *js_val = OBJECT_TO_JSVAL(js_object);
      else
        result = JS_FALSE;
    } else {
      result = JS_FALSE;
    }
  }
  return result;
}

static JSBool ConvertNativeToJSDate(JSContext *cx,
                                    const Variant &native_val,
                                    jsval *js_val) {
  std::string new_date_script =
      StringPrintf("new Date(%ju)", VariantValue<Date>()(native_val).value);
  return JS_EvaluateScript(cx, JS_GetGlobalObject(cx), new_date_script.c_str(),
                           static_cast<uintN>(new_date_script.length()),
                           "", 1, js_val);
}

static JSBool ConvertNativeToJSFunction(JSContext *cx,
                                        const Variant &native_val,
                                        jsval *js_val) {
  GGL_UNUSED(cx);
  GGL_UNUSED(native_val);
  GGL_UNUSED(js_val);
  DLOG("Reading native function in JavaScript");
  // Just leave the value that SpiderMonkey recorded in SetProperty.
  return JS_TRUE;
}

static JSBool ConvertJSONToJS(JSContext *cx,
                              const Variant &native_val,
                              jsval *js_val) {
  JSONString json_str = VariantValue<JSONString>()(native_val);
  return JSONDecode(cx, json_str.value.c_str(), js_val);
}

JSBool ConvertNativeToJS(JSContext *cx,
                         const Variant &native_val,
                         jsval *js_val) {
  switch (native_val.type()) {
    case Variant::TYPE_VOID:
      return ConvertNativeToJSVoid(cx, native_val, js_val);
    case Variant::TYPE_BOOL:
      return ConvertNativeToJSBool(cx, native_val, js_val);
    case Variant::TYPE_INT64:
      return ConvertNativeToJSInt(cx, native_val, js_val);
    case Variant::TYPE_DOUBLE:
      return ConvertNativeToJSDouble(cx, native_val, js_val);
    case Variant::TYPE_STRING:
      return ConvertNativeToJSString(cx, native_val, js_val);
    case Variant::TYPE_JSON:
      return ConvertJSONToJS(cx, native_val, js_val);
    case Variant::TYPE_UTF16STRING:
      return ConvertNativeUTF16ToJSString(cx, native_val, js_val);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertNativeToJSObject(cx, native_val, js_val);
    case Variant::TYPE_SLOT:
      return ConvertNativeToJSFunction(cx, native_val, js_val);
    case Variant::TYPE_DATE:
      return ConvertNativeToJSDate(cx, native_val, js_val);
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
      return JS_FALSE;
    case Variant::TYPE_VARIANT:
      // Normally there is no real value of this type, so convert it to void.
      return ConvertNativeToJSVoid(cx, native_val, js_val);
    default:
      return JS_FALSE;
  }
}

JSFunction *CompileFunction(JSContext *cx, const char *script,
                            const char *filename, int lineno) {
  if (!script)
    return NULL;

  std::string massaged_script = MassageJScript(script, false, filename, lineno);
  UTF16String utf16_string;
  if (ConvertStringUTF8ToUTF16(massaged_script, &utf16_string) ==
      massaged_script.size()) {
    return JS_CompileUCFunction(cx, NULL, NULL, 0, NULL,
                                utf16_string.c_str(), utf16_string.size(),
                                filename, lineno);
  } else {
    JS_ReportWarning(cx, "Script %s contains invalid UTF-8 sequences "
                     "and will be treated as ISO8859-1", filename);
    return JS_CompileFunction(cx, NULL, NULL, 0, NULL,
                              massaged_script.c_str(), massaged_script.size(),
                              filename, lineno);
  }
}

JSBool EvaluateScript(JSContext *cx, JSObject *object, const char *script,
                      const char *filename, int lineno, jsval *rval) {
  if (!script)
    return JS_FALSE;

  std::string massaged_script = MassageJScript(script, false, filename, lineno);
  UTF16String utf16_string;
  if (ConvertStringUTF8ToUTF16(massaged_script, &utf16_string) ==
      massaged_script.size()) {
    return JS_EvaluateUCScript(cx, object, utf16_string.c_str(),
                               static_cast<uintN>(utf16_string.size()),
                               filename, lineno, rval);
  } else {
    JS_ReportWarning(cx, "Script %s contains invalid UTF-8 sequences "
                     "and will be treated as ISO8859-1", filename);
    return JS_EvaluateScript(cx, object, massaged_script.c_str(),
                             static_cast<uintN>(massaged_script.size()),
                             filename, lineno, rval);
  }
}

JSBool CheckException(JSContext *cx, ScriptableInterface *scriptable) {
  if (!cx || !scriptable)
    return JS_FALSE;

  ScriptableInterface *exception = scriptable->GetPendingException(true);
  if (!exception)
    return JS_TRUE;

  jsval js_exception;
  if (!ConvertNativeToJSObject(cx, Variant(exception), &js_exception)) {
    JS_ReportError(cx, "Failed to convert native exception to jsval");
    return JS_FALSE;
  }

  JS_SetPendingException(cx, js_exception);
  return JS_FALSE;
}

// This dummy JSErrorCallback converts a error message into an exception.
// It's better than JS_SetPendingException() because it will generate a
// full error report with the current file name and line number.
static const JSErrorFormatString *ErrorCallback(void *, const char *,
                                                const uintN) {
  static const JSErrorFormatString kErrorFormatString = {
    "{0}", 1,
    0, // JSEXN_ERR, not defined in old version of js.
  };
  return &kErrorFormatString;
}

JSBool RaiseException(JSContext *cx, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string message = StringVPrintf(format, ap);
  va_end(ap);
  JS_ReportErrorNumber(cx, ErrorCallback, NULL, 1, message.c_str());
  return JS_FALSE;
}

} // namespace smjs
} // namespace ggadget
