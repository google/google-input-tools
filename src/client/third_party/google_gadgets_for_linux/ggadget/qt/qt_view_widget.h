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

#ifndef GGADGET_QT_QT_VIEW_WIDGET_H__
#define GGADGET_QT_QT_VIEW_WIDGET_H__

#include <string>
#include <vector>
#include <QtGui/QWidget>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {
namespace qt {

/**
 * @ingroup QtLibrary
 * A class to draw a view onto a QWidget.
 */
class QtViewWidget : public QWidget {
  Q_OBJECT
 public:
  enum Flag {
    FLAG_NONE = 0,
    FLAG_COMPOSITE = 0x1,
    FLAG_WM_DECORATED = 0x2,
    FLAG_MOVABLE = 0x4,
    FLAG_INPUT_MASK = 0x8
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  QtViewWidget(ViewInterface* view, Flags flags);
  ~QtViewWidget();
  void EnableInputShapeMask(bool enable);
  void SetChild(QWidget *widget);
  /**
   * Set properties for window that's not decorated by wm, including
   * skip_taskbar, skip_pager, show_on_all_desktop.
   */
  void SetUndecoratedWMProperties();
  void AdjustToViewSize();
  void SetKeepAbove(bool above);
  void SetView(ViewInterface *view);
  virtual QSize sizeHint () const;

  void QueueDraw();

 protected:
  virtual void paintEvent(QPaintEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void enterEvent(QEvent *event);
  virtual void leaveEvent(QEvent *event);
  virtual void wheelEvent(QWheelEvent * event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void keyReleaseEvent(QKeyEvent *event);
  virtual void inputMethodEvent(QInputMethodEvent *event);
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dragLeaveEvent(QDragLeaveEvent *event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dropEvent(QDropEvent *event);
  virtual void resizeEvent(QResizeEvent *event);
  virtual void focusInEvent(QFocusEvent *event);
  virtual void focusOutEvent(QFocusEvent *event);
  virtual void timerEvent(QTimerEvent *event);
  virtual void closeEvent(QCloseEvent *event);

 signals:
  void moved(int x, int y);
  void geometryChanged(int dleft, int dtop, int dw, int dh);
  void closeBySystem();

 public slots:
  void UnsetMinimumSizeHint();

 private:
  class Impl;
  Impl* impl_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(QtViewWidget);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QtViewWidget::Flags)

} // end of namespace qt
} // end of namespace ggadget

#endif // GGADGET_QT_QT_VIEW_WIDGET_H__
