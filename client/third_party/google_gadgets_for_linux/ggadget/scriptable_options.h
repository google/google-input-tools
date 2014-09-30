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

#ifndef GGADGET_SCRIPTABLE_OPTIONS_H__
#define GGADGET_SCRIPTABLE_OPTIONS_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class OptionsInterface;

/**
 * @ingroup ScriptableObjects
 *
 * Scriptable decorator for OptionsInterface.
 */
class ScriptableOptions : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x1a7bc9215ef74743, ScriptableInterface)

  /**
   * @param options
   * @param raw_objects weather pass raw @c Variant objects to/from script or
   *     encode them into JSONString.
   */
  ScriptableOptions(OptionsInterface *options, bool raw_objects);
  virtual ~ScriptableOptions();

 protected:
  virtual void DoRegister();

 public:
  const OptionsInterface *GetOptions() const;
  OptionsInterface *GetOptions();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableOptions);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_OPTIONS_H__
