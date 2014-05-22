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

#ifndef GGADGET_DOCKED_MAIN_VIEW_DECORATOR_H__
#define GGADGET_DOCKED_MAIN_VIEW_DECORATOR_H__

#include <ggadget/main_view_decorator_base.h>

namespace ggadget {

/**
 * @ingroup ViewDecorator
 *
 * A view decorator class suitable for display a main view in sidebar.
 */
class DockedMainViewDecorator : public MainViewDecoratorBase {
 public:
  DockedMainViewDecorator(ViewHostInterface *host);
  virtual ~DockedMainViewDecorator();

  /**
   * Shows or hides one or more resize borders.
   *
   * by default, only bottom resize border is visible.
   */
  void SetResizeBorderVisible(int borders);

  /**
   * Connects a handler to OnUndock signal.
   * This signal will be emitted when undock menu item is activated by user.
   * Host shall connect to this signal and perform the real dock action.
   */
  Connection *ConnectOnUndock(Slot0<void> *slot);

 protected:
  virtual void GetMargins(double *left, double *top,
                          double *right, double *bottom) const;

 protected:
  virtual void OnAddDecoratorMenuItems(MenuInterface *menu);
  virtual void OnShowDecorator();
  virtual void OnHideDecorator();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DockedMainViewDecorator);
};

} // namespace ggadget

#endif // GGADGET_DOCKED_MAIN_VIEW_DECORATOR_H__
