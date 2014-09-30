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

#ifndef GGADGET_QT_QT_VIEW_HOST_H__
#define GGADGET_QT_QT_VIEW_HOST_H__

#include <set>

#include <QtGui/QWidget>
#include <QtCore/QTimer>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_graphics.h>

namespace ggadget {
namespace qt {

/**
 * @ingroup QtLibrary
 * An implementation of @c ViewHostInterface based on Qt.
 */
class QtViewHost : public ViewHostInterface {
 public:
  enum Flag {
    FLAG_NONE = 0,
    FLAG_COMPOSITE = 0x1,
    FLAG_WM_DECORATED = 0x2,
    FLAG_RECORD_STATES = 0x4
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  /**
   * @param parent If not null, this view host will be shown at the popup
   *               position of parent
   */
  QtViewHost(ViewHostInterface::Type type,
             double zoom, Flags flags,
             int debug_mode, QWidget* parent);
  virtual ~QtViewHost();

  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const;
  virtual GraphicsInterface *NewGraphics() const;
  virtual void *GetNativeWidget() const;
  virtual void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const;
  virtual void NativeWidgetCoordToViewCoord(
      double x, double y, double *view_x, double *view_y) const;
  virtual void QueueDraw();
  virtual void QueueResize();
  virtual void EnableInputShapeMask(bool enable);
  virtual void SetResizable(ViewInterface::ResizableMode mode);
  virtual void SetCaption(const std::string &caption);
  virtual void SetShowCaptionAlways(bool always);
  virtual void SetCursor(ViewInterface::CursorType type);
  virtual void ShowTooltip(const std::string &tooltip);
  virtual void ShowTooltipAtPosition(const std::string &tooltip,
                                     double x, double y);
  virtual bool ShowView(bool modal, int flags,
                        Slot1<bool, int> *feedback_handler);
  virtual void CloseView();
  virtual bool ShowContextMenu(int button);
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {
    GGL_UNUSED(button);
    GGL_UNUSED(hittest);
  }
  virtual void BeginMoveDrag(int button);

  virtual void Alert(const ViewInterface *view, const char *message);
  virtual ConfirmResponse Confirm(const ViewInterface *view,
                                  const char *message, bool cancel_button);
  virtual std::string Prompt(const ViewInterface *view,
                             const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

  virtual Connection *ConnectOnEndMoveDrag(Slot2<void, int, int> *slot);
  virtual Connection *ConnectOnShowContextMenu(
      Slot1<bool, MenuInterface*> *slot);

  QObject *GetQObject();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(QtViewHost);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QtViewHost::Flags)

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_VIEW_HOST_H__
