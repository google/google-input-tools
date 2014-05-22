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

#ifndef GGADGET_DISPLAY_WINDOW_H__
#define GGADGET_DISPLAY_WINDOW_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

class View;

/**
 * @ingroup View
 *
 * This class wraps a view into the old @c GoogleDesktopDisplayWindow interface
 * for old style options dialog. This class is only for scripting.
 */
class DisplayWindow: public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x0466c36b78944d34, ScriptableInterface);

  DisplayWindow(View *view);
  virtual ~DisplayWindow();

  /**
   * Adjust the size of the window to proper size.
   * @return @c false if there is no control added in this object.
   */
  bool AdjustSize();

 protected:
  virtual void DoClassRegister();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DisplayWindow);
};

} // namespace ggadget

#endif // GGADGET_DISPLAY_WINDOW_H__
