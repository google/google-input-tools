/*
  Copyright 2014 Google Inc.

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

#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <string>
#include <cstring>
#include <ios>
#include <sstream>

#include "base/basictypes.h"
#include "base/log_severity.h"
#include "base/scoped_ptr.h"
#include "base/vlog_is_on.h"

#if defined(_WINDOWS) && !defined(OS_WINDOWS)
#define OS_WINDOWS
#endif

// This file provides Google3-like logging facility for Windows client apps. It
// is mostly a reorganized and cleaned up version of google3/base/logging.h
//
// Differences from Google3:
//
// - No support for PLOG (not very useful for Windows and more work to
//   implement).
//
// - CHECK is supported. If ENABLE_CHECK is defined or compile under debug mode,
//   CHECK failure will result in crash. If under release mode and ENABLE_CHECK
//   is not defined, CHECK will be compiled to if statement but does nothing
//   when the check result is not expected.
//
//   Using DCHECK is encouraged. Shipping code with a check in it is broken.
//   We may also want to remove LOG() for the same reason. But there are some
//   things we do want to log in release mode, and without module support, this
//   is the best we have right now. Ideally, we would have the default be no
//   logging, but for critical things, different modules could be enabled for
//   logging in release modes.
//
// - No support for *_EVERY_N. This addes some extra complexity to the logging
//   and stream implementations, and shouldn't be too common in client code.
//
// - Most of the stuff here in the logging namespace and some of the macros
//   that are just used internally to the file are undefined at the end.
//
// - The numeric log levels start with LOG_ to avoid collisions. You can still
//   use LOG(ERROR), etc.
//
// - The LogMessage class is totally rewritten, and we don't support "output
//   methods" which seem to be unnecessary for client apps purposes.
//
// - Instead of using command line arguments, we supply functions for the
//   client app to call to configure the logging environment. This allows
//   more customized methods for controlling logging.
//
// - The implementation of LogMessage is totally new and vastly simplified as
//   compared to Google3, which does a lot more, work, for example, to make
//   PLOG work.
//
// The macros that remain have not changed their definition from Google3.
//
//
// Optional message capabilities
// -----------------------------
// Assertion failed messages and fatal errors are displayed in a dialog box
// before the application exits. However, running this UI creates a message
// loop, which causes application messages to be processed and potentially
// dispatched to existing application windows. Since the application is in a
// bad state when this assertion dialog is displayed, these messages may not
// get processed and hang the dialog, or the application might go crazy.
//
// Therefore, it can be beneficial to display the error dialog in a separate
// process from the main application. When the logging system needs to display
// a fatal error dialog box, it will look for a program called
// "DebugMessage.exe" in the same directory as the application executable. It
// will run this application with the message as the command line, and will
// not include the name of the application as is traditional for easier
// parsing.
//
// The code for DebugMessage.exe is only one line. In WinMain, do:
//   MessageBox(NULL, GetCommandLineW(), L"Fatal Error", 0);
//
// If DebugMessage.exe is not found, the logging code will use a normal
// MessageBox, potentially causing the problems discussed above.

// Instructions
// ------------
//
// Make a bunch of macros for logging.  The way to log things is to stream
// things to LOG(<a particular severity level>).  E.g.,
//
//   LOG(INFO) << "Found " << num_cookies << " cookies";
//
// You can also do conditional logging:
//
//   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// There are also "debug mode" logging macros like the ones above:
//
//   DLOG(INFO) << "Found cookies";
//
//   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// All "debug mode" logging is compiled away to nothing for non-debug mode
// compiles.  LOG_IF and development flags also work well together
// because the code can be compiled away sometimes.
//
// We also have
//
//   LOG_ASSERT(assertion);
//   DLOG_ASSERT(assertion);
//
// which is syntactic sugar for {,D}LOG_IF(FATAL, assert fails) << assertion;
//
// We also override the standard 'assert' to use 'DLOG_ASSERT'.
//
// The supported severity levels for macros that allow you to specify one
// are (in increasing order of severity) INFO, WARNING, ERROR, and FATAL.
//
// Very important: logging a message at the FATAL severity level causes
// the program to terminate (after the message is logged).

namespace logging {

// Where to record logging output? A flat file and/or system debug log via
// OutputDebugString. Defaults to LOG_ONLY_TO_FILE.
enum LoggingDestination { LOG_ONLY_TO_FILE, 
                          LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
                          LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG };

// Indicates that the log file should be locked when being written to.
// Often, there is no locking, which is fine for a single threaded program.
// If logging is being done from multiple threads or there can be more than
// one process doing the logging, the file should be locked during writes to
// make each log outut atomic. Other writers will block.
//
// All processes writing to the log file must have their locking set for it to
// work properly. Defaults to DONT_LOCK_LOG_FILE.
enum LogLockingState { LOCK_LOG_FILE, DONT_LOCK_LOG_FILE };

// On startup, should we delete or append to an existing log file (if any)?
// Defaults to APPEND_TO_OLD_LOG_FILE.
enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };

// Sets the log file name and other global logging state. Calling this function
// is recommended, and is normally done at the beginning of application init.
// If you don't call it, all the flags will be initialized to their default
// values, and there is a race condition that may leak a critical section
// object if two threads try to do the first log at the same time.
// See the definition of the enums above for descriptions and default values.
//
// The default log file is initialized to "debug.log" in the application
// directory. You probably don't want this, especially since the program
// directory may not be writable on an enduser's system.
//
// Sets log_file to NULL for only modifying other settings without changing
// the default log file path.
void InitLogging(const char* log_file, LoggingDestination logging_dest,
                 LogLockingState lock_log, OldFileDeletionState delete_old);

// Sets the log level. Anything at or above this level will be written to the
// log file/displayed to the user (if applicable). Anything below this level
// will be silently ignored. The log level defaults to 0 (everything is logged)
// if this function is not called.
void SetMinLogLevel(int level);

// Sets the common items you want to be prepended to each log message.
// process and thread IDs default to off, the timestamp defaults to on.
// If this function is not called, logging defaults to writing the timestamp
// only.
void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount);

// Sets the Log Assert Handler that will be used to notify of check failures.
// The default handler shows a dialog box, however clients can use this 
// function to override with their own handling (e.g. a silent one for Unit
// Tests)
typedef void (*LogAssertHandlerFunction)(const std::string& str);
void SetLogAssertHandler(LogAssertHandlerFunction handler);

// Google3 defined these without the LOG prefix, but this can cause
// conflicts. For example, the Windows SDK defines ERROR to be some number,
// which causes problems here. You can still use LOG(ERROR), however (see
// below).
typedef int LogSeverity;
const LogSeverity LOG_INFO = 0;
const LogSeverity LOG_WARNING = 1;
const LogSeverity LOG_ERROR = 2;
const LogSeverity LOG_FATAL = 3;
const LogSeverity LOG_NUM_SEVERITIES = 4;

// LOG_DFATAL is LOG_FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
const LogSeverity LOG_DFATAL = LOG_ERROR;
#else
const LogSeverity LOG_DFATAL = LOG_FATAL;
#endif

class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) { }
};

// A few definitions of macros that don't generate much code. These are used
// by LOG() and LOG_IF, etc. Since these are used all over our code, it's
// better to have compact code for these operations.
#define COMPACT_GOOGLE_LOG_INFO \
  logging::LogMessage(__FILE__, __LINE__)
#define COMPACT_GOOGLE_LOG_WARNING \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_WARNING)
#define COMPACT_GOOGLE_LOG_ERROR \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR)
#define COMPACT_GOOGLE_LOG_FATAL \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_FATAL)
#define COMPACT_GOOGLE_LOG_DFATAL \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_DFATAL)

// wingdi.h defines ERROR to be 0. When we call LOG(ERROR), it gets
// substituted with 0, and it expands to COMPACT_GOOGLE_LOG_0. To allow us
// to keep using this syntax, we define this macro to do the same thing
// as COMPACT_GOOGLE_LOG_ERROR, and also define ERROR the same way that
// the Windows SDK does for consistency.
#if defined(OS_WINDOWS)
#define ERROR 0
#define COMPACT_GOOGLE_LOG_0 \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR)
#endif  // OS_WINDOWS

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token COMPACT_GOOGLE_LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of LogMessage which seems to avoid the problem.

#define LOG(severity) COMPACT_GOOGLE_LOG_ ## severity.stream()
#define SYSLOG(severity) LOG(severity)

#define LOG_IF(severity, condition) \
  !(condition) ? (void)0 : logging::LogMessageVoidify() & LOG(severity)
#define SYSLOG_IF(severity, condition) LOG_IF(severity, condition)

#define LOG_ASSERT(condition)  \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "
#define SYSLOG_ASSERT(condition) \
  SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

// A container for a string pointer which can be evaluated to a bool -
// true iff the pointer is NULL.
struct CheckOpString {
  CheckOpString(std::string* str) : str_(str) { }
  // No destructor: if str_ is non-NULL, we're about to LOG(FATAL),
  // so there's no point in cleaning up str_.
  operator bool() const { return str_ != NULL; }
  std::string* str_;
};

// Build the error message string.  This is separate from the "Impl"
// function template because it is not performance critical and so can
// be out of line, while the "Impl" code should be inline.
template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
  std::stringstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  return new std::string(ss.str());
}

extern std::string* MakeCheckOpStringIntInt(int v1, int v2, const char* names);

template<int, int>
std::string* MakeCheckOpString(const int& v1, const int& v2,
                               const char* names) {
  return MakeCheckOpStringIntInt(v1, v2, names);
}

// Helper functions for INTERNAL_CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <class t1, class t2> \
  inline std::string* Check##name##Impl(const t1& v1, const t2& v2, \
                                        const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  } \
  inline std::string* Check##name##Impl(int v1, int v2, const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  }
DEFINE_CHECK_OP_IMPL(EQ, ==)
DEFINE_CHECK_OP_IMPL(NE, !=)
DEFINE_CHECK_OP_IMPL(LE, <=)
DEFINE_CHECK_OP_IMPL(LT, < )
DEFINE_CHECK_OP_IMPL(GE, >=)
DEFINE_CHECK_OP_IMPL(GT, > )
#undef DEFINE_CHECK_OP_IMPL

// Helper macro for binary operators.
// Don't use this macro directly in your code, use CHECK_EQ or DCHECK_EQ et al
// below.
#define INTERNAL_CHECK_OP(name, op, val1, val2)  \
  while (logging::CheckOpString _result = \
         logging::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
    logging::LogMessage(__FILE__, __LINE__, _result).stream()

// Helper functions for string comparisons.
// To avoid bloat, the definitions are in logging.cc.
#define DECLARE_CHECK_STROP_IMPL(func, expected) \
  std::string* Check##func##expected##Impl(const char* s1, \
                                           const char* s2, \
                                           const char* names);
DECLARE_CHECK_STROP_IMPL(strcmp, true)
DECLARE_CHECK_STROP_IMPL(strcmp, false)
DECLARE_CHECK_STROP_IMPL(_stricmp, true)
DECLARE_CHECK_STROP_IMPL(_stricmp, false)
#undef DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ or DCHECK_STREQ
// et al below.
#define INTERNAL_CHECK_STROP(func, op, expected, s1, s2) \
  while (logging::CheckOpString _result = \
      logging::Check##func##expected##Impl((s1), (s2), \
                                           #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_

// Allows CHECK or DCHECK to be followed by a sequence of stream parameters
// if compiled to no-op.
class NotLog {
 public:
  NotLog& operator <<(const char* value) { return *this; }
  NotLog& operator <<(const wchar_t* value) { return *this; }
  NotLog& operator <<(const std::string& value) { return *this; }
#if !defined(__ANDROID__) && !defined(OS_LINUX) && !defined(OS_MACOSX)
  NotLog& operator <<(const wstring& value) { return *this; }
#endif
  NotLog& operator <<(size_t value) { return *this; }
  NotLog& operator <<(void* value) { return *this; }
  NotLog& operator <<(std::ios_base& (*)(std::ios_base&)) { return *this; }
};
#define NOT_LOG logging::NotLog()

#if defined(ENABLE_CHECK) || !defined(NDEBUG)

#define CHECK(condition) \
    LOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "

#define CHECK_ERR(invocation) \
    LOG_IF(FATAL, (invocation) == -1)

// Equality/Inequality checks - compare two values, and log a LOG_FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(ostream, ...) defined.
//
// You may append to the error message like so:
//   CHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   INTERNAL_CHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.
#define CHECK_EQ(val1, val2) INTERNAL_CHECK_OP(EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) INTERNAL_CHECK_OP(NE, !=, val1, val2)
#define CHECK_LE(val1, val2) INTERNAL_CHECK_OP(LE, <=, val1, val2)
#define CHECK_LT(val1, val2) INTERNAL_CHECK_OP(LT, < , val1, val2)
#define CHECK_GE(val1, val2) INTERNAL_CHECK_OP(GE, >=, val1, val2)
#define CHECK_GT(val1, val2) INTERNAL_CHECK_OP(GT, > , val1, val2)
#define CHECK_NOTNULL(val) \
    CheckNotNull(__FILE__, __LINE__, "'" #val "' must be not NULL", (val))

// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. CHECK_STREQ(Foo().c_str(), Bar().c_str())).
#define CHECK_STREQ(s1, s2) INTERNAL_CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) INTERNAL_CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) INTERNAL_CHECK_STROP(_stricmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) \
    INTERNAL_CHECK_STROP(_stricmp, !=, false, s1, s2)

#define CHECK_INDEX(I,A) CHECK(I < (sizeof(A)/sizeof(A[0])))
#define CHECK_BOUND(B,A) CHECK(B <= (sizeof(A)/sizeof(A[0])))

#else  // !(defined(ENABLE_CHECK) || !defined(NDEBUG))
// Note that we could not simply compile them to no-op since expression embedded
// in CHECK statements may be a function call etc. which should be executed.

#define CHECK(condition) if (!(condition)) NOT_LOG
#define CHECK_ERR(invocation) if ((invocation) != -1) NOT_LOG
#define CHECK_EQ(val1, val2) if (!((val1) == (val2))) NOT_LOG
#define CHECK_NE(val1, val2) if (!((val1) != (val2))) NOT_LOG
#define CHECK_LE(val1, val2) if (!((val1) <= (val2))) NOT_LOG
#define CHECK_LT(val1, val2) if (!((val1) < (val2))) NOT_LOG
#define CHECK_GE(val1, val2) if (!((val1) >= (val2))) NOT_LOG
#define CHECK_GT(val1, val2) if (!((val1) > (val2))) NOT_LOG
#define CHECK_NOTNULL(val) if ((val) == NULL) NOT_LOG
#define CHECK_STREQ(str1, str2) if (strcmp((str1), (str2)) != 0) NOT_LOG
#define CHECK_STRNE(str1, str2) if (strcmp((str1), (str2)) == 0) NOT_LOG
#define CHECK_STRCASEEQ(str1, str2) if (_stricmp((str1), (str2)) != 0) NOT_LOG
#define CHECK_STRCASENE(str1, str2) if (_stricmp((str1), (str2)) == 0) NOT_LOG
#define CHECK_INDEX(I,A) if (!(I < (sizeof(A)/sizeof(A[0])))) NOT_LOG
#define CHECK_BOUND(B,A) if (!(B <= (sizeof(A)/sizeof(A[0])))) NOT_LOG

#endif  // defined(ENABLE_CHECK) || !defined(NDEBUG)

// DCHECK does CHECK work on debug build, but compile to no-op on opt build.
#ifndef NDEBUG

#define DLOG(severity) LOG(severity)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)

#define DCHECK(condition) \
    LOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "

#define DCHECK_EQ(val1, val2) INTERNAL_CHECK_OP(EQ, ==, val1, val2)
#define DCHECK_NE(val1, val2) INTERNAL_CHECK_OP(NE, !=, val1, val2)
#define DCHECK_LE(val1, val2) INTERNAL_CHECK_OP(LE, <=, val1, val2)
#define DCHECK_LT(val1, val2) INTERNAL_CHECK_OP(LT, < , val1, val2)
#define DCHECK_GE(val1, val2) INTERNAL_CHECK_OP(GE, >=, val1, val2)
#define DCHECK_GT(val1, val2) INTERNAL_CHECK_OP(GT, > , val1, val2)
#define DCHECK_STREQ(s1, s2) INTERNAL_CHECK_STROP(strcmp, ==, true, s1, s2)
#define DCHECK_STRNE(s1, s2) INTERNAL_CHECK_STROP(strcmp, !=, false, s1, s2)
#define DCHECK_STRCASEEQ(s1, s2) \
    INTERNAL_CHECK_STROP(_stricmp, ==, true, s1, s2)
#define DCHECK_STRCASENE(s1, s2) \
    INTERNAL_CHECK_STROP(_stricmp, !=, false, s1, s2)
#define DCHECK_INDEX(I,A) DCHECK(I < (sizeof(A)/sizeof(A[0])))
#define DCHECK_BOUND(B,A) DCHECK(B <= (sizeof(A)/sizeof(A[0])))

#else  // NDEBUG

#define DLOG(severity) \
    true ? (void)0 : logging::LogMessageVoidify() & LOG(severity)
#define DLOG_IF(severity, condition) \
    true ? (void)0 : logging::LogMessageVoidify() & LOG(severity)
#define DLOG_ASSERT(condition) while (false) LOG_IF(INFO, condition)

#define DCHECK(condition) while (false) NOT_LOG
#define DCHECK_EQ(val1, val2) while (false) NOT_LOG
#define DCHECK_NE(val1, val2) while (false) NOT_LOG
#define DCHECK_LE(val1, val2) while (false) NOT_LOG
#define DCHECK_LT(val1, val2) while (false) NOT_LOG
#define DCHECK_GE(val1, val2) while (false) NOT_LOG
#define DCHECK_GT(val1, val2) while (false) NOT_LOG
#define DCHECK_STREQ(str1, str2) while (false) NOT_LOG
#define DCHECK_STRNE(str1, str2) while (false) NOT_LOG
#define DCHECK_STRCASEEQ(str1, str2) while (false) NOT_LOG
#define DCHECK_STRCASENE(str1, str2) while (false) NOT_LOG

#endif  // NDEBUG

// Debug-only checking. Executed in NDEBUG mode.
#define VERIFY(condition) \
  do { \
    bool result = (condition); \
    DCHECK(result); \
  } while (false)

#define NOTREACHED() DCHECK(false)

// Redefine the standard assert to use our nice log files
#undef assert
#define assert(x) DLOG_ASSERT(x)

// This class more or less represents a particular log message.  You
// create an instance of LogMessage and then stream stuff to it.
// When you finish streaming to it, ~LogMessage is called and the
// full message gets streamed to the appropriate destination.
//
// You shouldn't actually use LogMessage's constructor to log things,
// though.  You should use the LOG() macro (and variants thereof)
// above.
class LogMessage {
 public:
  LogMessage(const char* file, int line, LogSeverity severity, int ctr);

  // Two special constructors that generate reduced amounts of code at
  // LOG call sites for common cases.
  //
  // Used for LOG(INFO): Implied are:
  // severity = LOG_INFO, ctr = 0
  //
  // Using this constructor instead of the more complex constructor above
  // saves a couple of bytes per call site.
  LogMessage(const char* file, int line);

  // Used for LOG(severity) where severity != INFO.  Implied
  // are: ctr = 0
  //
  // Using this constructor instead of the more complex constructor above
  // saves a couple of bytes per call site.
  LogMessage(const char* file, int line, LogSeverity severity);

  // A special constructor used for check failures.
  // Implied severity = LOG_FATAL
  LogMessage(const char* file, int line, const CheckOpString& result);

  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  void Init(const char* file, int line);

  LogSeverity severity_;
  std::stringstream stream_;

  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// A non-macro interface to the log facility; (useful
// when the logging level is not a compile-time constant).
inline void LogAtLevel(int const log_level, std::string const &msg) {
  LogMessage(__FILE__, __LINE__, log_level).stream() << msg;
}

// Closes the log file explicitly if open.
// NOTE: Since the log file is opened as necessary by the action of logging
//       statements, there's no guarantee that it will stay closed
//       after this call.
void CloseLogFile();

} // namespace Logging

inline void CHECK_NEAR(double val1, double val2, double margin) {
  CHECK_LE(val1, val2 + margin);
  CHECK_GE(val1, val2 - margin);
}

inline void CHECK_DOUBLE_EQ(double val1, double val2) {
  CHECK_NEAR(val1, val2, 0.000000000000001L);
}

template <typename T>
T& CheckNotNullCommon(const char* file, int line, const char* names, T& t) {
  if (t == NULL) {
    logging::LogMessage(file, line, logging::LOG_FATAL);
  }
  return t;
}

template <typename T>
T* CheckNotNull(const char* file, int line, const char* names, T* t) {
  return CheckNotNullCommon(file, line, names, t);
}

template <typename T>
T& CheckNotNull(const char* file, int line, const char* names, T& t) {
  return CheckNotNullCommon(file, line, names, t);
}

template <typename T>
const T& CheckNotNull(const char* file, int line, const char* names,
                      const T& t) {
  return CheckNotNullCommon(file, line, names, t);
}

#ifdef _WINDOWS
// These functions are provided as a convenience for logging, which is where we
// use streams (it is against Google style to use streams in other places). It
// is designed to allow you to emit non-ASCII Unicode strings to the log file,
// which is normally ASCII. It is relatively slow, so try not to use it for
// common cases. Non-ASCII characters will be converted to UTF-8 by these
// operators.
std::ostream& operator<<(std::ostream& out, const wchar_t* wstr);
inline std::ostream& operator<<(std::ostream& out, const std::wstring& wstr) {
  return out << wstr.c_str();
}
#endif

// Interface ported from //depot/google3/base/logging.h with empty
// implementation.
void GWQStatusMessage(const char* msg);

// TODO(zyb): Implementation.
#define LOG_STRING(severity, outvec) LOG(severity)

#endif  // BASE_LOGGING_H_
