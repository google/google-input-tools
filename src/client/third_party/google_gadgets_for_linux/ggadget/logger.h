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

#ifndef GGADGET_LOGGER_H__
#define GGADGET_LOGGER_H__

#include <string>
#include <ggadget/common.h>

namespace ggadget {

class Connection;
template <typename R, typename P1, typename P2, typename P3, typename P4>
class Slot4;

#undef LOG
#undef ASSERT_M
#undef VERIFY
#undef VERIFY_M
#undef DLOG

/**
 * @defgroup Logger Debug logging utilities
 * @ingroup Utilities
 * @{
 */

/** The supported debug message output level */
enum LogLevel {
  /** For verbose trace messages. */
  LOG_TRACE = 0,
  /** For normal information. */
  LOG_INFO = 1,
  /** For non-fatal errors. */
  LOG_WARNING = 2,
  /** For errors. */
  LOG_ERROR = 3
};

struct LogHelper {
  LogHelper(LogLevel level, const char *file, int line);
  void operator()(const char *format, ...) PRINTF_ATTRIBUTE(2, 3);
  LogLevel level_;
  const char *file_;
  int line_;
};

/**
 * Print log with printf format parameters.
 * It works in both debug and release versions.
 */
#define LOG ::ggadget::LogHelper(::ggadget::LOG_WARNING, __FILE__, __LINE__)
#define LOGT ::ggadget::LogHelper(::ggadget::LOG_TRACE, __FILE__, __LINE__)
#define LOGI ::ggadget::LogHelper(::ggadget::LOG_INFO, __FILE__, __LINE__)
#define LOGW LOG
#define LOGE ::ggadget::LogHelper(::ggadget::LOG_ERROR, __FILE__, __LINE__)

#ifdef NDEBUG
#define ASSERT_M(x, y)
#define EXPECT_M(x, y)
#define VERIFY(x) (x)
#define VERIFY_M(x, y) (x)
#define DLOG  true ? (void) 0 : LOGT
#else // NDEBUG

/**
 * Assert an expression with a message in printf format.
 * It only works in debug versions.
 * Sample usage: <code>ASSERT_M(a==b, ("%d==%d failed\n", a, b));</code>
 */
#define ASSERT_M(x, y) do { if (!(x)) { LOGE y; ASSERT(false); } } while(0)

/**
 * Prints a message in printf format if an expression is not true.
 * It only works in debug versions.
 * Sample usage: <code>EXPECT_M(a==b, ("%d==%d failed\n", a, b));</code>
 */
#define EXPECT_M(x, y) do { if (!(x)) LOGW y; } while(0)

/**
 * Verify an expression.
 * In debug mode, it's same as ASSERT(x), in release mode, it executes (x) as
 * normal. So it's suitable for verifying code with side effects.
 * For simple statement use ASSERT instead.
 */
#define VERIFY(x) ASSERT(x)

/**
 * Verify an expression with a message in printf format.
 * In debug mode, it's same as ASSERT(x), in release mode, it executes (x) as
 * normal. So it's suitable for verifying code with side effects.
 * For simple statement use ASSERT_M instead.
 * Sample usage: <code>VERIFY_M(a==b, ("%d==%d failed\n", a, b));</code>
 */
#define VERIFY_M(x, y) ASSERT_M(x, y)

/**
 * Print debug log with printf format parameters.
 * It only works in debug versions.
 */
#define DLOG LOGT

#endif // else NDEBUG

/**
 * This class helps manage stack-based log contexts. The log context at the
 * top of the stack is the current log context. If any listener is connected,
 * all logs will be sent to the corresponding context log listerner.
 */
class ScopedLogContext {
 public:
  ScopedLogContext(void *context);
  ~ScopedLogContext();
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScopedLogContext);
};

/**
 * C-style log context management. Can only be used when @c ScopedLogContext
 * is not applicable. The @a log_context parameter of @c PopLogContext() is
 * used to ensure the calls are connectly paired.
 */
void PushLogContext(void *context);
void PopLogContext(void *context);

/**
 * The prototype of the listener is like:
 * <code>
 * std::string listener(LogLevel log_level, const char *filename, int lineno,
 *                      const std::string &log_message);
 * </code>
 * The listener should return the input log_message or return the string
 * modified from log_message.
 */
typedef Slot4<std::string, LogLevel, const char *, int, const std::string &>
    LogListener;

/**
 * Connect a global log listener. All logs will be send to the listener.
 */
Connection *ConnectGlobalLogListener(LogListener *listener);

/**
 * Connect a log listener for a log context.
 * @c ConnectContextLogListener(NULL, listener) will let the @a listener
 * receive all logs that don't belong to any context.
 * @see ScopedLogContext
 * @see ConnectGlobalLogListener()
 */
Connection *ConnectContextLogListener(void *context, LogListener *listener);

/**
 * Remove a logger context.
 */
void RemoveLogContext(void *context);

/**
 * Force to finalize the logger. The logger will clear its internal states,
 * in order to ensure not to use any freed contexts.
 * Logs after this call will be printed directly onto the standard output.
 */
void FinalizeLogger();

/** @} */

} // namespace ggadget

#endif  // GGADGET_LOGGER_H__
