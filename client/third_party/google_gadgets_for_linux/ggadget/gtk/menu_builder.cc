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

#include "menu_builder.h"

#include <math.h>

#include <ggadget/common.h>
#include <ggadget/image_interface.h>
#include <ggadget/logger.h>
#include <ggadget/math_utils.h>
#include <ggadget/slot.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {
namespace gtk {

namespace {

const char kMenuItemTextTag[] = "menu-item-text";
const char kMenuItemStyleTag[] = "menu-item-style";
const char kMenuItemPriorityTag[] = "menu-item-priority";
const char kMenuItemCallbackTag[] = "menu-item-callback";
const char kMenuItemBuilderTag[] = "menu-item-builder";
const char kMenuItemNoCallbackTag[] = "menu-item-no-callback";
const char kMenuPositionHintTag[] = "menu-position-hint";

// Must keep the same order as MenuInterface::MenuItemStockIcon
const char *kStockIcons[] = {
  NULL,
  GTK_STOCK_ABOUT,
  GTK_STOCK_ADD,
  GTK_STOCK_APPLY,
  GTK_STOCK_CANCEL,
  GTK_STOCK_CLOSE,
  GTK_STOCK_COPY,
  GTK_STOCK_CUT,
  GTK_STOCK_DELETE,
  GTK_STOCK_HELP,
  GTK_STOCK_NEW,
  GTK_STOCK_NO,
  GTK_STOCK_OK,
  GTK_STOCK_OPEN,
  GTK_STOCK_PASTE,
  GTK_STOCK_PREFERENCES,
  GTK_STOCK_QUIT,
  GTK_STOCK_REFRESH,
  GTK_STOCK_REMOVE,
  GTK_STOCK_STOP,
  GTK_STOCK_YES,
  GTK_STOCK_ZOOM_100,
  GTK_STOCK_ZOOM_FIT,
  GTK_STOCK_ZOOM_IN,
  GTK_STOCK_ZOOM_OUT
};

const int kNumberOfStockIcons = static_cast<int>(arraysize(kStockIcons));

}  // namespace

class MenuBuilder::Impl {
 public:
  Impl(ViewHostInterface *view_host, GtkMenuShell *gtk_menu)
      : view_host_(view_host),
        gtk_menu_(gtk_menu),
        item_added_(false) {
    ASSERT(GTK_IS_MENU_SHELL(gtk_menu_));
    g_object_ref(G_OBJECT(gtk_menu_));
  }
  ~Impl() {
    g_object_unref(G_OBJECT(gtk_menu_));
  }

  // Windows version uses '&' as the mnemonic indicator, and this has been
  // taken as the part of the Gadget API.
  static std::string ConvertWindowsStyleMnemonics(const char *text) {
    std::string result;
    if (text) {
      while (*text) {
        switch (*text) {
          case '&': result += '_'; break;
          case '_': result += "__";
          default: result += *text;
        }
        text++;
      }
    }
    return result;
  }

  static void SetMenuItemStyle(GtkMenuItem *item, int style) {
    // Set a signature to disable callback, to avoid trigger handler when
    // setting checked state.
    g_object_set_data(G_OBJECT(item), kMenuItemNoCallbackTag,
                      static_cast<gpointer>(item));

    gtk_widget_set_sensitive(GTK_WIDGET(item),
        (style & ggadget::MenuInterface::MENU_ITEM_FLAG_GRAYED) == 0);

    if (GTK_IS_CHECK_MENU_ITEM(item)) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
          (style & ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED) != 0);
    }

    // Clear the signature.
    g_object_set_data(G_OBJECT(item), kMenuItemNoCallbackTag, NULL);

