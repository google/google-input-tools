/*
  Copyright 2008 Google Inc.

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

#ifndef GGADGET_SCRIPT_RUNTIME_INTERFACE_H__
#define GGADGET_SCRIPT_RUNTIME_INTERFACE_H__

#include <ggadget/variant.h>

namespace ggadget {

class ScriptContextInterface;
template <typename R, typename P1> class Slot1;
class Connection;

/**
 * @ingroup Interfaces
 * @ingroup ScriptRuntime
 * The script engine runtime.
 * Normally there is one @c ScriptRuntimeInterface instance in a process
 * for each script engine.
 */
class ScriptRuntimeInterface {
 public:
  virtual ~ScriptRuntimeInterface() { }

  /**
   * Creates a new @c ScriptContextInterface instance.
   * Must call @c DestroyContext after use.
   * @return the created context.
   */
  virtual ScriptContextInterface *CreateContext() = 0;
};

} // namespace ggadget

#endif // GGADGET_SCRIPT_RUNTIME_INTERFACE_H__
