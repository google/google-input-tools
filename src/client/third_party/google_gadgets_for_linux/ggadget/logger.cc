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


#include "logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include "main_loop_interface.h"
#include "signals.h"
#include "string_utils.h"

#if defined(OS_WIN)
#include "win32/thread_local_singleton_holder.h"
#endif

namespace ggadget {

namespace {

typedef Signal4<std::string, LogLevel, const char *, int,
                const std::string &> LogSignal;
typedef LightMap<void *, LogSignal *> ContextSignalMap;

class LogGlobalData {
 public:
  LogGlobalData()
      : in_logger(false) {
  }

  ~LogGlobalData() {
  }

  static LogGlobalData *Get() {
#if defined(OS_WIN)
    LogGlobalData *log =
        win32::ThreadLocalSingletonHolder<LogGlobalData>::GetValue();
    if (!log) {
      log = new LogGlobalData();
      const bool result =
          win32::ThreadLocalSingletonHolder<LogGlobalData>::SetValue(log);
      ASSERT(result);
    }
    return log;
#else
    if (!log_)
      log_ = new LogGlobalData();
    return log_;
#endif
  }

  static void Finalize() {
#if defined(OS_WIN)
    delete win32::ThreadLocalSingletonHolder<LogGlobalData>::GetValue();
    win32::ThreadLocalSingletonHolder<LogGlobalData>::SetValue(NULL);
#else
    delete log_;
    log_ = NULL;
#endif
  }

  LogSignal global_signal;
  ContextSignalMap context_signals;
  std::vector<void *> context_stack;
  bool in_logger;

#if !defined(OS_WIN)
 private:
  static LogGlobalData *log_;
#endif
};

#if !defined(OS_WIN)
LogGlobalData *LogGlobalData::log_ = NULL;
#endif

}  // namespace

LogHelper::LogHelper(LogLevel level, const char *file, int line)
    : level_(level), file_(file), line_(line) {
}

static void DoLog(LogLevel level, const char *file, int line,
                  const std::string &message) {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);

  if (log->in_logger)
    return;

  log->in_logger = true;
  std::string new_message;
  void *context =
      log->context_stack.empty() ? NULL : log->context_stack.back();
  ContextSignalMap::const_iterator it = log->context_signals.find(context);
  if (it != log->context_signals.end())
    new_message = (*it->second)(level, file, line, message);
  else
    new_message = message;

  if (log->global_signal.HasActiveConnections()) {
    log->global_signal(level, file, line, new_message);
  } else {
    printf("%s:%d: %s\n", file, line, new_message.c_str());
    // Prints out the log message immediately.
    fflush(stdout);
  }
  log->in_logger = false;
}

#if !defined(OS_WIN)
// Run in the main thread if LoggerHelper is called in another thread.
class LogTask : public WatchCallbackInterface {
 public:
  LogTask(LogLevel level, const char *file, int line,
          const std::string &message)
      : level_(level), file_(file), line_(line), message_(message) {
  }
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    DoLog(level_, VariantValue<const char *>()(file_), line_, message_);
    return false;
  }
  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    delete this;
  }

  LogLevel level_;
  // Variant can correctly handle NULL char *, and use decoupled memory space.
  Variant file_;
  int line_;
  std::string message_;
};
#endif

void LogHelper::operator()(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string message = StringVPrintf(format, ap);
  va_end(ap);

#if defined(OS_WIN)
  DoLog(level_, file_, line_, message);
#else
  MainLoopInterface *main_loop = GetGlobalMainLoop();
  if (!main_loop || main_loop->IsMainThread()) {
    DoLog(level_, file_, line_, message);
  } else {
    main_loop->AddTimeoutWatch(0, new LogTask(level_, file_, line_, message));
  }
#endif
}

ScopedLogContext::ScopedLogContext(void *context) {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  log->context_stack.push_back(context);
}

ScopedLogContext::~ScopedLogContext() {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  log->context_stack.pop_back();
}

void PushLogContext(void *context) {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  log->context_stack.push_back(context);
}

void PopLogContext(void *log_context) {
  GGL_UNUSED(log_context);
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  ASSERT(log_context == log->context_stack.back());
  log->context_stack.pop_back();
}

Connection *ConnectGlobalLogListener(LogListener *listener) {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  return log->global_signal.Connect(listener);
}

Connection *ConnectContextLogListener(void *context, LogListener *listener) {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  ContextSignalMap::const_iterator it = log->context_signals.find(context);
  LogSignal *signal;
  if (it == log->context_signals.end()) {
    signal = new LogSignal();
    log->context_signals[context] = signal;
  } else {
    signal = it->second;
  }
  return signal->Connect(listener);
}

void RemoveLogContext(void *context) {
  LogGlobalData *log = LogGlobalData::Get();
  ASSERT(log);
  ContextSignalMap::iterator it = log->context_signals.find(context);
  if (it != log->context_signals.end()) {
    delete it->second;
    log->context_signals.erase(it);
  }
}

void FinalizeLogger() {
  LogGlobalData::Finalize();
}

} // namespace ggadget
