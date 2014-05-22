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

#include <cstring>
#include <ggadget/common.h>
#include "js_utils.h"

namespace ggadget {
namespace js {
static const char kDatePrefix[] = "\"\\/Date(";
static const char kDatePrefixReplace[] = "new Date(";
static const char kDatePostfix[] = ")\\/\"";
static const char kDatePostfixReplace[] = ")";

bool ConvertJSONToJavaScript(const char *json, std::string *script) {
  if (!json || !json[0]) return false;

  // Valid chars in state 0.
  // Our JSON decoding is stricter than the standard, according to the
  // format we are outputing.
  static const char *kValidChars = ",:{}[]0123456789.-+eE ";
  // State: 0: normal; 1: in word; 2: in string.
  int state = 0;
  const char *word_start = json;
  for (const char *p = json; *p; p++) {
    switch (state) {
      case 0:
        if (*p >= 'a' && *p <= 'z') {
          word_start = p;
          state = 1;
        } else if (*p == '"') {
          state = 2;
        } else if (strchr(kValidChars, *p) == NULL) {
          // Invalid JSON format.
          return false;
        }
        break;
      case 1:
        if (*p < 'a' || *p > 'z') {
          state = 0;
          if (strncmp("true", word_start, 4) != 0 &&
              strncmp("false", word_start, 5) != 0 &&
              strncmp("null", word_start, 4) != 0)
            return false;
        }
        break;
      case 2:
        if (*p == '\\')
          p++;  // Omit the next char. Also works for \x, \" and \uXXXX cases.
        else if (*p == '\n' || *p == '\r')
          return false;
        else if (*p == '"')
          state = 0;
        break;
      default:
        break;
    }
  }

  // Add '()' around the expression to avoid ambiguity of '{}'.
  // See http://www.json.org/json.js.
  std::string json_script(1, '(');
  json_script += json;
  json_script += ')';

  // Now change all "\/Date(.......)\/" into new Date(.......).
  std::string::size_type pos = 0;
  while (pos != std::string::npos) {
    pos = json_script.find(kDatePrefix, pos);
    if (pos != std::string::npos) {
      json_script.replace(pos, arraysize(kDatePrefix) - 1, kDatePrefixReplace);
      pos += arraysize(kDatePrefixReplace) - 1;

      while (json_script[pos] >= '0' && json_script[pos] <= '9')
        pos++;
      if (strncmp(kDatePostfix, json_script.c_str() + pos,
                  arraysize(kDatePostfix) - 1) != 0)
        return false;
      json_script.replace(pos, arraysize(kDatePostfix) - 1,
                          kDatePostfixReplace);
      pos += arraysize(kDatePostfixReplace) - 1;
    }
  }
  *script = json_script;
  return true;
}

} // namespace js
} // namespace ggadget
