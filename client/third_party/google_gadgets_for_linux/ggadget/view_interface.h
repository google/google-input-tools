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

#ifndef GGADGET_VIEW_INTERFACE_H__
#define GGADGET_VIEW_INTERFACE_H__

#include <ggadget/event.h>
#include <ggadget/signals.h>

namespace ggadget {

class CanvasInterface;
class FileManagerInterface;
class GraphicsInterface;
class ScriptContextInterface;
class MenuInterface;
class ViewHostInterface;
class GadgetInterface;
class ClipRegion;
class Rectangle;

/**
 * @ingroup Interfaces
 * @ingroup View
 *
 * Interface for representing a View in the Gadget API.
 */
class ViewInterface {
 public:
  /** Used in @c SetResizable(). */
  enum ResizableMode {
    RESIZABLE_FALSE,
    RESIZABLE_TRUE,
    RESIZABLE_ZOOM,
    /** The user can resize the view while keeping the original aspect ratio. */
    RESIZABLE_KEEP_RATIO
  };

  /** Flags used in detail view. */
  enum DetailsViewFlags {
    DETAILS_VIEW_FLAG_NONE = 0,
    /** Makes the details view title clickable like a button. */
    DETAILS_VIEW_FLAG_TOOLBAR_OPEN = 1,
    /** Adds a negative feedback button in the details view. */
    DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK = 2,
    /** Adds a "Remove" button in the details view. */
    DETAILS_VIEW_FLAG_REMOVE_BUTTON = 4,
    /** Adds a button to display the friends list. */
    DETAILS_VIEW_FLAG_SHARE_WITH_BUTTON = 8,
    DETAILS_VIEW_FLAG_DISABLE_AUTO_CLOSE = 16,
    /** Disables decoration frame. */
    DETAILS_VIEW_FLAG_NO_FRAME = 32
  };

  /** Flags used in options view. */
  enum OptionsViewFlags {
    OPTIONS_VIEW_FLAG_NONE = 0,
    /** Adds an OK button in the options view. */
    OPTIONS_VIEW_FLAG_OK = 1,
    /** Adds an Cancel button in the options view. */
    OPTIONS_VIEW_FLAG_CANCEL = 2
  };

  /** Cursor types that can be used by elements. */
  enum CursorType {
    CURSOR_DEFAULT = 0, // Used as the default value.
    CURSOR_ARROW,
    CURSOR_IBEAM,
    CURSOR_WAIT,
    CURSOR_CROSS,
    CURSOR_UPARROW,
    CURSOR_SIZE,
    CURSOR_SIZENWSE,
    CURSOR_SIZENESW,
    CURSOR_SIZEWE,
    CURSOR_SIZENS,
    CURSOR_SIZEALL,
    CURSOR_NO,
    CURSOR_HAND,
    CURSOR_BUSY,
    CURSOR_HELP
  };

  /**
   * The supported debug modes for drawing view.
   * They can be used together as bit masks.
   * Only take effect when compiling with _DEBUG defined.
   */
  enum DebugMode {
    DEBUG_DISABLED = 0,    // No debug at all.
    DEBUG_CONTAINER = 1,   // Draw bounding boxes around container elements.
    DEBUG_ALL = 2,         // Draw bounding boxes around all elements.
    DEBUG_CLIP_REGION = 4 // Draw bounding boxes around clip region.
  };

  /** Hit test enumerates for both View and BasicElement */
  enum HitTest {
    HT_TRANSPARENT,
    HT_NOWHERE,
    HT_CLIENT,
    HT_CAPTION,
    HT_SYSMENU,
    HT_SIZE,
    HT_MENU,
    HT_HSCROLL,
    HT_VSCROLL,
    HT_MINBUTTON,
    HT_MAXBUTTON,
    HT_LEFT,
    HT_RIGHT,
    HT_TOP,
    HT_TOPLEFT,
    HT_TOPRIGHT,
    HT_BOTTOM,
    HT_BOTTOMLEFT,
    HT_BOTTOMRIGHT,
    HT_BORDER,
    HT_OBJECT,
    HT_CLOSE,
    HT_HELP
  };

  virtual ~ViewInterface() { }

  /**
   * @return the Gadget instance which owns this view.
   */
  virtual GadgetInterface* GetGadget() const = 0;

  /**
   * Returns the @c GraphicsInterface associated with this view.
   *
   * The returned @c GraphicsInterface instance is owned by this View, the
   * caller shall not delete it.
   */
  virtual GraphicsInterface *GetGraphics() const = 0;

  /**
   * Switches the view to another view host. The old view host will be
   * returned.
   *
   * The caller has responsibility to destroy the returned old view host.
   */
  virtual ViewHostInterface *SwitchViewHost(ViewHostInterface *new_host) = 0;

  /**
   * Gets the view host currently used by the view. The caller shall not
   * destroy it.
   */
  virtual ViewHostInterface *GetViewHost() const = 0;

  /**
   * Set the width of the view.
   * */
  virtual void SetWidth(double width) = 0;

  /**
   * Set the height of the view.
   */
  virtual void SetHeight(double height) = 0;

