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
#ifndef GGADGET_QT_QT_MENU_INTERNAL_H__
#define GGADGET_QT_QT_MENU_INTERNAL_H__
namespace ggadget {
namespace qt {

class MenuItemInfo : public QObject {
  Q_OBJECT
 public:
  MenuItemInfo(QObject *parent, const char *text,
               ggadget::Slot1<void, const char *> *handler,
               QAction *action)
    : QObject(parent), item_text_(text), handler_(handler), action_(action) {
      connect(action, SIGNAL(triggered()), this, SLOT(OnTriggered()));
  }

  std::string item_text_;
  ggadget::Slot1<void, const char *> *handler_;
  QAction *action_;

 public slots:
  void OnTriggered() {
    if (handler_) (*handler_)(item_text_.c_str());
  }
};

} // namesapce qt
} // namespace ggadget

#endif
