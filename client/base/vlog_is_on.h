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

// TODO: VLOG is currently not supported on platforms other than
// windows, on which VLOG(x) will always be suppressed.

#ifndef BASE_VLOG_IS_ON_H_
#define BASE_VLOG_IS_ON_H_

#include <map>
#include <string>

#include "base/logging.h"

namespace ime_shared {

#if defined(OS_WINDOWS)
#define VLOG(verboselevel) LOG_IF(INFO, VLOG_IS_ON(verboselevel))
#else // !OS_WINDOWS
#define VLOG(verboselevel) LOG_IF(INFO, 0)
#endif  // OS_WINDOWS

#define VLOG_IF(verboselevel, condition) \
      LOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#define VLOG_EVERY_N(verboselevel, n) \
      LOG_IF_EVERY_N(INFO, VLOG_IS_ON(verboselevel), n)

#define VLOG_IF_EVERY_N(verboselevel, condition, n) \
      LOG_IF_EVERY_N(INFO, (condition) && VLOG_IS_ON(verboselevel), n)

#define DVLOG(verboselevel) DLOG_IF(INFO, VLOG_IS_ON(verboselevel))
#define DVLOG_IF(verboselevel, condition) \
  DLOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#if defined(OS_WINDOWS)
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#define VLOG_IS_ON(verboselevel) \
    ::ime_shared::VLog::IsOn(__WFILE__, verboselevel)

class VLog {
 public:
  static bool IsOn(const wchar_t* file, int level);
  static int GetVerboseLevel(const wchar_t* file);

  static void SetModule(const char* vmodule);
  static void SetModule(const wchar_t* vmodule);
  static void SetLevel(int32 vlevel);
  static void SetFromEnvironment(const wchar_t* vmodule_name,
                                 const wchar_t* vlevel_name);
 private:
  static void ConstructIntialModuleMap();
  typedef map<wstring, int> ModuleMap;
  static ModuleMap module_map_;
  static ModuleMap initial_module_map_;
};
#else  // !OS_WINDOWS
#define VLOG_IS_ON(verboselevel) ((verboselevel) == 0)
#endif  // OS_WINDOWS

}  // namespace ime_shared
#endif  // BASE_VLOG_IS_ON_H_
