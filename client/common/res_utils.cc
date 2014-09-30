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

#include "common/res_utils.h"

namespace ime_goopy {

bool ResUtils::LoadResource(const wchar_t* module_name,
                            const wchar_t* resource_name,
                            const wchar_t* resource_type,
                            string* content) {
  HMODULE ime = NULL;
  // If module_name is given, then locates the resource in the specified module,
  // otherwise, locates the resource in the exe which is used to create the
  // current process.
  if (module_name) {
    ime = GetModuleHandle(module_name);
    if (ime == NULL)
      return false;
  }
  HRSRC resource = FindResource(ime,
                                resource_name,
                                resource_type);
  if (resource == NULL) return false;
  size_t size = SizeofResource(ime, resource);
  HGLOBAL data = ::LoadResource(ime, resource);
  if (data == NULL) return false;
  void *buffer = LockResource(data);
  if (buffer != NULL) {
    content->assign(reinterpret_cast<char*>(buffer), size);
  }
  FreeResource(data);
  return buffer != NULL;
}

}  // namespace ime_goopy
