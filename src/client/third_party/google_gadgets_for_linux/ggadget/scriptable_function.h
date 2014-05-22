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

#ifndef GGADGET_SCRIPTABLE_FUNCTION_H__
#define GGADGET_SCRIPTABLE_FUNCTION_H__

#include <ggadget/scriptable_helper.h>
#include <ggadget/slot.h>

namespace ggadget {

/**
 * @ingroup ScriptableObjects
 *
 * This class is used to reflect a dynamic native slot to script.
 */
class ScriptableFunction : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x54df2407983135b8, ScriptableInterface);

 public:
  /**
   * Wrap a slot as an scriptable object.
   * This class will take the ownership of the slot.
   * @param slot dynamic slot, must be created with operator new.
   */
  ScriptableFunction(Slot *slot) {
    RegisterMethod("", slot);
  }

 protected:
  virtual ~ScriptableFunction() {
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableFunction);
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_FUNCTION_H__
