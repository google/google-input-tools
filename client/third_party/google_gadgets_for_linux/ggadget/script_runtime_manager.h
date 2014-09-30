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

#ifndef GGADGET_SCRIPT_RUNTIME_MANAGER_H__
#define GGADGET_SCRIPT_RUNTIME_MANAGER_H__

#include <ggadget/common.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>

namespace ggadget {

/**
 * @defgroup ScriptRuntime Script runtime
 * @ingroup CoreLibrary
 * Script runtime related classes
 * @{
 */

/**
 * Manager to manage multiple script runtime objects.
 */
class ScriptRuntimeManager {
 public:
  /**
   * Registers a new ScriptRuntimeInterface implementation object.
   * @param tag_name The tag name of the subclass, shall be file suffix for
   *     that kind of script file, such as "js", "py", etc.
   * @param runtime Pointer to the ScriptRuntimeInterface implementation
   *     object.
   * @return @c true if registered successfully, or @c false if the specified
   *     tag name already exists.
   */
  bool RegisterScriptRuntime(const char *tag_name,
                             ScriptRuntimeInterface *runtime);

  /**
   * Create a new @c ScriptContextInterface instance.
   * Must call @c DestroyContext after use.
   * @return the created context.
   */
  ScriptContextInterface *CreateScriptContext(const char *tag_name);

  /**
   * Returns the ScriptRuntimeInterface implementation object for a
   * script file type.
   */
  ScriptRuntimeInterface *GetScriptRuntime(const char *tag_name);

 public:
  /**
   * Get the singleton of ScriptRuntimeManager.
   */
  static ScriptRuntimeManager *get();

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating ScriptRuntimeManager object
   * directly.
   */
  ScriptRuntimeManager();

  /**
   * Private destructor to prevent deleting ScriptRuntimeManager object
   * directly.
   */
  ~ScriptRuntimeManager();

  DISALLOW_EVIL_CONSTRUCTORS(ScriptRuntimeManager);
};

/** @} */

} // namespace ggadget

#endif // GGADGET_SCRIPT_RUNTIME_MANAGER_H__
