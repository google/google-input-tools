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
#include <cmath>
#include <cstdlib>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_holder.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/variant.h>
#include <ggadget/js/jscript_massager.h>
#include "js_script_context.h"
#include "json.h"

using namespace ggadget::js;

namespace ggadget {
namespace webkit {

static bool ConvertJSToNativeVoid(JSScriptContext *ctx, JSValueRef js_val,
                                  Variant *native_val) {
  *native_val = Variant();
  return true;
}

static bool ConvertJSToNativeBool(JSScriptContext *ctx, JSValueRef js_val,
                                  Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsString(js_ctx, js_val)) {
    JSValueRef exception = NULL;
    JSStringRef str = JSValueToStringCopy(js_ctx, js_val, &exception);
    if (!str) {
      ctx->CheckJSException(exception);
      return false;
    }
    *native_val = Variant(
        JSStringGetLength(str) && !JSStringIsEqualToUTF8CString(str, "false"));
    JSStringRelease(str);
    return true;
  }

  *native_val = Variant(JSValueToBoolean(js_ctx, js_val));
  return true;
}

static bool ConvertJSToNativeInt(JSScriptContext *ctx, JSValueRef js_val,
                                 Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsUndefined(js_ctx, js_val) || JSValueIsNull(js_ctx, js_val)) {
    *native_val = Variant(0);
    return true;
  }

  // Only converts finite number to int.
  if (ctx->IsFinite(js_val)) {
    JSValueRef exception = NULL;
    double result = JSValueToNumber(js_ctx, js_val, &exception);
    *native_val = Variant(static_cast<int64_t>(round(result)));
    return ctx->CheckJSException(exception);
  }
  return false;
}

static bool ConvertJSToNativeDouble(JSScriptContext *ctx, JSValueRef js_val,
                                    Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsUndefined(js_ctx, js_val) || JSValueIsNull(js_ctx, js_val)) {
    *native_val = Variant(0.0);
    return true;
  }

  // If js_val is a number, then any value is allowed.
  // Otherwise, anything that can only be converted to NaN is not allowed.
  if (JSValueIsNumber(js_ctx, js_val) || !ctx->IsNaN(js_val)) {
    JSValueRef exception = NULL;
    *native_val = Variant(JSValueToNumber(js_ctx, js_val, &exception));
    return ctx->CheckJSException(exception);
  }
  return false;
}

static bool ConvertJSToNativeNumber(JSScriptContext *ctx, JSValueRef js_val,
                                    Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsUndefined(js_ctx, js_val) || JSValueIsNull(js_ctx, js_val)) {
    *native_val = Variant(0);
    return true;
  }

  // If js_val is a number, then any value is allowed.
  // Otherwise, anything that can only be converted to NaN is not allowed.
  if (JSValueIsNumber(js_ctx, js_val) || !ctx->IsNaN(js_val)) {
    JSValueRef exception = NULL;
    double double_value = JSValueToNumber(js_ctx, js_val, &exception);
    // convert to int if possible.
    if (ceil(double_value) == floor(double_value) &&
        double_value <= LONG_MAX && double_value >= LONG_MIN) {
      *native_val = Variant(static_cast<int64_t>(double_value));
    } else {
      *native_val = Variant(double_value);
    }
    return ctx->CheckJSException(exception);
  }
  return false;
}

static bool ConvertJSToNativeString(JSScriptContext *ctx, JSValueRef js_val,
                                    Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsNull(js_ctx, js_val)) {
    *native_val = Variant(static_cast<const char *>(NULL));
    return true;
  }
  if (JSValueIsUndefined(js_ctx, js_val)) {
    // Default value of a string is "";
    *native_val = Variant("");
    return true;
  }

  JSValueRef exception = NULL;
  if (JSValueIsObject(js_ctx, js_val)) {
    // Here allows asssigning ScriptableBinaryData to a native string, because
    // Windows version also allows it.
    JSObjectRef js_obj = JSValueToObject(js_ctx, js_val, &exception);
    if (!js_obj) {
      ctx->CheckJSException(exception);
      return false;
    }
    ScriptableInterface *scriptable = ctx->UnwrapScriptable(js_obj);
    if (scriptable &&
        scriptable->IsInstanceOf(ScriptableBinaryData::CLASS_ID)) {
      ScriptableBinaryData *data =
          down_cast<ScriptableBinaryData *>(scriptable);
      *native_val = Variant(data->data());
      DLOG("Convert binary data to string: length=%zu", data->data().size());
      return true;
    }
  }
  JSStringRef js_str = JSValueToStringCopy(js_ctx, js_val, &exception);
  if (!js_str) {
    ctx->CheckJSException(exception);
    return false;
  }
  *native_val = Variant(ConvertJSStringToUTF8(js_str));
  JSStringRelease(js_str);
  return true;
}

