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

#include <cmath>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string>
#include <ggadget/file_manager_interface.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/logger.h>
#include <ggadget/math_utils.h>
#include <ggadget/messages.h>
#include <ggadget/options_interface.h>

#include "cairo_graphics.h"
#include "key_convert.h"
#include "menu_builder.h"
#include "single_view_host.h"
#include "tooltip.h"
#include "utilities.h"
#include "view_widget_binder.h"

// It might not be necessary, because X server will grab the pointer
// implicitly when button is pressed.
// But using explicit mouse grabbing may avoid some issues by preventing some
// events from sending to client window when mouse is grabbed.
#define GRAB_POINTER_EXPLICITLY
namespace ggadget {
namespace gtk {

static const double kMinimumZoom = 0.5;
static const double kMaximumZoom = 2.0;
static const int kStopMoveDragTimeout = 200;
static const char kMainViewWindowRole[] = "Google-Gadgets";

class SingleViewHost::Impl {
 public:
  Impl(SingleViewHost *owner, ViewHostInterface::Type type,
       double zoom, int flags, int debug_mode)
    : owner_(owner),
      view_(NULL),
      window_(NULL),
      widget_(NULL),
      fixed_(NULL),
      context_menu_(NULL),
      ok_button_(NULL),
      cancel_button_(NULL),
      tooltip_(new Tooltip(kShowTooltipDelay, kHideTooltipDelay)),
      binder_(NULL),
      type_(type),
      initial_zoom_(zoom),
      flags_(flags),
      debug_mode_(debug_mode),
      stop_move_drag_source_(0),
      win_x_(0),
      win_y_(0),
      win_width_(0),
      win_height_(0),
      resize_view_zoom_(0),
      resize_view_width_(0),
      resize_view_height_(0),
      resize_win_x_(0),
      resize_win_y_(0),
      resize_win_width_(0),
      resize_win_height_(0),
      resize_button_(0),
      resize_mouse_x_(0),
      resize_mouse_y_(0),
      resize_width_mode_(0),
      resize_height_mode_(0),
      resizable_mode_(ViewInterface::RESIZABLE_TRUE),
      is_keep_above_(false),
      move_dragging_(false),
      enable_signals_(true),
      queue_resize_timer_(0),
      fixed_width_from_view_(0),
      fixed_height_from_view_(0),
      feedback_handler_(NULL),
      can_close_dialog_(false) {
    ASSERT(owner);
  }

  ~Impl() {
    Detach();

    delete tooltip_;
    tooltip_ = NULL;
  }

  void Detach() {
    // To make sure that it won't be accessed anymore.
    view_ = NULL;

    if (queue_resize_timer_)
      g_source_remove(queue_resize_timer_);
    queue_resize_timer_ = 0;

    if (stop_move_drag_source_)
      g_source_remove(stop_move_drag_source_);
    stop_move_drag_source_ = 0;

    delete feedback_handler_;
    feedback_handler_ = NULL;

    delete binder_;
    binder_ = NULL;

    if (window_) {
      gtk_widget_destroy(window_);
      window_ = NULL;
    }
    if (context_menu_) {
      gtk_widget_destroy(context_menu_);
      context_menu_ = NULL;
    }
    widget_ = NULL;
    fixed_ = NULL;
    ok_button_ = NULL;
    cancel_button_ = NULL;
  }

  void SetView(ViewInterface *view) {
    if (view_ == view)
      return;

    Detach();

    if (view == NULL) {
      on_view_changed_signal_();
      return;
    }

    view_ = view;
    bool transparent = false;
    // Initialize window and widget.
    // All views must be held inside GTKFixed widgets in order to support the
    // browser element.
    fixed_ = gtk_fixed_new();
    gtk_widget_show(fixed_);
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      // Options view needs run in a dialog with ok and cancel buttons.
      // Options view's background is always opaque.
      window_ = gtk_dialog_new();
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window_)->vbox), fixed_);
      cancel_button_ = gtk_dialog_add_button(GTK_DIALOG(window_),
                                             GTK_STOCK_CANCEL,
                                             GTK_RESPONSE_CANCEL);
      ok_button_ = gtk_dialog_add_button(GTK_DIALOG(window_),
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK);
      gtk_dialog_set_default_response(GTK_DIALOG(window_), GTK_RESPONSE_OK);
      g_signal_connect(G_OBJECT(window_), "response",
                       G_CALLBACK(DialogResponseHandler), this);
      gtk_fixed_set_has_window(GTK_FIXED(fixed_), TRUE);
      widget_ = fixed_;
    } else {
      // details and main view only need a toplevel window.
      // buttons of details view shall be provided by view decorator.
      window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_role(GTK_WINDOW(window_), kMainViewWindowRole);
      gtk_container_add(GTK_CONTAINER(window_), fixed_);
      transparent = !(flags_ & OPAQUE_BACKGROUND);
      if (transparent)
        DisableWidgetBackground(window_);
      widget_ = window_;
    }

    bool skip_wm = !(flags_ & WM_MANAGEABLE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window_), skip_wm);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window_), skip_wm);
    gtk_window_set_decorated(GTK_WINDOW(window_), (flags_ & DECORATED));
    gtk_window_set_gravity(GTK_WINDOW(window_), GDK_GRAVITY_STATIC);
    SetResizable(view_->GetResizable());

    if (flags_ & DIALOG_TYPE_HINT) {
      gtk_window_set_type_hint(GTK_WINDOW(window_),
                               GDK_WINDOW_TYPE_HINT_DIALOG);
    }

    g_signal_connect(G_OBJECT(window_), "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(G_OBJECT(window_), "focus-in-event",
                     G_CALLBACK(FocusInHandler), this);
