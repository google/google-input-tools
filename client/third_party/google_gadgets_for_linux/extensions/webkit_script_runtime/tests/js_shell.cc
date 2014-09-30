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

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>
#include "../js_script_context.h"
#include "../js_script_runtime.h"
#include "../converter.h"
#include "../json.h"

using ggadget::webkit::JSONEncode;
using ggadget::webkit::JSONDecode;
using ggadget::webkit::ConvertJSStringToUTF8;
using ggadget::webkit::PrintJSValue;
using ggadget::webkit::JSScriptContext;
using ggadget::webkit::JSScriptRuntime;
using ggadget::ScriptContextInterface;

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

static bool GetLine(char *buffer, size_t size, const char *prompt) {
  char *linep = readline(prompt);
  if (!linep)
    return false;
  if (linep[0] != '\0')
    add_history(linep);
  strncpy(buffer, linep, size - 2);
  free(linep);
  buffer[size - 2] = '\0';
  strncat(buffer, "\n", 1);
  return true;
}

static void ProcessScript(JSScriptContext *context, const char *script,
                          const char *filename, int startline) {
  JSStringRef js_script = JSStringCreateWithUTF8CString(script);
  JSStringRef js_src_url = NULL;
  if (filename)
    js_src_url = JSStringCreateWithUTF8CString(filename);
  JSValueRef exception = NULL;
  JSEvaluateScript(context->GetContext(), js_script, NULL,
                   js_src_url, startline, &exception);
  JSStringRelease(js_script);
  if (js_src_url)
    JSStringRelease(js_src_url);
  context->CheckJSException(exception);
}

