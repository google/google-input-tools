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

// This file contains the implementation of the MenuBuilder class, including
// the definition of the ineer implementation class MenuBuilder::Impl, and a
// struct MenuItem that holds the information of each single menu item.

#include "menu_builder.h"

#include <algorithm>
// Gdiplus.h uses the function min and max but doesn't define them, So
// use these two functions before include gdiplus.h.
using std::max;
using std::min;
#include <gdiplus.h>
#include <string>
#include <vector>

#include <ggadget/common.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/image_interface.h>
#include <ggadget/logger.h>
#include <ggadget/math_utils.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/slot.h>
#include "gdiplus_canvas.h"
#include "gdiplus_image.h"

namespace ggadget {
namespace win32 {

// This struct contains the information of a menu item.
struct MenuItem {
  // The menu text in utf8.
  std::string text;
  // The action handler of the menu item.
  Slot1<void, const char *> *handler;
  // The priority of this item, see MenuItemPriority.
  int priority;
  // A pointer to the Win32MenuBuilder object of sub menu. It is set to NULL if
  // this item is not a sub menu (pop up menu).
  MenuBuilder* child;
  // A unique identifier of this item. It identified which item is selected when
  // the menu is clicked on.
  int command_id;
  // The style of the menu item. It is a combination of MenuItemFlag.
  int style;
  // The bitmap handler of the menu item icon.
  HBITMAP icon_bitmap;
};

namespace {

bool CompareMenuItem(const MenuItem& a, const MenuItem& b) {
  return a.priority < b.priority;
}
bool IsRTLLayout() {
  // We can't detect the layout of application in ipc_console, so we use the
  // layout of shell window as the default layout.
  return ::GetWindowLong(::GetShellWindow(), GWL_EXSTYLE) &
         WS_EX_LAYOUTRTL;
}

}  // namespace

class MenuBuilder::Impl {
 public:
  Impl() : position_hint_() {
  }

  ~Impl() {
    for (size_t i = 0; i < menu_items_.size(); ++i) {
      delete menu_items_[i].handler;
      delete menu_items_[i].child;
      if (menu_items_[i].icon_bitmap != 0)
        DeleteObject(menu_items_[i].icon_bitmap);
    }
  }

  void AddItem(const char* item_text,
               int style,
               ImageInterface* image_icon,
               Slot1<void, const char*>* handler,
               int priority) {
    MenuItem item;
    item.handler = handler;
    item.priority = priority;
    item.command_id = -1;
    item.child = NULL;
    item.style = style;
    if (item_text && *item_text)
      item.text = item_text;
    else
      item.style |= MenuInterface::MENU_ITEM_FLAG_SEPARATOR;
    if (image_icon) {
      SetItemIconBitmap(image_icon, style, &item.icon_bitmap);
      DestroyImage(image_icon);
    } else {
      item.icon_bitmap = 0;
    }
    menu_items_.push_back(item);
  }

  void SetItemStyle(const char* item_text, int style) {
    std::string item_text_utf8 = item_text ? item_text : "";
    for (size_t i = 0; i < menu_items_.size(); ++i) {
      if (menu_items_[i].text == item_text_utf8) {
        menu_items_[i].style = style;
        break;
      }
    }
  }

  // Get the bitmap handler for a menu item icon.
  void SetItemIconBitmap(ImageInterface* icon,
                         int style,
                         HBITMAP* icon_bitmap) {
    GdiplusImage mark;
    // If it's checked menu item, draw the checked mark
    // image to mark; Otherwise, leave mark blank.
    if (style & MenuInterface::MENU_ITEM_FLAG_CHECKED) {
      FileManagerInterface* fm = GetGlobalFileManager();
      std::string image_data;
      fm->ReadFile(kMenuCheckedMarkIcon, &image_data);
      mark.Init(kMenuCheckedMarkIcon, image_data, false);
    } else {
      mark.Init(0, 0);
    }
    CombineImagesToHBitmap(static_cast<ImageInterface*>(&mark),
                           icon,
                           icon_bitmap);
  }

