/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_SCRIPTABLE_MENU_H__
#define GGADGET_SCRIPTABLE_MENU_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class BasicElement;
class GadgetInterface;
class MenuInterface;

/**
 * @ingroup ScriptableObjects
 *
 * Scriptable decorator for MenuInterface.
 */
class ScriptableMenu : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x95432249155845d6, ScriptableInterface)

  ScriptableMenu(GadgetInterface *gadget, MenuInterface *menu);
  virtual ~ScriptableMenu();

  MenuInterface *GetMenu() const;

  /**
   * Sets the menu's position hint to the boundary box of the given element.
   */
  void SetPositionHint(const BasicElement *element);

 protected:
  virtual void DoClassRegister();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableMenu);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_MENU_H__
