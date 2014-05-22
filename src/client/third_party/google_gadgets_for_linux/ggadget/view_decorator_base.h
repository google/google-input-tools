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

#ifndef GGADGET_VIEW_DECORATOR_BASE_H__
#define GGADGET_VIEW_DECORATOR_BASE_H__

#include <ggadget/common.h>
#include <ggadget/view.h>
#include <ggadget/view_element.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>

namespace ggadget {

/**
 * @defgroup ViewDecorator View decorator
 * @ingroup CoreLibrary
 * Helper classes to draw view decorations, such as borders, buttons, etc.
 */

/**
 * @ingroup ViewDecorator
 *
 * Base class for all kinds of view decorators.
 * A view decorator is a special kind of view, which contains a child view and
 * some additional elements to decorate the child view, such as a resize
 * border, a menu box, a title bar, etc.
 */
class ViewDecoratorBase : public View {
 public:
  enum Border {
    BORDER_NONE = 0x0,
    BORDER_TOP = 0x1,
    BORDER_LEFT = 0x2,
    BORDER_BOTTOM = 0x4,
    BORDER_RIGHT = 0x8
  };
  /**
   * Constructor.
   *
   * @param host The real view host which will be used by this view decorator.
   * @param option_prefix A prefix for storing options related to this view
   *        decorator. NULL means don't save any states.
   * @param allow_x_margin If it's true then horizontal blank margin is allowed
   *        between child view and view decorator border.
   * @param allow_y_margin If it's true then vertical blank margin is allowed
   *        between child view and view decorator border.
   */
  ViewDecoratorBase(ViewHostInterface *host, const char *option_prefix,
                    bool allow_x_margin, bool allow_y_margin);
  virtual ~ViewDecoratorBase();

  /** Sets the child view which will be displayed inside this view decorator. */
  void SetChildView(View *child_view);

  /** Gets the child view. */
  View *GetChildView() const;

  /** Allows or disallows x margin. */
  void SetAllowXMargin(bool allow);

  /** Allows or disallows y margin. */
  void SetAllowYMargin(bool allow);

  /**
   * Updates the size of view decorator.
   *
   * This function will be called when the size of child view or decoration
   * elements are changed.
   */
  void UpdateViewSize();

  /** Loads previously saved child view size from gadget's options. */
  bool LoadChildViewSize();

  /** Saves current child view size to gadget's options. */
  bool SaveChildViewSize() const;

  /** Shows or hides child view. */
  void SetChildViewVisible(bool visible);

  /** Checks if child view is visible. */
  bool IsChildViewVisible() const;

  /**
   * Freezes or unfreezes child view's image.
   * A snapshot of child view will be shown when it's frozen.
   */
  void SetChildViewFrozen(bool frozen);

  /** Checks if a snapshot of child view is currently shown. */
  bool IsChildViewFrozen() const;

  /** Sets scale factor of child view. */
  void SetChildViewScale(double scale);

  /** Gets scale factor of child view. */
  double GetChildViewScale() const;

  /**
   * Sets opacity of child view or its snapshot.
   *
   * If it's in frozen mode, then opacity of child view's snapshot will be
   * set. Otherwise opacity of child view itself will be set.
   */
  void SetChildViewOpacity(double opacity);

  /** Gets opacity of child view or its snapshot. */
  double GetChildViewOpacity() const;

  /** Sets cursor type required by child view. */
  void SetChildViewCursor(ViewInterface::CursorType type);

  /** Shows tooltip required by child view. */
  void ShowChildViewTooltip(const std::string &tooltip);

  /**
   * Shows tooltip required by child view at specific position in child view's
   * coordinates.
   */
  void ShowChildViewTooltipAtPosition(const std::string &tooltip,
                                      double x, double y);

  /** Sets the option prefix **/
  void SetOptionPrefix(const char *option_prefix);

  /** Gets the option prefix **/
  std::string GetOptionPrefix() const;

  /** Checks if options are available **/
  bool HasOptions() const;

  /** Gets value of an option **/
  Variant GetOption(const std::string &name) const;

  /** Sets value of an option **/
  void SetOption(const std::string &name, Variant value) const;