static bool ConvertJSToNativeUTF16String(JSScriptContext *ctx,
                                         JSValueRef js_val,
                                         Variant *native_val) {
  static const UTF16Char kEmptyUTF16String[] = { 0 };
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsNull(js_ctx, js_val)) {
    *native_val = Variant(static_cast<const UTF16Char *>(NULL));
  }
  if (JSValueIsUndefined(js_ctx, js_val)) {
    *native_val = Variant(kEmptyUTF16String);
  }

  JSValueRef exception = NULL;
  JSStringRef js_str = JSValueToStringCopy(js_ctx, js_val, &exception);
  if (!js_str) {
    ctx->CheckJSException(exception);
    return false;
  }
  const JSChar *js_chars = JSStringGetCharactersPtr(js_str);
  if (js_chars)
    *native_val = Variant(UTF16String(js_chars, JSStringGetLength(js_str)));
  JSStringRelease(js_str);
  return js_chars != NULL;
}

static bool ConvertJSToScriptable(JSScriptContext *ctx, JSValueRef js_val,
                                  Variant *native_val) {
  bool result = true;
  ScriptableInterface *scriptable = NULL;
  JSValueRef exception = NULL;
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsUndefined(js_ctx, js_val) || JSValueIsNull(js_ctx, js_val) ||
      (JSValueIsNumber(js_ctx, js_val) &&
       JSValueToNumber(js_ctx, js_val, &exception) == 0)) {
    scriptable = NULL;
  } else if (JSValueIsObject(js_ctx, js_val)) {
    scriptable = ctx->WrapJSObject(JSValueToObject(js_ctx, js_val, &exception));
  } else {
    result = false;
  }

  if (result && ctx->CheckJSException(exception)) {
    *native_val = Variant(scriptable);
    return true;
  }
  return false;
}

static bool ConvertJSToSlot(JSScriptContext *ctx, JSObjectRef owner,
                            const Variant &prototype, JSValueRef js_val,
                            Variant *native_val) {
  bool result = true;
  JSObjectRef func_obj = NULL;
  JSValueRef exception = NULL;
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsNull(js_ctx, js_val) || JSValueIsUndefined(js_ctx, js_val) ||
      (JSValueIsNumber(js_ctx, js_val) &&
       JSValueToNumber(js_ctx, js_val, &exception) == 0)) {
    func_obj = NULL;
  } else if (JSValueIsString(js_ctx, js_val)) {
    JSStringRef js_body = JSValueToStringCopy(js_ctx, js_val, &exception);
    std::string body = ConvertJSStringToUTF8(js_body);
    JSStringRelease(js_body);
    std::string filename;
    int lineno;
    ctx->GetCurrentFileAndLine(&filename, &lineno);
    func_obj = CompileFunction(ctx, body.c_str(), filename.c_str(), lineno,
                               &exception);
  } else if (JSValueIsObject(js_ctx, js_val)) {
    func_obj = JSValueToObject(js_ctx, js_val, &exception);
    if (!func_obj || !JSObjectIsFunction(js_ctx, func_obj))
      result = false;
  }

  if (result && ctx->CheckJSException(exception)) {
    Slot *slot = NULL;
    if (func_obj) {
      slot = ctx->WrapJSObjectIntoSlot(VariantValue<Slot *>()(prototype),
                                       owner, func_obj);
    }
    *native_val = Variant(slot);
    return true;
  }
  return false;
}

