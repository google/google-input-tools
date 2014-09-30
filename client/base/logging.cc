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

#include "base/logging.h"

#if defined(OS_WINDOWS)
#include <windows.h>
#elif defined(__ANDROID__)
#include <android/log.h>
#endif  // OS_WINDOWS
#include <stdio.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "base/commandlineflags.h"
#include "base/sysinfo.h"

#ifdef OS_WINDOWS
void OpenSyslog() {
  // Do nothing.
}
#define PATH_SEPERATOR '\\'
#endif

DEFINE_bool(logtostderr, BoolFromEnv("GOOGLE_LOGTOSTDERR", false),
            "log messages go to stderr instead of logfiles");

DEFINE_bool(log_fatal_error_to_stderr,
            BoolFromEnv("IME_LOG_FATAL_TO_STDERR", false),
            "If set true, does not pop up dialog when logging FATAL. "
            "Instead, logging the message to stderr.");

const char*const LogSeverityNames[NUM_SEVERITIES] = {
    "INFO", "WARNING", "ERROR", "FATAL"
};

#if defined OS_LINUX || OS_MACOSX || __ANDROID__ || OS_NACL
#include <pthread.h>
#include <stdint.h>

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define INFINITE -1
#define PATH_SEPERATOR '/'

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef void* HANDLE;
typedef pthread_mutex_t CRITICAL_SECTION;

HANDLE CreateMutexA(void *, bool, const char *) {
  return (void *)1;
}

void ReleaseMutex(HANDLE) {
  // Do nothing
}

void InitializeCriticalSection(CRITICAL_SECTION *cs) {
  pthread_mutex_init(cs, NULL);
}

void EnterCriticalSection(CRITICAL_SECTION *cs) {
  pthread_mutex_lock(cs);
}

void LeaveCriticalSection(CRITICAL_SECTION *cs) {
  pthread_mutex_unlock(cs);
}

void WaitForSingleObject(HANDLE handle, int time) {
  // Do nothing
}

bool IsDebuggerPresent() {
  return false;
}

void DebugBreak() {
  // Do nothing.
}

void OutputDebugStringA(const char *msg) {
#if defined(OS_NACL)
  fprintf(stdout, "%s", msg);
#else
  fprintf(stderr, "%s", msg);
#endif  // OS_NACL
}

void DeleteFileA(const char *file_name) {
#if !defined(OS_NACL)
  unlink(file_name);
#endif  // OS_NACL
}

#ifdef OS_LINUX
void GetModuleFileNameA(void *, char *buffer, int buf_len) {
  static char name[1024];
  static bool got_name = false;
  const char prefix[] = "Name:\t";
  const int prefix_len = sizeof(prefix) - 1;

  if (!got_name) {
    snprintf(name, sizeof(name), "/proc/%d/status", getpid());
    FILE *fp = fopen(name, "r");
    while (fp && fgets(name, sizeof(name), fp)) {
      if (!strncmp(prefix, name, prefix_len)) {
        memmove(reinterpret_cast<void *>(name),
                reinterpret_cast<void *>(name + prefix_len),
                strlen(name) + 1 - prefix_len);
        got_name = true;
        break;
      }
    }

    if (fp)
      fclose(fp);
  }

  if (got_name) {
    strncpy(buffer, name, buf_len);
  } else {
    const char *default_name = "Unknown";
    const int default_name_len = sizeof(default_name) - 1;
    strncpy(buffer, default_name, buf_len);
  }
}
#else
void GetModuleFileNameA(void *, char *buffer, int buf_len) {
  const char *default_name = "Unknown";
  const int default_name_len = sizeof(default_name) - 1;
  strncpy(buffer, default_name, buf_len);
}
#endif

int GetTempPathA(int buffer_len, char *buffer) {
  static const char temp_dir[] = "/tmp";
  static const int size = sizeof(temp_dir);
  if (buffer_len > size)
    strncpy(buffer, temp_dir, size);
  return size;
}

int GetCurrentProcessId() {
  return getpid();
}

int GetCurrentThreadId() {
  return GetTID();
}

time_t GetTickCount() {
  return time(NULL);
}

void DisplayDebugMessage(const std::string& str) {
  // Do nothing.
}

