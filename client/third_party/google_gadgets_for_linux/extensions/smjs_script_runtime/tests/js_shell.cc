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

// This file was written using SpiderMonkey's js.c as a sample.
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jsapi.h>

#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>
#include "../js_script_context.h"
#include "../js_script_runtime.h"
#include "../converter.h"
#include "../json.h"

using ggadget::smjs::PrintJSValue;
using ggadget::smjs::JSScriptContext;
using ggadget::smjs::JSScriptRuntime;

// The exception value thrown by Assert function.
const int kAssertExceptionMagic = 135792468;

bool g_interactive = false;

enum QuitCode {
  QUIT_OK = 0,
  DONT_QUIT = 1,
  QUIT_ERROR = -1,
  QUIT_JSERROR = -2,
  QUIT_ASSERT = -3,
};
QuitCode g_quit_code = DONT_QUIT;

extern "C" {
// We use the editline library in SpiderMonkey.
char *readline(const char *prompt);
void add_history(const char *line);
}

static JSBool GetLine(char *buffer, size_t size, const char *prompt) {
  char *linep = readline(prompt);
  if (!linep)
    return JS_FALSE;
  if (linep[0] != '\0')
    add_history(linep);
  strncpy(buffer, linep, size - 2);
  free(linep);
  buffer[size - 2] = '\0';
  strncat(buffer, "\n", 1);
  return JS_TRUE;
}

static JSBool IsCompilableUnit(JSContext *cx, JSObject *obj,
                               const char *buffer) {
  if (!JS_BufferIsCompilableUnit(cx, obj, buffer, strlen(buffer)))
    return JS_FALSE;
  // JS_BufferIsCompilableUnit in SpiderMonkey version 1.6 and 1.7
  // can't judge multiline comments correctly.
  const char *p = buffer;
  while (*p) {
    switch (*p) {
      case '/':
        if (p[1] == '/') {
          p++;
          do p++; while (*p && *p != '\n');
        } else if (p[1] == '*') {
          p += 2;
          bool found = false;
          while (*p && p[1]) {
            if (*p == '*' && p[1] == '/') {
              p += 2;
              found = true;
              break;
            }
            p++;
          }
          if (!found) return JS_FALSE;
        } else {
          p++;
        }
        break;
      case '"':
        do p++; while (*p && (*p != '"' || (p != buffer && p[-1] == '\\')));
        if (*p == '"') p++;
        break;
      case '\'':
        do p++; while (*p && (*p != '\'' || (p != buffer && p[-1] == '\\')));
        if (*p == '\'') p++;
        break;
      default:
        p++;
        break;
    }
  }
  // Omit errors in string literals, because JS engine will handle them.
  return JS_TRUE;
}

static void ProcessScript(JSContext *cx, JSObject *obj,
                          const char *script, size_t length,
                          const char *filename, int startline) {
  ggadget::UTF16String utf16_string;
  ggadget::ConvertStringUTF8ToUTF16(script, length, &utf16_string);
  JSScript *js_script = JS_CompileUCScript(cx, obj,
                                           utf16_string.c_str(),
                                           utf16_string.size(),
                                           filename, startline);
  if (js_script) {
    jsval result;
    if (JS_ExecuteScript(cx, obj, js_script, &result) &&
        result != JSVAL_VOID &&
        g_interactive) {
      puts(PrintJSValue(cx, result).c_str());
    }
    JS_DestroyScript(cx, js_script);
  }
  JS_ClearPendingException(cx);
}

static char g_buffer[10 * 1024 * 1024];
static void Process(JSContext *cx, JSObject *obj, const char *filename) {
  if (!filename || strcmp(filename, "-") == 0) {
    g_interactive = true;
    int lineno = 1;
    bool eof = false;
    do {
      char *bufp = g_buffer;
      size_t remain_size = sizeof(g_buffer);
      *bufp = '\0';
      int startline = lineno;
      do {
        if (!GetLine(bufp, remain_size,
                     startline == lineno ? "js> " : "  > ")) {
          eof = true;
          break;
        }
        size_t line_len = strlen(bufp);
        bufp += line_len;
        remain_size -= line_len;
        lineno++;
      } while (!IsCompilableUnit(cx, obj, g_buffer));
      ProcessScript(cx, obj, g_buffer, sizeof(g_buffer) - remain_size,
                    filename, startline);
    } while (!eof && g_quit_code == DONT_QUIT);
  } else {
    g_interactive = false;
    FILE *file = fopen(filename, "r");
    if (!file) {
      fprintf(stderr, "Can't open file: %s\n", filename);
      g_quit_code = QUIT_ERROR;
      return;
    }
    fprintf(stderr, "Load from file: %s\n", filename);
    size_t size = fread(g_buffer, 1, sizeof(g_buffer), file);
    ProcessScript(cx, obj, g_buffer, size, filename, 1);
  }
}

static JSBool Print(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *rval) {
  for (uintN i = 0; i < argc; i++)
    printf("%s ", PrintJSValue(cx, argv[i]).c_str());
  putchar('\n');
  fflush(stdout);
  return JS_TRUE;
}

static JSBool Load(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval) {
  if (argc >= 1) {
    JSString *js_string = JS_ValueToString(cx, argv[0]);
    if (js_string) {
      char *bytes = JS_GetStringBytes(js_string);
      if (bytes)
        Process(cx, JS_GetGlobalObject(cx), bytes);
    }
  }
  return JS_TRUE;
}

static JSBool Quit(JSContext *cx, JSObject *obj,
                   uintN argc, jsval *argv, jsval *rval) {
  int32 code = QUIT_OK;
  if (argc >= 1)
    JS_ValueToInt32(cx, argv[0], &code);
  g_quit_code = static_cast<QuitCode>(code);
  return JS_FALSE;
}

