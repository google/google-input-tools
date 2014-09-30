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

#ifndef BASE_LOG_SEVERITY_H_
#define BASE_LOG_SEVERITY_H_

#include "base/port.h"
#include "base/commandlineflags_declare.h"

// Variables of type LogSeverity are widely taken to lie in the range
// [0, NUM_SEVERITIES-1].  Be careful to preserve this assumption if
// you ever need to change their values or add a new severity.
typedef int LogSeverity;

namespace base_logging {

// Severity-level definitions.  These are C++ constants representing
// the four severity levels INFO through FATAL.
//
// These severity constants can be used outside of a LOG statement, or
// in a variable-severity statement such as:
//
//   LOG(LEVEL(is_error ? base_logging::ERROR : base_logging::WARNING)) << ...;
//
// Another common use of these symbols is:
//
//   using base_logging::INFO;
//   testing::ScopedMockLog log;
//   EXPECT_CALL(log, Log(INFO, testing::_, "..."));
const LogSeverity INFO = 0;
const LogSeverity WARNING = 1;
const LogSeverity ERROR = 2;
const LogSeverity FATAL = 3;
const int NUM_SEVERITIES = 4;

}  // namespace base_logging

// TODO(jmacd): Replace uses of ::INFO, ::WARNING, ::ERROR, and
// ::FATAL with the base_logging members.  (A benefit of this change
// is that it avoids namespace collisions with ~1000 existing enums
// named "INFO", "WARNING", "ERROR", and "FATAL" in google3 .proto
// files alone.)
const int INFO = base_logging::INFO;
const int WARNING = base_logging::WARNING;
const int ERROR = base_logging::ERROR;
const int FATAL = base_logging::FATAL;
const int NUM_SEVERITIES = base_logging::NUM_SEVERITIES;

// An array containing "INFO", "WARNING", "ERROR", "FATAL".
extern const char* const LogSeverityNames[NUM_SEVERITIES];

// Some flags needed for VLOG and RAW_VLOG
DECLARE_int32(v);
DECLARE_bool(silent_init);

// NDEBUG usage helpers related to (RAW_)DCHECK:
//
// DEBUG_MODE is for small !NDEBUG uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of substantially more verbose
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
#ifdef NDEBUG
const bool DEBUG_MODE = false;
#else
const bool DEBUG_MODE = true;
#endif

namespace base_logging {

// Use base_logging::DFATAL to refer to the LogSeverity level
// "DFATAL" outside of a log statement.
const LogSeverity DFATAL = DEBUG_MODE ? FATAL : ERROR;

// If "s" is less than base_logging::INFO, returns base_logging::INFO.
// If "s" is greater than base_logging::FATAL, returns
// base_logging::ERROR.  Otherwise, returns "s".
LogSeverity NormalizeSeverity(LogSeverity s);

}  // namespace base_logging

namespace base {
namespace internal {
// TODO(jmacd): Remove this (one use in webserver/...)
inline LogSeverity NormalizeSeverity(LogSeverity s) {
  return base_logging::NormalizeSeverity(s);
}
}  // namespace internal
}  // namespace base

// Special-purpose macros for use OUTSIDE //base/logging.h.  These are
// to support hygienic forms of other logging macros (e.g.,
// raw_logging.h).
#define BASE_LOG_SEVERITY_INFO (::base_logging::INFO)
#define BASE_LOG_SEVERITY_WARNING (::base_logging::WARNING)
#define BASE_LOG_SEVERITY_ERROR (::base_logging::ERROR)
#define BASE_LOG_SEVERITY_FATAL (::base_logging::FATAL)
#define BASE_LOG_SEVERITY_DFATAL (::base_logging::DFATAL)
#define BASE_LOG_SEVERITY_QFATAL (::base_logging::QFATAL)

#define BASE_LOG_SEVERITY_LEVEL(severity) \
  (::base_logging::NormalizeSeverity(severity))

// TODO(jmacd): Remove this once DFATAL_LEVEL is eliminated.
#define BASE_LOG_SEVERITY_DFATAL_LEVEL (::base_logging::DFATAL)

#endif  // BASE_LOG_SEVERITY_H_