static bool ConvertJSToNativeDate(JSScriptContext *ctx, JSValueRef js_val,
                                  Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (JSValueIsUndefined(js_ctx, js_val) || JSValueIsNull(js_ctx, js_val)) {
    // Special rule to keep compatibile with Windows version.
    *native_val = Variant(Date(0));
    return true;
  }

  if (JSValueIsObject(js_ctx, js_val)) {
    std::string time_string;
    if (DateGetTimeString(ctx, js_val, &time_string)) {
#if GGL_SIZEOF_LONG_INT >= 8
      *native_val = Variant(
          Date(static_cast<uint64_t>(strtol(time_string.c_str(), NULL, 10))));
#else
      *native_val = Variant(
          Date(static_cast<uint64_t>(strtoll(time_string.c_str(), NULL, 10))));
#endif
      return true;
    }
    return false;
  }

  Variant int_val(0);
  if (ConvertJSToNativeInt(ctx, js_val, &int_val)) {
    *native_val = Variant(Date(VariantValue<uint64_t>()(int_val)));
    return true;
  }
  return false;
}

static bool ConvertJSToJSON(JSScriptContext *ctx, JSValueRef js_val,
                            Variant *native_val) {
  std::string json;
  if (JSONEncode(ctx, js_val, &json)) {
    *native_val = Variant(JSONString(json));
    return true;
  }
  return false;
}

bool ConvertJSToNativeVariant(JSScriptContext *ctx, JSValueRef js_val,
                              Variant *native_val) {
  JSContextRef js_ctx = ctx->GetContext();
  switch(JSValueGetType(js_ctx, js_val)) {
    case kJSTypeUndefined:
    case kJSTypeNull:
      return ConvertJSToNativeVoid(ctx, js_val, native_val);
    case kJSTypeBoolean:
      return ConvertJSToNativeBool(ctx, js_val, native_val);
    case kJSTypeNumber:
      return ConvertJSToNativeNumber(ctx, js_val, native_val);
    case kJSTypeString:
      return ConvertJSToNativeString(ctx, js_val, native_val);
    case kJSTypeObject:
      // Don't try to convert the object to native Date, because JavaScript Date
      // is mutable, and sometimes the script may want to read it back and
      // change it. We only convert to native Date if the native side explicitly
      // requires a Date.
      // JS function is also wrapped into Scriptable instead of being
      // converted to a Slot, to ease memory management.
      return ConvertJSToScriptable(ctx, js_val, native_val);
    default:
      return false;
  }
}

bool ConvertJSToNative(JSScriptContext *ctx, JSObjectRef owner,
                       const Variant &prototype,
                       JSValueRef js_val, Variant *native_val) {
  switch (prototype.type()) {
    case Variant::TYPE_VOID:
      return ConvertJSToNativeVoid(ctx, js_val, native_val);
    case Variant::TYPE_BOOL:
      return ConvertJSToNativeBool(ctx, js_val, native_val);
    case Variant::TYPE_INT64:
      return ConvertJSToNativeInt(ctx, js_val, native_val);
    case Variant::TYPE_DOUBLE:
      return ConvertJSToNativeDouble(ctx, js_val, native_val);
    case Variant::TYPE_STRING:
      return ConvertJSToNativeString(ctx, js_val, native_val);
    case Variant::TYPE_JSON:
      return ConvertJSToJSON(ctx, js_val, native_val);
    case Variant::TYPE_UTF16STRING:
      return ConvertJSToNativeUTF16String(ctx, js_val, native_val);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertJSToScriptable(ctx, js_val, native_val);
    case Variant::TYPE_SLOT:
      return ConvertJSToSlot(ctx, owner, prototype, js_val, native_val);
    case Variant::TYPE_DATE:
      return ConvertJSToNativeDate(ctx, js_val, native_val);
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
      return false;
    case Variant::TYPE_VARIANT:
      return ConvertJSToNativeVariant(ctx, js_val, native_val);
    default:
      return false;
  }
}

void FreeNativeValue(const Variant &native_val) {
  // Delete the JSFunctionSlot object that was created by ConvertJSToNative().
  if (native_val.type() == Variant::TYPE_SLOT)
    delete VariantValue<Slot *>()(native_val);
}