static JSBool GC(JSContext *cx, JSObject *obj,
                 uintN argc, jsval *argv, jsval *rval) {
  JS_GC(cx);
  return JS_TRUE;
}

const char kAssertFailurePrefix[] = "Failure\n";

// This function is used in JavaScript unittests.
// It checks the result of a predicate function that returns a blank string
// on success or otherwise a string containing the assertion failure message.
// Usage: ASSERT(EQ(a, b), "Test a and b");
static JSBool Assert(JSContext *cx, JSObject *obj,
                     uintN argc, jsval *argv, jsval *rval) {
  if (argv[0] != JSVAL_NULL) {
    if (argc > 1)
      JS_ReportError(cx, "%s%s\n%s", kAssertFailurePrefix,
                     PrintJSValue(cx, argv[0]).c_str(),
                     PrintJSValue(cx, argv[1]).c_str());
    else
      JS_ReportError(cx, "%s%s", kAssertFailurePrefix,
                     PrintJSValue(cx, argv[0]).c_str());

    // Let the JavaScript test framework know the failure.
    // The exception value is null to tell the catcher not to print it again.
    JS_SetPendingException(cx, INT_TO_JSVAL(kAssertExceptionMagic));
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool g_verbose = JS_TRUE;

static void ErrorReporter(JSContext *cx, const char *message,
                          JSErrorReport *report) {
  if (!g_interactive &&
      // If the error is an assertion failure, don't quit now because
      // we have thrown an exception to be handled by the JavaScript code.
      strncmp(message, kAssertFailurePrefix,
              sizeof(kAssertFailurePrefix) - 1) != 0) {
    if (JSREPORT_IS_EXCEPTION(report->flags) ||
        JSREPORT_IS_STRICT(report->flags)) {
      // Unhandled exception or strict errors, quit.
      g_quit_code = QUIT_JSERROR;
    } else {
      // Convert this error into an exception, to make the tester be able to
      // catch it.
      JS_SetPendingException(cx,
          STRING_TO_JSVAL(JS_NewString(cx, strdup(message), strlen(message))));
    }
  }

  fflush(stdout);
  if (g_verbose)
    fprintf(stderr, "%s:%d: %s\n", report->filename, report->lineno, message);
  fflush(stderr);
}

static JSBool SetVerbose(JSContext *cx, JSObject *obj,
                         uintN argc, jsval *argv, jsval *rval) {
  return JS_ValueToBoolean(cx, argv[0], &g_verbose);
}

static void TempErrorReporter(JSContext *cx, const char *message,
                              JSErrorReport *report) {
  printf("%s:%d\n", report->filename, report->lineno);
}

static JSBool ShowFileAndLine(JSContext *cx, JSObject *obj,
                              uintN argc, jsval *argv, jsval *rval) {
  JSErrorReporter old_reporter = JS_SetErrorReporter(cx, TempErrorReporter);
  JS_ReportError(cx, "");
  JS_SetErrorReporter(cx, old_reporter);
  return JS_TRUE;
}

static JSBool JSONEncodeFunc(JSContext *cx, JSObject *obj,
                             uintN argc, jsval *argv, jsval *rval) {
  std::string json;
  if (ggadget::smjs::JSONEncode(cx, argv[0], &json)) {
    *rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx, json.c_str(), json.length()));
    return JS_TRUE;
  }
  argv[0] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "JSONEncode failed"));
  return Assert(cx, obj, argc, argv, rval);
}

static JSBool JSONDecodeFunc(JSContext *cx, JSObject *obj,
                             uintN argc, jsval *argv, jsval *rval) {
  JSString *str = JS_ValueToString(cx, argv[0]);
  if (str && ggadget::smjs::JSONDecode(cx, JS_GetStringBytes(str), rval))
    return JS_TRUE;
  argv[0] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "JSONDecode failed"));
  return Assert(cx, obj, argc, argv, rval);
}

static JSFunctionSpec global_functions[] = {
  { "print", Print, 0 },
  { "load", Load, 1 },
  { "quit", Quit, 0 },
  { "gc", GC, 0 },
  { "setVerbose", SetVerbose, 1 },
  { "showFileAndLine", ShowFileAndLine, 0 },
  { "jsonEncode", JSONEncodeFunc, 1 },
  { "jsonDecode", JSONDecodeFunc, 1 },
  { "ASSERT", Assert, 1 },
  { 0 }
};

// A hook to initialize custom objects before running scripts.
JSBool InitCustomObjects(JSScriptContext *context);
void DestroyCustomObjects(JSScriptContext *context);

int main(int argc, char *argv[]) {
#ifdef XPCOM_GLUE
  if (!ggadget::libmozjs::LibmozjsGlueStartup()) {
    printf("Failed to load libmozjs.so\n");
    return 1;
  }
#endif
  setlocale(LC_ALL, "");
  JSScriptRuntime *runtime = new JSScriptRuntime();
  JSScriptContext *context = ggadget::down_cast<JSScriptContext *>(
      runtime->CreateContext());
  JSContext *cx = context->context();
  if (!cx)
    return QUIT_ERROR;

  JS_SetErrorReporter(cx, ErrorReporter);
  if (!InitCustomObjects(context))
    return QUIT_ERROR;

  JSObject *global = JS_GetGlobalObject(cx);
  JS_DefineFunctions(cx, global, global_functions);

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      Process(cx, global, argv[i]);
      if (g_quit_code != DONT_QUIT)
        break;
    }
  } else {
    Process(cx, global, NULL);
  }

  DestroyCustomObjects(context);
  context->Destroy();
  delete runtime;

  if (g_quit_code == DONT_QUIT)
    g_quit_code = QUIT_OK;
  return g_quit_code;
}