  // Converting Gdiplus::Bitmap to HBITMAP preseving the alpha channel.
  HBITMAP ToHBitMap(Gdiplus::Bitmap* bmp) {
    BITMAPINFO info = {0};
    int size = bmp->GetWidth() * bmp->GetHeight() * 4;
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = bmp->GetWidth();
    // Negative height means the bitmap's origin is the upper-left corner.
    info.bmiHeader.biHeight = -static_cast<LONG>(bmp->GetHeight());
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = size;
    void* bits = NULL;
    HDC dc = ::GetDC(NULL);
    HBITMAP hbitmap = ::CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits,
                                         NULL, 0);
    ::ReleaseDC(NULL, dc);
    Gdiplus::BitmapData data = {0};
    data.Width = bmp->GetWidth();
    data.Height = bmp->GetHeight();
    data.PixelFormat = PixelFormat32bppPARGB;
    data.Scan0 = bits;
    data.Stride = data.Width * 4;

    Gdiplus::Rect rect(0, 0, bmp->GetWidth(), bmp->GetHeight());
    bmp->LockBits(
        &rect,
        Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf,
        PixelFormat32bppPARGB, &data);
    bmp->UnlockBits(&data);
    return hbitmap;
  }

  // Combine two images to one, and return the handler of corresponding bitmap.
  void CombineImagesToHBitmap(ImageInterface* left_image,
                              ImageInterface* right_image,
                              HBITMAP* hbitmap) {
    // TODO(jiangwei): Consider to do some smooth stretches to these images
    // if they don't fit the size of menu items well.
    // Calculate the size of the combined image.
    int width = static_cast<int>(
        std::max(left_image->GetWidth(), right_image->GetWidth()) * 2);
    int height = static_cast<int>(
        std::max(left_image->GetHeight(), right_image->GetHeight()));
    // Calculate the relative position of each image.
    int left_image_pos_x =
        static_cast<int>((width / 2 - left_image->GetWidth()) / 2);
    int left_image_pos_y =
        static_cast<int>((height - left_image->GetHeight()) / 2);
    int right_image_pos_x =
        static_cast<int>(width / 2 +
                           (width / 2 - right_image->GetWidth()) / 2);
    int right_image_pos_y =
        static_cast<int>((height - right_image->GetHeight()) / 2);
    // Draw each image on the canvas of the new image;
    GdiplusCanvas canvas;
    canvas.Init(NULL, width, height, true);
    if (IsRTLLayout()) {
      // Windows automatically flip the menu icon when showing the menu with in
      // rtl layout, actually we don't want to flip the content of both image,
      // but only flip the arrangement of these two image, so we flip the image
      // content first.
      GdiplusCanvas left;
      left.Init(NULL, left_image->GetWidth(), left_image->GetHeight(), true);
      left_image->Draw(&left, 0, 0);
      left.GetImage()->RotateFlip(Gdiplus::RotateNoneFlipX);
      canvas.DrawCanvas(left_image_pos_x, left_image_pos_y, &left);
      GdiplusCanvas right;
      right.Init(NULL, right_image->GetWidth(), right_image->GetHeight(), true);
      right_image->Draw(&right, 0, 0);
      right.GetImage()->RotateFlip(Gdiplus::RotateNoneFlipX);
      canvas.DrawCanvas(right_image_pos_x, right_image_pos_y, &right);
    } else {
      left_image->Draw(static_cast<ggadget::CanvasInterface*>(&canvas),
                       left_image_pos_x, left_image_pos_y);
      right_image->Draw(static_cast<ggadget::CanvasInterface*>(&canvas),
                        right_image_pos_x, right_image_pos_y);
    }
    // Get the bitmap handler directly from the canvas.
    *hbitmap = ToHBitMap(canvas.GetImage());
  }


