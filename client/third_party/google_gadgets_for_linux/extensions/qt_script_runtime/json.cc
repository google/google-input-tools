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

#include <vector>
#include <QtScript/QScriptValueIterator>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>
#include <ggadget/js/js_utils.h>
#include "json.h"

namespace ggadget {
namespace qt {

// Use Microsoft's method to encode/decode Date object in JSON.
// See http://msdn2.microsoft.com/en-us/library/bb299886.aspx.
static const char kDatePrefix[] = "\"\\/Date(";
static const char kDatePostfix[] = ")\\/\"";

static void AppendJSON(QScriptEngine *engine, const QScriptValue &qval,
                       std::string *json, std::vector<QScriptValue> *stack);

static void AppendArrayToJSON(QScriptEngine *engine, const QScriptValue &qval,
                              std::string *json, std::vector<QScriptValue> *stack) {
  (*json) += '[';
  int length = qval.property("length").toInt32();
  for (int i = 0; i < length; i++) {
    QScriptValue v = qval.property(i);
    AppendJSON(engine, v, json, stack);
    if (i != length - 1)
      (*json) += ',';
  }
  (*json) += ']';
}

static void AppendStringToJSON(QScriptEngine *engine, QString str,
                               std::string *json) {
  GGL_UNUSED(engine);
  *json += EncodeJavaScriptString(str.utf16(), '"');
}

static void AppendObjectToJSON(QScriptEngine *engine, const QScriptValue &qval,
                               std::string *json, std::vector<QScriptValue> *stack) {
  (*json) += '{';
  QScriptValueIterator it(qval);

  while (it.hasNext()) {
    it.next();
    // Don't output methods.
    if (!it.value().isFunction()) {
      AppendStringToJSON(engine, it.name(), json);
      (*json) += ':';
      AppendJSON(engine, it.value(), json, stack);
      (*json) += ',';
    }
  }
  if (json->length() > 0 && *(json->end() - 1) == ',')
    json->erase(json->end() - 1);
  (*json) += '}';
}

static void AppendNumberToJSON(QScriptEngine *engine, const QScriptValue &qval,
                               std::string *json) {
  GGL_UNUSED(engine);
  std::string str = qval.toString().toStdString();
  if (str.empty() || str[0] == 'I' || str[1] == 'I' || str[0] == 'N')
    *json += '0';
  else
    *json += str;
}

static void AppendDateToJSON(QScriptEngine *engine, const QScriptValue &qval,
                               std::string *json) {
  GGL_UNUSED(engine);
  *json += kDatePrefix;
  uint64_t v = static_cast<uint64_t>(qval.toNumber());
  char buf[30];
  snprintf(buf, 30, "%ju", v);
  *json += buf;
  *json += kDatePostfix;
}

static void AppendJSON(QScriptEngine *engine, const QScriptValue &qval,
                       std::string *json, std::vector<QScriptValue> *stack) {
  // Object should be handled after function, string, array ...
  if (qval.isFunction()) {
    (*json) += "null";
  } else if (qval.isDate()) {
    AppendDateToJSON(engine, qval, json);
  } else if (qval.isString()) {
    AppendStringToJSON(engine, qval.toString(), json);
  } else if (qval.isNumber()) {
    AppendNumberToJSON(engine, qval, json);
  } else if (qval.isBoolean()) {
    (*json) += qval.toBoolean() ? "true" : "false";
  } else if (qval.isArray()) {
    AppendArrayToJSON(engine, qval, json, stack);
  } else if (qval.isObject()) {
    for (size_t i = 0; i < stack->size(); i++) {
      if ((*stack)[i].strictlyEquals(qval)) {
        (*json) += "null";
        return;
      }
    }
    stack->push_back(qval);
    AppendObjectToJSON(engine, qval, json, stack);
    stack->pop_back();
  } else {
    (*json) += "null";
  }
}

bool JSONEncode(QScriptEngine *engine, const QScriptValue &qval,
                std::string *json) {
  json->clear();
  std::vector<QScriptValue> stack;
  AppendJSON(engine, qval, json, &stack);
  return true;
}

bool JSONDecode(QScriptEngine* engine, const char *json, QScriptValue *qval) {
  if (!json || !json[0]) {
    *qval = engine->nullValue();
    return true;
  }
  std::string script;
  if (ggadget::js::ConvertJSONToJavaScript(json, &script)) {
    *qval = engine->evaluate(script.c_str());
    return true;
  } else {
    return false;
  }
}
} // namespace qt
} // namespace ggadget