std::string PrintJSValue(JSScriptContext *ctx, JSValueRef js_val) {
  JSContextRef js_ctx = ctx->GetContext();
  switch(JSValueGetType(js_ctx, js_val)) {
    case kJSTypeString: {
      Variant v;
      ConvertJSToNativeString(ctx, js_val, &v);
      return VariantValue<std::string>()(v);
    }
    case kJSTypeObject: {
      std::string json;
      JSONEncode(ctx, js_val, &json);
      return json;
    }
    default: {
      JSStringRef js_str = JSValueToStringCopy(js_ctx, js_val, NULL);
      if (js_str) {
        std::string utf8 = ConvertJSStringToUTF8(js_str);
        JSStringRelease(js_str);
        return utf8;
      }
      return "##ERROR##";
    }
  }
}

bool ConvertJSArgsToNative(JSScriptContext *ctx, JSObjectRef owner,
                           const char *name, Slot *slot,
                           size_t argc, const JSValueRef argv[],
                           Variant **params, size_t *expected_argc,
                           JSValueRef *exception) {
  *params = NULL;
  const Variant::Type *arg_types = NULL;
  *expected_argc = argc;
  const Variant *default_args = NULL;
  if (slot->HasMetadata()) {
    arg_types = slot->GetArgTypes();
    *expected_argc = static_cast<size_t>(slot->GetArgCount());
    if (*expected_argc == INT_MAX) {
      // Simply converts each arguments to native.
      *params = new Variant[argc];
      *expected_argc = argc;
      size_t arg_type_idx = 0;
      for (size_t i = 0; i < argc; ++i) {
        bool result = false;
        if (arg_types && arg_types[arg_type_idx] != Variant::TYPE_VOID) {
          result = ConvertJSToNative(ctx, owner,
                                     Variant(arg_types[arg_type_idx]),
                                     argv[i], &(*params)[i]);
          ++arg_type_idx;
        } else {
          result = ConvertJSToNativeVariant(ctx, argv[i], &(*params)[i]);
        }
        if (!result) {
          for (size_t j = 0; j < i; ++j)
            FreeNativeValue((*params)[j]);
          delete [] *params;
          *params = NULL;
          RaiseJSException(ctx, exception,
                           "Failed to convert argument %zu (%s) of function(%s)"
                           " to native.", i, PrintJSValue(ctx, argv[i]).c_str(),
                           name);
          return false;
        }
      }
      return true;
    }
    default_args = slot->GetDefaultArgs();
    if (argc != *expected_argc) {
      size_t min_argc = *expected_argc;
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
        RaiseJSException(ctx, exception,
                         "Wrong number of arguments for function(%s): %zu "
                         "(expected: %zu, at least: %zu)",
                         name, argc, *expected_argc, min_argc);
        return false;
      }
    }
  }

  if (*expected_argc > 0) {
    *params = new Variant[*expected_argc];
    // Fill up trailing default argument values.
    for (size_t i = argc; i < *expected_argc; ++i) {
      ASSERT(default_args);  // Otherwise already returned JS_FALSE.
      (*params)[i] = default_args[i];
    }

    JSContextRef js_ctx = ctx->GetContext();
    for (size_t i = 0; i < argc; ++i) {
      if (default_args && default_args[i].type() != Variant::TYPE_VOID &&
          JSValueIsUndefined(js_ctx, argv[i])) {
        // Use the default value.
        (*params)[i] = default_args[i];
      } else {
        bool result;
        if (arg_types) {
          result = ConvertJSToNative(ctx, owner,
                                     Variant(arg_types[i]), argv[i],
                                     &(*params)[i]);
        } else {
          result = ConvertJSToNativeVariant(ctx, argv[i], &(*params)[i]);
        }
        if (!result) {
          for (size_t j = 0; j < i; ++j)
            FreeNativeValue((*params)[j]);
          delete [] *params;
          *params = NULL;
          RaiseJSException(ctx, exception,
                           "Failed to convert argument %zu (%s) of function(%s)"
                           " to native.", i, PrintJSValue(ctx, argv[i]).c_str(),
                           name);
          return false;
        }
      }
    }
  }
  return true;
}