int GetCurrentProcess() {
  return 0;
}

void TerminateProcess(int, int) {
  abort();
}

void OpenSyslog() {
}

#define _T(x) x
#endif

namespace logging {

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "FATAL" };

int min_log_level = 0;
LogLockingState lock_log_file = DONT_LOCK_LOG_FILE;
LoggingDestination logging_destination = LOG_ONLY_TO_FILE;

// which log file to use? This is initialized by InitLogging or
// will be lazily initialized to the default value when it is
// first needed.
char log_file_name[MAX_PATH] = { 0 };

// this file is lazily opened and the handle may be NULL
FILE *log_file = NULL;

// what should be prepended to each message?
bool log_process_id = true;
bool log_thread_id = false;
bool log_timestamp = true;
bool log_tickcount = false;

// An assert handler override specified by the client to be called instead of
// the debug message dialog.
LogAssertHandlerFunction log_assert_handler = NULL;

// The critical section is used if log file locking is false. It helps us
// avoid problems with multiple threads writing to the log file at the same
// time.
bool initialized_critical_section = false;
CRITICAL_SECTION log_critical_section;

// When we don't use a critical section, we are using a global mutex. We
// need to do this because LockFileEx is not thread safe
HANDLE log_mutex = NULL;

void InitLogMutex() {
  if (!log_mutex) {
    // \ is not a legal character in mutex names so we replace \ with /
    std::string safe_name(log_file_name);
    std::replace(safe_name.begin(), safe_name.end(), '\\', '/');
    std::string t("Global\\");
    t.append(safe_name);
    log_mutex = ::CreateMutexA(NULL, FALSE, t.c_str());
  }
}

void InitLogging(const char* new_log_file, LoggingDestination logging_dest,
                 LogLockingState lock_log, OldFileDeletionState delete_old) {
  if (log_file) {
    // calling InitLogging twice or after some log call has already opened the
    // default log file will re-initialize to the new options
    fclose(log_file);
    log_file = NULL;
  }

  lock_log_file = lock_log;
  logging_destination = logging_dest;

  if (logging_destination == LOG_ONLY_TO_SYSTEM_DEBUG_LOG ||
      logging_destination == LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG) {
    OpenSyslog();
  }

  // ignore file options if logging is only to system
  if (logging_destination == LOG_ONLY_TO_SYSTEM_DEBUG_LOG)
    return;

  if (new_log_file && new_log_file[0]) {
    strncpy(log_file_name, new_log_file, MAX_PATH);
    log_file_name[MAX_PATH - 1] = ('\0');
    if (delete_old == DELETE_OLD_LOG_FILE)
      DeleteFileA(log_file_name);
  }

  if (lock_log_file == LOCK_LOG_FILE) {
    InitLogMutex();
  } else if (!initialized_critical_section) {
    // initialize the critical section
    InitializeCriticalSection(&log_critical_section);
    initialized_critical_section = true;
  }
}

void SetMinLogLevel(int level) {
  min_log_level = level;
}

void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount) {
  log_process_id = enable_process_id;
  log_thread_id = enable_thread_id;
  log_timestamp = enable_timestamp;
  log_tickcount = enable_tickcount;
}

void SetLogAssertHandler(LogAssertHandlerFunction handler) {
  log_assert_handler = handler;
}

// Called by logging functions to ensure that debug_file is initialized
// and can be used for writing. Returns false if the file could not be
// initialized. debug_file will be NULL in this case.
bool VerifyLogFileHandle() {
  if (log_file)
    return true;

  if (!log_file_name[0]) {
    // Try to put log file in temp path.
    uint32 ret = GetTempPathA(MAX_PATH, log_file_name);
    if (ret >= MAX_PATH) {
      // initialize the log file name to the same directory
      // with the binary file.
      GetModuleFileNameA(NULL, log_file_name, MAX_PATH);
      char* last_backslash = strrchr(log_file_name, PATH_SEPERATOR);
      if (last_backslash)
        last_backslash[1] = 0; // name now ends with the backslash
    }
    strcat(log_file_name, "/debug.log");
  }

  log_file = fopen(log_file_name, "a");
  if (log_file == NULL)
    return false;
  fseek(log_file, 0, SEEK_END);
  return true;
}

