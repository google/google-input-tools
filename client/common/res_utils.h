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

#ifndef GOOPY_COMMON_RES_UTILS_H_
#define GOOPY_COMMON_RES_UTILS_H_

#include "base/basictypes.h"

namespace ime_goopy {

class ResUtils {
 public:
  // Loads embedded extension script from IME module. module_name can either be
  // the name of the IME dll, or NULL. If module_name is not NULL, then locates
  // the resource in the specified module, otherwise, locates the resource in
  // the exe which is used to create the current process.
  static bool LoadResource(const wchar_t* module_name,
                           const wchar_t* resource_name,
                           const wchar_t* resource_type,
                           string* data);
};

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_RES_UTILS_H_
