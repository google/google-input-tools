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

#include <ggadget/main_loop_interface.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include "tooltip.h"

namespace ggadget {
namespace gtk {

class Tooltip::Impl {
 public:
  Impl(int show_timeout, int hide_timeout)
    : window_(gtk_window_new(GTK_WINDOW_POPUP)),
      label_(gtk_label_new(NULL)),
      show_timeout_(show_timeout),
      hide_timeout_(hide_timeout),
      show_timer_(0),
      hide_timer_(0) {
#if GTK_CHECK_VERSION(2,10,0)
    gtk_window_set_type_hint(GTK_WINDOW(window_), GDK_WINDOW_TYPE_HINT_TOOLTIP);
#endif
    gtk_widget_set_app_paintable(window_, TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(window_), 4);

    gtk_label_set_line_wrap(GTK_LABEL(label_), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(window_), label_);
    gtk_widget_show(label_);

    // TODO: Is there better way?
    GdkColor color = { 0, 0xffff, 0xffff, 0xb000 };
    gtk_widget_modify_bg(window_, GTK_STATE_NORMAL, &color);
    g_signal_connect(window_, "expose_event",
                     G_CALLBACK(PaintTooltipWindow), NULL);
  }

  ~Impl() {
    RemoveTimers();
    gtk_widget_destroy(window_);
  }

  void AdjustAndShowWidget(GdkScreen *screen, int x, int y, bool center) {
    gint monitor = gdk_screen_get_monitor_at_point(screen, x, y);
    GdkRectangle rect;
    gdk_screen_get_monitor_geometry(screen, monitor, &rect);
    GtkRequisition size;
    gtk_widget_size_request(window_, &size);

    if (center)
      x -= size.width / 2;

    // Adjust the position to display the whole tooltip window inside the
    // monitor region.
    if (size.width + x > rect.x + rect.width)
      x = rect.x + rect.width - size.width;
    if (size.height + y + 20 > rect.y + rect.height)
      y -= size.height;
    else
      y += 20;

    gtk_window_set_screen(GTK_WINDOW(window_), screen);
    gtk_window_move(GTK_WINDOW(window_), x, y);
    gtk_widget_show_all(window_);
  }

  bool DelayedShow(int watch_id) {
    GGL_UNUSED(watch_id);
    GdkScreen *screen;
    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, NULL);
    AdjustAndShowWidget(screen, x, y, false);
    show_timer_ = 0;
    return false;
  }

  bool DelayedHide(int watch_id) {
    GGL_UNUSED(watch_id);
    gtk_widget_hide(window_);
    hide_timer_ = 0;
    return false;
  }

  void RemoveTimers() {
    if (show_timer_) {
      GetGlobalMainLoop()->RemoveWatch(show_timer_);
      show_timer_ = 0;
    }
    if (hide_timer_) {
      GetGlobalMainLoop()->RemoveWatch(hide_timer_);
      hide_timer_ = 0;
    }
  }

  void Show(const char *tooltip) {
    Hide();
    if (tooltip && *tooltip) {
      gtk_label_set_text(GTK_LABEL(label_), tooltip);
      if (show_timeout_ > 0) {
        show_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
            show_timeout_,
            new WatchCallbackSlot(NewSlot(this, &Impl::DelayedShow)));
      } else {
        DelayedShow(0);
      }

      if (hide_timeout_ > 0) {
        hide_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
            hide_timeout_,
            new WatchCallbackSlot(NewSlot(this, &Impl::DelayedHide)));
      }
    }
  }

  void ShowAtPosition(const char *tooltip,
                      GdkScreen *screen, int x, int y) {
    Hide();
    if (tooltip && *tooltip) {
      gtk_label_set_text(GTK_LABEL(label_), tooltip);
      AdjustAndShowWidget(screen, x, y, true);

      if (hide_timeout_ > 0) {
        hide_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
            hide_timeout_,
            new WatchCallbackSlot(NewSlot(this, &Impl::DelayedHide)));
      }
    }
  }

  void Hide() {
    RemoveTimers();
    gtk_widget_hide(window_);
  }

  static gboolean PaintTooltipWindow(GtkWidget *widget, GdkEventExpose *event,
                                     gpointer user_data) {
    GGL_UNUSED(event);
    GGL_UNUSED(user_data);
    GtkRequisition req;
    gtk_widget_size_request(widget, &req);
    gtk_paint_flat_box(widget->style, widget->window,
                       GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                       NULL, widget, "tooltip",
                       0, 0, req.width, req.height);
    return FALSE;
  }

  GtkWidget *window_;
  GtkWidget *label_;
  int show_timeout_;
  int hide_timeout_;
  int show_timer_;
  int hide_timer_;
};

Tooltip::Tooltip(int show_timeout, int hide_timeout)
  : impl_(new Impl(show_timeout, hide_timeout)) {
}

Tooltip::~Tooltip() {
  delete impl_;
  impl_ = NULL;
}

void Tooltip::Show(const char *tooltip) {
  impl_->Show(tooltip);
}

void Tooltip::ShowAtPosition(const char *tooltip,
                             GdkScreen *screen, int x, int y) {
  impl_->ShowAtPosition(tooltip, screen, x, y);
}

void Tooltip::Hide() {
  impl_->Hide();
}

} // namespace gtk
} // namespace ggadget