#ifdef _DEBUG
    g_signal_connect(G_OBJECT(window_), "focus-out-event",
                     G_CALLBACK(FocusOutHandler), this);
#endif
    g_signal_connect(G_OBJECT(window_), "enter-notify-event",
                     G_CALLBACK(EnterNotifyHandler), this);
    g_signal_connect(G_OBJECT(window_), "show",
                     G_CALLBACK(WindowShowHandler), this);
    g_signal_connect_after(G_OBJECT(window_), "hide",
                     G_CALLBACK(WindowHideHandler), this);
    g_signal_connect(G_OBJECT(window_), "configure-event",
                     G_CALLBACK(ConfigureHandler), this);

    // For resize drag.
    g_signal_connect(G_OBJECT(window_), "motion-notify-event",
                     G_CALLBACK(MotionNotifyHandler), this);
    g_signal_connect(G_OBJECT(window_), "button-release-event",
                     G_CALLBACK(ButtonReleaseHandler), this);

    g_signal_connect(G_OBJECT(widget_), "size-request",
                     G_CALLBACK(WidgetSizeRequestHandler), this);

    g_signal_connect(G_OBJECT(fixed_), "size-allocate",
                     G_CALLBACK(FixedSizeAllocateHandler), this);

    g_signal_connect(G_OBJECT(fixed_), "set-focus-child",
                     G_CALLBACK(FixedSetFocusChildHandler), this);

    // For details and main view, the view is bound to the toplevel window
    // instead of the GtkFixed widget, to get better performance and make the
    // input event mask effective.
    binder_ = new ViewWidgetBinder(view_, owner_, widget_, transparent);

    gtk_widget_realize(fixed_);
    gtk_widget_realize(window_);
    DLOG("Window created: %p, fixed: %p", window_, fixed_);
    on_view_changed_signal_();
  }

  void ViewCoordToNativeWidgetCoord(
      double x, double y, double *widget_x, double *widget_y) const {
    double zoom = view_->GetGraphics()->GetZoom();
    if (widget_x)
      *widget_x = x * zoom;
    if (widget_y)
      *widget_y = y * zoom;
  }

  void NativeWidgetCoordToViewCoord(double x, double y,
                                    double *view_x, double *view_y) const {
    double zoom = view_->GetGraphics()->GetZoom();
    if (zoom == 0) return;
    if (view_x) *view_x = x / zoom;
    if (view_y) *view_y = y / zoom;
  }

  void AdjustWindowSize() {
    ASSERT(view_);

    double zoom = view_->GetGraphics()->GetZoom();
    int width = static_cast<int>(ceil(view_->GetWidth() * zoom));
    int height = static_cast<int>(ceil(view_->GetHeight() * zoom));

    // Stores the expected size of the GtkFixed widget, which will be used in
    // FixedSizeAllocateHandler().
    fixed_width_from_view_ = width;
    fixed_height_from_view_ = height;

    GtkRequisition req;
    gtk_widget_set_size_request(widget_, width, height);
    gtk_widget_size_request(window_, &req);
    gtk_widget_set_size_request(widget_, -1, -1);

    // If the window is resizable, resize the window directly.
    // Otherwise do nothing. Because gtk_widget_set_size_request() will queue a
    // resize request, which will adjust the window size according to view's
    // size. See WidgetSizeRequestHandler().
    if (gtk_window_get_resizable(GTK_WINDOW(window_)))
      gtk_window_resize(GTK_WINDOW(window_), req.width, req.height);

    // If the window is not mapped yet, then save the window size as initial
    // size.
    if (!GTK_WIDGET_MAPPED(window_)) {
      win_width_ = req.width;
      win_height_ = req.height;
    }
  }

  void QueueResize() {
    // When doing resize drag, MotionNotifyHandler() is in charge of resizing
    // the window, so don't do it here.
    if (resize_width_mode_ == 0 && resize_height_mode_ == 0 &&
        queue_resize_timer_ == 0) {
      queue_resize_timer_ = g_idle_add(QueueResizeTimeoutHandler, this);
    }
  }

  void EnableInputShapeMask(bool enable) {
    if (binder_) {
      DLOG("SingleViewHost::EnableInputShapeMask(%s)",
           enable ? "true" : "false");
      binder_->EnableInputShapeMask(enable);
    }
  }

  void QueueDraw() {
    ASSERT(GTK_IS_WIDGET(widget_));
    if (binder_)
      binder_->QueueDraw();
  }

  void SetResizable(ViewInterface::ResizableMode mode) {
    ASSERT(GTK_IS_WINDOW(window_));
    if (resizable_mode_ != mode) {
      resizable_mode_ = mode;
      bool resizable = (mode == ViewInterface::RESIZABLE_TRUE ||
                        mode == ViewInterface::RESIZABLE_KEEP_RATIO ||
                        (mode == ViewInterface::RESIZABLE_ZOOM &&
                         type_ != ViewHostInterface::VIEW_HOST_OPTIONS));
      gtk_window_set_resizable(GTK_WINDOW(window_), resizable);

      // Reset the zoom factor to 1 if the child view is changed to
      // resizable.
      if ((mode == ViewInterface::RESIZABLE_TRUE ||
           mode == ViewInterface::RESIZABLE_KEEP_RATIO) &&
          view_->GetGraphics()->GetZoom() != 1.0) {
        view_->GetGraphics()->SetZoom(1.0);
        view_->MarkRedraw();
      }
    }
  }

  void SetCaption(const std::string &caption) {
    ASSERT(GTK_IS_WINDOW(window_));
    gtk_window_set_title(GTK_WINDOW(window_), caption.c_str());
  }

  void SetShowCaptionAlways(bool always) {
    GGL_UNUSED(always);
    // SingleViewHost will always show caption when window decorator is shown.
  }

  void SetCursor(ViewInterface::CursorType type) {
    // Don't change cursor if it's in resize dragging mode.
    if (resize_width_mode_ || resize_height_mode_)
      return;
    GdkCursor *cursor = CreateCursor(type, view_->GetHitTest());
    if (widget_->window)
      gdk_window_set_cursor(widget_->window, cursor);
    if (cursor)
      gdk_cursor_unref(cursor);
  }

  void ShowTooltip(const std::string &tooltip) {
    // DLOG("SingleViewHost::ShowTooltip(%s)", tooltip.c_str());
    tooltip_->Show(tooltip.c_str());
  }

  void ShowTooltipAtPosition(const std::string &tooltip, double x, double y) {
    ASSERT(window_);
    ViewCoordToNativeWidgetCoord(x, y, &x, &y);
    gint screen_x = static_cast<gint>(x) + win_x_;
    gint screen_y = static_cast<gint>(y) + win_y_;
    // It's in options dialog, the native widget is not the toplevel window.
    if (widget_ != window_) {
      GdkWindow *window = widget_->window;
      GdkWindow *toplevel = gdk_window_get_toplevel(window);
      for (; window != toplevel; window = gdk_window_get_parent(window)) {
        gint pos_x, pos_y;
        gdk_window_get_position(window, &pos_x, &pos_y);
        screen_x += pos_x;
        screen_y += pos_y;
      }
    }
    DLOG("SingleViewHost::ShowTooltipAtPosition(%s, %d, %d)",
         tooltip.c_str(), screen_x, screen_y);
    tooltip_->ShowAtPosition(tooltip.c_str(), gtk_widget_get_screen(window_),
                             screen_x, screen_y);
  }

  bool ShowView(bool modal, int flags, Slot1<bool, int> *feedback_handler) {
    ASSERT(view_);
    ASSERT(window_);

    delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    SetGadgetWindowIcon(GTK_WINDOW(window_), view_->GetGadget());

    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
        gtk_widget_show(ok_button_);
      else
        gtk_widget_hide(ok_button_);

      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
        gtk_widget_show(cancel_button_);
      else
        gtk_widget_hide(cancel_button_);
    }

    // Adjust the window size just before showing the view, to make sure that
    // the window size has correct default size when showing.
    AdjustWindowSize();
    LoadWindowStates();

    // Can't use gtk_widget_show_now() here, because in some cases, it'll cause
    // nested main loop and prevent ggl-gtk from being quitted.
    gtk_widget_show(window_);
    gtk_window_present(GTK_WINDOW(window_));
    gdk_window_raise(window_->window);

    // gtk_window_stick() must be called everytime.
    if (!(flags_ & WM_MANAGEABLE))
      gtk_window_stick(GTK_WINDOW(window_));

    // Load window states again to make sure it's still correct
    // after the window is shown.
    LoadWindowStates();

    // Make sure the view is inside screen.
    EnsureInsideScreen();

    // Main view and details view doesn't support modal.
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS && modal) {
      can_close_dialog_ = false;
      while (!can_close_dialog_)
        gtk_dialog_run(GTK_DIALOG(window_));
      CloseView();
    }
    return true;
  }

  void CloseView() {
    ASSERT(window_);
    if (window_)
      gtk_widget_hide(window_);
  }

  void SetWindowPosition(int x, int y) {
    ASSERT(window_);
    if (window_) {
      win_x_ = x;
      win_y_ = y;
      gtk_window_move(GTK_WINDOW(window_), x, y);
      SaveWindowStates(true, false);
    }
  }

  void SetKeepAbove(bool keep_above) {
    ASSERT(window_);
    if (window_ && window_->window) {
      gtk_window_set_keep_above(GTK_WINDOW(window_), keep_above);
      if (is_keep_above_ != keep_above) {
        is_keep_above_ = keep_above;
        SaveWindowStates(false, true);
      }
    }
  }

  void SetWindowType(GdkWindowTypeHint type) {
    ASSERT(window_);
    if (window_ && window_->window) {
      gdk_window_set_type_hint(window_->window, type);
      gtk_window_set_keep_above(GTK_WINDOW(window_), is_keep_above_);
    }
  }

  std::string GetViewPositionOptionPrefix() {
    switch (type_) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        return "main_view";
      case ViewHostInterface::VIEW_HOST_OPTIONS:
        return "options_view";
      case ViewHostInterface::VIEW_HOST_DETAILS:
        return "details_view";
      default:
        return "view";
    }
    return "";
  }

  void SaveWindowStates(bool save_position, bool save_keep_above) {
    if ((flags_ & RECORD_STATES) && view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();
      if (save_position) {
        opt->PutInternalValue((opt_prefix + "_x").c_str(),
                              Variant(win_x_));
        opt->PutInternalValue((opt_prefix + "_y").c_str(),
                              Variant(win_y_));
      }
      if (save_keep_above) {
        opt->PutInternalValue((opt_prefix + "_keep_above").c_str(),
                              Variant(is_keep_above_));
      }
    }
    // Don't save size and zoom information, it's conflict with view
    // decorator.
  }

  void LoadWindowStates() {
    if ((flags_ & RECORD_STATES) && view_ && view_->GetGadget()) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();

      // Restore window position.
      Variant vx = opt->GetInternalValue((opt_prefix + "_x").c_str());
      Variant vy = opt->GetInternalValue((opt_prefix + "_y").c_str());
      int x, y;
      if (vx.ConvertToInt(&x) && vy.ConvertToInt(&y)) {
        win_x_ = x;
        win_y_ = y;
        gtk_window_move(GTK_WINDOW(window_), x, y);
      } else {
        // Always place the window to the center of the screen if the window
        // position was not saved before.
        gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
      }

      // Restore keep above state.
      Variant keep_above =
          opt->GetInternalValue((opt_prefix + "_keep_above").c_str());
      if (keep_above.ConvertToBool(&is_keep_above_))
        SetKeepAbove(is_keep_above_);
    } else {
      gtk_window_set_position(GTK_WINDOW(window_), GTK_WIN_POS_CENTER);
    }
    // Don't load size and zoom information, it's conflict with view
    // decorator.
  }

  void KeepAboveMenuCallback(const char *, bool keep_above) {
    SetKeepAbove(keep_above);
  }

  bool ShowContextMenu(int button) {
    ASSERT(view_);
    DLOG("Show context menu.");

    if (context_menu_)
      gtk_widget_destroy(context_menu_);

    context_menu_ = gtk_menu_new();
    MenuBuilder menu_builder(owner_, GTK_MENU_SHELL(context_menu_));

    // If it returns true, then means that it's allowed to add additional menu
    // items.
    if (view_->OnAddContextMenuItems(&menu_builder) &&
        type_ == ViewHostInterface::VIEW_HOST_MAIN) {
      menu_builder.AddItem(
          GM_("MENU_ITEM_ALWAYS_ON_TOP"),
          is_keep_above_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
          NewSlot(this, &Impl::KeepAboveMenuCallback, !is_keep_above_),
          MenuInterface::MENU_ITEM_PRI_HOST);
    }

    if (menu_builder.ItemAdded()) {
      int gtk_button;
      switch (button) {
        case MouseEvent::BUTTON_LEFT: gtk_button = 1; break;
        case MouseEvent::BUTTON_MIDDLE: gtk_button = 2; break;
        case MouseEvent::BUTTON_RIGHT: gtk_button = 3; break;
        default: gtk_button = 3; break;
      }

      if (on_show_context_menu_signal_(&menu_builder))
        // don't set button parameter, which will cause problem.
        menu_builder.Popup(0, gtk_get_current_event_time());
      return true;
    }
    return false;
  }

  void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {
    ASSERT(window_);
    if (!GTK_WIDGET_MAPPED(window_))
      return;

    if (resizable_mode_ == ViewInterface::RESIZABLE_FALSE)
      return;

    // Determine the resize drag edge.
    resize_width_mode_ = 0;
    resize_height_mode_ = 0;
    if (hittest == ViewInterface::HT_LEFT) {
      resize_width_mode_ = -1;
    } else if (hittest == ViewInterface::HT_RIGHT) {
      resize_width_mode_ = 1;
    } else if (hittest == ViewInterface::HT_TOP) {
      resize_height_mode_ = -1;
    } else if (hittest == ViewInterface::HT_BOTTOM) {
      resize_height_mode_ = 1;
    } else if (hittest == ViewInterface::HT_TOPLEFT) {
      resize_height_mode_ = -1;
      resize_width_mode_ = -1;
    } else if (hittest == ViewInterface::HT_TOPRIGHT) {
      resize_height_mode_ = -1;
      resize_width_mode_ = 1;
    } else if (hittest == ViewInterface::HT_BOTTOMLEFT) {
      resize_height_mode_ = 1;
      resize_width_mode_ = -1;
    } else if (hittest == ViewInterface::HT_BOTTOMRIGHT) {
      resize_height_mode_ = 1;
      resize_width_mode_ = 1;
    } else {
      // Unsupported hittest;
      return;
    }

    if (on_begin_resize_drag_signal_(button, hittest)) {
      resize_width_mode_ = 0;
      resize_height_mode_ = 0;
      return;
    }

    resize_view_zoom_ = view_->GetGraphics()->GetZoom();
    resize_view_width_ = view_->GetWidth();
    resize_view_height_ = view_->GetHeight();
    resize_win_x_ = win_x_;
    resize_win_y_ = win_y_;
    resize_win_width_ = win_width_;
    resize_win_height_ = win_height_;
    resize_button_ = button;

    GdkEvent *event = gtk_get_current_event();
    if (!event || !gdk_event_get_root_coords(event, &resize_mouse_x_,
                                             &resize_mouse_y_)) {
      gint x, y;
      gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
      resize_mouse_x_ = x;
      resize_mouse_y_ = y;
    }

    if (event)
      gdk_event_free(event);

#ifdef GRAB_POINTER_EXPLICITLY
    // Grabbing mouse explicitly is not necessary.
#ifdef _DEBUG
    GdkGrabStatus grab_status =
#endif
    gdk_pointer_grab(window_->window, FALSE,
                     (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                    GDK_BUTTON_MOTION_MASK |
                                    GDK_POINTER_MOTION_MASK |
                                    GDK_POINTER_MOTION_HINT_MASK),
                     NULL, NULL, gtk_get_current_event_time());
#ifdef _DEBUG
    DLOG("BeginResizeDrag: grab status: %d", grab_status);
#endif
#endif
  }

  void StopResizeDrag() {
    if (resize_width_mode_ || resize_height_mode_) {
      resize_width_mode_ = 0;
      resize_height_mode_ = 0;
#ifdef GRAB_POINTER_EXPLICITLY
      gdk_pointer_ungrab(gtk_get_current_event_time());
#endif
      QueueResize();
      on_end_resize_drag_signal_();
      SetCursor(ViewInterface::CURSOR_DEFAULT);
    }
  }

  void BeginMoveDrag(int button) {
    ASSERT(window_);
    if (!GTK_WIDGET_MAPPED(window_))
      return;

    if (on_begin_move_drag_signal_(button))
      return;

    move_dragging_ = true;

    if (stop_move_drag_source_)
      g_source_remove(stop_move_drag_source_);

    stop_move_drag_source_ =
        g_timeout_add(kStopMoveDragTimeout,
                      StopMoveDragTimeoutHandler, this);

    gint x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    int gtk_button = (button == MouseEvent::BUTTON_LEFT ? 1 :
                      button == MouseEvent::BUTTON_MIDDLE ? 2 : 3);
    gtk_window_begin_move_drag(GTK_WINDOW(window_), gtk_button,
                               x, y, gtk_get_current_event_time());
  }

  void StopMoveDrag() {
    if (move_dragging_) {
      DLOG("Stop move dragging.");
      move_dragging_ = false;
      on_end_move_drag_signal_();
    }
    if (stop_move_drag_source_) {
      g_source_remove(stop_move_drag_source_);
      stop_move_drag_source_ = 0;
    }
    SetCursor(ViewInterface::CURSOR_DEFAULT);
  }

  void EnsureInsideScreen() {
    GdkScreen *screen = gtk_widget_get_screen(window_);
    int screen_width = gdk_screen_get_width(screen);
    int screen_height = gdk_screen_get_height(screen);
    int win_center_x = win_x_ + win_width_ / 2;
    int win_center_y = win_y_ + win_width_ / 2;

    if (win_center_x < 0 || win_center_x >= screen_width ||
        win_center_y < 0 || win_center_y >= screen_height) {
      DLOG("View is out of screen: sw: %d, sh: %d, x: %d, y: %d",
           screen_width, screen_height, win_center_x, win_center_y);
      win_x_ = (screen_width - win_width_) / 2;
      win_y_ = (screen_height - win_height_) / 2;
      gtk_window_move(GTK_WINDOW(window_), win_x_, win_y_);
    }
  }

  // gtk signal handlers.
  static gboolean FocusInHandler(GtkWidget *widget, GdkEventFocus *event,
                                 gpointer user_data) {
    GGL_UNUSED(event);
    DLOG("FocusInHandler(%p)", widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->move_dragging_)
      impl->StopMoveDrag();
    return FALSE;
  }

