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

#include "ggadget/common.h"
#include "ggadget/slot.h"
#include "ggadget/win32/menu_builder.h"
#include "unittest/gtest.h"

using ggadget::scoped_ptr;
using ggadget::MenuInterface;
using ggadget::win32::MenuBuilder;
using ggadget::down_cast;
using ggadget::NewSlot;

class MenuBuilderTest : public testing::Test {
 public:
  void ItemHandler(const char* text, int item_id) {
    GGL_UNUSED(text);
    clicked_item_id_ = item_id;
  }

 protected:
  MenuBuilderTest() : clicked_item_id_(-1) {
    menu_builder_.reset(new MenuBuilder);
  }

  scoped_ptr<MenuBuilder> menu_builder_;
  int clicked_item_id_;
};

TEST_F(MenuBuilderTest, InsertAndModifyItem) {
  // The actual menu will be like:
  // |------------|
  // | Second Item|
  // |------------|
  // | First Item |
  // |------------|
  menu_builder_->AddItem("First Item", MenuInterface::MENU_ITEM_FLAG_GRAYED, 0,
                         NewSlot(down_cast<MenuBuilderTest*>(this),
                                 &MenuBuilderTest::ItemHandler, 1),
                         MenuInterface::MENU_ITEM_PRI_HOST);
  menu_builder_->AddItem("", MenuInterface::MENU_ITEM_FLAG_SEPARATOR, 0, NULL,
                         MenuInterface::MENU_ITEM_PRI_HOST);
  menu_builder_->AddItem("Second Item", MenuInterface::MENU_ITEM_FLAG_CHECKED,
                         0,
                         NewSlot(down_cast<MenuBuilderTest*>(this),
                                 &MenuBuilderTest::ItemHandler, 3),
                         MenuInterface::MENU_ITEM_PRI_CLIENT);
  HMENU menu = CreatePopupMenu();
  menu_builder_->BuildMenu(0, &menu);
  EXPECT_NE(0, ::GetMenuItemCount(menu));
  MENUITEMINFO menu_item_info = {0};
  menu_item_info.cbSize = sizeof(menu_item_info);
  menu_item_info.fMask = MIIM_FTYPE | MIIM_STATE;
  ::GetMenuItemInfo(menu, 0, true, &menu_item_info);
  EXPECT_NE(static_cast<unsigned int>(0),
            (menu_item_info.fState & MFS_CHECKED));
  menu_builder_->SetItemStyle("Second Item",
                              MenuInterface::MENU_ITEM_FLAG_GRAYED);
  ::DestroyMenu(menu);
  menu = CreatePopupMenu();
  menu_builder_->BuildMenu(0, &menu);
  ::GetMenuItemInfo(menu, 0, true, &menu_item_info);
  EXPECT_EQ(static_cast<unsigned int>(0),
            (menu_item_info.fState & MFS_CHECKED));
  EXPECT_NE(static_cast<unsigned int>(0), (menu_item_info.fState & MFS_GRAYED));
  menu_builder_->SetItemStyle("First Item",
                              MenuInterface::MENU_ITEM_FLAG_SEPARATOR);
  ::DestroyMenu(menu);
  menu = CreatePopupMenu();
  menu_builder_->BuildMenu(0, &menu);
  ::GetMenuItemInfo(menu, 2, true, &menu_item_info);
  EXPECT_NE(static_cast<unsigned int>(0),
            (menu_item_info.fType & MFT_SEPARATOR));
  ::DestroyMenu(menu);
}

TEST_F(MenuBuilderTest, SubMenuAndCommand) {
  // The actual menu will be like:
  // |-------------|----------------|
  // | pop up      | First Sub Item |
  // |-------------|----------------|
  // | First Item  |
  // | Second Item |
  // |-------------|
  menu_builder_->AddItem("First Item", MenuInterface::MENU_ITEM_FLAG_GRAYED,
                         0,
                         NewSlot(down_cast<MenuBuilderTest*>(this),
                                 &MenuBuilderTest::ItemHandler, 1),
                         MenuInterface::MENU_ITEM_PRI_DECORATOR);
  menu_builder_->AddItem("", MenuInterface::MENU_ITEM_FLAG_SEPARATOR, 0, NULL,
                         MenuInterface::MENU_ITEM_PRI_CLIENT);
  MenuBuilder* sub_menu =
      down_cast<MenuBuilder*>(menu_builder_->AddPopup(
          "pop up", MenuInterface::MENU_ITEM_PRI_CLIENT));
  sub_menu->AddItem("First Sub Item", 0, 0,
                    NewSlot(down_cast<MenuBuilderTest*>(this),
                            &MenuBuilderTest::ItemHandler, 11),
                    MenuInterface::MENU_ITEM_PRI_HOST);
  menu_builder_->AddItem("Second Item", MenuInterface::MENU_ITEM_FLAG_CHECKED,
                         0,
                         NewSlot(down_cast<MenuBuilderTest*>(this),
                                 &MenuBuilderTest::ItemHandler, 3),
                         MenuInterface::MENU_ITEM_PRI_HOST);
  HMENU menu = CreatePopupMenu();
  menu_builder_->BuildMenu(0, &menu);
  EXPECT_TRUE(menu != NULL);
  EXPECT_TRUE(menu_builder_->OnCommand(0));
  EXPECT_EQ(11, clicked_item_id_);
  EXPECT_TRUE(menu_builder_->OnCommand(1));
  EXPECT_EQ(1, clicked_item_id_);
  EXPECT_TRUE(menu_builder_->OnCommand(2));
  EXPECT_EQ(3, clicked_item_id_);
  EXPECT_FALSE(menu_builder_->OnCommand(10));
  ::DestroyMenu(menu);
}

int main(int argc, char *argv[]) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
