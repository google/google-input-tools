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

#ifndef BASE_COMMANDLINEFLAGS_DECLARE_H_
#define BASE_COMMANDLINEFLAGS_DECLARE_H_

#include <string>

#include "base/basictypes.h"
#include "base/port.h"

// namespc should be 'std::', and type 'string', for a var of type 'std::string'
#define DECLARE_VARIABLE(namespc, type, name)                                 \
  namespace Flag_Names_##type {                                               \
    extern namespc type& FLAGS_##name;                                        \
  }                                                                           \
  using Flag_Names_##type::FLAGS_##name

#define DECLARE_bool(name)             DECLARE_VARIABLE(, bool, name)
#define DECLARE_int32(name)            DECLARE_VARIABLE(, int32, name)
#define DECLARE_int64(name)            DECLARE_VARIABLE(, int64, name)
#define DECLARE_uint64(name)           DECLARE_VARIABLE(, uint64, name)
#define DECLARE_double(name)           DECLARE_VARIABLE(, double, name)
#define DECLARE_string(name)           DECLARE_VARIABLE(, string, name)

#endif  // BASE_COMMANDLINEFLAGS_DECLARE_H_
