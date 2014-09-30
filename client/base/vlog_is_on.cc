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

#include "base/vlog_is_on.h"

#if defined(OS_WINDOWS)
#include <shlwapi.h>
#include <strsafe.h>
#endif  // OS_WINDOWS
#include <vector>

#include "base/basictypes.h"
#include "base/commandlineflags.h"
#include "base/port.h"
#if defined(OS_WINDOWS)
#include "base/string_utils_win.h"
#endif  // OS_WINDOWS

static const int kDefaultVLevel = 1;

// The format of vmodule is like "module1=1,module2=2,module3=0,module4".
// A verbose level 0 means no logging for that modual.
// If no level number is provided, like module4, we will use default level 1.
// For the modules not listed in vmodule flags, FLAGS_v will be used.
DEFINE_string(vmodule, "", "verbose logging modules.");
DEFINE_int32(v, kDefaultVLevel, "Show all VLOG(m) messages for m <= this.");

namespace ime_shared {

#if defined(OS_WINDOWS)
static const wchar_t kSeparator[] = L",";
static const wchar_t kEqualSign = L'=';

VLog::ModuleMap VLog::module_map_;
VLog::ModuleMap VLog::initial_module_map_;

bool VLog::IsOn(const wchar_t* file, int level) {
  return GetVerboseLevel(file) >= level;
}

int VLog::GetVerboseLevel(const wchar_t* file) {
  if (FLAGS_vmodule.empty()) return FLAGS_v;
  if (initial_module_map_.empty()) ConstructIntialModuleMap();

  wchar_t buffer[MAX_PATH];
  StringCchCopy(buffer, MAX_PATH, file);
  PathRemoveExtension(buffer);
  wstring base_name = buffer;

  ModuleMap::const_iterator module_iter = module_map_.find(base_name);
  if (module_iter != module_map_.end()) return module_iter->second;

  for (ModuleMap::const_iterator iter = initial_module_map_.begin();
       iter != initial_module_map_.end();
       iter++) {
    if (base_name.find(iter->first) != wstring::npos) {
      module_map_[base_name] = iter->second;
      return iter->second;
    }
  }
  return FLAGS_v;
}

void VLog::ConstructIntialModuleMap() {
  initial_module_map_.clear();
  wstring vmodule = ime_goopy::Utf8ToWide(FLAGS_vmodule);
  if (vmodule.empty()) return;
  scoped_ptr<wchar_t[]> vmoduel_array(new wchar_t[vmodule.size() + 1]);
  wcscpy(vmoduel_array.get(), vmodule.c_str());
  wchar_t* token = wcstok(vmoduel_array.get(), kSeparator);
  while (token) {
    wchar_t* equal_sign = wcschr(token, kEqualSign);
    if (!equal_sign) {
      initial_module_map_[token] = kDefaultVLevel;
    } else {
      *equal_sign = 0;
      initial_module_map_[token] = _wtoi(equal_sign + 1);
      *equal_sign = kEqualSign;
    }
    token = wcstok(NULL, kSeparator);
  }
}

void VLog::SetFromEnvironment(const wchar_t* vmodule_name,
                              const wchar_t* vlevel_name) {
  wchar_t vmodule[MAX_PATH], vlevel[MAX_PATH];
  if (GetEnvironmentVariable(vmodule_name, vmodule, MAX_PATH)) {
    SetModule(vmodule);
  }
  if (GetEnvironmentVariable(vlevel_name, vlevel, MAX_PATH)) {
    SetLevel(_wtoi(vlevel));
  }
}

void VLog::SetModule(const wchar_t* vmodule) {
  SetModule(ime_goopy::WideToUtf8(vmodule).c_str());
}

void VLog::SetModule(const char* vmodule) {
  FLAGS_vmodule = vmodule;

  module_map_.clear();
  ConstructIntialModuleMap();
}

void VLog::SetLevel(int32 vlevel) {
  FLAGS_v = static_cast<int32>(vlevel);
}
#endif  // OS_WINDOWS
}  // namespace ime_shared