static bool ConvertNativeToJSVoid(JSScriptContext *ctx,
                                  const Variant &native_val,
                                  JSValueRef *js_val) {
  *js_val = JSValueMakeUndefined(ctx->GetContext());
  return true;
}

static bool ConvertNativeToJSBool(JSScriptContext *ctx,
                                  const Variant &native_val,
                                  JSValueRef *js_val) {
  *js_val = JSValueMakeBoolean(ctx->GetContext(),
                               VariantValue<bool>()(native_val));
  return true;
}

static bool ConvertNativeToJSInt(JSScriptContext *ctx,
                                 const Variant &native_val,
                                 JSValueRef *js_val) {
  int64_t value = VariantValue<int64_t>()(native_val);
  *js_val = JSValueMakeNumber(ctx->GetContext(), static_cast<double>(value));
  return true;
}

static bool ConvertNativeToJSDouble(JSScriptContext *ctx,
                                    const Variant &native_val,
                                    JSValueRef *js_val) {
  *js_val = JSValueMakeNumber(ctx->GetContext(),
                              VariantValue<double>()(native_val));
  return true;
}

static bool ConvertNativeToJSString(JSScriptContext *ctx,
                                    const Variant &native_val,
                                    JSValueRef *js_val) {
  JSContextRef js_ctx = ctx->GetContext();
  if (!VariantValue<const char *>()(native_val)) {
    *js_val = JSValueMakeNull(js_ctx);
  } else {
    std::string src = VariantValue<std::string>()(native_val);
    size_t src_len = src.length();
    UTF16String dest;
    if (ConvertStringUTF8ToUTF16(src, &dest) != src_len) {
      DLOG("Convert non-UTF8 string data to fake UTF16, length=%zu", src_len);
      size_t dest_len = (src_len + 1) / 2;
      dest.clear();
      dest.reserve(dest_len);
      const char *src_ptr = src.c_str();
      // Failed to convert to utf16, the source may contain arbitrary binary
      // data. This is mainly for compatibility of XMLHttpRequest.responseBody
      // to Microsoft, that combines each two bytes into one 16 bit word.
      for (size_t i = 0; i < src_len; i += 2) {
        dest += static_cast<UTF16Char>(
            static_cast<unsigned char>(src_ptr[i]) |
            (static_cast<unsigned char>(src_ptr[i + 1]) << 8));
      }
      ASSERT(dest.length() == dest_len);
    }

    JSStringRef js_str = JSStringCreateWithCharacters(
        static_cast<const JSChar *>(dest.c_str()), dest.length());
    *js_val = JSValueMakeString(js_ctx, js_str);
    JSStringRelease(js_str);
  }
  return true;
}

static bool ConvertNativeUTF16ToJSString(JSScriptContext *ctx,
                                         const Variant &native_val,
                                         JSValueRef *js_val) {
  const UTF16Char *ptr = VariantValue<const UTF16Char *>()(native_val);
  JSContextRef js_ctx = ctx->GetContext();
  if (!ptr) {
    *js_val = JSValueMakeNull(js_ctx);
  } else {
    size_t len = VariantValue<const UTF16String &>()(native_val).length();
    JSStringRef js_str =
        JSStringCreateWithCharacters(static_cast<const JSChar *>(ptr), len);
    *js_val = JSValueMakeString(js_ctx, js_str);
    JSStringRelease(js_str);
  }
  return true;
}

static JSValueRef ReturnSelf(JSContextRef ctx, JSObjectRef function,
                             JSObjectRef thisObject, size_t argc,
                             const JSValueRef argv[], JSValueRef *exception) {
  return thisObject;
}

static JSValueRef GetCollectionItem(JSContextRef ctx, JSObjectRef function,
                                    JSObjectRef thisObject, size_t argc,
                                    const JSValueRef argv[],
                                    JSValueRef *exception) {
  if (argc >= 1) {
    JSValueRef local_exception = NULL;
    double idx = JSValueToNumber(ctx, argv[0], &local_exception);
    if (!local_exception) {
      return JSObjectGetPropertyAtIndex(ctx, thisObject,
                                        static_cast<unsigned>(idx), exception);
    }
    *exception = local_exception;
  }
  return JSValueMakeUndefined(ctx);
}

