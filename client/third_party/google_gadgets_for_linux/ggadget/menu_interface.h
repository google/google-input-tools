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

#ifndef GGADGET_MENU_INTERFACE_H__
#define GGADGET_MENU_INTERFACE_H__

#include <ggadget/variant.h>

namespace ggadget {

class ImageInterface;
class Rectangle;
template <typename R, typename P1> class Slot1;

/**
 * @ingroup Interfaces
 * Interface class for abstracting menu related operations.
 */
class MenuInterface {
 protected:
  virtual ~MenuInterface() { }

 public:
  /** Compatible with menu flags defined in Windows. */
  enum MenuItemFlag {
    MENU_ITEM_FLAG_GRAYED    = 0x0001,
    MENU_ITEM_FLAG_CHECKED   = 0x0008,
    MENU_ITEM_FLAG_SEPARATOR = 0x0800
  };

  /**
   * Stock icons that can be used for a menu item.
   *
   * It's for C++ code only.
   */
  enum MenuItemStockIcon {
    MENU_ITEM_ICON_NONE = 0,
    MENU_ITEM_ICON_ABOUT,
    MENU_ITEM_ICON_ADD,
    MENU_ITEM_ICON_APPLY,
    MENU_ITEM_ICON_CANCEL,
    MENU_ITEM_ICON_CLOSE,
    MENU_ITEM_ICON_COPY,
    MENU_ITEM_ICON_CUT,
    MENU_ITEM_ICON_DELETE,
    MENU_ITEM_ICON_HELP,
    MENU_ITEM_ICON_NEW,
    MENU_ITEM_ICON_NO,
    MENU_ITEM_ICON_OK,
    MENU_ITEM_ICON_OPEN,
    MENU_ITEM_ICON_PASTE,
    MENU_ITEM_ICON_PREFERENCES,
    MENU_ITEM_ICON_QUIT,
    MENU_ITEM_ICON_REFRESH,
    MENU_ITEM_ICON_REMOVE,
    MENU_ITEM_ICON_STOP,
    MENU_ITEM_ICON_YES,
    MENU_ITEM_ICON_ZOOM_100,
    MENU_ITEM_ICON_ZOOM_FIT,
    MENU_ITEM_ICON_ZOOM_IN,
    MENU_ITEM_ICON_ZOOM_OUT
  };

  enum MenuItemPriority {
    /** For menu items added by client code, like elements or javascript. */
    MENU_ITEM_PRI_CLIENT = 0,
    /** For menu items added by view decorator. */
    MENU_ITEM_PRI_DECORATOR = 10,
    /** For menu items added by host. */
    MENU_ITEM_PRI_HOST = 20,
    /** For menu items added by Gadget. */
    MENU_ITEM_PRI_GADGET = 30
  };

  /**
   * Adds a single menu item. If @a item_text is blank or NULL, a menu
   * separator will be added.
   *
   * @param item_text the text displayed in the menu item. '&'s act as hotkey
   *     indicator. If it's blank or NULL, style is automatically treated as
   *     @c MENU_ITEM_FLAG_SEPARATOR.
   * @param style combination of <code>MenuItemFlag</code>s.
   * @param stock_icon Optional stock icon for this menu item, if
   *        MENU_ITEM_FLAG_CHECKED is used, then the icon will be ignored.
   * @param handler handles menu command.
   * @param priority Priority of the menu item, item with smaller priority will
   *     be placed to higher position in the menu. Must be >= 0.
   *     0-9 is reserved for menu items added by Element and JavaScript.
   *     10-19 is reserved for menu items added by View Decorator.
   *     20-29 is reserved for menu items added by host.
   *     30-39 is reserved for menu items added by Gadget.
   */
  virtual void AddItem(const char *item_text, int style, int stock_icon,
                       Slot1<void, const char *> *handler, int priority) = 0;

  /**
   * Adds a single menu item with specified menu icon.
   * The menu item will own the icon object, so the caller does not need
   * to delete the image.
   * If @a item_text is blank or NULL, a menu separator will be added.
   *
   * @param item_text the text displayed in the menu item. '&'s act as hotkey
   *     indicator. If it's blank or NULL, style is automatically treated as
   *     @c MENU_ITEM_FLAG_SEPARATOR.
   * @param style combination of <code>MenuItemFlag</code>s.
   * @param image_icon the icon of the menu item, it's wrapped into a
   *     ImageInterface* object.
   * @param handler handles menu command.
   * @param priority Priority of the menu item, item with smaller priority will
   *     be placed to higher position in the menu. Must be >= 0.
   *     0-9 is reserved for menu items added by Element and JavaScript.
   *     10-19 is reserved for menu items added by View Decorator.
   *     20-29 is reserved for menu items added by host.
   *     30-39 is reserved for menu items added by Gadget.
   */
  virtual void AddItem(const char *item_text, int style,
                       ImageInterface* image_icon,
                       Slot1<void, const char *> *handler, int priority) = 0;

  /**
   * Sets the style of the given menu item.
   * @param item_text
   * @param style combination of <code>MenuItemFlag</code>s.
   */
  virtual void SetItemStyle(const char *item_text, int style) = 0;

  /**
   * Adds a submenu/popup showing the given text.
   * @param popup_text
   * @param style combination of <code>MenuItemFlag</code>s.
   * @param priority of the popup menu item.
   * @return the menu object of the new popup menu.
   */
  virtual MenuInterface *AddPopup(const char *popup_text, int priority) = 0;

  /**
   * Sets hint for positioning the popup menu on the screen, which is a
   * rectangle area in the coordinates of ViewHost's native widget. If the hint
   * is set then the view host should try its best to show the menu outside the
   * area and attach the menu to one of the edges. If the hint is not set then
   * it's up to the view host to decide where to show the menu, usually it's
   * under the mouse pointer.
   * The hint only makes sense to the root menu.
   */
  virtual void SetPositionHint(const Rectangle &rect) = 0;
};

/**
 * Make sure that MenuInterface pointer can be transfered through
 * signal-slot. Some code depends on it.
 */
DECLARE_VARIANT_PTR_TYPE(MenuInterface);

} // namespace ggadget

#endif  // GGADGET_MENU_INTERFACE_H__
