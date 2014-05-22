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

#ifndef GGADGET_VIEW_HOST_INTERFACE_H__
#define GGADGET_VIEW_HOST_INTERFACE_H__

#include <string>
#include <ggadget/view_interface.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;

/**
 * @ingroup Interfaces
 * @ingroup View
 *
 * Interface for providing host services to views.
 * Each view contains a pointer to a @c ViewHostInterface object which is
 * dedicated to the view.
 * The @c ViewHostInterface implementation should depend on the host.
 * The services provided by @c ViewHostInterface are bi-directional.
 * The view calls methods in the @c ViewHostInterface, and the host callback
 * to the view's event handler methods.
 */
class ViewHostInterface {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~ViewHostInterface() { }

 public:
  enum Type {
    VIEW_HOST_MAIN = 0, /**< ViewHost to hold a main view. */
    VIEW_HOST_OPTIONS,  /**< ViewHost to hold an options view. */
    VIEW_HOST_DETAILS  /**< ViewHost to hold a details view. */
  };

  /** Gets the type of the Viewhost. */
  virtual Type GetType() const = 0;

  /**
   * Destroys this ViewHost instance.
   *
   * When this method get called, the resource of associated View instance may
   * already be unavailable, so in this method, ViewHost shall not access the
   * View instance.
   */
  virtual void Destroy() = 0;

  /**
   * Sets a View instance to the ViewHost instance. The ViewHost instance
   * doesn't own the View instance.
   */
  virtual void SetView(ViewInterface *view) = 0;

  /** Gets the associated view. */
  virtual ViewInterface *GetView() const = 0;

  /** Returns the @c GraphicsInterface associated with this host. */
  virtual GraphicsInterface *NewGraphics() const = 0;

  /**
   * Gets information about the native widget.
   * @return the pointer point to native widget.
   */
  virtual void *GetNativeWidget() const = 0;

  /**
   * Converts coordinates in the view's space to coordinates in the native
   * widget which holds the view.
   *
   * @param x x-coordinate in the view's space to convert.
   * @param y y-coordinate in the view's space to convert.
   * @param[out] widget_x parameter to store the converted widget x-coordinate.
   * @param[out] widget_y parameter to store the converted widget y-coordinate.
   */
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const = 0;

  /**
   * Converts coordinates in the native widget which holds the view to
   * coordinates in the view's space.
   *
   * @param x x-coordinate in the native widget which holds the view.
   * @param y y-coordinate in the native widget which holds the view.
   * @param[out] view_x parameter to store the converted view x-coordinate.
   * @param[out] view_y parameter to store the converted view y-coordinate.
   */
  virtual void NativeWidgetCoordToViewCoord(
      double x, double y, double *view_x, double *view_y) const = 0;

  /** Asks the view host to redraw the given view. */
  virtual void QueueDraw() = 0;

  /**
   * Asks the view host to resize the host window size according to the view
   * size.
   */
  virtual void QueueResize() = 0;

  /**
   * Asks the view host to enable or disable input shape mask.
   *
   * When input shape mask is enabled, the window pixels with opacity < 0.5
   * will not be able to receive mouse events.
   *
   * It may not be supported on all platforms.
   */
  virtual void EnableInputShapeMask(bool enable) = 0;

  /**
   * When the resizable field on the view is updated, the host needs to be
   * alerted of this change.
   */
  virtual void SetResizable(ViewInterface::ResizableMode mode) = 0;

  /**
   * Sets a caption to be shown when the View is in floating or expanded
   * mode.
   */
  virtual void SetCaption(const std::string &caption) = 0;

  /** Sets whether to always show the caption for this view. */
  virtual void SetShowCaptionAlways(bool always) = 0;

  /**
   * Sets the current mouse cursor.
   *
   * @param type the cursor type, see @c ViewInterface::CursorType.
   */
  virtual void SetCursor(ViewInterface::CursorType type) = 0;

  /**
   * Shows a tooltip popup after certain initial delay at the current mouse
   * position . The implementation should handle tooltip auto-hiding.
   * @param tooltip the tooltip to display. If it's empty, currently
   *     displayed tooltip will be hidden.
   */
  virtual void ShowTooltip(const std::string &tooltip) = 0;

  /**
   * Shows a tooltip popup at a specified position in view's coordinates.
   * The tooltip will be showed immediately.
   * @param tooltip the tooltip to display. If it's empty, currently
   *     displayed tooltip will be hidden.
   * @param x, y position to show the tooltip, in view's coordinates.
   */
  virtual void ShowTooltipAtPosition(const std::string &tooltip,
                                     double x, double y) = 0;

  /**
   * Shows the associated View by proper method according to type of the View.
   *
   * The behavior of this function will be different for different types of
   * view:
   * - For main view, all parameters will be ignored. The feedback_handler will
   *   just be deleted if it's not NULL.
   * - For options view, the flags is combination of @c OptionsViewFlags,
   *   feedback_handler will be called when closing the options view, with one
   *   of OptionsViewFlags as the parameter.
   * - For details view, the flags is combination of DetailsViewFlags,
   *   feedback_handler will be called when closing the details view, with one
   *   of DetailsViewFlags as the parameter.
   *
   * The ViewHost shall fire EVENT_OPEN event by calling
   * ViewInterface::OnOtherEvent() method as soon as the View is shown.
   *
   * @param modal if it's true then the view will be displayed in modal mode,
   *        and this function won't return until the view is closed.
   * @param flags for options view, it's combination of OptionsViewFlags,
   *        for details view, it's combination of DetailsViewFlags.
   * @param feedback_handler a callback that will be called when the view is
   *        closed. It has no effect for main view.
   * @return true if the View is shown correctly. Otherwise returns false.
   */
  virtual bool ShowView(bool modal, int flags,
                        Slot1<bool, int> *feedback_handler) = 0;