#ifdef _DEBUG
  static gboolean FocusOutHandler(GtkWidget *widget, GdkEventFocus *event,
                                  gpointer user_data) {
    GGL_UNUSED(event);
    GGL_UNUSED(user_data);
    DLOG("FocusOutHandler(%p)", widget);
    return FALSE;
  }
#endif

  static gboolean EnterNotifyHandler(GtkWidget *widget, GdkEventCrossing *event,
                                     gpointer user_data) {
    DLOG("EnterNotifyHandler(%p): %d, %d", widget, event->mode, event->detail);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->move_dragging_)
      impl->StopMoveDrag();
    return FALSE;
  }

  static void WindowShowHandler(GtkWidget *widget, gpointer user_data) {
    GGL_UNUSED(widget);
    DLOG("View window is going to be shown.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->view_ && impl->enable_signals_)
      impl->on_show_hide_signal_(true);
  }

  static void WindowHideHandler(GtkWidget *widget, gpointer user_data) {
    GGL_UNUSED(widget);
    DLOG("View window is going to be hidden.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->view_ && impl->enable_signals_) {
      impl->on_show_hide_signal_(false);

      if (impl->feedback_handler_ &&
          impl->type_ == ViewHostInterface::VIEW_HOST_DETAILS) {
        (*impl->feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
        delete impl->feedback_handler_;
        impl->feedback_handler_ = NULL;
      } else if (impl->type_ == ViewHostInterface::VIEW_HOST_MAIN &&
                 (impl->flags_ & REMOVE_ON_CLOSE) && impl->view_->GetGadget()) {
        impl->view_->GetGadget()->RemoveMe(true);
      }
    }
  }

  static gboolean ConfigureHandler(GtkWidget *widget, GdkEventConfigure *event,
                                   gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->enable_signals_) {
      if (impl->win_x_ != event->x || impl->win_y_ != event->y) {
        impl->win_x_ = event->x;
        impl->win_y_ = event->y;
        impl->on_moved_signal_(event->x, event->y);
        // SaveWindowStates() only saves window position.
        impl->SaveWindowStates(true, false);
      }
      if (impl->win_width_ != event->width ||
          impl->win_height_ != event->height) {
        impl->win_width_ = event->width;
        impl->win_height_ = event->height;
        impl->on_resized_signal_(event->width, event->height);
      }
    }
    return FALSE;
  }

  static void DialogResponseHandler(GtkDialog *dialog, gint response,
                                    gpointer user_data) {
    GGL_UNUSED(dialog);
    DLOG("%s button clicked in options dialog.",
         response == GTK_RESPONSE_OK ? "Ok" :
         response == GTK_RESPONSE_CANCEL ? "Cancel" : "No");

    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->feedback_handler_) {
      bool result = (*impl->feedback_handler_)(
          response == GTK_RESPONSE_OK ?
              ViewInterface::OPTIONS_VIEW_FLAG_OK :
              ViewInterface::OPTIONS_VIEW_FLAG_CANCEL);
      // 5.8 API allows the onok handler to cancel the default action.
      if (response != GTK_RESPONSE_OK || result) {
        delete impl->feedback_handler_;
        impl->feedback_handler_ = NULL;
        impl->can_close_dialog_ = true;
      }
    } else {
      impl->can_close_dialog_ = true;
    }
  }

  static gboolean MotionNotifyHandler(GtkWidget *widget, GdkEventMotion *event,
                                      gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->resize_width_mode_ || impl->resize_height_mode_) {
      if (event->is_hint) {
        // Since motion hint is enabled, we must notify GTK that we're ready to
        // receive the next motion event.
#if GTK_CHECK_VERSION(2,12,0)
        gdk_event_request_motions(event); // requires version 2.12
#else
        int x, y;
        gdk_window_get_pointer(widget->window, &x, &y, NULL);
#endif
      }

      if (impl->binder_ && impl->binder_->DrawQueued())
        return TRUE;

      int button = ConvertGdkModifierToButton(event->state);
      if (button == impl->resize_button_) {
        double original_width =
            impl->resize_view_width_ * impl->resize_view_zoom_;
        double original_height =
            impl->resize_view_height_ * impl->resize_view_zoom_;
        double delta_x = event->x_root - impl->resize_mouse_x_;
        double delta_y = event->y_root - impl->resize_mouse_y_;
        double width = original_width;
        double height = original_height;
        double new_width = width + impl->resize_width_mode_ * delta_x;
        double new_height = height + impl->resize_height_mode_ * delta_y;
        if (impl->resizable_mode_ == ViewInterface::RESIZABLE_TRUE ||
            impl->resizable_mode_ == ViewInterface::RESIZABLE_KEEP_RATIO) {
          double view_width = new_width / impl->resize_view_zoom_;
          double view_height = new_height / impl->resize_view_zoom_;
          if (impl->view_->OnSizing(&view_width, &view_height)) {
            impl->view_->SetSize(view_width, view_height);
            width = impl->view_->GetWidth() * impl->resize_view_zoom_;
            height = impl->view_->GetHeight() * impl->resize_view_zoom_;
          }
        } else if (impl->resize_view_width_ && impl->resize_view_height_) {
          double xzoom = new_width / impl->resize_view_width_;
          double yzoom = new_height / impl->resize_view_height_;
          double zoom = std::min(xzoom, yzoom);
          zoom = Clamp(zoom, kMinimumZoom, kMaximumZoom);
          impl->view_->GetGraphics()->SetZoom(zoom);
          impl->view_->MarkRedraw();
          width = impl->resize_view_width_ * zoom;
          height = impl->resize_view_height_ * zoom;
        }

        if (width != original_width || height != original_height) {
          delta_x = width - original_width;
          delta_y = height - original_height;
          int x = impl->resize_win_x_;
          int y = impl->resize_win_y_;
          if (impl->resize_width_mode_ == -1)
            x -= int(delta_x);
          if (impl->resize_height_mode_ == -1)
            y -= int(delta_y);
          int win_width = impl->resize_win_width_ + int(delta_x);
          int win_height = impl->resize_win_height_ + int(delta_y);
          gdk_window_move_resize(widget->window, x, y, win_width, win_height);
        }

        return TRUE;
      } else {
        impl->StopResizeDrag();
      }
    }
    return FALSE;
  }

  static gboolean ButtonReleaseHandler(GtkWidget *widget, GdkEventButton *event,
                                       gpointer user_data) {
    GGL_UNUSED(widget);
    GGL_UNUSED(event);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->resize_width_mode_ || impl->resize_height_mode_) {
      impl->StopResizeDrag();
      return TRUE;
    }
    return FALSE;
  }

  static void WidgetSizeRequestHandler(GtkWidget *widget,
                                      GtkRequisition *requisition,
                                      gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    double zoom = impl->view_->GetGraphics()->GetZoom();
    if (impl->resizable_mode_ == ViewInterface::RESIZABLE_FALSE) {
      double width = impl->view_->GetWidth() * zoom;
      double height = impl->view_->GetHeight() * zoom;
      requisition->width = static_cast<int>(ceil(width));
      requisition->height = static_cast<int>(ceil(height));
    } else if (impl->type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      double width, height;
      // Don't allow user to shrink options dialog.
      impl->view_->GetDefaultSize(&width, &height);
      requisition->width = static_cast<int>(ceil(width * zoom));
      requisition->height = static_cast<int>(ceil(height * zoom));
    } else {
      // To make sure that user can resize the toplevel window freely.
      requisition->width = 1;
      requisition->height = 1;
    }
    DLOG("%s window size request(%d, %d)",
         (impl->type_ == ViewHostInterface::VIEW_HOST_OPTIONS ? "Options" :
          (impl->type_ == ViewHostInterface::VIEW_HOST_MAIN ? "Main" :
           "Details")),
         requisition->width, requisition->height);
  }

  static void FixedSizeAllocateHandler(GtkWidget *widget,
                                       GtkAllocation *allocation,
                                       gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (GTK_WIDGET_VISIBLE(impl->window_) &&
        !impl->resize_width_mode_ && !impl->resize_height_mode_ &&
        !impl->queue_resize_timer_ &&
        allocation->width >= 1 && allocation->height >= 1 &&
        (impl->fixed_width_from_view_ != allocation->width ||
         impl->fixed_height_from_view_ != allocation->height)) {
      double old_width = impl->view_->GetWidth();
      double old_height = impl->view_->GetHeight();
      double old_zoom = impl->view_->GetGraphics()->GetZoom();
      if (impl->resizable_mode_ == ViewInterface::RESIZABLE_TRUE ||
          impl->resizable_mode_ == ViewInterface::RESIZABLE_KEEP_RATIO) {
        double new_width = allocation->width / old_zoom;
        double new_height = allocation->height / old_zoom;
        if (impl->view_->OnSizing(&new_width, &new_height) &&
            (new_width != old_width || new_height != old_height)) {
          impl->view_->SetSize(new_width, new_height);
        }
      } else if (impl->resizable_mode_ == ViewInterface::RESIZABLE_ZOOM &&
                 impl->type_ != ViewHostInterface::VIEW_HOST_OPTIONS) {
        double xzoom = allocation->width / old_width;
        double yzoom = allocation->height / old_height;
        double new_zoom = Clamp(std::min(xzoom, yzoom),
                                kMinimumZoom, kMaximumZoom);
        if (old_zoom != new_zoom) {
          impl->view_->GetGraphics()->SetZoom(new_zoom);
          impl->view_->MarkRedraw();
        }
      }
      impl->QueueResize();
    }
  }

  static gboolean StopMoveDragTimeoutHandler(gpointer data) {
    Impl *impl = reinterpret_cast<Impl *>(data);
    if (impl->move_dragging_) {
      GdkDisplay *display = gtk_widget_get_display(impl->window_);
      GdkModifierType mod;
      gdk_display_get_pointer(display, NULL, NULL, NULL, &mod);
      int btn_mods = GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK;
      if ((mod & btn_mods) == 0) {
        impl->stop_move_drag_source_ = 0;
        impl->StopMoveDrag();
        return FALSE;
      }
      return TRUE;
    }
    impl->stop_move_drag_source_ = 0;
    return FALSE;
  }

  static gboolean QueueResizeTimeoutHandler(gpointer data) {
    Impl *impl = reinterpret_cast<Impl *>(data);
    impl->AdjustWindowSize();
    impl->queue_resize_timer_ = 0;
    return false;
  }

  // Some elements may create gtk native widgets under this widget. When such
  // a widget get focus, we must update the focus chain though the view
  // hierachy.
  static void FixedSetFocusChildHandler(GtkContainer *container,
                                        GtkWidget *widget,
                                        gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    DLOG("FixedSetFocusChildHandler widget: %p, view: %p, child: %p",
         container, impl->view_, widget);
    if (widget) {
      // Send fake MOUSE_DOWN/MOUSE_UP events to update the focus chain
      // from the hosted view down to the element containing the native
      // widget.
      gint x = widget->allocation.x + widget->allocation.width / 2;
      gint y = widget->allocation.y + widget->allocation.height / 2;
      double view_x = 0, view_y = 0;
      impl->NativeWidgetCoordToViewCoord(x, y, &view_x, &view_y);
      MouseEvent down_event(Event::EVENT_MOUSE_DOWN, view_x, view_y,
                            0, 0, MouseEvent::BUTTON_LEFT, 0);
      impl->view_->OnMouseEvent(down_event);
      MouseEvent up_event(Event::EVENT_MOUSE_UP, view_x, view_y,
                          0, 0, MouseEvent::BUTTON_LEFT, 0);
      impl->view_->OnMouseEvent(up_event);
    }
  }

  SingleViewHost *owner_;
  ViewInterface *view_;

  GtkWidget *window_;
  GtkWidget *widget_;
  GtkWidget *fixed_;
  GtkWidget *context_menu_;

  // For options view.
  GtkWidget *ok_button_;
  GtkWidget *cancel_button_;

  Tooltip *tooltip_;
  ViewWidgetBinder *binder_;

  ViewHostInterface::Type type_;
  double initial_zoom_;
  int flags_;
  int debug_mode_;

  int stop_move_drag_source_;

  int win_x_;
  int win_y_;
  int win_width_;
  int win_height_;

  // For resize drag
  double resize_view_zoom_;
  double resize_view_width_;
  double resize_view_height_;

  int resize_win_x_;
  int resize_win_y_;
  int resize_win_width_;
  int resize_win_height_;

  int resize_button_;
  gdouble resize_mouse_x_;
  gdouble resize_mouse_y_;

  // -1 to resize left, 1 to resize right.
  int resize_width_mode_;
  // -1 to resize top, 1 to resize bottom.
  int resize_height_mode_;
  // End of resize drag variants.

  ViewInterface::ResizableMode resizable_mode_;

  bool is_keep_above_;
  bool move_dragging_;
  bool enable_signals_;

  guint queue_resize_timer_;
  int fixed_width_from_view_;
  int fixed_height_from_view_;

  Slot1<bool, int> *feedback_handler_;
  bool can_close_dialog_; // Only useful when a model dialog is running.

  Signal0<void> on_view_changed_signal_;
  Signal1<void, bool> on_show_hide_signal_;

  Signal2<bool, int, int> on_begin_resize_drag_signal_;
  Signal2<void, int, int> on_resized_signal_;
  Signal0<void> on_end_resize_drag_signal_;

  Signal1<bool, int> on_begin_move_drag_signal_;
  Signal2<void, int, int> on_moved_signal_;
  Signal0<void> on_end_move_drag_signal_;

  Signal1<bool, MenuInterface*> on_show_context_menu_signal_;

  static const unsigned int kShowTooltipDelay = 500;
  static const unsigned int kHideTooltipDelay = 4000;
};

