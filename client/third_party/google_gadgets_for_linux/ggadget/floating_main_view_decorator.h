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

#ifndef GGADGET_FLOATING_MAIN_VIEW_DECORATOR_H__
#define GGADGET_FLOATING_MAIN_VIEW_DECORATOR_H__

#include <ggadget/main_view_decorator_base.h>

namespace ggadget {

/**
 * @ingroup ViewDecorator
 *
 * A view decorator class suitable for display a main view in a floating
 * window.
 */
class FloatingMainViewDecorator : public MainViewDecoratorBase {
 public:
  FloatingMainViewDecorator(ViewHostInterface *host,
                            bool transparent_background);
  virtual ~FloatingMainViewDecorator();

  /**
   * Connects a handler to OnDock signal.
   * This signal will be emitted when dock menu item is activated by user.
   * Host shall connect to this signal and perform the real dock action.
   */
  Connection *ConnectOnDock(Slot0<void> *slot);

 public:
  virtual void SetResizable(ResizableMode resizable);

 protected:
  virtual void DoLayout();
  virtual void GetMargins(double *left, double *top,
                          double *right, double *bottom) const;

 protected:
  virtual void OnAddDecoratorMenuItems(MenuInterface *menu);
  virtual void OnShowDecorator();
  virtual void OnHideDecorator();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(FloatingMainViewDecorator);
};

} // namespace ggadget

#endif // GGADGET_FLOATING_MAIN_VIEW_DECORATOR_H__
