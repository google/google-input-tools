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

#include <algorithm>
#include <cstring>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/js/js_utils.h>
#include "json.h"

namespace ggadget {
namespace smjs {

// Use Microsoft's method to encode/decode Date object in JSON.
// See http://msdn2.microsoft.com/en-us/library/bb299886.aspx.
static const char kDatePrefix[] = "\"\\/Date(";
static const char kDatePostfix[] = ")\\/\"";

static void AppendJSON(JSContext *cx, jsval js_val, std::string *json,
                       std::vector<jsval> *stack);

static void AppendArrayToJSON(JSContext *cx, JSObject *array,
                              std::string *json, std::vector<jsval> *stack) {
  (*json) += '[';
  jsuint length = 0;
  JS_GetArrayLength(cx, array, &length);
  for (jsuint i = 0; i < length; i++) {
    jsval value = JSVAL_NULL;
    JS_GetElement(cx, array, static_cast<jsint>(i), &value);
    AppendJSON(cx, value, json, stack);
    if (i != length - 1)
      (*json) += ',';
  }
  (*json) += ']';
}

static void AppendStringToJSON(JSContext *cx, JSString *str,
                               std::string *json) {
  GGL_UNUSED(cx);
  *json += '"';
  const jschar *chars = JS_GetStringChars(str);
  if (chars) {
    size_t length = JS_GetStringLength(str);
    for (size_t i = 0; i < length; i++) {
      jschar c = chars[i];
      switch (c) {
        // The following special chars are not so complete, but also works.
        case '"': *json += "\\\""; break;
        case '\\': *json += "\\\\"; break;
        case '\n': *json += "\\n"; break;
        case '\r': *json += "\\r"; break;
        default:
          if (c >= 0x7f || c < 0x20) {
            char buf[10];
            snprintf(buf, sizeof(buf), "\\u%04X", c);
            *json += buf;
          } else {
            *json += static_cast<char>(c);
          }
          break;
      }
    }
  }
  *json += '"';
}

static void AppendObjectToJSON(JSContext *cx, JSObject *object,
                               std::string *json, std::vector<jsval> *stack) {
  (*json) += '{';
  JSIdArray *id_array = JS_Enumerate(cx, object);
  if (id_array) {
    for (int i = 0; i < id_array->length; i++) {
      jsid id = id_array->vector[i];
      jsval key = JSVAL_VOID;
      JS_IdToValue(cx, id, &key);
      if (JSVAL_IS_STRING(key)) {
        JSString *key_str = JSVAL_TO_STRING(key);
        jschar *key_chars = JS_GetStringChars(key_str);
        if (key_chars) {
          jsval value = JSVAL_VOID;
          JS_GetUCProperty(cx, object,
                           key_chars, JS_GetStringLength(key_str),
                           &value);
          // Don't output methods.
          if (JS_TypeOfValue(cx, value) != JSTYPE_FUNCTION &&
              // Not an internal property.
              key_chars[0] != '[') {
            AppendStringToJSON(cx, key_str, json);
            (*json) += ':';
            AppendJSON(cx, value, json, stack);
            (*json) += ',';
          }
        }
      }
      // Otherwise, ignore the property.
    }
    // FIXME: We don't support serializing properties of prototypes.
    // Remove the last extra ','.
    if (json->length() > 0 && *(json->end() - 1) == ',')
      json->erase(json->end() - 1);
  }
  (*json) += '}';
  JS_DestroyIdArray(cx, id_array);
}

static void AppendNumberToJSON(JSContext *cx, jsval js_val, std::string *json) {
  JSString *str = JS_ValueToString(cx, js_val);
  if (str) {
    char *bytes = JS_GetStringBytes(str);
    // Treat Infinity, -Infinity and NaN as zero.
    if (bytes && bytes[0] != 'I' && bytes[1] != 'I' && bytes[0] != 'N')
      *json += bytes;
    else
      *json += '0';
  } else {
    *json += '0';
  }
}

static JSBool AppendDateToJSON(JSContext *cx, JSObject *obj,
                               std::string *json) {
  JSClass *cls = JS_GET_CLASS(cx, obj);
  if (!cls || strcmp("Date", cls->name) != 0)
    return JS_FALSE;

  jsval rval;
  if (!JS_CallFunctionName(cx, obj, "getTime", 0, NULL, &rval))
    return JS_FALSE;

  *json += kDatePrefix;
  AppendNumberToJSON(cx, rval, json);
  *json += kDatePostfix;
  return JS_TRUE;
}

static void AppendJSON(JSContext *cx, jsval js_val, std::string *json,
                       std::vector<jsval> *stack) {
  switch (JS_TypeOfValue(cx, js_val)) {
    case JSTYPE_OBJECT:
      if (find(stack->begin(), stack->end(), js_val) != stack->end()) {
        // Break the infinite reference loops.
        (*json) += "null";
      } else {
        stack->push_back(js_val);
        JSObject *obj = JSVAL_TO_OBJECT(js_val);
        if (!obj)
          (*json) += "null";
        else if (JS_IsArrayObject(cx, obj))
          AppendArrayToJSON(cx, obj, json, stack);
        else if (!AppendDateToJSON(cx, obj, json))
          AppendObjectToJSON(cx, obj, json, stack);
        stack->pop_back();
      }
      break;
    case JSTYPE_STRING:
      AppendStringToJSON(cx, JSVAL_TO_STRING(js_val), json);
      break;
    case JSTYPE_NUMBER:
      AppendNumberToJSON(cx, js_val, json);
      break;
    case JSTYPE_BOOLEAN:
      (*json) += JSVAL_TO_BOOLEAN(js_val) ? "true" : "false";
      break;
    default:
      (*json) += "null";
      break;
  }
}

JSBool JSONEncode(JSContext *cx, jsval js_val, std::string *json) {
  json->clear();
  std::vector<jsval> stack;
  AppendJSON(cx, js_val, json, &stack);
  return JS_TRUE;
}

JSBool JSONDecode(JSContext *cx, const char *json, jsval *js_val) {
  if (!json || !json[0]) {
    *js_val = JSVAL_VOID;
    return JS_TRUE;
  }
  std::string json_script;
  if (!ggadget::js::ConvertJSONToJavaScript(json, &json_script))
    return JS_FALSE;

  std::string json_filename("JSON:");
  json_filename += json;
  return JS_EvaluateScript(cx, JS_GetGlobalObject(cx), json_script.c_str(),
                           static_cast<uintN>(json_script.length()),
                           json_filename.c_str(), 1, js_val);
}

} // namespace smjs
} // namespace ggadget
