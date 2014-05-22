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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/qt/qt_main_loop.h>
#include "../js_script_context.h"
#include "../js_script_runtime.h"
#include "../converter.h"
#include "../json.h"
using namespace ggadget::qt;
ggadget::qt::QtMainLoop g_main_loop;

// The exception value thrown by Assert function.
const int kAssertExceptionMagic = 135792468;

enum QuitCode {
  QUIT_OK = 0,
  DONT_QUIT = 1,
  QUIT_ERROR = -1,
  QUIT_JSERROR = -2,
  QUIT_ASSERT = -3,
};
QuitCode g_quit_code = DONT_QUIT;

static const size_t kBufferSize = 655360;
static char g_buffer[kBufferSize];
static void Process(JSScriptContext *context, const char *filename) {
  FILE *file;
  file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Can't open file: %s\n", filename);
    g_quit_code = QUIT_ERROR;
    return;
  }

  size_t s = fread(g_buffer, 1,  kBufferSize, file);
  if (s == kBufferSize) {
    std::cout << "Buffer is too small for script to be run\n";
    exit(1);
  }

  g_buffer[s] = '\0';

  context->Execute(g_buffer, filename, 1);
}

static QScriptValue Print(QScriptContext *ctx, QScriptEngine *engine) {
  for (int i = 0; i < ctx->argumentCount(); i++)
    std::cout << ctx->argument(i).toString().toStdString() << " ";
  std::cout << "\n";
  return engine->undefinedValue();
}

static QScriptValue Quit(QScriptContext *ctx, QScriptEngine *engine) {
  int code = QUIT_OK;
  if (ctx->argumentCount() >= 1) {
    code = ctx->argument(0).toInt32();
  }
  g_quit_code = static_cast<QuitCode>(code);
  return engine->undefinedValue();
}

static QScriptValue GC(QScriptContext *ctx, QScriptEngine *engine) {
  return engine->undefinedValue();
}
const char kAssertFailurePrefix[] = "Failure\n";

// This function is used in JavaScript unittests.
// It checks the result of a predicate function that returns a blank string
// on success or otherwise a string containing the assertion failure message.
// Usage: ASSERT(EQ(a, b), "Test a and b");
static QScriptValue Assert(QScriptContext *ctx, QScriptEngine *engine) {
  QScriptValue arg0 = ctx->argument(0);
  if (!arg0.isNull()) {
    g_quit_code = QUIT_ASSERT;
    std::cout << kAssertFailurePrefix << arg0.toString().toStdString();
    if (ctx->argumentCount() > 1) {
      QScriptValue arg1 = ctx->argument(1);
      std::cout << " " << arg1.toString().toStdString();
    }
    std::cout << "\n";
    ctx->throwError("");
  }
  return engine->undefinedValue();
}

static QScriptValue SetVerbose(QScriptContext *ctx, QScriptEngine *engine) {
  return engine->undefinedValue();
}

static QScriptValue ShowFileAndLine(QScriptContext *ctx, QScriptEngine *engine) {
  return engine->undefinedValue();
}

static QScriptValue JSONEncodeFunc(QScriptContext *ctx, QScriptEngine *engine) {
  if (ctx->argumentCount() == 0) return engine->undefinedValue();
  QScriptValue arg0 = ctx->argument(0);
  std::string json;
  if (ggadget::qt::JSONEncode(engine, arg0, &json)) {
    return QScriptValue(engine, json.c_str());
  } else {
    ctx->throwError("");
    return engine->undefinedValue();
  }
}

static QScriptValue JSONDecodeFunc(QScriptContext *ctx, QScriptEngine *engine) {
  if (ctx->argumentCount() == 0) return engine->undefinedValue();
  QScriptValue arg0 = ctx->argument(0);
  if (!arg0.isString()) return engine->undefinedValue();
  std::string str = arg0.toString().toStdString();
  QScriptValue ret;
  if (!ggadget::qt::JSONDecode(engine, str.c_str(), &ret))
    ctx->throwError("");
  return ret;
}

struct FunctionSpec {
  const char *name;
  QScriptEngine::FunctionSignature fun;
};

static FunctionSpec global_functions[] = {
  { "print", Print },
  { "quit", Quit },
  { "gc", GC },
  { "setVerbose", SetVerbose },
  { "showFileAndLine", ShowFileAndLine },
  { "jsonEncode", JSONEncodeFunc },
  { "jsonDecode", JSONDecodeFunc },
  { "ASSERT", Assert },
  { 0 }
};

static void DefineGlobalFunctions(QScriptEngine *engine) {
  QScriptValue global = engine->globalObject();
  for (int i = 0; global_functions[i].name != 0; i++) {
    global.setProperty(global_functions[i].name,
                       engine->newFunction(global_functions[i].fun));
  }
}

// A hook to initialize custom objects before running scripts.
bool InitCustomObjects(JSScriptContext *context);
void DestroyCustomObjects(JSScriptContext *context);

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  QCoreApplication app(argc, argv);
  ggadget::SetGlobalMainLoop(&g_main_loop);

  JSScriptRuntime *runtime = new JSScriptRuntime();
  JSScriptContext *context = ggadget::down_cast<JSScriptContext *>(
      runtime->CreateContext());
  DefineGlobalFunctions(context->engine());

  if (!InitCustomObjects(context))
    return QUIT_ERROR;

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      Process(context, argv[i]);
      if (g_quit_code != DONT_QUIT)
        break;
    }
  }

  DestroyCustomObjects(context);
  context->Destroy();
  delete runtime;

  if (g_quit_code == DONT_QUIT)
    g_quit_code = QUIT_OK;
  return g_quit_code;
}