  MenuInterface* AddPopup(const char* popup_text, int priority) {
    MenuBuilder* pop_up_menu = new MenuBuilder;
    MenuItem item;
    item.text = popup_text;
    item.handler = NULL;
    item.priority = priority;
    item.command_id = -1;
    item.child = pop_up_menu;
    item.style = 0;
    item.icon_bitmap = 0;
    menu_items_.push_back(item);
    return pop_up_menu;
  }

  // Assigns a unique command id for each menu item (including the items of
  // sub menus) sequentially. The parameter unused_id will be the command id of
  // the first menu item and will be modified to the last menu item's command id
  // + 1 after calling this function.
  void AssignCommandID(int16_t* unused_id) {
    for (size_t i = 0; i < menu_items_.size(); ++i) {
      if (menu_items_[i].child == NULL) {
        if (menu_items_[i].style != MenuInterface::MENU_ITEM_FLAG_SEPARATOR) {
          menu_items_[i].command_id = *unused_id;
          ++*unused_id;
        }
      } else {
        menu_items_[i].child->impl_->AssignCommandID(unused_id);
      }
    }
  }

  // Finds a menu item in all sub items and all sub menus.
  // @param command_id: the command id of the menu item to be found.
  const MenuItem* FindMenuItemByCommandId(uint16_t command_id) const {
    const MenuItem* item = NULL;
    for (size_t i = 0; i < menu_items_.size(); ++i) {
      if (menu_items_[i].child == NULL &&
          menu_items_[i].command_id == command_id) {
        return &menu_items_[i];
      } else if (menu_items_[i].child != NULL) {
        item = menu_items_[i].child->impl_->FindMenuItemByCommandId(command_id);
        if (item != NULL)
          return item;
      }
    }
    return item;
  }

  bool OnCommand(int16_t command_id) const {
    const MenuItem* item = FindMenuItemByCommandId(command_id);
    if (item != NULL && item->handler != NULL) {
      (*item->handler)(item->text.c_str());
      return true;
    }
    return false;
  }

  // Sorts the menu items according to their priorities.
  void SortMenuItems() {
    std::stable_sort(menu_items_.begin(), menu_items_.end(), CompareMenuItem);
    for (size_t i = 0; i < menu_items_.size(); ++i) {
      if (menu_items_[i].child != NULL)
        menu_items_[i].child->impl_->SortMenuItems();
    }
  }

  void BuildNativeMenu(HMENU* menu) {
    int previous_piority = -1;
    for (size_t i = 0; i < menu_items_.size(); ++i) {
      // Add a seperator between menu items with different piorities.
      if (i > 0 &&
          menu_items_[i].priority != previous_piority &&
          !(menu_items_[i-1].style & MenuInterface::MENU_ITEM_FLAG_SEPARATOR) &&
          !(menu_items_[i].style & MenuInterface::MENU_ITEM_FLAG_SEPARATOR)) {
        ::AppendMenu(*menu, MF_SEPARATOR, 0, NULL);
      }
      previous_piority = menu_items_[i].priority;
      UINT menu_flags = MF_STRING;
      if (menu_items_[i].style & MenuInterface::MENU_ITEM_FLAG_CHECKED)
        menu_flags |= MF_CHECKED;
      if (menu_items_[i].style & MenuInterface::MENU_ITEM_FLAG_GRAYED)
        menu_flags |= MF_GRAYED;
      if (menu_items_[i].style & MenuInterface::MENU_ITEM_FLAG_SEPARATOR)
        menu_flags = MF_SEPARATOR;
      std::string item_text_utf8 = menu_items_[i].text;
      UTF16String item_text_utf16;
      ConvertStringUTF8ToUTF16(item_text_utf8, &item_text_utf16);
      if (menu_items_[i].child == NULL) {
        ::AppendMenu(*menu, menu_flags, menu_items_[i].command_id,
                     item_text_utf16.c_str());
      } else {
        HMENU sub_menu = ::CreatePopupMenu();
        menu_items_[i].child->impl_->BuildNativeMenu(&sub_menu);
        menu_flags |= MF_POPUP;
        ::AppendMenu(*menu, menu_flags, reinterpret_cast<UINT_PTR>(sub_menu),
                     item_text_utf16.c_str());
      }
      if (menu_items_[i].icon_bitmap != 0) {
        // It's ok to set the unchecked/checked icons with the same bitmap,
        // since only one of them will be shown.
        ::SetMenuItemBitmaps(*menu, i, MF_BYPOSITION,
                             menu_items_[i].icon_bitmap,
                             menu_items_[i].icon_bitmap);
      }
    }
  }