static char g_buffer[10 * 1024 * 1024];
static void Process(JSScriptContext *context, const char *filename) {
  if (!filename || strcmp(filename, "-") == 0) {
    g_interactive = true;
    int lineno = 1;
    bool eof = false;
    do {
      char *bufp = g_buffer;
      size_t remain_size = sizeof(g_buffer);
      *bufp = '\0';
      int startline = lineno;
      while(1) {
        if (!GetLine(bufp, remain_size,
                     startline == lineno ? "js> " : "  > ")) {
          eof = true;
          break;
        }
        size_t line_len = strlen(bufp);
        bufp += line_len;
        remain_size -= line_len;
        lineno++;
        if (line_len == 2 && *(bufp - 2) == '/') {
          *(bufp - 2) = '\n';
          break;
        }
      };
      ProcessScript(context, g_buffer, filename, startline);
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
    size_t size = fread(g_buffer, 1, sizeof(g_buffer) - 1, file);
    g_buffer[size] = '\0';
    ProcessScript(context, g_buffer, filename, 1);
  }
}

static JSValueRef Print(JSContextRef ctx, JSObjectRef function,
                        JSObjectRef this_object, size_t argument_count,
                        const JSValueRef arguments[], JSValueRef* exception) {
  JSScriptContext *context =
      static_cast<JSScriptContext *>(JSObjectGetPrivate(function));
  for (size_t i = 0; i < argument_count; ++i) {
    printf("%s ", PrintJSValue(context, arguments[i]).c_str());
  }
  putchar('\n');
  fflush(stdout);
  return JSValueMakeUndefined(ctx);
}

static JSValueRef Load(JSContextRef ctx, JSObjectRef function,
                       JSObjectRef this_object, size_t argument_count,
                       const JSValueRef arguments[], JSValueRef* exception) {
  JSScriptContext *context =
      static_cast<JSScriptContext *>(JSObjectGetPrivate(function));
  if (argument_count >= 1) {
    JSStringRef js_string = JSValueToStringCopy(ctx, arguments[0], NULL);
    if (js_string) {
      std::string filename = ConvertJSStringToUTF8(js_string);
      Process(context, filename.c_str());
    }
  }
  return JSValueMakeUndefined(ctx);
}

static JSValueRef Quit(JSContextRef ctx, JSObjectRef function,
                       JSObjectRef this_object, size_t argument_count,
                       const JSValueRef arguments[], JSValueRef* exception) {
  int code = QUIT_OK;
  if (argument_count >= 1) {
    code = static_cast<int>(JSValueToNumber(ctx, arguments[0], NULL));
  }
  g_quit_code = static_cast<QuitCode>(code);
  return JSValueMakeUndefined(ctx);
}

static JSValueRef GC(JSContextRef ctx, JSObjectRef function,
                     JSObjectRef this_object, size_t argument_count,
                     const JSValueRef arguments[], JSValueRef* exception) {
  JSScriptContext *context =
      static_cast<JSScriptContext *>(JSObjectGetPrivate(function));
  context->CollectGarbage();
  return JSValueMakeUndefined(ctx);
}

const char kAssertFailurePrefix[] = "Failure\n";

static JSValueRef Assert(JSContextRef ctx, JSObjectRef function,
                         JSObjectRef this_object, size_t argument_count,
                         const JSValueRef arguments[], JSValueRef* exception) {
  JSScriptContext *context =
      static_cast<JSScriptContext *>(JSObjectGetPrivate(function));
  if (argument_count >= 1) {
    if (!JSValueIsNull(ctx, arguments[0])) {
      if (argument_count > 1)
        printf("%s%s\n%s\n", kAssertFailurePrefix,
               PrintJSValue(context, arguments[0]).c_str(),
               PrintJSValue(context, arguments[1]).c_str());
      else
        printf("%s%s\n", kAssertFailurePrefix,
               PrintJSValue(context, arguments[0]).c_str());
      if (exception)
        *exception = JSValueMakeNumber(ctx, kAssertExceptionMagic);
      return NULL;
    }
  }
  return JSValueMakeUndefined(ctx);
}

bool g_verbose = true;

static JSValueRef SetVerbose(JSContextRef ctx, JSObjectRef function,
                             JSObjectRef this_object, size_t argument_count,
                             const JSValueRef arguments[], JSValueRef* exception) {
  bool old_verbose = g_verbose;
  if (argument_count > 0)
    g_verbose = JSValueToBoolean(ctx, arguments[0]);
  return JSValueMakeBoolean(ctx, old_verbose);
}

static JSValueRef ShowFileAndLine(JSContextRef ctx, JSObjectRef function,
                                  JSObjectRef this_object,
                                  size_t argument_count,
                                  const JSValueRef arguments[],
                                  JSValueRef* exception) {
  // FIXME
  return JSValueMakeUndefined(ctx);
}

static JSValueRef JSONEncodeFunc(JSContextRef ctx, JSObjectRef function,
                                 JSObjectRef this_object,
                                 size_t argument_count,
                                 const JSValueRef arguments[],
                                  JSValueRef* exception) {
  JSScriptContext *context =
      static_cast<JSScriptContext *>(JSObjectGetPrivate(function));

  if (argument_count > 0) {
    std::string json;
    if (JSONEncode(context, arguments[0], &json)) {
      JSStringRef js_json = JSStringCreateWithUTF8CString(json.c_str());
      JSValueRef result = JSValueMakeString(ctx, js_json);
      JSStringRelease(js_json);
      return result;
    }
  }

  JSStringRef error_msg = JSStringCreateWithUTF8CString("JSONEncode failed");
  JSValueRef error = JSValueMakeString(ctx, error_msg);
  JSStringRelease(error_msg);
  return Assert(ctx, function, this_object, 1, &error, exception);
}

static JSValueRef JSONDecodeFunc(JSContextRef ctx, JSObjectRef function,
                                 JSObjectRef this_object,
                                 size_t argument_count,
                                 const JSValueRef arguments[],
                                  JSValueRef* exception) {
  JSScriptContext *context =
      static_cast<JSScriptContext *>(JSObjectGetPrivate(function));

  if (argument_count > 0) {
    JSStringRef js_json = JSValueToStringCopy(ctx, arguments[0], exception);
    std::string json = ConvertJSStringToUTF8(js_json);
    JSStringRelease(js_json);
    JSValueRef result = NULL;
    if (JSONDecode(context, json.c_str(), &result))
      return result;
  }

  JSStringRef error_msg = JSStringCreateWithUTF8CString("JSONDecode failed");
  JSValueRef error = JSValueMakeString(ctx, error_msg);
  JSStringRelease(error_msg);
  return Assert(ctx, function, this_object, 1, &error, exception);
}

struct GlobalFunction {
  const char *name;
  JSObjectCallAsFunctionCallback callback;
};

static GlobalFunction kGlobalFunctions[] = {
  { "print", Print },
  { "load", Load },
  { "quit", Quit },
  { "gc", GC },
  { "setVerbose", SetVerbose },
  { "showFileAndLine", ShowFileAndLine },
  { "jsonEncode", JSONEncodeFunc },
  { "jsonDecode", JSONDecodeFunc },
  { "ASSERT", Assert },
  { NULL, NULL }
};

static void InitGlobalFunctions(JSScriptContext *context) {
  for (size_t i = 0; kGlobalFunctions[i].name; ++i) {
    context->RegisterGlobalFunction(kGlobalFunctions[i].name,
                                    kGlobalFunctions[i].callback);
  }
}

// A hook to initialize custom objects before running scripts.
bool InitCustomObjects(ScriptContextInterface *context);
void DestroyCustomObjects(ScriptContextInterface *context);

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  JSScriptRuntime *runtime = new JSScriptRuntime();
  JSScriptContext *context = ggadget::down_cast<JSScriptContext *>(
      runtime->CreateContext());

  if (!InitCustomObjects(context))
    return QUIT_ERROR;

  InitGlobalFunctions(context);

  context->AssignFromNative(NULL, NULL, "isWebkit", ggadget::Variant(true));

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      Process(context, argv[i]);
      if (g_quit_code != DONT_QUIT)
        break;
    }
  } else {
    Process(context, NULL);
  }

  DestroyCustomObjects(context);
  context->Destroy();
  delete runtime;

  if (g_quit_code == DONT_QUIT)
    g_quit_code = QUIT_OK;
  return g_quit_code;
}
