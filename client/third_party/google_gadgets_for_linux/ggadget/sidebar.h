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

#ifndef GGADGET_SIDEBAR_H__
#define GGADGET_SIDEBAR_H__

#include <ggadget/common.h>
#include <ggadget/variant.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/menu_interface.h>
#include <ggadget/view.h>

namespace ggadget {

class HostInterface;
class ViewElement;

/**
 * @defgroup SideBar SideBar
 * @ingroup CoreLibrary
 * Platform-independent helper class for sidebar host.
 */

/**
 * @ingroup SideBar
 *
 * Object that represent the side bar.
 * SideBar is a container of view element, each view element is combined with a
 * ViewHost.
 */
class SideBar {
 public:
  /*
   * Constructor.
   * @param view_host @c SideBar acts as @c View, @c view_host is the ViewHost
   *        instance that is associated with the @c SideBar instance.
   *        It's not owned by the object, and shall be destroyed after
   *        destroying the object.
   */
  SideBar(ViewHostInterface *view_host);

  /* Destructor. */
  virtual ~SideBar();

 public:
  /**
   * Enables or disables initialization mode.
   *
   * In initialization mode, the index specified when calling NewVhewHost() is
   * the initial index, and all newly created view hosts will be sorted by
   * initial indexes.
   */
  void SetInitializing(bool initializing);

  /**
   * Creates a new ViewHost instance and of curse a new view element
   * hold in the side bar.
   *
   * @param index The index in sidebar of the new ViewHost instance.
   * @return a new Viewhost instance.
   */
  ViewHostInterface *NewViewHost(size_t index);

  /**
   * @return the ViewHost instance associated with the sidebar instance.
   */
  ViewHostInterface *GetSideBarViewHost() const;

  /**
   * Set the size of the sidebar.
   */
  void SetSize(double width, double height);

  /** Retrieves the width of the side bar in pixels. */
  double GetWidth() const;

  /** Retrieves the height of side bar in pixels. */
  double GetHeight() const;

  /** Shows sidebar window. */
  void Show();

  /** Hides sidebar window. */
  void Hide();

  /**
   * Minimizes sidebar at specified orientation.
   *
   * If vertical is true, then sidebar will be minimized to a thin line which
   * can be snapped to monitor left or right border. Otherwise, sidebar will be
   * minimized to a small window only containing the button icons.
   *
   * If sidebar is minimized, the size of sidebar will be changed accordingly.
   * And the new size set by SetSize() will be taken effect when it's restored.
   */
  void Minimize(bool vertical);

  /** Restores sidebar's size. */
  void Restore();

  /** Checks if sidebar is in minimized mode. */
  bool IsMinimized() const;

  /**
   * Retrieves the index of the view in the sidebar at sepcified vertical
   * position. If the position is below all of the views, the count of view
   * will be returned, indicating that a new view can be inserted here.
   * @param y the vertical position in sidebar.
   */
  size_t GetIndexOfPosition(double y) const;

  /**
   * Retrieves the index of a specified view, if the view is not in the sidebar
   * then @c kInvalidIndex will be returned.
   */
  size_t GetIndexOfView(const ViewInterface *view) const;

  /**
   * Insert a place holder in the side bar.
   *
   * @param index The index of position in the sidebar of the place holder.
   * @param height The height of the place holder.
   */
  void InsertPlaceholder(size_t index, double height);

  /**
   * Clear place holder
   */
  void ClearPlaceholder();

  /**
   * Enumerate views in the sidebar.
   * The callback will receive two parameters, first is the index of the
   * view and the second is the pointer point to the view and should return
   * true if it wants the enumeration to continue, or false to break the
   * enumeration.
   */
  void EnumerateViews(Slot2<bool, size_t, View *> *slot);

  /**
   * Event connection methods.
   */
  /**
   * Connects a slot to OnUndock signal, which will be emitted when user wants
   * to undock a gadget by dragging it.
   * The first parameter is the child view to be undocked.
   * The second parameter is the index of the child view.
   * The third and fourth parameter are the mouse position relative to the
   * child view's coordinates.
   */
  Connection *ConnectOnUndock(Slot4<void, View*, size_t, double, double> *slot);

  /**
   * Connects a slot to OnClick signal, which will be emitted when a mouse
   * click event occurred.
   * The first parameter is the child view under the mouse pointer, or NULL if
   * the click event is occurred on sidebar.
   */
  Connection *ConnectOnClick(Slot1<void, View*> *slot);

  /**
   * Connects a slot to OnAddGadget signal, which will be emitted when add
   * gadget button is clicked.
   */
  Connection *ConnectOnAddGadget(Slot0<void> *slot);

  /**
   * Connects a slot to OnMenu signal, which will be emitted when menu button
   * is clicked.
   */
  Connection *ConnectOnMenu(Slot1<void, MenuInterface *> *slot);

  /**
   * Connects a slot to OnClose signal, which will be emitted when close button
   * is clicked.
   */
  Connection *ConnectOnClose(Slot0<void> *slot);

  /**
   * Connects a slot to OnSizeEvent signal, which will be emitted when sidebar's
   * size is changed.
   */
  Connection *ConnectOnSizeEvent(Slot0<void> *slot);

  /**
   * Connects a slot to OnViewMoved signal, which will be emitted when a child
   * view is moved inside sidebar.
   */
  Connection *ConnectOnViewMoved(Slot1<void, View *> *slot);

  /**
   * Connects a slot to Google icon's onclick signal, which will be emitted when
   * Google icon is clicked.
   */
  Connection *ConnectOnGoogleIconClicked(Slot0<void> *slot);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(SideBar);
};

}  // namespace ggadget

#endif  // GGADGET_SIDEBAR_H__
