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

#ifndef GGADGET_QT_QT_MENU_H__
#define GGADGET_QT_QT_MENU_H__

#include <QtGui/QMenu>
#include <map>
#include <string>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/menu_interface.h>

namespace ggadget {

class ImageInterface;

namespace qt {

/**
 * @ingroup QtLibrary
 * An implementation of @c MenuInterface based on Qt.
 */
class QtMenu : public ggadget::MenuInterface {
 public:
  QtMenu(QMenu *qmenu);
  virtual ~QtMenu();

  virtual void AddItem(const char *item_text, int style, int stock_icon,
                       ggadget::Slot1<void, const char *> *handler,
                       int priority);
  virtual void AddItem(const char* item_text, int style,
                       ImageInterface* image_icon,
                       Slot1<void, const char*>* handler, int priority);
  virtual void SetItemStyle(const char *item_text, int style);
  virtual MenuInterface *AddPopup(const char *popup_text, int priority);
  virtual void SetPositionHint(const Rectangle &rect);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(QtMenu);

  class Impl;
  Impl *impl_;
};

} // namesapce qt
} // namespace ggadget

#endif // GGADGET_QT_QT_MENU_H__
