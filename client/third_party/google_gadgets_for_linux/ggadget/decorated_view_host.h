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

#ifndef GGADGET_DECORATED_VIEW_HOST_H__
#define GGADGET_DECORATED_VIEW_HOST_H__

#include <ggadget/common.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/view_decorator_base.h>

namespace ggadget {

/**
 * @ingroup ViewDecorator
 *
 * DecoratedViewHost shows a view with the appropiate decorations.
 * It uses a special view derived from @c ViewDecoratorBase to hold the child
 * view and draw the decorations.
 */
class DecoratedViewHost : public ViewHostInterface {
 protected:
  virtual ~DecoratedViewHost();

 public:
  /**
   * Constructor.
   *
   * @param view_decorator A @c ViewDecoratorBase object to hold child view and
   *        draw view decorations. It'll be deleted when the DecoratedViewHost
   *        object is destroyed.
   */
  DecoratedViewHost(ViewDecoratorBase *view_decorator);

  /**
   * Gets the view which contains the decoration and the child view.
   * The caller shall not destroy the returned view.
   */
  ViewDecoratorBase *GetViewDecorator() const;

  /**
   * Lets the view decorator to load previously stored child view size.
   */
  void LoadChildViewSize();

  /**
   * Enables or disables auto load child view size.
   *
   * If it's enabled, then the view decorator will load previously stored
   * child view size automatically when the child view is attached to the
   * decorator or it's shown the first time.
   *
   * It's enabled by default.
   */
  void SetAutoLoadChildViewSize(bool auto_load);

  /** Gets the state of auto restore child view size. */
  bool IsAutoLoadChildViewSize() const;

 public:
  /** Returns the ViewHost type. */
  virtual Type GetType() const;
  virtual void Destroy();

  /** Sets the inner view which will be hosted in the decorator. */
  virtual void SetView(ViewInterface *view);

  /** Gets the inner view hosted in the decorator. */
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
  virtual void Alert(const ViewInterface *view, const char *message);
  virtual ConfirmResponse Confirm(const ViewInterface *view,
                                  const char *message, bool cancel_button);
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value);
  virtual int GetDebugMode() const;

  virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest);
  virtual void BeginMoveDrag(int button);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DecoratedViewHost);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_DECORATED_VIEW_HOST_H__