#ifdef OS_WINDOWS
// Displays a message box to the user with the error message in it. For
// Windows programs, it's possible that the message loop is messed up on
// a fatal error, and creating a MessageBox will cause that message loop
// to be run. Instead, we try to spawn another process that displays its
// command line. We look for "Debug Message.exe" in the same directory as
// the application. If it exists, we use it, otherwise, we use a regular
// message box.
void DisplayDebugMessage(const std::string& str) {
  if (str.empty())
    return;

  // look for the debug dialog program next to our application
  wchar_t prog_name[MAX_PATH + 1] = { 0 };
  GetModuleFileNameW(NULL, prog_name, MAX_PATH);
  wchar_t* backslash = wcsrchr(prog_name, '\\');
  if (backslash)
    backslash[1] = 0;
  wcsncat(prog_name, L"DebugMessage.exe", MAX_PATH - wcslen(prog_name));
  prog_name[MAX_PATH - 1] = L'\0';

  // stupid CreateProcess requires a non-const command line and may modify it.
  // We also want to use the wide string
  int charcount = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
  if (!charcount)
    return;
  scoped_ptr<wchar_t[]> cmdline(new wchar_t[charcount]);
  if (!MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                           -1, cmdline.get(), charcount))
    return;

  STARTUPINFO startup_info;
  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);

  PROCESS_INFORMATION process_info;
  if (CreateProcessW(prog_name, cmdline.get(), NULL, NULL, false, 0, NULL,
                     NULL, static_cast<LPSTARTUPINFO>(&startup_info),
                     &process_info)) {
    WaitForSingleObject(process_info.hProcess, INFINITE);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
  } else {
    // debug process broken, let's just do a message box
    MessageBoxW(NULL, cmdline.get(), L"Fatal error", MB_OK | MB_ICONHAND);
  }
}
#endif

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       int ctr)
    : severity_(severity) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, const CheckOpString& result)
    : severity_(LOG_FATAL) {
  Init(file, line);
  stream_ << "Check failed: " << (*result.str_);

#if defined(OS_NACL)
  std::string info;
  stream_ >> info;
  fprintf(stdout, "%s\n", info.c_str());
#endif  // OS_NACL
}

LogMessage::LogMessage(const char* file, int line)
     : severity_(LOG_INFO) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity) {
  Init(file, line);
}

// writes the common header info to the stream
void LogMessage::Init(const char* file, int line) {
  // When PROJECT_ROOT is defined, Shrink the file name to be relative to the
  // project root.
#ifdef PROJECT_ROOT
  char* project_root = PROJECT_ROOT;
  const char* project_prefix = strstr(file, project_root);
  if (project_prefix) file += strlen(project_root) + 1;
#else
  const char* last_slash = strrchr(file, '\\');
  if (last_slash) file = last_slash + 1;
#endif

  // TODO: It might be nice if the columns were fixed width.

  stream_ <<  '[';
  if (log_process_id)
    stream_ << GetCurrentProcessId() << ':';
  if (log_thread_id)
    stream_ << GetCurrentThreadId() << ':';
  if (log_timestamp) {
    time_t t = time(NULL);
    struct tm* tm_time = localtime(&t);
    stream_ << std::setfill('0')
            << std::setw(2) << 1 + tm_time->tm_mon
            << std::setw(2) << tm_time->tm_mday
            << '/'
            << std::setw(2) << tm_time->tm_hour
            << std::setw(2) << tm_time->tm_min
            << std::setw(2) << tm_time->tm_sec
            << ':';
  }
  if (log_tickcount)
    stream_ << GetTickCount() << ':';
  stream_ << log_severity_names[severity_] << ":"
          << file << "(" << line << ")] ";

#if defined(OS_NACL)
  std::string info;
  stream_ >> info;
  fprintf(stdout, "%s\n", info.c_str());
#endif  // OS_NACL
}