    // Attach the style value to the menu item for later reference.
    g_object_set_data(G_OBJECT(item), kMenuItemStyleTag,
                      GINT_TO_POINTER(style));
  }

  static void DestroyHandlerCallback(gpointer handler) {
    delete reinterpret_cast<ggadget::Slot1<void, const char *> *>(handler);
  }

  static void OnItemActivate(GtkMenuItem *item, gpointer data) {
    GGL_UNUSED(data);
    if (g_object_get_data(G_OBJECT(item), kMenuItemNoCallbackTag) != NULL)
      return;

    ggadget::Slot1<void, const char *> *handler =
        reinterpret_cast<ggadget::Slot1<void, const char *> *>(
            g_object_get_data(G_OBJECT(item), kMenuItemCallbackTag));
    const char *text =
        reinterpret_cast<const char *>(
            g_object_get_data(G_OBJECT(item), kMenuItemTextTag));

    if (handler) (*handler)(text);
  }

  static GtkMenuItem *NewMenuItem(const char *text, int style, int stock_icon,
                                  Slot1<void, const char *> *handler,
                                  int priority) {
    GtkMenuItem *item = NULL;

    if (!text || !*text || (style & MENU_ITEM_FLAG_SEPARATOR)) {
      item = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    } else if (style & MENU_ITEM_FLAG_CHECKED) {
      item = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(
        ConvertWindowsStyleMnemonics(text).c_str()));
    } else if (stock_icon > 0 && stock_icon < kNumberOfStockIcons) {
      item = GTK_MENU_ITEM(gtk_image_menu_item_new_with_mnemonic(
        ConvertWindowsStyleMnemonics(text).c_str()));
      GtkWidget *icon = gtk_image_new_from_stock(kStockIcons[stock_icon],
                                                 GTK_ICON_SIZE_MENU);
      if (icon)
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
    } else {
      item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(
        ConvertWindowsStyleMnemonics(text).c_str()));
    }

    if (item) {
      SetMenuItemStyle(item, style);
      g_object_set_data(G_OBJECT(item), kMenuItemPriorityTag,
                        GINT_TO_POINTER(priority < 0 ? 0 : priority));

      if (text && *text)
        g_object_set_data_full(G_OBJECT(item), kMenuItemTextTag,
                               g_strdup(text), g_free);
      if (handler)
        g_object_set_data_full(G_OBJECT(item), kMenuItemCallbackTag,
                               handler, DestroyHandlerCallback);

      gtk_widget_show(GTK_WIDGET(item));
      g_signal_connect(item, "activate", G_CALLBACK(OnItemActivate), NULL);
    }
    return item;
  }

  struct FindItemData {
    FindItemData(const char *text, int min_pri, int max_pri, bool first)
      : text_(text), min_priority_(min_pri), max_priority_(max_pri),
        first_(first), item_(NULL), index_(-1), count_(0) {
    }

    // input members.
    const char *text_;
    int min_priority_;
    int max_priority_;
    bool first_;

    // output members.
    GtkMenuItem *item_;
    int index_;
    int count_;
  };

  static void FindItemByTextCallback(GtkWidget *item, gpointer data) {
    const char *text = reinterpret_cast<const char *>(
        g_object_get_data(G_OBJECT(item), kMenuItemTextTag));
    FindItemData *item_data = reinterpret_cast<FindItemData *>(data);
    if ((!item_data->first_ || !item_data->item_) &&
        text && strcmp(text, item_data->text_) == 0) {
      item_data->item_ = GTK_MENU_ITEM(item);
      item_data->index_ = item_data->count_;
    }
    item_data->count_++;
  }

  GtkMenuItem *FindItemByText(const char *text, bool first,
                              int *index, int *count) {
    FindItemData data(text, 0, 0, first);
    gtk_container_foreach(GTK_CONTAINER(gtk_menu_), FindItemByTextCallback,
                          reinterpret_cast<gpointer>(&data));
    if (index) *index = data.index_;
    if (count) *count = data.count_;
    return data.item_;
  }

  static void FindItemByPriorityCallback(GtkWidget *item, gpointer data) {
    int priority = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
                                                     kMenuItemPriorityTag));
    FindItemData *item_data = reinterpret_cast<FindItemData *>(data);
    if ((!item_data->first_ || !item_data->item_) &&
        priority >= item_data->min_priority_ &&
        (priority <= item_data->max_priority_ ||
         item_data->max_priority_ < 0)) {
      item_data->item_ = GTK_MENU_ITEM(item);
      item_data->index_ = item_data->count_;
    }
    item_data->count_++;
  }

  // Find the last built-in or external menu item.
  GtkMenuItem *FindItemByPriority(int min_pri, int max_pri, bool first,
                                  int *index, int *count) {
    FindItemData data(NULL, min_pri, max_pri, first);
    gtk_container_foreach(GTK_CONTAINER(gtk_menu_), FindItemByPriorityCallback,
                          reinterpret_cast<gpointer>(&data));

    if (index) *index = data.index_;
    if (count) *count = data.count_;
    return data.item_;
  }

  GtkMenuItem *AddMenuItem(const char *text, int style, int stock_icon,
                           ggadget::Slot1<void, const char *> *handler,
                           int priority) {
    ASSERT(priority >= 0);
    GtkMenuItem *item = NewMenuItem(text, style, stock_icon, handler, priority);
    if (item) {
      int last_index = 0;
      int count = 0;
      GtkMenuItem *last_item =
          FindItemByPriority(0, priority, false, &last_index, &count);

      // If last item is not found, then the last_index will be -1, then the
      // item will be inserted before all other items.
      if (last_index < count - 1)
        gtk_menu_shell_insert(gtk_menu_, GTK_WIDGET(item), last_index + 1);
      else
        gtk_menu_shell_append(gtk_menu_, GTK_WIDGET(item));

      if (!GTK_IS_SEPARATOR_MENU_ITEM(item)) {
        // Add a separator between the last item and the new item if necessary.
        if (last_item) {
          int last_priority = GPOINTER_TO_INT(
              g_object_get_data(G_OBJECT(last_item), kMenuItemPriorityTag));

          if (last_priority != priority &&
              !GTK_IS_SEPARATOR_MENU_ITEM(last_item)) {
            GtkMenuItem *sep = NewMenuItem(NULL, 0, 0, NULL, priority);
            gtk_menu_shell_insert(gtk_menu_, GTK_WIDGET(sep), last_index + 1);
          }
        }

        // Add a separator between the new item and the next item if necessary.
        if (count && last_index < count - 1) {
          int next_index = 0;
          GtkMenuItem *next_item =
              FindItemByPriority(priority + 1, -1, true, &next_index, &count);
          if (next_item && last_index + 1 == next_index - 1 &&
              !GTK_IS_SEPARATOR_MENU_ITEM(next_item)) {
            int next_priority = GPOINTER_TO_INT(
                g_object_get_data(G_OBJECT(next_item), kMenuItemPriorityTag));
            GtkMenuItem *sep = NewMenuItem(NULL, 0, 0, NULL, next_priority);
            gtk_menu_shell_insert(gtk_menu_, GTK_WIDGET(sep), next_index);
          }
        }
      }

      item_added_ = true;
    }
    return item;
  }

  void SetItemStyle(const char *text, int style) {
    int index, count;
    GtkMenuItem *item = FindItemByText(text, false, &index, &count);
    if (item) {
      int old_style = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
                                                        kMenuItemStyleTag));
      // If the checked or separator flag has been changed, then we need
      // re-create the menu item with new style.
      if ((old_style ^ style) &
          (MENU_ITEM_FLAG_CHECKED | MENU_ITEM_FLAG_SEPARATOR)) {
        // Can't re-create the item with a submenu attached.
        if (gtk_menu_item_get_submenu(item) != NULL) {
          ASSERT_M(false,
               ("Can't change the checked style of a menu item with submenu."));
          return;
        }

        // Steal the callback handler.
        ggadget::Slot1<void, const char *> *handler =
            reinterpret_cast<ggadget::Slot1<void, const char *> *>(
                g_object_steal_data(G_OBJECT(item), kMenuItemCallbackTag));

        int priority = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
                                                         kMenuItemPriorityTag));

        gtk_widget_destroy(GTK_WIDGET(item));
        item = NewMenuItem(text, style, 0, handler, priority);
        if (item)
          gtk_menu_shell_insert(gtk_menu_, GTK_WIDGET(item), index);
      } else {
        SetMenuItemStyle(item, style);
      }
    }
  }

  static void DestroyMenuBuilderCallback(gpointer data) {
    delete reinterpret_cast<MenuBuilder *>(data);
  }

  MenuInterface *AddPopup(const char *text, int priority) {
    MenuBuilder *submenu = NULL;
    GtkMenuItem *item = AddMenuItem(text, 0, 0, NULL, priority);
    if (item) {
      GtkMenu *popup = GTK_MENU(gtk_menu_new());
      gtk_widget_show(GTK_WIDGET(popup));
      submenu = new MenuBuilder(view_host_, GTK_MENU_SHELL(popup));

      gtk_menu_item_set_submenu(item, GTK_WIDGET(popup));
      g_object_set_data_full(G_OBJECT(item), kMenuItemBuilderTag,
                             reinterpret_cast<gpointer>(submenu),
                             DestroyMenuBuilderCallback);
    }
    return submenu;
  }

  static void DestroyMenuPositionHintCallback(gpointer data) {
    delete reinterpret_cast<Rectangle*>(data);
  }

  void SetPositionHint(const Rectangle &rect) {
    Rectangle *hint = new Rectangle(rect);
    g_object_set_data_full(G_OBJECT(gtk_menu_), kMenuPositionHintTag,
                           reinterpret_cast<gpointer>(hint),
                           DestroyMenuPositionHintCallback);
  }

  static void TranslateCoordinatesToScreen(ViewHostInterface *view_host,
                                           const Rectangle &src_rect,
                                           gint *left,
                                           gint *top,
                                           gint *right,
                                           gint *bottom) {
    GtkWidget *widget = GTK_WIDGET(view_host->GetNativeWidget());
    ASSERT(widget);

    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
    ASSERT(toplevel);
    ASSERT(GTK_IS_WINDOW(toplevel));

    *left = static_cast<gint>(::round(src_rect.x));
    *top = static_cast<gint>(::round(src_rect.y));
    *right = static_cast<gint>(::round(src_rect.x + src_rect.w));
    *bottom = static_cast<gint>(::round(src_rect.y + src_rect.h));

    if (widget != toplevel) {
      gtk_widget_translate_coordinates(
          widget, toplevel, *left, *top, left, top);
      gtk_widget_translate_coordinates(
          widget, toplevel, *right, *bottom, right, bottom);
    }

    gint window_x, window_y;
    gtk_window_get_position(GTK_WINDOW(toplevel), &window_x, &window_y);

    *left += window_x;
    *top += window_y;
    *right += window_x;
    *bottom += window_y;
  }

  static void PositionMenuCallback(GtkMenu *menu,
                                   gint *x,
                                   gint *y,
                                   gboolean *push_in,
                                   gpointer data) {
    ViewHostInterface *view_host = reinterpret_cast<ViewHostInterface*>(data);
    ASSERT(view_host);

    Rectangle *hint = reinterpret_cast<Rectangle*>(
        g_object_get_data(G_OBJECT(menu), kMenuPositionHintTag));
    ASSERT(hint);

    GdkScreen *screen =
        gtk_widget_get_screen(GTK_WIDGET(view_host->GetNativeWidget()));
    ASSERT(screen);

    const gint screen_width = gdk_screen_get_width(screen);
    const gint screen_height = gdk_screen_get_height(screen);

    gtk_menu_set_screen(menu, screen);

    GtkRequisition menu_size;
    gtk_widget_size_request(GTK_WIDGET(menu), &menu_size);

    gint left, top, right, bottom;
    TranslateCoordinatesToScreen(view_host, *hint,
                                 &left, &top, &right, &bottom);

    *x = (left + menu_size.width < screen_width) ?
        left : (screen_width - menu_size.width);
    *y = (bottom + menu_size.height < screen_height) ?
        bottom : (top - menu_size.height);
    *x = std::max(0, *x);
    *y = std::max(0, *y);
    *push_in = false;
  }

  void Popup(guint button, guint32 activate_time) {
    if (view_host_ &&
        g_object_get_data(G_OBJECT(gtk_menu_), kMenuPositionHintTag)) {
      gtk_menu_popup(GTK_MENU(gtk_menu_), NULL, NULL,
                     PositionMenuCallback, view_host_,
                     button, activate_time);
    } else {
      gtk_menu_popup(GTK_MENU(gtk_menu_), NULL, NULL, NULL, NULL,
                     button, activate_time);
    }
  }

  ViewHostInterface *view_host_;
  GtkMenuShell *gtk_menu_;
  bool item_added_;
};