  /**
   * Gets the size of child view.
   *
   * This method returns the actual size of child view, no matter if it's
   * invisible or not.
   */
  void GetChildViewSize(double *width, double *height) const;

  /**
   * Queue draw child view.
   *
   * This method will be called by DecoratedViewHost when child view calls
   * QueueDraw().
   */
  void QueueDrawChildView();

  /** Converts coordinate from child view to ViewDecorator. */
  void ChildViewCoordToViewCoord(double child_x, double child_y,
                                 double *view_x, double *view_y) const;

  /** Converts coordinate from ViewDecorator view to child view. */
  void ViewCoordToChildViewCoord(double view_x, double view_y,
                                 double *child_x, double *child_y) const;

  /**
   * Connects a handler to OnClose signal.
   * This signal will be emitted when close button is clicked by user.
   * Host shall connect to this signal and perform the real close action.
   */
  Connection *ConnectOnClose(Slot0<void> *slot);

 public:
  // Overridden methods.
  virtual GadgetInterface *GetGadget() const;
  virtual bool OnAddContextMenuItems(MenuInterface *menu);
  virtual EventResult OnOtherEvent(const Event &event);
  virtual bool OnSizing(double *width, double *height);
  virtual void SetResizable(ResizableMode resizable);
  virtual std::string GetCaption() const;
  virtual void SetSize(double width, double height);

 public:
  /**
   * Shows this ViewDecorator.
   *
   * It delegates to real view host's ShowView() method.
   */
  virtual bool ShowDecoratedView(bool modal, int flags,
                                 Slot1<bool, int> *feedback_handler);
  /** Closes this ViewDecorator. */
  virtual void CloseDecoratedView();

 protected:
  /**
   * Posts the close signal.
   *
   * Derived class shall call this method to emit close signal when using
   * clicks the close button.
   */
  void PostCloseSignal();

  /**
   * Inserts a decorator element.
   *
   * This method is only for derived classes.
   *
   * @param element The element to be inserted.
   * @param background True if the element is a background element.
   * @return true on success.
   */
  bool InsertDecoratorElement(BasicElement *element, bool background);

  /**
   * Gets resizable mode of child view.
   *
   * The result is cached, so even the child view is switched out, the
   * resizable mode of previous child view will still be returned.
   */
  ResizableMode GetChildViewResizable() const;

  /* Add zoom menu item to context menu */
  void AddZoomMenuItem(MenuInterface *menu) const;

 protected:
  /**
   * This method will be called if child view is changed.
   *
   * Derived class shall override this method if it's interested in the change
   * of child view.
   */
  virtual void OnChildViewChanged();

  /**
   * This method will be called when it's necessary to adjust the layout of
   * decoration elements.
   *
   * Derived class shall override this method to layout its own decoration
   * elements. The parent's DoLayout() method must be called in the overriden
   * method.
   */
  virtual void DoLayout();

  /**
   * Gets the margins of the decorator.
   *
   * The default implementation returns zero for all sides.
   * Derived class shall override this method to provide correct values.
   */
  virtual void GetMargins(double *left, double *top,
                          double *right, double *bottom) const;

  /**
   * Gets the minimum size of client area, the area to show child view or
   * related information.
   *
   * The default implementation returns zero.
   * Derived class shall override this method to provide correct values.
   */
  virtual void GetMinimumClientExtents(double *width, double *height) const;

  /**
   * Gets the size of current client elements. Client elements are displayed
   * when child view is hidden, which may include any information such as
   * child view's icon and caption.
   * This method will only be called when child view is hidden (so called
   * minimize mode).
   *
   * The default implementation returns zero.
   * Derived class shall override this method to provide correct values.
   */
  virtual void GetClientExtents(double *width, double *height) const;

  /**
   * Checks if specified client area size is acceptable.
   * This method will only be called when child view is hidden.
   *
   * The default implementation returns true.
   * Derived class shall override this method to adjust the size or cancel the
   * size change operation by returning false.
   */
  virtual bool OnClientSizing(double *width, double *height);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ViewDecoratorBase);
};

} // namespace ggadget

#endif // GGADGET_VIEW_DECORATOR_BASE_H__