static JSObjectRef CreateJSArray(JSContextRef ctx) {
  JSStringRef array_class_name = JSStringCreateWithUTF8CString("Array");
  JSObjectRef global_object = JSContextGetGlobalObject(ctx);
  JSValueRef array_class = JSObjectGetProperty(ctx, global_object,
                                               array_class_name, NULL);
  JSStringRelease(array_class_name);
  if (JSValueIsObject(ctx, array_class)) {
    JSObjectRef array_class_obj = JSValueToObject(ctx, array_class, NULL);
    if (JSObjectIsConstructor(ctx, array_class_obj)) {
      JSValueRef array = JSObjectCallAsFunction(ctx, array_class_obj, NULL,
                                                0, NULL, NULL);
      if (JSValueIsInstanceOfConstructor(ctx, array, array_class_obj, NULL)) {
        return JSValueToObject(ctx, array, NULL);
      }
    }
  }
  return NULL;
}

static JSObjectRef CustomizeJSArray(JSContextRef ctx, JSObjectRef array) {
  if (array) {
    JSStringRef toArray_str = JSStringCreateWithUTF8CString("toArray");
    JSObjectRef toArray =
        JSObjectMakeFunctionWithCallback(ctx, NULL, ReturnSelf);
    JSObjectSetProperty(ctx, array, toArray_str, toArray,
                        kJSPropertyAttributeDontEnum, NULL);
    JSStringRelease(toArray_str);

    JSStringRef item_str = JSStringCreateWithUTF8CString("item");
    JSObjectRef item =
        JSObjectMakeFunctionWithCallback(ctx, NULL, GetCollectionItem);
    JSObjectSetProperty(ctx, array, item_str, item,
                        kJSPropertyAttributeDontEnum, NULL);
    JSStringRelease(item_str);

    JSStringRef count_str = JSStringCreateWithUTF8CString("count");
    JSStringRef length_str = JSStringCreateWithUTF8CString("length");
    JSValueRef length = JSObjectGetProperty(ctx, array, length_str, NULL);
    JSObjectSetProperty(ctx, array, count_str, length,
                        kJSPropertyAttributeReadOnly |
                        kJSPropertyAttributeDontEnum, NULL);
    JSStringRelease(count_str);
    JSStringRelease(length_str);
  }
  return array;
}

static bool ConvertNativeArrayToJS(JSScriptContext *ctx,
                                   ScriptableArray *array,
                                   JSValueRef *js_val) {
  // To make sure that the array will be destroyed correctly.
  ScriptableHolder<ScriptableArray> array_holder(array);
  size_t length = array->GetCount();
  if (length > INT_MAX)
    return false;

  JSContextRef js_ctx = ctx->GetContext();
  JSObjectRef js_array = CreateJSArray(js_ctx);
  if (!js_array)
    return false;

  for (size_t i = 0; i < length; ++i) {
    JSValueRef item;
    if (ConvertNativeToJS(ctx, array->GetItem(i), &item)) {
      JSValueRef exception = NULL;
      JSObjectSetPropertyAtIndex(js_ctx, js_array, static_cast<unsigned>(i),
                                 item, &exception);
      if (exception)
        ctx->CheckJSException(exception);
    }
  }

  *js_val = CustomizeJSArray(js_ctx, js_array);
  return true;
}

static bool ConvertNativeToJSObject(JSScriptContext *ctx,
                                    const Variant &native_val,
                                    JSValueRef *js_val) {
  bool result = true;
  ScriptableInterface *scriptable =
      VariantValue<ScriptableInterface *>()(native_val);
  if (!scriptable) {
    *js_val = JSValueMakeNull(ctx->GetContext());
  } else if (scriptable->IsInstanceOf(ScriptableArray::CLASS_ID)) {
    result = ConvertNativeArrayToJS(
        ctx, down_cast<ScriptableArray *>(scriptable), js_val);
  } else {
    // If scriptable is a JSScriptableWrapper of this context, then just unwrap
    // it.
    *js_val = ctx->UnwrapJSObject(scriptable);
    // If failed to unwrap it, then wrap it into a JSObjectRef.
    if (!*js_val)
      *js_val = ctx->WrapScriptable(scriptable);
  }
  return result;
}