MenuBuilder::MenuBuilder(ViewHostInterface *view_host, GtkMenuShell *gtk_menu)
    : impl_(new Impl(view_host, gtk_menu)) {
  DLOG("Create MenuBuilder.");
}

MenuBuilder::~MenuBuilder() {
  DLOG("Destroy MenuBuilder.");
  delete impl_;
  impl_ = NULL;
}

void MenuBuilder::AddItem(const char *item_text, int style, int stock_icon,
                          Slot1<void, const char *> *handler, int priority) {
  impl_->AddMenuItem(item_text, style, stock_icon, handler, priority);
}

void MenuBuilder::AddItem(const char* item_text, int style,
                          ImageInterface* image_icon,
                          Slot1<void, const char*>* handler, int priority) {
  // TODO(jiangwei): Implement this method.
  DestroyImage(image_icon);
  AddItem(item_text, style, 0, handler, priority);
}

void MenuBuilder::SetItemStyle(const char *item_text, int style) {
  impl_->SetItemStyle(item_text, style);
}

ggadget::MenuInterface *MenuBuilder::AddPopup(const char *popup_text,
                                              int priority) {
  return impl_->AddPopup(popup_text, priority);
}

void MenuBuilder::SetPositionHint(const Rectangle &rect) {
  impl_->SetPositionHint(rect);
}

GtkMenuShell *MenuBuilder::GetGtkMenuShell() const {
  return impl_->gtk_menu_;
}

bool MenuBuilder::ItemAdded() const {
  return impl_->item_added_;
}

void MenuBuilder::Popup(guint button, guint32 activate_time) const {
  impl_->Popup(button, activate_time);
}

} // namespace gtk
} // namespace ggadget