  void BuildMenu(int16_t start_id, HMENU* menu) {
    PreBuildMenu(start_id);
    BuildNativeMenu(menu);
  }

  void PreBuildMenu(int16_t start_id) {
    SortMenuItems();
    AssignCommandID(&start_id);
  }

  void GetMenuItem(int index, std::string* text, int* style,
                   HBITMAP* image_icon, int* command_id,
                   MenuBuilder** sub_menu) const {
    ASSERT(text);
    ASSERT(style);
    ASSERT(image_icon);
    ASSERT(command_id);
    ASSERT(sub_menu);
    ASSERT(static_cast<size_t>(index) < menu_items_.size());
    const MenuItem& item = menu_items_[index];
    *text = item.text;
    *style = item.style;
    *image_icon = item.icon_bitmap;
    *command_id = item.command_id;
    *sub_menu = item.child;
    *command_id = item.command_id;
  }

 private:
  std::vector<MenuItem> menu_items_;
  Rectangle position_hint_;
  friend class MenuBuilder;
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

MenuBuilder::MenuBuilder() : impl_(new Impl) {
}

MenuBuilder::~MenuBuilder() {
}

void MenuBuilder::AddItem(
    const char* item_text,
    int style,
    int stock_icon,
    Slot1<void, const char*>* handler,
    int priority) {
  GGL_UNUSED(stock_icon);
  impl_->AddItem(item_text, style, NULL, handler, priority);
}

void MenuBuilder::AddItem(
    const char* item_text,
    int style,
    ImageInterface* image_icon,
    Slot1<void, const char*>* handler,
    int priority) {
  impl_->AddItem(item_text, style, image_icon, handler, priority);
}

void MenuBuilder::SetItemStyle(const char* item_text, int style) {
  impl_->SetItemStyle(item_text, style);
}

MenuInterface* MenuBuilder::AddPopup(const char* popup_text,
                                     int priority) {
  return impl_->AddPopup(popup_text, priority);
}

void MenuBuilder::SetPositionHint(const Rectangle &rect) {
  impl_->position_hint_ = rect;
}

Rectangle MenuBuilder::GetPositionHint() {
  return impl_->position_hint_;
}

void MenuBuilder::BuildMenu(int16_t start_id, HMENU* menu) {
  impl_->BuildMenu(start_id, menu);
}

bool MenuBuilder::OnCommand(int16_t command_id) const {
  return impl_->OnCommand(command_id);
}

bool MenuBuilder::IsEmpty() {
  return impl_->menu_items_.empty();
}

void MenuBuilder::PreBuildMenu(int16_t start_id) {
  impl_->PreBuildMenu(start_id);
}

int MenuBuilder::GetItemCount() const {
  return impl_->menu_items_.size();
}

void MenuBuilder::GetMenuItem(int index, std::string* text, int* style,
                              HBITMAP* image_icon, int* command_id,
                              MenuBuilder** sub_menu) const {
  impl_->GetMenuItem(index, text, style, image_icon, command_id, sub_menu);
}
}  // namespace win32
}  // namespace ggadget