LogMessage::~LogMessage() {
  // TODO: modify the macros so that nothing is executed when the log
  // level is too high or there is
  if (severity_ < min_log_level)
    return;
  std::string str_newline(stream_.str());
  str_newline.append("\r\n");

#if defined(__ANDROID__)
  int android_log_severity = ANDROID_LOG_INFO;
  if (severity_ == logging::LOG_WARNING) {
    android_log_severity = ANDROID_LOG_WARN;
  } else if (severity_ == logging::LOG_ERROR ||
             severity_ == logging::LOG_FATAL) {
    android_log_severity = ANDROID_LOG_ERROR;
  }
  __android_log_print(android_log_severity, "t13n_shared_engine",
                      str_newline.c_str());
#elif defined(OS_NACL)
  fprintf(stdout, "%s", str_newline.c_str());
#else
  if (logging_destination != LOG_ONLY_TO_FILE)
    OutputDebugStringA(str_newline.c_str());

  // write to log file
  if (logging_destination != LOG_ONLY_TO_SYSTEM_DEBUG_LOG &&
      VerifyLogFileHandle()) {
    // we can have multiple threads and/or processes, so try to prevent them
    // from clobbering each other's writes
    if (lock_log_file == LOCK_LOG_FILE) {
      // Ensure that the mutex is initialized in case the client app did not
      // call InitLogging. This is not thread safe. See below
      InitLogMutex();

      ::WaitForSingleObject(log_mutex, INFINITE);
    } else {
      // use the critical section
      if (!initialized_critical_section) {
        // The client app did not call InitLogging, and so the critical section
        // has not been created. We do this on demand, but if two threads try to
        // do this at the same time, there will be a race condition to create
        // the critical section. This is why InitLogging should be called from
        // the main thread at the beginning of execution.
        InitializeCriticalSection(&log_critical_section);
        initialized_critical_section = true;
      }
      EnterCriticalSection(&log_critical_section);
    }

    fseek(log_file, 0, SEEK_END);
    int num_written = fprintf(log_file, "%s", str_newline.c_str());

    if (lock_log_file == LOCK_LOG_FILE) {
      ReleaseMutex(log_mutex);
    } else {
      LeaveCriticalSection(&log_critical_section);
    }
  }

  if (severity_ == LOG_FATAL) {
    fflush(log_file);
    // display a message or break into the debugger on a fatal error
    if (::IsDebuggerPresent()) {
      DebugBreak();
    } else {
      if (log_assert_handler) {
        log_assert_handler(std::string(stream_.str()));
      } else {
        if (!FLAGS_log_fatal_error_to_stderr) {
          // Don't use the string with the newline, get a fresh version to send
          // to the debug message process
          DisplayDebugMessage(std::string(stream_.str()));
        } else {
          fprintf(stderr, "%s\n", stream_.str().c_str());
          fflush(stderr);
        }
        TerminateProcess(GetCurrentProcess(), 1);
      }
    }
  }
#endif  // __ANDROID__
}

void CloseLogFile() {
  if (!log_file)
    return;

  fclose(log_file);
  log_file = NULL;
}

// Helper functions for string comparisons.
#define DEFINE_CHECK_STROP_IMPL(name, func, expected) \
  string* Check##func##expected##Impl(const char* s1, const char* s2, \
                                      const char* names) { \
    bool equal = s1 == s2 || (s1 && s2 && !func(s1, s2)); \
    if (equal == expected) { \
      return NULL; \
    } else { \
      stringstream ss; \
      ss << #name " failed: " << names << " (" << s1 << " vs. " << s2 << ")"; \
      return new string(ss.str()); \
    } \
  }
DEFINE_CHECK_STROP_IMPL(CHECK_STREQ, strcmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRNE, strcmp, false)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASEEQ, strcasecmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASENE, strcasecmp, false)
#undef DEFINE_CHECK_STROP_IMPL

} // namespace logging

#ifdef OS_WINDOWS
std::ostream& operator<<(std::ostream& out, const wchar_t* wstr) {
  if (!wstr || !wstr[0])
    return out;

  // compute the length of the buffer we'll need
  int charcount = WideCharToMultiByte(CP_UTF8, 0, wstr, -1,
                                      NULL, 0, NULL, NULL);
  if (charcount == 0)
    return out;

  // convert
  scoped_ptr<char[]> buf(new char[charcount]);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf.get(), charcount, NULL, NULL);
  return out << buf.get();
}
#endif  // OS__WINDOWS

void GWQStatusMessage(const char* msg) {
}
