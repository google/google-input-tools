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

#ifndef GGADGET_QT_UTILITIES_INTERNAL_H__
#define GGADGET_QT_UTILITIES_INTERNAL_H__

#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QCheckBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

#include <ggadget/messages.h>

namespace ggadget {

class GadgetInterface;

namespace qt {

class DebugConsole : public QWidget {
  Q_OBJECT
 public:
  DebugConsole(GadgetInterface *gadget, QWidget **widget)
    : text_(NULL), log_conn_(NULL), level_(0), self_(widget) {
    QVBoxLayout *vb = new QVBoxLayout();
    QHBoxLayout *hb = new QHBoxLayout();
    QWidget *group = new QWidget();

    QAbstractButton *button = new QPushButton(QString::fromUtf8(GM_("DEBUG_CLEAR")));
    connect(button, SIGNAL(clicked()), this, SLOT(OnClear()));
    hb->addWidget(button);
    button = new QRadioButton(QString::fromUtf8(GM_("DEBUG_TRACE")));
    connect(button, SIGNAL(clicked()), this, SLOT(OnTrace()));
    button->setChecked(true);
    hb->addWidget(button);
    button = new QRadioButton(QString::fromUtf8(GM_("DEBUG_INFO")));
    connect(button, SIGNAL(clicked()), this, SLOT(OnInfo()));
    hb->addWidget(button);
    button = new QRadioButton(QString::fromUtf8(GM_("DEBUG_WARNING")));
    connect(button, SIGNAL(clicked()), this, SLOT(OnWarning()));
    hb->addWidget(button);
    button = new QRadioButton(QString::fromUtf8(GM_("DEBUG_ERROR")));
    connect(button, SIGNAL(clicked()), this, SLOT(OnError()));
    hb->addWidget(button);

    group->setLayout(hb);
    vb->addWidget(group);
    text_ = new QTextEdit();
    text_->setReadOnly(true);
    vb->addWidget(text_);
    setLayout(vb);

    log_conn_ = gadget->ConnectLogListener(NewSlot(this, &DebugConsole::OnDebugConsoleLog));
    if (self_) *self_ = this;
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(
        QString::fromUtf8(gadget->GetManifestInfo(kManifestName).c_str()));
  }
  ~DebugConsole() {
    log_conn_->Disconnect();
    if (self_) *self_ = NULL;
  }
  void OnDebugConsoleLog(LogLevel level, const std::string &message) {
    if (level < level_) return;
    text_->append(QString::fromUtf8(message.c_str()));
  }
  QTextEdit *text_;
  Connection *log_conn_;
  int level_;
  QWidget** self_;
 protected slots:
  void OnClear() {
    text_->clear();
  }
  void OnTrace() {
    level_ = 0;
  }
  void OnInfo() {
    level_ = 1;
  }
  void OnWarning() {
    level_ = 2;
  }
  void OnError() {
    level_ = 3;
  }
};

} // namespace qt
} // namespace ggadget

#endif
