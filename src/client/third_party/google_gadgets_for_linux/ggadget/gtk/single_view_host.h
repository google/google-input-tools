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

#ifndef GGADGET_GTK_SINGLE_VIEW_HOST_H__
#define GGADGET_GTK_SINGLE_VIEW_HOST_H__

#include <gtk/gtk.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>
#include <ggadget/gtk/cairo_graphics.h>

namespace ggadget {
namespace gtk {

/**
 * @ingroup GtkLibrary
 * An implementation of @c ViewHostInterface based on Gtk.
 *
 * This host can only show one View in single GtkWindow.
 *
 * Following View events are not implemented:
 * - ondock
 * - onminimize
 * - onpopin
 * - onpopout
 * - onrestore
 * - onundock
 */
class SingleViewHost : public ViewHostInterface {
 public:
  /**
   * Flags to control toplevel window behavior.
   *
   * - DECORATED
   *   Enables window manager's decoration.
   * - REMOVE_ON_CLOSE
   *   Removes gadget when the view is closed.
   * - RECORD_STATES
   *   Records window related states, like position and keep above state.
   * - WM_MANAGEABLE
   *   Enables window manager to manage the window, for example, show the
   *   window on taskbar and pager.
   * - OPAQUE_BACKGROUND
   *   Uses opaque background.
   * - DIALOG_TYPE_HINT
   *   Uses GDK_WINDOW_TYPE_HINT_DIALOG by default. To workaround problems on
   *   some special window managers, like matchbox.
   */
  enum Flags {
    DEFAULT           = 0,
    DECORATED         = 0x01,
    REMOVE_ON_CLOSE   = 0x02,
    RECORD_STATES     = 0x04,
    WM_MANAGEABLE     = 0x08,
    OPAQUE_BACKGROUND = 0x10,
    DIALOG_TYPE_HINT  = 0x20
  };

  /**
   * @param view The View instance associated to this host.
   * @param zoom Zoom factor used by the Graphics object.
   * @param flags Flags to control the window behavior.
   * @param debug_mode DebugMode when drawing elements.
   */
  SingleViewHost(ViewHostInterface::Type type,
                 double zoom, int flags, int debug_mode);
  virtual ~SingleViewHost();

  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const;
  virtual GraphicsInterface *NewGraphics() const;
  virtual void *GetNativeWidget() const;
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const;
  virtual void NativeWidgetCoordToViewCoord(
      double x, double y, double *view_x, double *view_y) const;
  virtual void QueueDraw();
  virtual void QueueResize();
  virtual void EnableInputShapeMask(bool enable);
  virtual void SetResizable(ViewInterface::ResizableMode mode);
  virtual void SetCaption(const std::string &caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(ViewInterface::CursorType type);
  virtual void ShowTooltip(const std::string &tooltip);
  virtual void ShowTooltipAtPosition(const std::string &tooltip,
                                     double x, double y);
  virtual bool ShowView(bool modal, int flags,
                        Slot1<bool, int> *feedback_handler);
  virtual void CloseView();
  virtual bool ShowContextMenu(int button);
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);

  virtual void BeginMoveDrag(int button);

  virtual void Alert(const ViewInterface *view, const char *message);
  virtual ConfirmResponse Confirm(const ViewInterface *view,
                                  const char *message, bool cancel_button);
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

  virtual void GetWindowPosition(int *x, int *y);
  virtual void SetWindowPosition(int x, int y);
  virtual void GetWindowSize(int *width, int *height);

  virtual Connection *ConnectOnEndMoveDrag(Slot0<void> *slot);
  virtual Connection *ConnectOnShowContextMenu(
      Slot1<bool, MenuInterface*> *slot);

 public:
  /** Gets the top level gtk window. */
  GtkWidget *GetWindow() const;

  /** Gets and sets keep-above state. */
  bool IsKeepAbove() const;
  void SetKeepAbove(bool keep_above);

  /** Checks if the top level window is visible or not. */
  bool IsVisible() const;

  /** Sets the gtk window type hint. */
  void SetWindowType(GdkWindowTypeHint type);

 public:
  /**
   * Connects a slot to OnViewChanged signal.
   *
   * The slot will be called when the attached view has been changed.
   */
  Connection *ConnectOnViewChanged(Slot0<void> *slot);

  /**
   * Connects a slot to OnShowHide signal.
   *
   * The slot will be called when the show/hide state of the top level window
   * has been changed. The first parameter of the slot indicates the new
   * show/hide state, true means the top level window has been shown.
   */
  Connection *ConnectOnShowHide(Slot1<void, bool> *slot);

  /**
   * Connects a slot to OnBeginResizeDrag signal.
   *
   * The slot will be called when BeginResizeDrag() method is called, the first
   * parameter of the slot is the mouse button initiated the drag. See
   * @MouseEvent::Button for definition of mouse button. The second parameter
   * is the hittest value representing the border or corner to be dragged.
   *
   * If the slot returns @false then the default resize drag operation will be
   * performed for the topleve GtkWindow, otherwise no other action will be
   * performed.
   */
  Connection *ConnectOnBeginResizeDrag(Slot2<bool, int, int> *slot);

  /**
   * Connects a slot to OnResized signal.
   *
   * The slot will be called when the top level window size is changed.
   * The two parameters are the new width and height of the window.
   */
  Connection *ConnectOnResized(Slot2<void, int, int> *slot);

  /**
   * Connects a slot to OnEndResizeDrag signal.
   *
   * The slot will be called when the resize drag has been finished.
   */
  Connection *ConnectOnEndResizeDrag(Slot0<void> *slot);

  /**
   * Connects a slot to OnBeginMoveDrag signal.
   *
   * The slot will be called when BeginMoveDrag() method is called, the first
   * parameter of the slot is the mouse button initiated the drag. @See
   * @MouseEvent::Button for definition of mouse button.
   *
   * If the slot returns @false then the default move drag operation will be
   * performed for the topleve GtkWindow, otherwise no other action will be
   * performed.
   */
  Connection *ConnectOnBeginMoveDrag(Slot1<bool, int> *slot);

  /**
   * Connects a slot to OnMoved signal.
   *
   * The slot will be called when the top level window position is changed.
   * The two parameters are the new x and y position of the top level window's
   * top left corner, related to the screen.
   */
  Connection *ConnectOnMoved(Slot2<void, int, int> *slot);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SingleViewHost);
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_SINGLE_VIEW_HOST_H__
