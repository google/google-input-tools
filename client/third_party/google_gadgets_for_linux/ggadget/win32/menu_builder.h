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

#ifndef GGADGET_WIN32_MENU_BUILDER_H_
#define GGADGET_WIN32_MENU_BUILDER_H_

#include <ggadget/menu_interface.h>
#include <ggadget/scoped_ptr.h>

namespace ggadget {

class ImageInterface;

namespace win32 {
// This class implements MenuInterface using win32 sdk.
// All the texts used in this class should be in UTF8 format.
class MenuBuilder : public MenuInterface {
 public:
  MenuBuilder();
  virtual ~MenuBuilder();
  // Adds a single menu item with style specified by the parameter style and the
  // display text specified by the parameter item_text. If item_text is NULL
  // or empty, a menu separator will be added.
  virtual void AddItem(const char* item_text, int style, int stock_icon,
                       Slot1<void, const char*>* handler, int priority);
  // Adds a single menu item with style specified by the parameter style, the
  // display text specified by the parameter item_text and the ImageInterface*
  // of menu item icon specified by the parameter image_icon.
  // If item_text is NULL or empty, a menu separator will be added.
  virtual void AddItem(const char* item_text, int style,
                       ImageInterface* image_icon,
                       Slot1<void, const char*>* handler, int priority);
  // Sets the style of the menu item whose display text is item_text.
  virtual void SetItemStyle(const char* item_text, int style);
  // Adds a submenu/popup showing the given text.
  // Returns the MenuInterface pointer of the submenu.
  virtual MenuInterface* AddPopup(const char* popup_text, int priority);
  // Sets hint for positioning the popup menu on the screen. The viewhost should
  // try its best to avoid overlapping the rectangular hint area when showing
  // the context menu.
  virtual void SetPositionHint(const Rectangle &rect);
  // Returns the hint rectangle.
  Rectangle GetPositionHint();
  // Builds the native menu given in the HMENU object.
  // This function will first sort menu items according to their priorities and
  // then assign a unique command id (start from start_id) for each menu item
  // under this menu and all the sub menus sequentially, and finally insert the
  // menu items into the HMENU object.
  // The command id is need for finding the selected menu item when a specific
  // menu item is clicked.
  // This method will insert the menu items into the HMENU, so caller should
  // provide a valid blank HMENU to this method, and the caller should also
  // manage the life cycle of the HMENU object.
  void BuildMenu(int16_t start_id, HMENU* menu);
  // Executes the event handler of selected menu item indicates by the
  // command_id which is the identifier of the menu item, usually obtained
  // from the WM_COMMAND message.
  bool OnCommand(int16_t command_id) const;
  // Returns true if no menu item has been added.
  bool IsEmpty();
  // Sorts the menu items and assign command ids for each menu items.
  // Must be called before calling GetMenuItem.
  void PreBuildMenu(int16_t start_id);
  // Returns the count of the menu items.
  int GetItemCount() const;
  // Gets the information for the menu item of the given index.
  // PreBuildMenu must be called before calling this function.
  void GetMenuItem(int index, std::string* text, int* style,
                   HBITMAP* image_icon, int* command_id,
                   MenuBuilder** sub_menu) const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(MenuBuilder);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_MENU_BUILDER_H_