SingleViewHost::SingleViewHost(ViewHostInterface::Type type,
                               double zoom, int flags, int debug_mode)
  : impl_(new Impl(this, type, zoom, flags, debug_mode)) {
}

SingleViewHost::~SingleViewHost() {
  DLOG("SingleViewHost Dtor: %p", this);
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface::Type SingleViewHost::GetType() const {
  return impl_->type_;
}

void SingleViewHost::Destroy() {
  delete this;
}

void SingleViewHost::SetView(ViewInterface *view) {
  impl_->SetView(view);
}

ViewInterface *SingleViewHost::GetView() const {
  return impl_->view_;
}

void *SingleViewHost::GetNativeWidget() const {
  return impl_->fixed_;
}

void SingleViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  impl_->ViewCoordToNativeWidgetCoord(x, y, widget_x, widget_y);
}

void SingleViewHost::NativeWidgetCoordToViewCoord(
    double x, double y, double *view_x, double *view_y) const {
  impl_->NativeWidgetCoordToViewCoord(x, y, view_x, view_y);
}

void SingleViewHost::QueueDraw() {
  impl_->QueueDraw();
}

void SingleViewHost::QueueResize() {
  impl_->QueueResize();
}

void SingleViewHost::EnableInputShapeMask(bool enable) {
  impl_->EnableInputShapeMask(enable);
}

void SingleViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  impl_->SetResizable(mode);
}

void SingleViewHost::SetCaption(const std::string &caption) {
  impl_->SetCaption(caption);
}

void SingleViewHost::SetShowCaptionAlways(bool always) {
  impl_->SetShowCaptionAlways(always);
}

void SingleViewHost::SetCursor(ViewInterface::CursorType type) {
  impl_->SetCursor(type);
}

void SingleViewHost::ShowTooltip(const std::string &tooltip) {
  impl_->ShowTooltip(tooltip);
}

void SingleViewHost::ShowTooltipAtPosition(const std::string &tooltip,
                                           double x, double y) {
  impl_->ShowTooltipAtPosition(tooltip, x, y);
}

bool SingleViewHost::ShowView(bool modal, int flags,
                              Slot1<bool, int> *feedback_handler) {
  return impl_->ShowView(modal, flags, feedback_handler);
}

void SingleViewHost::CloseView() {
  impl_->CloseView();
}

bool SingleViewHost::ShowContextMenu(int button) {
  return impl_->ShowContextMenu(button);
}

void SingleViewHost::BeginResizeDrag(int button,
                                     ViewInterface::HitTest hittest) {
  impl_->BeginResizeDrag(button, hittest);
}

void SingleViewHost::BeginMoveDrag(int button) {
  impl_->BeginMoveDrag(button);
}