static bool ConvertNativeToJSDate(JSScriptContext *ctx,
                                  const Variant &native_val,
                                  JSValueRef *js_val) {
  std::string new_date_script =
      StringPrintf("new Date(%ju)", VariantValue<Date>()(native_val).value);
  JSStringRef js_script =
      JSStringCreateWithUTF8CString(new_date_script.c_str());
  JSValueRef exception = NULL;
  *js_val = JSEvaluateScript(ctx->GetContext(), js_script,
                             NULL, NULL, 0, &exception);
  return ctx->CheckJSException(exception);
}

static bool ConvertNativeToJSFunction(JSScriptContext *ctx,
                                      const Variant &native_val,
                                      JSValueRef *js_val) {
  return ctx->UnwrapJSFunctionSlot(VariantValue<Slot *>()(native_val), js_val);
}

static bool ConvertJSONToJS(JSScriptContext *ctx,
                            const Variant &native_val,
                            JSValueRef *js_val) {
  JSONString json_str = VariantValue<JSONString>()(native_val);
  return JSONDecode(ctx, json_str.value.c_str(), js_val);
}

bool ConvertNativeToJS(JSScriptContext *ctx, const Variant &native_val,
                       JSValueRef *js_val) {
  switch (native_val.type()) {
    case Variant::TYPE_VOID:
      return ConvertNativeToJSVoid(ctx, native_val, js_val);
    case Variant::TYPE_BOOL:
      return ConvertNativeToJSBool(ctx, native_val, js_val);
    case Variant::TYPE_INT64:
      return ConvertNativeToJSInt(ctx, native_val, js_val);
    case Variant::TYPE_DOUBLE:
      return ConvertNativeToJSDouble(ctx, native_val, js_val);
    case Variant::TYPE_STRING:
      return ConvertNativeToJSString(ctx, native_val, js_val);
    case Variant::TYPE_JSON:
      return ConvertJSONToJS(ctx, native_val, js_val);
    case Variant::TYPE_UTF16STRING:
      return ConvertNativeUTF16ToJSString(ctx, native_val, js_val);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertNativeToJSObject(ctx, native_val, js_val);
    case Variant::TYPE_SLOT:
      return ConvertNativeToJSFunction(ctx, native_val, js_val);
    case Variant::TYPE_DATE:
      return ConvertNativeToJSDate(ctx, native_val, js_val);
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
      return false;
    case Variant::TYPE_VARIANT:
      // Normally there is no real value of this type, so convert it to void.
      return ConvertNativeToJSVoid(ctx, native_val, js_val);
    default:
      return false;
  }
}

JSObjectRef CompileFunction(JSScriptContext *ctx, const char *script,
                            const char *filename, int lineno,
                            JSValueRef *exception) {
  if (!script)
    return NULL;

  std::string massaged_script = MassageJScript(script, false, filename, lineno);
  JSStringRef js_script =
      JSStringCreateWithUTF8CString(massaged_script.c_str());
  JSStringRef src_url =
      (filename && *filename) ? JSStringCreateWithUTF8CString(filename) : NULL;
  JSObjectRef result = JSObjectMakeFunction(
      ctx->GetContext(), NULL, 0, NULL, js_script, src_url, lineno, exception);
  JSStringRelease(js_script);
  if (src_url)
    JSStringRelease(src_url);
  return result;
}

void RaiseJSException(JSScriptContext *ctx, JSValueRef *exception,
                      const char *format, ...) {
  if(exception && !*exception) {
    va_list ap;
    va_start(ap, format);
    std::string message = StringVPrintf(format, ap);
    va_end(ap);
    JSStringRef js_str = JSStringCreateWithUTF8CString(message.c_str());
    *exception = JSValueMakeString(ctx->GetContext(), js_str);
    JSStringRelease(js_str);
  }
}

} // namespace webkit
} // namespace ggadget
