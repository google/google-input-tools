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

#ifndef GGADGET_MAIN_VIEW_DECORATOR_BASE_H__
#define GGADGET_MAIN_VIEW_DECORATOR_BASE_H__

#include <ggadget/view_decorator_base.h>

namespace ggadget {

/**
 * @ingroup ViewDecorator
 *
 * Base class for main view decorators.
 *
 * A decorator for main view may inherit this class to get common behavior for
 * main view.
 */
class MainViewDecoratorBase : public ViewDecoratorBase {
 public:
  /**
   * Buttons that can be displayed in MainViewDecorator's button box.
   */
  enum ButtonId {
    BACK_BUTTON = 0,
    FORWARD_BUTTON,
    POP_IN_OUT_BUTTON,
    MENU_BUTTON,
    CLOSE_BUTTON,
    NUMBER_OF_BUTTONS
  };

  /** Possible positions of button box. */
  enum ButtonBoxPosition {
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
  };

  /** Possible orientations of button box. */
  enum ButtonBoxOrientation {
    HORIZONTAL = 0,
    VERTICAL
  };

  /** Possible directions where the pop out view will be shown. */
  enum PopOutDirection {
    POPOUT_TO_LEFT = 0,
    POPOUT_TO_RIGHT
  };

  /**
   * Constructors
   *
   * @sa ViewDecoratorBase
   *
   * @param show_minimized_background If it's true then a background will be
   *        shown in minimized mode.
   */
  MainViewDecoratorBase(ViewHostInterface *host, const char *option_prefix,
                        bool allow_x_margin, bool allow_y_margin,
                        bool show_minimized_background);
  virtual ~MainViewDecoratorBase();

  /** Shows or hides a specified button. */
  void SetButtonVisible(ButtonId button_id, bool visible);

  /** Checks if a specified button is visible. */
  bool IsButtonVisible(ButtonId button_id) const;

  /** Shows or hides the button box. */
  void SetButtonBoxVisible(bool visible);

  /** Checks if the button box is visible. */
  bool IsButtonBoxVisible() const;

  /** Shows or hides the minimized icon. */
  void SetMinimizedIconVisible(bool visible);

  /** Checks if the minimized icon is visible. */
  bool IsMinimizedIconVisible() const;

  /** Shows or hides the minimized caption. */
  void SetMinimizedCaptionVisible(bool visible);

  /** Checks if the minimized caption is visible. */
  bool IsMinimizedCaptionVisible() const;

  /** Sets display position of the button box. */
  void SetButtonBoxPosition(ButtonBoxPosition position);
  /** Gets display position of the button box. */
  ButtonBoxPosition GetButtonBoxPosition() const;

  /** Sets display orientation of the button box. */
  void SetButtonBoxOrientation(ButtonBoxOrientation orientation);

  /** Gets display orientations of the button box. */
  ButtonBoxOrientation GetButtonBoxOrientation() const;

  /** Gets actual size of the button box, in pixels. */
  void GetButtonBoxSize(double *width, double *height) const;

  /**
   * Sets the direction where pop out view will be shown.
   * It controls the direction of pop in/out button.
   */
  void SetPopOutDirection(PopOutDirection direction);

  /** Gets the direction where pop out view will be shown. */
  PopOutDirection GetPopOutDirection() const;

  /** Enables or disables minimized mode. */
  void SetMinimized(bool minimized);

  /** Checks if it's in minimized mode. */
  bool IsMinimized() const;

  /**
   * Pops out/in the view.
   *
   * This method just emits OnPopOut/OnPopIn signals,
   * Host shall connect to these signals and perform the real action.
   */
  void SetPoppedOut(bool popout);

  /** Checks if the main view is popped out. */
  bool IsPoppedOut() const;

  /**
   * Sets show/hide timeout of main view decorator.
   *
   * When mouse is moved over the main view, OnShowDecorator() will be
   * called after show_timeout milliseconds.
   *
   * when mouse is moved out of the main view, OnHideDecorator() will be
   * called after hide_timeout milliseconds.
   *
   * If the specified timeout is zero, then it'll be showed/hid immediately.
   * If the specified timeout is negative, the corresponding handler will never
   * be called.
   */
  void SetDecoratorShowHideTimeout(int show_timeout, int hide_timeout);

  /**
   * Connects a handler to OnPopIn signal.
   * This signal will be emitted when popin or close button is clicked by user.
   * Host shall connect to this signal and perform the real popin action.
   */
  Connection *ConnectOnPopIn(Slot0<void> *slot);

  /**
   * Connects a handler to OnPopOut signal.
   * This signal will be emitted when popout button is clicked by user.
   * Host shall connect to this signal and perform the real popout action.
   */
  Connection *ConnectOnPopOut(Slot0<void> *slot);

 public:
  virtual GadgetInterface *GetGadget() const;
  virtual bool OnAddContextMenuItems(MenuInterface *menu);
  virtual EventResult OnOtherEvent(const Event &event);
  virtual void SetResizable(ResizableMode resizable);
  virtual void SetCaption(const std::string &caption);

  virtual bool ShowDecoratedView(bool modal, int flags,
                                 Slot1<bool, int> *feedback_handler);

 protected:
  virtual void OnChildViewChanged();
  virtual void DoLayout();
  virtual void GetMinimumClientExtents(double *width, double *height) const;
  virtual void GetClientExtents(double *width, double *height) const;
  virtual bool OnClientSizing(double *width, double *height);

  /* Add Collapse/Expand menu item to menu */
  void AddCollapseExpandMenuItem(MenuInterface *menu) const;

 protected:
  /**
   * This method will be called when displaying context menu.
   *
   * The default implementation adds only one "Remove" menu item
   * for removing the gadget.
   *
   * Derived class shall override this method to add decorator specific menu
   * items, and call through parent's OnAddDecoratorMenuItems().
   */
  virtual void OnAddDecoratorMenuItems(MenuInterface *menu);

  /**
   * This method will be called if it's time to show view decorator.
   *
   * The default implementation calls SetButtonBoxVisible(true) to show the
   * button box.
   *
   * Derived class shall override this method to show additional decoration
   * elements.
   */
  virtual void OnShowDecorator();

  /**
   * This method will be called if it's time to hide view decorator.
   *
   * The default implementation calls SetButtonBoxVisible(false) to hide the
   * button box.
   *
   * Derived class shall override this method to hide additional decoration
   * elements.
   */
  virtual void OnHideDecorator();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(MainViewDecoratorBase);
};

} // namespace ggadget

#endif // GGADGET_MAIN_VIEW_DECORATOR_BASE_H__
