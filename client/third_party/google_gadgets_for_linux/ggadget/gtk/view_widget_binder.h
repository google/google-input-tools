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

#ifndef GGADGET_GTK_VIEW_WIDGET_BINDER_H__
#define GGADGET_GTK_VIEW_WIDGET_BINDER_H__

#include <gtk/gtk.h>
#include <ggadget/common.h>

namespace ggadget {

class ViewHostInterface;
class ViewInterface;

namespace gtk {

/**
 * @ingroup GtkLibrary
 * A class to bind a View with a GtkWidget.
 *
 * The specified View will be drawn on the specified GtkWidget, and all events
 * will be delegated to the View from the GtkWidget.
 *
 * The specified GtkWidget must have its own GdkWindow.
 *
 * The ViewWidgetBinder instance will take effect as soon as it is created,
 * unless any parameter is invalid.
 */
class ViewWidgetBinder {
 public:
  /**
   * @param no_background if it's true then doesn't draw widget's background.
   */
  ViewWidgetBinder(ViewInterface *view,
                   ViewHostInterface *host, GtkWidget *widget,
                   bool no_background);
  ~ViewWidgetBinder();

  /**
   * @see @c ViewHostInterface::EnableInputShapeMask()
   */
  void EnableInputShapeMask(bool enable);

  /** Called by ViewHost to queue a redraw request. */
  void QueueDraw();

  /** Checks if a redraw request has been queued. */
  bool DrawQueued();

  /** Redraws the gadget immediately. */
  void DrawImmediately();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ViewWidgetBinder);
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_VIEW_WIDGET_H__
