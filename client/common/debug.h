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

#ifndef GOOPY_COMMON_DEBUG_H__
#define GOOPY_COMMON_DEBUG_H__

#include <string>
#include "base/basictypes.h"

// __FUNCTION__ is very very long if it's a member function of a template
// class. This macro strips the annoying long class name.
#define __SHORT_FUNCTION__ ime_goopy::Debug::FindFunctionName(__FUNCTION__)

namespace ime_goopy {
class Debug {
 public:
  static const char* FindFunctionName(const char* full_name);
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_DEBUG_H__
