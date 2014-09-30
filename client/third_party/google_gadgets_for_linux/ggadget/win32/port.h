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

/*
  This file includes some macros, typedefs and header files to port ggadget
  to windows.
*/
#ifndef GGADGET_WIN32_PORT_H__
#define GGADGET_WIN32_PORT_H__

#if defined(OS_WIN)  // only works on windows

#include <io.h>
#include <direct.h>
#include <process.h>
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <functional>
// VC10 and above have this header.
#if defined(_MSC_VER) && _MSC_VER >= 1600
#include <stdint.h>
#endif  // _MSC_VER >= 1600

#include <ggadget/build_config.h>
#include <ggadget/win32/sysdeps.h>

// UINTxx is defined in windows.h
typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;

// Defined in <BaseTsd.h>, included in <windows.h>.
typedef SSIZE_T ssize_t;

// VC10 has already defined this.
#if defined(_MSC_VER) && _MSC_VER < 1600
#define UINT64_C(v) v##UI64
#define INT64_C(v) v##I64
#endif  // _MSC_VER < 1600

#define strcasecmp _stricmp
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#define strncasecmp _strnicmp

#define strtoll  _strtoi64
#define strtoull _strtoui64

#if defined(OS_WIN)
// Undefines windows macro max/min to avoid conflicting with std::min/max.
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

/*
  IO
*/
// These two macros are used to check struct stat::st_mode
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)  // is directory
#define S_ISREG(m) (((m) & S_IFREG) == S_IFREG)  // is regular file

// These four macros are used for the second parameter of function access
#define R_OK 0x04  // read permissions
#define X_OK 0x04  // execuation permissions
#define F_OK 0x00  // existence
#define W_OK 0x02  // write permissions

// From stat.h
typedef unsigned int mode_t;

/*
  Process
*/
#define getpid _getpid

/*
  Math
*/
#ifndef M_PI
#define M_PI 3.1415926535898
#endif  // M_PI
#ifndef M_PI_2
#define M_PI_2 (M_PI/2)
#endif  // M_PI_2

// Returns the closest integer around the real number v
#define round(v) floor((v)+0.5)

// Returns the remainder of numerator divided by denominater.
#define remainder(numerator, denominater) fmod((numerator), (denominater))

/*
  Time
*/
// Returns the number of seconds and extra microseconds since 1/1/1970 00:00
// Parameter tz is not used.
namespace ggadget {
inline int gettimeofday(struct timeval *tv, struct timezone *tz) {
  (void)tz;
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  uint64_t t = file_time.dwHighDateTime;
  t = (t<<32) | file_time.dwLowDateTime;
  t -= 116444736000000000LL;  // because file time is since 1/1/1601 00:00
  tv->tv_sec = static_cast<uint32_t>(t / 10000000);  // t is in 100-nanosecond
  tv->tv_usec = static_cast<uint32_t>(t % 10000000 / 10);  // extra microseconds
  return 0;
}
}  // namespace ggadget
#endif  // OS_WIN
#endif  // GGADGET_WIN32_PORT_H__
