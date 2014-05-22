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

#include <ggadget/image_interface.h>
#include <ggadget/light_map.h>
#include <ggadget/logger.h>
#include <ggadget/slot.h>
#include "qt_menu.h"
#include "qt_menu_internal.h"

namespace ggadget {
namespace qt {

class QtMenu::Impl {
 public:
  Impl(QMenu *qmenu) : qt_menu_(qmenu) {}

  void AddItem(const char *item_text, int style, int stock_icon,
      ggadget::Slot1<void, const char *> *handler,
      int priority) {
    GGL_UNUSED(stock_icon);
    // FIXME: support stock icons.
    QAction *action;
    if (!item_text || !*item_text) {
      action = new QAction(qt_menu_);
      action->setSeparator(true);
    } else {
      action = new QAction(QString::fromUtf8(item_text), qt_menu_);
      MenuItemInfo *info = new MenuItemInfo(qt_menu_, item_text, handler, action);
      menu_items_[item_text] = info;
    }
    ApplyStyle(action, style);
    AddAction(action, priority);
  }

  void SetItemStyle(const char *item_text, int style) {
    LightMap<std::string, MenuItemInfo*>::iterator iter;
    iter = menu_items_.find(item_text);
    if (iter == menu_items_.end()) return;
    QAction *action = iter->second->action_;
    ApplyStyle(action, style);
  }

  MenuInterface *AddPopup(const char *popup_text, int priority){
    std::string text_str(popup_text ? popup_text : "");
    QMenu *submenu = new QMenu(QString::fromUtf8(text_str.c_str()));
    AddAction(submenu->menuAction(), priority);
    return new QtMenu(submenu);
  }

  void AddAction(QAction *action, int priority) {
    LightMap<int, QAction*>::iterator iter = prio_map_.begin();
    int prev = -1, next = -1;

    for (; iter != prio_map_.end(); iter++) {
      if (iter->first < priority) {
        prev = iter->first;
        continue;
      }
      if (iter->first > priority) {
        next = iter->first;
        break;
      }
    }
    if (next != -1) {
      qt_menu_->insertAction(iter->second, action);
    } else {
      qt_menu_->addAction(action);
    }
    if (prio_map_.find(priority) == prio_map_.end()) {
      // A new priority added
      if (prev != -1) {
        prio_map_[priority] = qt_menu_->insertSeparator(action);
      } else if (next != -1) {
        QAction *before = prio_map_[next];
        prio_map_[next] = qt_menu_->insertSeparator(before);
      } else {
        prio_map_[priority] = action;
      }
    }
  }

  void ApplyStyle(QAction *action, int style) {
    if (style & MENU_ITEM_FLAG_GRAYED)
      action->setDisabled(true);
    else
      action->setDisabled(false);

    if (style & MENU_ITEM_FLAG_CHECKED) {
      action->setCheckable(true);
      action->setChecked(true);
    } else {
      action->setChecked(false);
    }

    if (style & MENU_ITEM_FLAG_SEPARATOR)
      action->setSeparator(true);
  }

  QMenu *GetNativeMenu() { return qt_menu_; }
  QMenu *qt_menu_;
  LightMap<std::string, MenuItemInfo*> menu_items_;
  // store the first action of each priority
  LightMap<int, QAction*> prio_map_;
};

QtMenu::QtMenu(QMenu *qmenu)
    : impl_(new Impl(qmenu)) {
}

QtMenu::~QtMenu() {
  delete impl_;
}

void QtMenu::AddItem(const char *item_text, int style, int stock_icon,
                     ggadget::Slot1<void, const char *> *handler,
                     int priority) {
  impl_->AddItem(item_text, style, stock_icon, handler, priority);
}

void QtMenu::AddItem(const char* item_text, int style,
                     ImageInterface* image_icon,
                     Slot1<void, const char*>* handler, int priority) {
  // TODO(jiangwei): Implement this method.
  DestroyImage(image_icon);
  AddItem(item_text, style, 0, handler, priority);
}

void QtMenu::SetItemStyle(const char *item_text, int style) {
  impl_->SetItemStyle(item_text, style);
}

ggadget::MenuInterface *QtMenu::AddPopup(const char *popup_text, int priority) {
  return impl_->AddPopup(popup_text, priority);
}

void QtMenu::SetPositionHint(const Rectangle &rect) {
  GGL_UNUSED(rect);
  LOGE("Not implemented.");
}

} // namespace qt
} // namespace ggadget
#include "qt_menu_internal.moc"
