/*
  Copyright 2013 Google Inc.

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
#ifndef GGADGET_MAC_SINGLE_VIEW_HOST_H_
#define GGADGET_MAC_SINGLE_VIEW_HOST_H_

#include "ggadget/view_host_interface.h"
#include "ggadget/scoped_ptr.h"

namespace ggadget {
namespace mac {

class SingleViewHost : public ViewHostInterface {
 public:
  SingleViewHost();
  ~SingleViewHost();
  virtual Type GetType() const;
  virtual void Destroy();
  virtual void SetView(ViewInterface *view);
  virtual ViewInterface *GetView() const;
  virtual GraphicsInterface *NewGraphics() const;

  // Return a bridged NSWindow pointer
  //
  // The caller should use (__bridge NSWindow*) bridge cast to get the NSWindow
  // object.
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
  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);
  virtual void BeginMoveDrag(int button);
  virtual void Alert(const ViewInterface *view, const char *message);
  virtual ConfirmResponse Confirm(const ViewInterface *view,
                                  const char *message, bool cancel_button);
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

  virtual void GetWindowPosition(int *x, int *y);
  virtual void SetWindowPosition(int x, int y);
  virtual void GetWindowSize(int *width, int *height);
  virtual void SetFocusable(bool focusable);
  virtual void SetOpacity(double opacity);
  virtual void SetFontScale(double scale);
  virtual void SetZoom(double zoom);
  virtual Connection *ConnectOnEndMoveDrag(Slot2<void, int, int> *slot);
  virtual Connection *ConnectOnShowContextMenu(
      Slot1<bool, MenuInterface*> *slot);

 public:
  void SetKeepAbove(bool keepAbove);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(SingleViewHost);
};

} // namespace mac
} // namespace ggadget

#endif // GGADGET_MAC_SINGLE_VIEW_HOST_H_