void SingleViewHost::Alert(const ViewInterface *view, const char *message) {
  ShowAlertDialog(view->GetCaption().c_str(), message);
}

ViewHostInterface::ConfirmResponse SingleViewHost::Confirm(
    const ViewInterface *view, const char *message, bool cancel_button) {
  return ShowConfirmDialog(view->GetCaption().c_str(), message, cancel_button);
}

std::string SingleViewHost::Prompt(const ViewInterface *view,
                                   const char *message,
                                   const char *default_value) {
  return ShowPromptDialog(view->GetCaption().c_str(), message, default_value);
}

int SingleViewHost::GetDebugMode() const {
  return impl_->debug_mode_;
}

GraphicsInterface *SingleViewHost::NewGraphics() const {
  return new CairoGraphics(impl_->initial_zoom_);
}

GtkWidget *SingleViewHost::GetWindow() const {
  return impl_->window_;
}

void SingleViewHost::GetWindowPosition(int *x, int *y) {
  if (x) *x = impl_->win_x_;
  if (y) *y = impl_->win_y_;
}

void SingleViewHost::SetWindowPosition(int x, int y) {
  impl_->SetWindowPosition(x, y);
}

void SingleViewHost::GetWindowSize(int *width, int *height) {
  if (width) *width = impl_->win_width_;
  if (height) *height = impl_->win_height_;
}

