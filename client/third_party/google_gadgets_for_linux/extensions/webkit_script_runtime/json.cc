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
#include <vector>
#include <cstring>
#include <cstdio>
#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/js/js_utils.h>
#include "json.h"
#include "js_script_context.h"

namespace ggadget {
namespace webkit {

// Use Microsoft's method to encode/decode Date object in JSON.
// See http://msdn2.microsoft.com/en-us/library/bb299886.aspx.
static const char kDatePrefix[] = "\"\\/Date(";
static const char kDatePostfix[] = ")\\/\"";

static void AppendJSON(JSScriptContext *ctx, JSValueRef value,
                       std::string *json, std::vector<JSValueRef> *stack);

static bool IsFunction(JSContextRef ctx, JSValueRef value) {
  return JSValueIsObject(ctx, value) &&
    JSObjectIsFunction(ctx, JSValueToObject(ctx, value, NULL));
}

static void AppendArrayToJSON(JSScriptContext *ctx, JSObjectRef array,
                              std::string *json,
                              std::vector<JSValueRef> *stack) {
  (*json) += '[';
  unsigned int length = ctx->GetArrayLength(array);
  JSContextRef js_ctx = ctx->GetContext();
  for (unsigned int i = 0; i < length; ++i) {
    JSValueRef prop = JSObjectGetPropertyAtIndex(js_ctx, array, i, NULL);
    AppendJSON(ctx, prop, json, stack);
    if (i != length - 1)
      (*json) += ',';
  }
  (*json) += ']';
}

static void AppendStringToJSON(JSScriptContext *ctx, JSStringRef str,
                               std::string *json) {
  *json += '"';
  size_t length = JSStringGetLength(str);
  const JSChar *chars = JSStringGetCharactersPtr(str);
  for (size_t i = 0; i < length; ++i) {
    JSChar c = chars[i];
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
  *json += '"';
}

static void AppendObjectToJSON(JSScriptContext *ctx, JSObjectRef object,
                               std::string *json,
                               std::vector<JSValueRef> *stack) {
  JSContextRef js_ctx = ctx->GetContext();
  (*json) += '{';
  JSPropertyNameArrayRef prop_names = JSObjectCopyPropertyNames(js_ctx, object);
  size_t length = JSPropertyNameArrayGetCount(prop_names);
  for (size_t i = 0; i < length; ++i) {
    JSStringRef name = JSPropertyNameArrayGetNameAtIndex(prop_names, i);
    JSValueRef prop = JSObjectGetProperty(js_ctx, object, name, NULL);
    // Ignore function properties.
    if (!IsFunction(js_ctx, prop)) {
      AppendStringToJSON(ctx, name, json);
      (*json) += ':';
      AppendJSON(ctx, prop, json, stack);
      (*json) += ',';
    }
  }
  JSPropertyNameArrayRelease(prop_names);
  // Remove the last extra ','.
  if (json->length() > 0 && *(json->end() - 1) == ',')
    json->erase(json->end() - 1);
  (*json) += '}';
}

static void AppendNumberToJSON(JSScriptContext *ctx, JSValueRef value,
                               std::string *json) {
  JSStringRef str = JSValueToStringCopy(ctx->GetContext(), value, NULL);
  if (str) {
    std::string utf8 = ConvertJSStringToUTF8(str);
    // Treat Infinity, -Infinity and NaN as zero.
    if (utf8.length() && utf8[0] != 'I' && utf8[1] != 'I' && utf8[0] != 'N')
      *json += utf8;
    else
      *json += '0';
    JSStringRelease(str);
  } else {
    *json += '0';
  }
}

static bool DateGetTimeStringInternal(JSScriptContext *ctx, JSObjectRef date,
                                      std::string *time_string) {
  static JSStringRef get_time_name = JSStringCreateWithUTF8CString("getTime");
  JSContextRef js_ctx = ctx->GetContext();

  JSValueRef get_time = JSObjectGetProperty(js_ctx, date, get_time_name, NULL);
  if (JSValueIsObject(js_ctx, get_time)) {
    JSObjectRef get_time_obj = JSValueToObject(js_ctx, get_time, NULL);
    if (JSObjectIsFunction(js_ctx, get_time_obj)) {
      JSValueRef result =
          JSObjectCallAsFunction(js_ctx, get_time_obj, date, 0, NULL, NULL);
      if (result) {
        AppendNumberToJSON(ctx, result, time_string);
        return true;
      }
    }
  }
  return false;
}

static bool AppendDateToJSON(JSScriptContext *ctx, JSObjectRef date,
                             std::string *json) {
  std::string time_string;
  if (DateGetTimeStringInternal(ctx, date, &time_string)) {
    *json += kDatePrefix;
    *json += time_string;
    *json += kDatePostfix;
    return true;
  }
  return false;
}

static void AppendJSON(JSScriptContext *ctx, JSValueRef value,
                       std::string *json, std::vector<JSValueRef> *stack) {
  JSContextRef js_ctx = ctx->GetContext();
  switch (JSValueGetType(js_ctx, value)) {
    case kJSTypeObject:
      if (std::find(stack->begin(), stack->end(), value) != stack->end()) {
        // Break the infinite reference loops.
        (*json) += "null";
      } else {
        stack->push_back(value);
        JSObjectRef object = JSValueToObject(js_ctx, value, NULL);
        if (!object) {
          (*json) += "null";
        } else if (ctx->IsArray(object)) {
          AppendArrayToJSON(ctx, object, json, stack);
        } else if (ctx->IsDate(object)) {
          if (!AppendDateToJSON(ctx, object, json))
            AppendObjectToJSON(ctx, object, json, stack);
        } else if (JSObjectIsFunction(js_ctx, object) ||
                   JSObjectIsConstructor(js_ctx, object)) {
          (*json) += "null";
        } else {
          AppendObjectToJSON(ctx, object, json, stack);
        }
        stack->pop_back();
      }
      break;
    case kJSTypeString:
      {
        JSStringRef str = JSValueToStringCopy(js_ctx, value, NULL);
        AppendStringToJSON(ctx, str, json);
        JSStringRelease(str);
      }
      break;
    case kJSTypeNumber:
      AppendNumberToJSON(ctx, value, json);
      break;
    case kJSTypeBoolean:
      (*json) += (JSValueToBoolean(js_ctx, value) ? "true" : "false");
      break;
    default:
      (*json) += "null";
      break;
  }
}

bool JSONEncode(JSScriptContext *ctx, JSValueRef value, std::string *json) {
  json->clear();
  std::vector<JSValueRef> stack;
  AppendJSON(ctx, value, json, &stack);
  return true;
}

bool JSONDecode(JSScriptContext *ctx, const char *json, JSValueRef *value) {
  JSContextRef js_ctx = ctx->GetContext();
  if (!json || !json[0]) {
    *value = JSValueMakeNull(js_ctx);
    return true;
  }
  std::string json_script;
  if (!ggadget::js::ConvertJSONToJavaScript(json, &json_script))
    return false;

  std::string json_filename("JSON:");
  json_filename += json;
  JSStringRef script_str = JSStringCreateWithUTF8CString(json_script.c_str());
  JSStringRef source_str = JSStringCreateWithUTF8CString(json_filename.c_str());
  JSValueRef exception = NULL;
  *value = JSEvaluateScript(js_ctx, script_str,
                            JSContextGetGlobalObject(js_ctx),
                            source_str, 1, &exception);
  JSStringRelease(script_str);
  JSStringRelease(source_str);
  return ctx->CheckJSException(exception);
}

std::string ConvertJSStringToUTF8(JSStringRef str) {
  std::string result;
  if (str) {
    size_t length = JSStringGetLength(str);
    const JSChar *chars = JSStringGetCharactersPtr(str);
    ConvertStringUTF16ToUTF8(chars, length, &result);
  }
  return result;
}

bool DateGetTimeString(JSScriptContext *ctx, JSValueRef date,
                       std::string *result) {
  ASSERT(result);
  JSContextRef js_ctx = ctx->GetContext();
  if (ctx->IsDate(date)) {
    JSObjectRef date_obj = JSValueToObject(js_ctx, date, NULL);
    if (date_obj)
      return DateGetTimeStringInternal(ctx, date_obj, result);
  }
  return false;
}

} // namespace smjs
} // namespace ggadget