  /**
   * Closes the view if it's opened by calling ShowView().
   *
   * The ViewHost shall fire EVENT_CLOSE event by calling
   * ViewInterface::OnOtherEvent() method just before the View is closed.
   */
  virtual void CloseView() = 0;

  /**
   * Shows the context menu for the view.
   * For main view, some default menu items shall be implemented by the view
   * host, such as:
   * - Collapse
   * - Undock from Sidebar
   *
   * This method shall call OnAddContextMenuItems() method of the view, so that
   * the view can add its customized context menu items. If
   * OnAddContextMenuItems() method of the view returns false, then above
   * default menu items shall not be added.
   *
   * @param button The mouse button which triggers the context menu, it should
   *        be one of @c MouseEvent::Button enums.
   *
   * @return true if the context menu is shown.
   */
  virtual bool ShowContextMenu(int button) = 0;

  /**
   * Starts resizing the View.
   *
   * This method call might not be honoured due to different ViewHost
   * implementation, or if the View is not resizable or zoomable.
   *
   * During resize drag, @View::OnSizing() method will be called for each
   * resize request, if the request is accepted by View, then @View::SetSize()
   * method will be called to perform the resize action.
   *
   * @param button The mouse button which initiates the drag, see
   *        @MouseEvent::Button.
   * @param hittest Represents the border or corner to be dragged, only
   *        accepts: HT_LEFT, HT_RIGHT, HT_TOP, HT_TOPLEFT, HT_TOPRIGHT,
   *        HT_BOTTOM, HT_BOTTOMLEFT, HT_BOTTOMRIGHT.
   */
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest) = 0;

  /**
   * Starts moving the View.
   *
   * This method call might not be honoured due to different ViewHost
   * implementation.
   *
   * The move drag is fully transparent to the View, so the View won't be
   * notified during move drag.
   *
   * @param button The mouse button which initiates the drag, see
   *        @MouseEvent::Button.
   */
  virtual void BeginMoveDrag(int button) = 0;

  /**
   * Displays a message box containing the message string.
   * @param view the real calling view, useful when this call is delegated.
   * @param message the message string.
   */
  virtual void Alert(const ViewInterface *view, const char *message) = 0;

  enum ConfirmResponse {
    CONFIRM_CANCEL = -1,
    CONFIRM_NO = 0,
    CONFIRM_YES = 1
  };

  /**
   * Displays a dialog containing the message string and Yes and No buttons.
   * @param view the real calling view, useful when this call is delegated.
   * @param message the message string.
   * @param cancel_button indicates whether to display the Cancel button.
   * @return which button is pressed. Note: if @a cancel_button is false,
   *     this method returns 0 if the user closes the dialog without pressing
   *     Yes or No button, to keep backward compatibility.
   */
  virtual ConfirmResponse Confirm(const ViewInterface *view,
                                  const char *message, bool cancel_button) = 0;

  /**
   * Displays a dialog asking the user to enter text.
   * @param view the real calling view, useful when this call is delegated.
   * @param message the message string displayed before the edit box.
   * @param default_value the initial default value dispalyed in the edit box.
   * @return the user inputted text, or an empty string if user canceled the
   *     dialog.
   */
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value) = 0;

  /** Gets the debug mode for drawing view. */
  virtual int GetDebugMode() const = 0;

  /**
   * Gets the position of the top level window.
   *
   * On Mac OSX, it's the position of the botton-left of the window. On other
   * positions it's the top-left.
   */
  virtual void GetWindowPosition(int *x, int *y) = 0;

  /** Sets the position of the top level window. */
  virtual void SetWindowPosition(int x, int y) = 0;

  /** Gets the size of the top level window. */
  virtual void GetWindowSize(int *width, int *height) = 0;

  /** Sets whether the vhew host can steal focus from other applications. */
  virtual void SetFocusable(bool focusable) = 0;

  /**
   * Sets the opacity of the top level window.
   *
   * @param opacity The opacity in range. 1.0 is fully opaque, 0.0 is fully
   *     transparent.
   */
  virtual void SetOpacity(double opacity) = 0;

  /**
   * Sets font scale.
   */
  virtual void SetFontScale(double scale) = 0;

  /**
   * Sets the current zoom level.
   */
  virtual void SetZoom(double zoom) = 0;

  /**
   * Connect a slot to OnEndMoveDrag signal.
   *
   * The slot will be called when the user release the mouse button when
   * dragging to moving to window.
   */
  virtual Connection *ConnectOnEndMoveDrag(Slot2<void, int, int> *slot) = 0;

  /** Connect a slot to OnShowContextMenu signal.
   *
   * The slot will be called when the view is about to show a context menu. If
   * the slot returns false, the viewhost will not show the context menu.
   */
  virtual Connection *ConnectOnShowContextMenu(
      Slot1<bool, MenuInterface*> *slot) = 0;
};

} // namespace ggadget

#endif // GGADGET_VIEW_HOST_INTERFACE_H__