bool SingleViewHost::IsKeepAbove() const {
  return impl_->is_keep_above_;
}

void SingleViewHost::SetKeepAbove(bool keep_above) {
  impl_->SetKeepAbove(keep_above);
}

bool SingleViewHost::IsVisible() const {
  return impl_->window_ && GTK_WIDGET_VISIBLE(impl_->window_);
}

void SingleViewHost::SetWindowType(GdkWindowTypeHint type) {
  impl_->SetWindowType(type);
}

Connection *SingleViewHost::ConnectOnViewChanged(Slot0<void> *slot) {
  return impl_->on_view_changed_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnShowHide(Slot1<void, bool> *slot) {
  return impl_->on_show_hide_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnBeginResizeDrag(
    Slot2<bool, int, int> *slot) {
  return impl_->on_begin_resize_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnResized(Slot2<void, int, int> *slot) {
  return impl_->on_resized_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnEndResizeDrag(Slot0<void> *slot) {
  return impl_->on_end_resize_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnBeginMoveDrag(Slot1<bool, int> *slot) {
  return impl_->on_begin_move_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnMoved(Slot2<void, int, int> *slot) {
  return impl_->on_moved_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnEndMoveDrag(Slot0<void> *slot) {
  return impl_->on_end_move_drag_signal_.Connect(slot);
}

Connection *SingleViewHost::ConnectOnShowContextMenu(
    Slot1<bool, MenuInterface*> *slot) {
}

} // namespace gtk
} // namespace ggadget