  /**
   * Set the size of the view. Use this when setting both height and width
   * to prevent two invocations of the sizing event.
   * */
  virtual void SetSize(double width, double height) = 0;

  /** Retrieves the width of the view in pixels. */
  virtual double GetWidth() const = 0;

  /** Retrieves the height of view in pixels. */
  virtual double GetHeight() const = 0;

  /**
   * Retrieves the default size of view in pixels.
   *
   * Normally, the size set in view's xml description file will be returned.
   * If there is no default size in xml description, then the first size set to
   * View will be used as the default size.
   */
  virtual void GetDefaultSize(double *width, double *height) const = 0;

  /**
   * Indicates what happens when the user attempts to resize the gadget using
   * the window border.
   * @see ResizableMode
   */
  virtual void SetResizable(ResizableMode resizable) = 0;
  virtual ResizableMode GetResizable() const = 0;

  /**
   * Caption is the title of the view, by default shown when a gadget is in
   * floating/expanded mode but not shown when the gadget is in the Sidebar.
   * @see SetShowCaptionAlways()
   */
  virtual void SetCaption(const std::string &caption) = 0;
  virtual std::string GetCaption() const = 0;

  /**
   * When @c true, the Sidebar always shows the caption for this view.
   * By default this value is @c false.
   */
  virtual void SetShowCaptionAlways(bool show_always) = 0;
  virtual bool GetShowCaptionAlways() const = 0;

  /**
   * Sets the view's rectangular resize area.
   *
   * It's only valid for resizable view.
   * To define a non-rectangular resize border, use regular UI elements
   * (such as img) and the hitTest property.
   */
  virtual void SetResizeBorder(double left, double top,
                               double right, double bottom) = 0;

  /**
   * Gets the view's rectangular resize ares.
   *
   * If resize area is not defined yet, then this method returns false.
   */
  virtual bool GetResizeBorder(double *left, double *top,
                               double *right, double *bottom) const = 0;

  /**
   * Sets a redraw mark, so that all things of this view will be redrawed
   * during the next draw.
   */
  virtual void MarkRedraw() = 0;

  /**
   * Layout the view to make sure the clip region is up-to-date.
   * It'll be called by view host just before calling Draw().
   */
  virtual void Layout() = 0;

  /**
   * Draws the current view to a canvas.
   * The specified canvas shall already be prepared to be drawn directly
   * without any transformation.
   * @param canvas A canvas for the view to be drawn on. It shall have the same
   * zooming factory as the whole gadget.
   */
  virtual void Draw(CanvasInterface *canvas) = 0;

  /**
   * Gets current clip region of the view.
   *
   * Usually it'll be called by host before calling Draw() method to draw the
   * View onto screen. View host can optimize the drawing operation by
   * making use of the clip region.
   *
   * If NULL or an empty ClipRegion is returned, then the whole region of the
   * view will be redrawn.
   *
   * The clip region must already be integerized.
   */
  virtual const ClipRegion *GetClipRegion() const = 0;

  /**
   * Adds a rectangle to current clip region.
   *
   * It's usually be called by view host before calling Draw(), if the host
   * wants to draw additional area of the view besides the area covered by
   * current clip region.
   *
   * The specified rectangle shall already be integerized.
   */
  virtual void AddRectangleToClipRegion(const Rectangle &rect) = 0;

 public: // Event handlers.
  /** Handler of the mouse events. */
  virtual EventResult OnMouseEvent(const MouseEvent &event) = 0;

  /** Handler of the keyboard events. */
  virtual EventResult OnKeyEvent(const KeyboardEvent &event) = 0;

  /**
   * Handler of the drag and drop events.
   * @param event the drag and drop event.
   * @return @c EVENT_RESULT_HANDLED if the dragged contents are accepted by
   *     an element.
   */
  virtual EventResult OnDragEvent(const DragEvent &event) = 0;

  /**
   * Handler of other simple events, except the onsizing event.
   * @param event the input event.
   * @return the result of event handling.
   */
  virtual EventResult OnOtherEvent(const Event &event) = 0;

  /**
   * Gets the hit test value for this view.
   *
   * This method will be called immediately after calling OnMouseEvent().
   * The default implementation returns the hit test value of the element
   * currently pointed by the moust pointer.
   */
  virtual HitTest GetHitTest() const = 0;

  /**
   * Called by the host to let the view add customized context menu items, and
   * control whether the context menu should be shown.
   * @return @c false if the handler doesn't want the default menu items shown.
   *     If no menu item is added in this handler, and @c false is returned,
   *     the host won't show the whole context menu.
   */
  virtual bool OnAddContextMenuItems(MenuInterface *menu) = 0;

  /**
   * Called by the host to negotiate the sizing operation.
   * @param[in/out] width The new width of the View that host wants. The view
   *                can adjust the value.
   * @param[in/out] height The new height of the View that host wants. The view
   *                can adjust the value.
   * @return true if the sizing request is accepted, then the host shall set the
   *         size of the View to the new value stored in width and height.
   *         Otherwise returns false.
   */
  virtual bool OnSizing(double *width, double *height) = 0;
};

} // namespace ggadget

#endif // GGADGET_VIEW_INTERFACE_H__
