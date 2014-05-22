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

#ifndef EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JSON_H__
#define EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JSON_H__

#include <string>
#include "java_script_core.h"

namespace ggadget {
namespace webkit {

class JSScriptContext;

/**
 * Encodes a JavaScript value into JSON string.
 */
bool JSONEncode(JSScriptContext *ctx, JSValueRef value, std::string *json);

/**
 * Decodes a JSON string into a JavaScript value.
 */
bool JSONDecode(JSScriptContext *ctx, const char *json, JSValueRef *value);

/**
 * Converts a JSStringRef string to std::string with UTF8 encoding.
 */
std::string ConvertJSStringToUTF8(JSStringRef str);

/**
 * Converts a Date object to a string containing its getTime() result.
 */
bool DateGetTimeString(JSScriptContext *ctx, JSValueRef date,
                       std::string *time_string);

} // namespace webkit
} // namespace ggadget

#endif  // EXTENSIONS_WEBKIT_SCRIPT_RUNTIME_JSON_H__
