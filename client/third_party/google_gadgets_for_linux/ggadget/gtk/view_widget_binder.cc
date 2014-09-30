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

#include <algorithm>
#include <string>
#include <vector>
#include <cairo.h>
#include <gdk/gdk.h>

#include "view_widget_binder.h"
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/event.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/view_interface.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/small_object.h>
#include <ggadget/clip_region.h>
#include <ggadget/math_utils.h>
#include "cairo_canvas.h"
#include "cairo_graphics.h"
#include "key_convert.h"
#include "utilities.h"

// It might not be necessary, because X server will grab the pointer
// implicitly when button is pressed.
// But using explicit mouse grabbing may avoid some issues by preventing some
// events from sending to client window when mouse is grabbed.
#define GRAB_POINTER_EXPLICITLY
namespace ggadget {
namespace gtk {

static const char kUriListTarget[] = "text/uri-list";
static const char kPlainTextTarget[] = "text/plain";

// A small motion threshold to prevent a click with tiny mouse move from being
// treated as window move or resize.
static const double kDragThreshold = 3;

#ifdef _DEBUG
static const uint64_t kFPSCountDuration = 5000;
#endif

// Update input shape mask once per second.
static const uint64_t kUpdateMaskInterval = 1000;

// Minimal interval between self draws.
static const unsigned int kSelfDrawInterval = 40;

class ViewWidgetBinder::Impl : public SmallObject<> {
 public:
  Impl(ViewInterface *view,
       ViewHostInterface *host, GtkWidget *widget,
       bool no_background)
    : view_(view),
      host_(host),
      widget_(widget),
#if GTK_CHECK_VERSION(2,10,0)
      input_shape_mask_(NULL),
      last_mask_time_(0),
      should_update_input_shape_mask_(false),
      enable_input_shape_mask_(false),
#endif
      handlers_(new gulong[kEventHandlersNum]),
      current_drag_event_(NULL),
      on_zoom_connection_(NULL),
      dbl_click_(false),
      composited_(false),
      no_background_(no_background),
      focused_(false),
      button_pressed_(false),
#ifdef GRAB_POINTER_EXPLICITLY
      pointer_grabbed_(false),
#endif
#ifdef _DEBUG
      draw_count_(0),
      last_fps_time_(0),
#endif
      zoom_(1.0),
      mouse_down_x_(-1),
      mouse_down_y_(-1),
      mouse_down_hittest_(ViewInterface::HT_CLIENT),
      self_draw_(false),
      self_draw_timer_(0),
      last_self_draw_time_(0),
      sys_clip_region_(NULL) {
    ASSERT(view);
    ASSERT(host);
    ASSERT(GTK_IS_WIDGET(widget));
    ASSERT(GTK_WIDGET_NO_WINDOW(widget) == FALSE);

    g_object_ref(G_OBJECT(widget_));
    gtk_widget_set_app_paintable(widget_, TRUE);
    gtk_widget_set_double_buffered(widget_, FALSE);

    gint events = GDK_EXPOSURE_MASK |
                  GDK_FOCUS_CHANGE_MASK |
                  GDK_ENTER_NOTIFY_MASK |
                  GDK_LEAVE_NOTIFY_MASK |
                  GDK_BUTTON_PRESS_MASK |
                  GDK_BUTTON_RELEASE_MASK |
                  GDK_POINTER_MOTION_MASK |
                  GDK_POINTER_MOTION_HINT_MASK |
                  GDK_STRUCTURE_MASK;

    if (GTK_WIDGET_REALIZED(widget_))
      gtk_widget_add_events(widget_, events);
    else
      gtk_widget_set_events(widget_, gtk_widget_get_events(widget_) | events);

    GTK_WIDGET_SET_FLAGS(widget_, GTK_CAN_FOCUS);

    static const GtkTargetEntry kDragTargets[] = {
      { const_cast<char *>(kUriListTarget), 0, 0 },
      { const_cast<char *>(kPlainTextTarget), 0, 0 },
    };

    gtk_drag_dest_set(widget_, static_cast<GtkDestDefaults>(0),
                      kDragTargets, arraysize(kDragTargets), GDK_ACTION_COPY);

    SetupBackgroundMode();

    for (size_t i = 0; i < kEventHandlersNum; ++i) {
      handlers_[i] = g_signal_connect(G_OBJECT(widget_),
                                      kEventHandlers[i].event,
                                      kEventHandlers[i].handler,
                                      this);
    }

    CairoGraphics *gfx= down_cast<CairoGraphics *>(view_->GetGraphics());
    ASSERT(gfx);

    zoom_ = gfx->GetZoom();
    on_zoom_connection_ = gfx->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
  }

  ~Impl() {
    view_ = NULL;

    if (self_draw_timer_) {
      g_source_remove(self_draw_timer_);
      self_draw_timer_ = 0;
    }

    if (sys_clip_region_) {
      gdk_region_destroy(sys_clip_region_);
      sys_clip_region_ = NULL;
    }

    for (size_t i = 0; i < kEventHandlersNum; ++i) {
      if (handlers_[i] > 0)
        g_signal_handler_disconnect(G_OBJECT(widget_), handlers_[i]);
      else
        DLOG("Handler %s was not connected.", kEventHandlers[i].event);
    }

    delete[] handlers_;
    handlers_ = NULL;

    if (current_drag_event_) {
      delete current_drag_event_;
      current_drag_event_ = NULL;
    }

    if (on_zoom_connection_) {
      on_zoom_connection_->Disconnect();
      on_zoom_connection_ = NULL;
    }

    g_object_unref(G_OBJECT(widget_));
#if GTK_CHECK_VERSION(2,10,0)
    if (input_shape_mask_) {
      g_object_unref(G_OBJECT(input_shape_mask_));
    }
#endif
  }

  void OnZoom(double zoom) {
    zoom_ = zoom;
  }

  // Disable background if required.
  void SetupBackgroundMode() {
    // Only try to disable background if explicitly required.
    if (no_background_) {
      composited_ = DisableWidgetBackground(widget_);
    }
  }

  GdkRegion *CreateExposeRegionFromViewClipRegion() {
    GdkRegion *region = gdk_region_new();
    const ClipRegion *view_region = view_->GetClipRegion();
    size_t count = view_region->GetRectangleCount();
    if (count) {
      Rectangle rect;
      GdkRectangle gdk_rect;
      for (size_t i = 0; i < count; ++i) {
        rect = view_region->GetRectangle(i);
        if (zoom_ != 1.0) {
          rect.Zoom(zoom_);
          rect.Integerize(true);
        }
        gdk_rect.x = static_cast<int>(rect.x);
        gdk_rect.y = static_cast<int>(rect.y);
        gdk_rect.width = static_cast<int>(rect.w);
        gdk_rect.height = static_cast<int>(rect.h);
        gdk_region_union_with_rect(region, &gdk_rect);
      }
    }
    return region;
  }

  void AddGdkRectToSystemClipRegion(GdkRectangle *rect) {
    if (!sys_clip_region_)
      sys_clip_region_ = gdk_region_new();
    gdk_region_union_with_rect(sys_clip_region_, rect);
  }

  void AddGdkRegionToSystemClipRegion(GdkRegion *region) {
    if (!sys_clip_region_)
      sys_clip_region_ = gdk_region_new();
    gdk_region_union(sys_clip_region_, region);
  }

  void AddWindowUpdateAreaToSystemClipRegion() {
    GdkRegion *region = gdk_window_get_update_area(widget_->window);
    if (region) {
      if (!sys_clip_region_) {
        sys_clip_region_ = region;
      } else {
        gdk_region_union(sys_clip_region_, region);
        gdk_region_destroy(region);
      }
    }
  }

  void AddGdkRectToViewClipRegion(const GdkRectangle &gdk_rect) {
    Rectangle rect(gdk_rect.x, gdk_rect.y, gdk_rect.width, gdk_rect.height);
    rect.Zoom(1.0 / zoom_);
    rect.Integerize(true);
    view_->AddRectangleToClipRegion(rect);
  }

  void AddGdkRegionToViewClipRegion(GdkRegion *region) {
    if (!gdk_region_empty(region)) {
      GdkRectangle *rects;
      gint n_rects;
      gdk_region_get_rectangles(region, &rects, &n_rects);
      for (gint i = 0; i < n_rects; ++i) {
        AddGdkRectToViewClipRegion(rects[i]);
      }
      g_free(rects);
    }
  }

#if GTK_CHECK_VERSION(2,10,0)
  bool ShouldUpdateInputShapeMask() {
    gint width, height;
    gdk_drawable_get_size(widget_->window, &width, &height);

    bool update_input_shape_mask = enable_input_shape_mask_ &&
        (GetCurrentTime() - last_mask_time_ > kUpdateMaskInterval) &&
        no_background_ && composited_;

    // We need set input shape mask if there is no background.
    if (update_input_shape_mask) {
      if (input_shape_mask_) {
        gint mask_width, mask_height;
        gdk_drawable_get_size(GDK_DRAWABLE(input_shape_mask_),
                              &mask_width, &mask_height);
        if (mask_width != width || mask_height != height) {
          // input shape mask needs recreate.
          g_object_unref(G_OBJECT(input_shape_mask_));
          input_shape_mask_ = NULL;
        }
      }

      if (input_shape_mask_ == NULL) {
        GdkRectangle rect;
        rect.x = 0;
        rect.y = 0;
        rect.width = width;
        rect.height = height;
        input_shape_mask_ = gdk_pixmap_new(NULL, width, height, 1);

        // Redraw whole view.
        AddGdkRectToSystemClipRegion(&rect);
      }
    }

    return update_input_shape_mask;
  }
#endif

  GdkRegion *GetInvalidateRegion() {
    view_->Layout();
    AddWindowUpdateAreaToSystemClipRegion();
#if GTK_CHECK_VERSION(2,10,0)
    should_update_input_shape_mask_ = ShouldUpdateInputShapeMask();
#endif
    GdkRegion *region = CreateExposeRegionFromViewClipRegion();
    if (sys_clip_region_) {
      AddGdkRegionToViewClipRegion(sys_clip_region_);
      gdk_region_union(region, sys_clip_region_);
      gdk_region_destroy(sys_clip_region_);
      sys_clip_region_ = NULL;
    }
    return region;
  }

  void SelfDraw() {
    if (!widget_->window || !gdk_window_is_visible(widget_->window))
      return;

    self_draw_ = true;
    GdkRegion *region = GetInvalidateRegion();
    if (!gdk_region_empty(region)) {
      gdk_window_invalidate_region(widget_->window, region, TRUE);
      gdk_window_process_updates(widget_->window, TRUE);
    }
    gdk_region_destroy(region);
    last_self_draw_time_ = GetCurrentTime();
    self_draw_ = false;
  }

  static gboolean ButtonPressHandler(GtkWidget *widget, GdkEventButton *event,
                                     gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    DLOG("ButtonPressHandler: widget: %p, view: %p, focused: %d, child: %p",
         widget, impl->view_, impl->focused_,
         gtk_window_get_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget))));

    EventResult result = EVENT_RESULT_UNHANDLED;

    // Clicking on this widget removes any native child widget focus.
    if (GTK_IS_WINDOW(widget)) {
      gtk_window_set_focus(GTK_WINDOW(widget), NULL);
    }

    impl->button_pressed_ = true;
    impl->host_->ShowTooltip("");

    if (!impl->focused_) {
      impl->focused_ = true;
      SimpleEvent e(Event::EVENT_FOCUS_IN);
      // Ignore the result.
      impl->view_->OnOtherEvent(e);
      if (!gtk_widget_is_focus(widget)) {
        gtk_widget_grab_focus(widget);
        gdk_window_focus(impl->widget_->window, event->time);
      }
    }

    int mod = ConvertGdkModifierToModifier(event->state);
    int button = event->button == 1 ? MouseEvent::BUTTON_LEFT :
                 event->button == 2 ? MouseEvent::BUTTON_MIDDLE :
                 event->button == 3 ? MouseEvent::BUTTON_RIGHT :
                                      MouseEvent::BUTTON_NONE;

    Event::Type type = Event::EVENT_INVALID;
    if (event->type == GDK_BUTTON_PRESS) {
      type = Event::EVENT_MOUSE_DOWN;
      impl->mouse_down_x_ = event->x_root;
      impl->mouse_down_y_ = event->y_root;
    } else if (event->type == GDK_2BUTTON_PRESS) {
      impl->dbl_click_ = true;
      if (button == MouseEvent::BUTTON_LEFT)
        type = Event::EVENT_MOUSE_DBLCLICK;
      else if (button == MouseEvent::BUTTON_RIGHT)
        type = Event::EVENT_MOUSE_RDBLCLICK;
    }

    if (button != MouseEvent::BUTTON_NONE && type != Event::EVENT_INVALID) {
      MouseEvent e(type, event->x / impl->zoom_, event->y / impl->zoom_,
                   0, 0, button, mod, event);

      result = impl->view_->OnMouseEvent(e);

      impl->mouse_down_hittest_ = impl->view_->GetHitTest();
      // If the View's hittest represents a special button, then handle it
      // here.
      if (result == EVENT_RESULT_UNHANDLED &&
          button == MouseEvent::BUTTON_LEFT &&
          type == Event::EVENT_MOUSE_DOWN) {
        if (impl->mouse_down_hittest_ == ViewInterface::HT_MENU) {
          impl->host_->ShowContextMenu(button);
        } else if (impl->mouse_down_hittest_ == ViewInterface::HT_CLOSE) {
          impl->host_->CloseView();
        }
        result = EVENT_RESULT_HANDLED;
      }
    }

    return result != EVENT_RESULT_UNHANDLED;
  }

  static gboolean ButtonReleaseHandler(GtkWidget *widget, GdkEventButton *event,
                                       gpointer user_data) {
    GGL_UNUSED(widget);
    DLOG("ButtonReleaseHandler.");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    EventResult result = EVENT_RESULT_UNHANDLED;
    EventResult result2 = EVENT_RESULT_UNHANDLED;

    impl->button_pressed_ = false;
    impl->host_->ShowTooltip("");
#ifdef GRAB_POINTER_EXPLICITLY
    if (impl->pointer_grabbed_) {
      gdk_pointer_ungrab(event->time);
      impl->pointer_grabbed_ = false;
    }
#endif

    int mod = ConvertGdkModifierToModifier(event->state);
    int button = event->button == 1 ? MouseEvent::BUTTON_LEFT :
                 event->button == 2 ? MouseEvent::BUTTON_MIDDLE :
                 event->button == 3 ? MouseEvent::BUTTON_RIGHT :
                                      MouseEvent::BUTTON_NONE;

    if (button != MouseEvent::BUTTON_NONE) {
      MouseEvent e(Event::EVENT_MOUSE_UP,
                   event->x / impl->zoom_, event->y / impl->zoom_,
                   0, 0, button, mod, event);
      result = impl->view_->OnMouseEvent(e);

      if (!impl->dbl_click_) {
        MouseEvent e2(button == MouseEvent::BUTTON_LEFT ?
                      Event::EVENT_MOUSE_CLICK : Event::EVENT_MOUSE_RCLICK,
                      event->x / impl->zoom_, event->y / impl->zoom_,
                      0, 0, button, mod);
        result2 = impl->view_->OnMouseEvent(e2);
      } else {
        impl->dbl_click_ = false;
      }
    }

    impl->mouse_down_x_ = -1;
    impl->mouse_down_y_ = -1;
    impl->mouse_down_hittest_ = ViewInterface::HT_CLIENT;

    return result != EVENT_RESULT_UNHANDLED ||
           result2 != EVENT_RESULT_UNHANDLED;
  }

  static gboolean KeyPressHandler(GtkWidget *widget, GdkEventKey *event,
                                  gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    EventResult result = EVENT_RESULT_UNHANDLED;
    EventResult result2 = EVENT_RESULT_UNHANDLED;

    impl->host_->ShowTooltip("");

    int mod = ConvertGdkModifierToModifier(event->state);
    unsigned int key_code = ConvertGdkKeyvalToKeyCode(event->keyval);
    if (key_code) {
      KeyboardEvent e(Event::EVENT_KEY_DOWN, key_code, mod, event);
      result = impl->view_->OnKeyEvent(e);
    } else {
      LOG("Unknown key: 0x%x", event->keyval);
    }

    guint32 key_char = 0;
    if ((event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) == 0) {
      if (key_code == KeyboardEvent::KEY_ESCAPE ||
          key_code == KeyboardEvent::KEY_RETURN ||
          key_code == KeyboardEvent::KEY_BACK ||
          key_code == KeyboardEvent::KEY_TAB) {
        // gdk_keyval_to_unicode doesn't support the above keys.
        key_char = key_code;
      } else {
        key_char = gdk_keyval_to_unicode(event->keyval);
      }
    } else if ((event->state & GDK_CONTROL_MASK) &&
               key_code >= 'A' && key_code <= 'Z') {
      // Convert CTRL+(A to Z) to key press code for compatibility.
      key_char = key_code - 'A' + 1;
    }

    if (key_char) {
      // Send the char code in KEY_PRESS event.
      KeyboardEvent e2(Event::EVENT_KEY_PRESS, key_char, mod, event);
      result2 = impl->view_->OnKeyEvent(e2);
    }

    return result != EVENT_RESULT_UNHANDLED ||
           result2 != EVENT_RESULT_UNHANDLED;
  }

  static gboolean KeyReleaseHandler(GtkWidget *widget, GdkEventKey *event,
                                    gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    EventResult result = EVENT_RESULT_UNHANDLED;

    int mod = ConvertGdkModifierToModifier(event->state);
    unsigned int key_code = ConvertGdkKeyvalToKeyCode(event->keyval);
    if (key_code) {
      KeyboardEvent e(Event::EVENT_KEY_UP, key_code, mod, event);
      result = impl->view_->OnKeyEvent(e);
    } else {
      LOG("Unknown key: 0x%x", event->keyval);
    }

    return result != EVENT_RESULT_UNHANDLED;
  }

  static gboolean ExposeHandler(GtkWidget *widget, GdkEventExpose *event,
                                gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);

    if (!impl->self_draw_) {
      impl->AddGdkRegionToSystemClipRegion(event->region);
      GdkRegion *invalidate_region = impl->GetInvalidateRegion();

      // We can't update the region outside event->region, so update them in a
      // new self draw request.
      gdk_region_subtract(invalidate_region, event->region);
      if (!gdk_region_empty(invalidate_region)) {
        impl->AddGdkRegionToSystemClipRegion(invalidate_region);
        if (!impl->self_draw_timer_) {
          impl->self_draw_timer_ =
              g_idle_add_full(GDK_PRIORITY_REDRAW, Impl::SelfDrawHandler,
                              impl, NULL);
        }
      }
      gdk_region_destroy(invalidate_region);
    }

    gdk_window_begin_paint_region(widget->window, event->region);

    cairo_t *cr = gdk_cairo_create(widget->window);

    // If background is disabled, and if composited is enabled,  the window
    // needs clearing every times.
    if (impl->no_background_ && impl->composited_) {
      cairo_operator_t op = cairo_get_operator(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(cr);
      cairo_set_operator(cr, op);
    }

    // Let View draw on the gdk window directly.
    // It's ok, because the View has canvas cache internally.
    CairoCanvas *canvas = new CairoCanvas(cr,
                                          impl->zoom_,
                                          impl->view_->GetWidth(),
                                          impl->view_->GetHeight());
    ASSERT(canvas);

    impl->view_->Draw(canvas);

    canvas->Destroy();
    cairo_destroy(cr);

#if GTK_CHECK_VERSION(2,10,0)
    // We need set input shape mask if there is no background.
    if (impl->should_update_input_shape_mask_ && impl->input_shape_mask_) {
      cairo_t *mask_cr = gdk_cairo_create(impl->input_shape_mask_);
      gdk_cairo_region(mask_cr, event->region);
      cairo_clip(mask_cr);
      cairo_set_operator(mask_cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(mask_cr);
      cairo_set_operator(mask_cr, CAIRO_OPERATOR_SOURCE);
      gdk_cairo_set_source_pixmap(mask_cr, widget->window, 0, 0);
      cairo_paint(mask_cr);
      cairo_destroy(mask_cr);
      gdk_window_input_shape_combine_mask(widget->window,
                                          impl->input_shape_mask_, 0, 0);
      impl->last_mask_time_ = GetCurrentTime();
    }
#endif

    // Copy off-screen buffer to screen.
    gdk_window_end_paint(widget->window);

#ifdef _DEBUG
    ++impl->draw_count_;
    uint64_t current_time = GetCurrentTime();
    uint64_t duration = current_time - impl->last_fps_time_;
    if (duration >= kFPSCountDuration) {
      impl->last_fps_time_ = current_time;
      DLOG("FPS of View %s: %f", impl->view_->GetCaption().c_str(),
           static_cast<double>(impl->draw_count_ * 1000) /
           static_cast<double>(duration));
      impl->draw_count_ = 0;
    }
#endif

    return TRUE;
  }

  static gboolean MotionNotifyHandler(GtkWidget *widget, GdkEventMotion *event,
                                      gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    int button = ConvertGdkModifierToButton(event->state);
    int mod = ConvertGdkModifierToModifier(event->state);
    MouseEvent e(Event::EVENT_MOUSE_MOVE,
                 event->x / impl->zoom_, event->y / impl->zoom_,
                 0, 0, button, mod, event);
#ifdef GRAB_POINTER_EXPLICITLY
    if (button != MouseEvent::BUTTON_NONE && !gdk_pointer_is_grabbed() &&
        !impl->pointer_grabbed_) {
      // Grab the cursor to prevent losing events.
      if (gdk_pointer_grab(widget->window, FALSE,
                           (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                          GDK_BUTTON_MOTION_MASK |
                                          GDK_POINTER_MOTION_MASK |
                                          GDK_POINTER_MOTION_HINT_MASK),
                           NULL, NULL, event->time) == GDK_GRAB_SUCCESS) {
        impl->pointer_grabbed_ = true;
      }
    }
#endif

    EventResult result = impl->view_->OnMouseEvent(e);

    if (result == EVENT_RESULT_UNHANDLED && button != MouseEvent::BUTTON_NONE &&
        impl->mouse_down_x_ >= 0 && impl->mouse_down_y_ >= 0 &&
        (std::abs(event->x_root - impl->mouse_down_x_) > kDragThreshold ||
         std::abs(event->y_root - impl->mouse_down_y_) > kDragThreshold ||
         impl->mouse_down_hittest_ != ViewInterface::HT_CLIENT)) {
      impl->button_pressed_ = false;
      // Send fake mouse up event to the view so that we can start to drag
      // the window. Note: no mouse click event is sent in this case, to prevent
      // unwanted action after window move.
      MouseEvent e(Event::EVENT_MOUSE_UP,
                   event->x / impl->zoom_, event->y / impl->zoom_,
                   0, 0, button, mod);

      // Ignore the result of this fake event.
      impl->view_->OnMouseEvent(e);

      ViewInterface::HitTest hittest = impl->mouse_down_hittest_;
      bool resize_drag = false;
      // Determine the resize drag edge.
      if (hittest == ViewInterface::HT_LEFT ||
          hittest == ViewInterface::HT_RIGHT ||
          hittest == ViewInterface::HT_TOP ||
          hittest == ViewInterface::HT_BOTTOM ||
          hittest == ViewInterface::HT_TOPLEFT ||
          hittest == ViewInterface::HT_TOPRIGHT ||
          hittest == ViewInterface::HT_BOTTOMLEFT ||
          hittest == ViewInterface::HT_BOTTOMRIGHT) {
        resize_drag = true;
      }

#ifdef GRAB_POINTER_EXPLICITLY
      // ungrab the pointer before starting move/resize drag.
      if (impl->pointer_grabbed_) {
        gdk_pointer_ungrab(gtk_get_current_event_time());
        impl->pointer_grabbed_ = false;
      }
#endif

      if (resize_drag) {
        impl->host_->BeginResizeDrag(button, hittest);
      } else {
        impl->host_->BeginMoveDrag(button);
      }

      impl->mouse_down_x_ = -1;
      impl->mouse_down_y_ = -1;
      impl->mouse_down_hittest_ = ViewInterface::HT_CLIENT;
    }

    // Since motion hint is enabled, we must notify GTK that we're ready to
    // receive the next motion event.
#if GTK_CHECK_VERSION(2,12,0)
    gdk_event_request_motions(event); // requires version 2.12
#else
    int x, y;
    gdk_window_get_pointer(widget->window, &x, &y, NULL);
#endif

    return result != EVENT_RESULT_UNHANDLED;
  }

  static gboolean ScrollHandler(GtkWidget *widget, GdkEventScroll *event,
                                gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    int delta_x = 0, delta_y = 0;
    if (event->direction == GDK_SCROLL_UP) {
      delta_y = MouseEvent::kWheelDelta;
    } else if (event->direction == GDK_SCROLL_DOWN) {
      delta_y = -MouseEvent::kWheelDelta;
    } else if (event->direction == GDK_SCROLL_LEFT) {
      delta_x = MouseEvent::kWheelDelta;
    } else if (event->direction == GDK_SCROLL_RIGHT) {
      delta_x = -MouseEvent::kWheelDelta;
    }

    MouseEvent e(Event::EVENT_MOUSE_WHEEL,
                 event->x / impl->zoom_, event->y / impl->zoom_,
                 delta_x, delta_y,
                 ConvertGdkModifierToButton(event->state),
                 ConvertGdkModifierToModifier(event->state));
    return impl->view_->OnMouseEvent(e) != EVENT_RESULT_UNHANDLED;
  }

  static gboolean LeaveNotifyHandler(GtkWidget *widget, GdkEventCrossing *event,
                                     gpointer user_data) {
    GGL_UNUSED(widget);
    if (event->mode != GDK_CROSSING_NORMAL ||
        event->detail == GDK_NOTIFY_INFERIOR)
      return FALSE;

    Impl *impl = reinterpret_cast<Impl *>(user_data);

    // Don't send mouse out event if the mouse is grabbed.
    if (impl->button_pressed_)
      return FALSE;

    impl->host_->ShowTooltip("");

    MouseEvent e(Event::EVENT_MOUSE_OUT,
                 event->x / impl->zoom_, event->y / impl->zoom_, 0, 0,
                 MouseEvent::BUTTON_NONE,
                 ConvertGdkModifierToModifier(event->state));
    return impl->view_->OnMouseEvent(e) != EVENT_RESULT_UNHANDLED;
  }

  static gboolean EnterNotifyHandler(GtkWidget *widget, GdkEventCrossing *event,
                                     gpointer user_data) {
    GGL_UNUSED(widget);
    if (event->mode != GDK_CROSSING_NORMAL ||
        event->detail == GDK_NOTIFY_INFERIOR)
      return FALSE;

    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->host_->ShowTooltip("");

    MouseEvent e(Event::EVENT_MOUSE_OVER,
                 event->x / impl->zoom_, event->y / impl->zoom_, 0, 0,
                 MouseEvent::BUTTON_NONE,
                 ConvertGdkModifierToModifier(event->state));
    return impl->view_->OnMouseEvent(e) != EVENT_RESULT_UNHANDLED;
  }

  static gboolean FocusInHandler(GtkWidget *widget, GdkEventFocus *event,
                                 gpointer user_data) {
    GGL_UNUSED(event);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    DLOG("FocusInHandler: widget: %p, view: %p, focused: %d, child: %p",
         widget, impl->view_, impl->focused_,
         gtk_window_get_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget))));
    if (!impl->focused_) {
      impl->focused_ = true;
      SimpleEvent e(Event::EVENT_FOCUS_IN);
      return impl->view_->OnOtherEvent(e) != EVENT_RESULT_UNHANDLED;
    }
    return FALSE;
  }

  static gboolean FocusOutHandler(GtkWidget *widget, GdkEventFocus *event,
                                  gpointer user_data) {
    GGL_UNUSED(event);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    DLOG("FocusOutHandler: widget: %p, view: %p, focused: %d, child: %p",
         widget, impl->view_, impl->focused_,
         gtk_window_get_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget))));
    if (impl->focused_) {
      impl->focused_ = false;
      SimpleEvent e(Event::EVENT_FOCUS_OUT);
#ifdef GRAB_POINTER_EXPLICITLY
      // Ungrab the pointer if the focus is lost.
      if (impl->pointer_grabbed_) {
        gdk_pointer_ungrab(gtk_get_current_event_time());
        impl->pointer_grabbed_ = false;
      }
#endif
      return impl->view_->OnOtherEvent(e) != EVENT_RESULT_UNHANDLED;
    }
    return FALSE;
  }

  static gboolean DragMotionHandler(GtkWidget *widget, GdkDragContext *context,
                                    gint x, gint y, guint time,
                                    gpointer user_data) {
    return OnDragEvent(widget, context, x, y, time,
                       Event::EVENT_DRAG_MOTION, user_data);
  }

  static void DragLeaveHandler(GtkWidget *widget, GdkDragContext *context,
                               guint time, gpointer user_data) {
    OnDragEvent(widget, context, 0, 0, time, Event::EVENT_DRAG_OUT, user_data);
  }

  static gboolean DragDropHandler(GtkWidget *widget, GdkDragContext *context,
                                  gint x, gint y, guint time,
                                  gpointer user_data) {
    return OnDragEvent(widget, context, x, y, time,
                       Event::EVENT_DRAG_DROP, user_data);
  }

#ifdef GRAB_POINTER_EXPLICITLY
  static gboolean GrabBrokenHandler(GtkWidget *widget, GdkEvent *event,
                                    gpointer user_data) {
    GGL_UNUSED(event);
    GGL_UNUSED(widget);
    GGL_UNUSED(user_data);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->pointer_grabbed_ = false;
    return FALSE;
  }
#endif

  static void DragDataReceivedHandler(GtkWidget *widget,
                                      GdkDragContext *context,
                                      gint x, gint y,
                                      GtkSelectionData *data,
                                      guint info, guint time,
                                      gpointer user_data) {
    GGL_UNUSED(widget);
    GGL_UNUSED(x);
    GGL_UNUSED(y);
    GGL_UNUSED(info);

    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (!impl->current_drag_event_) {
      // There are some cases that multiple drag events are fired in one event
      // loop. For example, drag_leave and drag_drop. Although current_drag_event
      // only contains the latter one, there are still multiple
      // drag_data_received events fired.
      return;
    }

    std::string drag_text;
    std::vector<std::string> uri_strings;
    gchar **uris = gtk_selection_data_get_uris(data);
    if (uris) {
      // The data is an uri-list.
      for (gchar **p = uris; *p; ++p) {
        if (**p) {
          if (drag_text.length())
            drag_text.append("\n");
          drag_text.append(*p);
          uri_strings.push_back(*p);
        }
      }
      g_strfreev(uris);
    } else {
      // Otherwise, try to get plain text from data.
      gchar *text = reinterpret_cast<gchar*>(gtk_selection_data_get_text(data));
      if (text && *text) {
        drag_text.append(text);
        gchar *prev = text;
        gchar *p = text;
        // Treates \n  or \r as separator of the url list.
        while(1) {
          if (*p == '\n' || *p == '\r' || *p == 0) {
            if (p > prev) {
              std::string str = TrimString(std::string(prev, p));
              if (str.length())
                uri_strings.push_back(str);
            }
            prev = p + 1;
          }
          if (*p == 0) break;
          ++p;
        }
      }
      g_free(text);
    }

    if (drag_text.length() == 0) {
      DLOG("No acceptable URI or text in drag data");
      gdk_drag_status(context, static_cast<GdkDragAction>(0), time);
      return;
    }

    const char **drag_files = NULL;
    const char **drag_urls = NULL;
    if (uri_strings.size()) {
      drag_files = g_new0(const char *, uri_strings.size() + 1);
      drag_urls = g_new0(const char *, uri_strings.size() + 1);
      size_t files_count = 0;
      size_t urls_count = 0;
      for (std::vector<std::string>::iterator it = uri_strings.begin();
           it != uri_strings.end(); ++it) {
        if (IsValidFileURL(it->c_str())) {
          gchar *hostname;
          gchar *filename = g_filename_from_uri(it->c_str(), &hostname, NULL);
          if (filename && !hostname) {
            *it = std::string(filename);
            drag_files[files_count++] = it->c_str();;
          }
          g_free(filename);
          g_free(hostname);
        } else if (IsValidURL(it->c_str())) {
          drag_urls[urls_count++] = it->c_str();
        }
      }
    }

    Event::Type type = impl->current_drag_event_->GetType();
    impl->current_drag_event_->SetDragFiles(drag_files);
    impl->current_drag_event_->SetDragUrls(drag_urls);
    impl->current_drag_event_->SetDragText(drag_text.c_str());
    EventResult result = impl->view_->OnDragEvent(*impl->current_drag_event_);
    if (result == EVENT_RESULT_HANDLED && type == Event::EVENT_DRAG_MOTION) {
      gdk_drag_status(context, GDK_ACTION_COPY, time);
    } else {
      gdk_drag_status(context, static_cast<GdkDragAction>(0), time);
    }
#ifdef _DEBUG
    const char *type_name = NULL;
    switch (type) {
      case Event::EVENT_DRAG_MOTION: type_name = "motion"; break;
      case Event::EVENT_DRAG_DROP: type_name = "drop"; break;
      case Event::EVENT_DRAG_OUT: type_name = "out"; break;
      default: type_name = "unknown"; break;
    }
    DLOG("Drag %s event was %s: x:%d, y:%d, time:%d, text:\n%s\n", type_name,
         (result == EVENT_RESULT_HANDLED ? "handled" : "not handled"),
         x, y, time, drag_text.c_str());
#endif
    if (type == Event::EVENT_DRAG_DROP) {
     DLOG("Drag operation finished.");
      gtk_drag_finish(context, result == EVENT_RESULT_HANDLED, FALSE, time);
    }

    delete impl->current_drag_event_;
    impl->current_drag_event_ = NULL;
    g_free(drag_files);
    g_free(drag_urls);
  }

  static void ScreenChangedHandler(GtkWidget *widget, GdkScreen *last_screen,
                                   gpointer user_data) {
    GGL_UNUSED(widget);
    GGL_UNUSED(last_screen);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->SetupBackgroundMode();
  }

  static void CompositedChangedHandler(GtkWidget *widget, gpointer user_data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->SetupBackgroundMode();
  }

  static gboolean OnDragEvent(GtkWidget *widget, GdkDragContext *context,
                              gint x, gint y, guint time,
                              Event::Type event_type, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->current_drag_event_) {
      // There are some cases that multiple drag events are fired in one event
      // loop. For example, drag_leave and drag_drop, we use the latter one.
      delete impl->current_drag_event_;
      impl->current_drag_event_ = NULL;
    }

    impl->current_drag_event_ = new DragEvent(event_type, x, y);
    GdkAtom target = gtk_drag_dest_find_target(
        widget, context, gtk_drag_dest_get_target_list(widget));
    if (target != GDK_NONE) {
      gtk_drag_get_data(widget, context, target, time);
      return TRUE;
    }
    DLOG("Drag target or action not acceptable");
    gdk_drag_status(context, static_cast<GdkDragAction>(0), time);
    return FALSE;
  }

  static gboolean SelfDrawHandler(gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->self_draw_timer_ = 0;
    impl->SelfDraw();
    return FALSE;
  }

  ViewInterface *view_;
  ViewHostInterface *host_;
  GtkWidget *widget_;
#if GTK_CHECK_VERSION(2,10,0)
  GdkBitmap *input_shape_mask_;
  uint64_t last_mask_time_;
  bool should_update_input_shape_mask_;
  bool enable_input_shape_mask_;
#endif
  gulong *handlers_;
  DragEvent *current_drag_event_;
  Connection *on_zoom_connection_;
  bool dbl_click_;
  bool composited_;
  bool no_background_;
  bool focused_;
  bool button_pressed_;
#ifdef GRAB_POINTER_EXPLICITLY
  bool pointer_grabbed_;
#endif
#ifdef _DEBUG
  int draw_count_;
  uint64_t last_fps_time_;
#endif
  double zoom_;
  double mouse_down_x_;
  double mouse_down_y_;
  ViewInterface::HitTest mouse_down_hittest_;

  bool self_draw_;
  guint self_draw_timer_;
  uint64_t last_self_draw_time_;
  GdkRegion *sys_clip_region_;

  struct EventHandlerInfo {
    const char *event;
    void (*handler)(void);
  };

  static EventHandlerInfo kEventHandlers[];
  static const size_t kEventHandlersNum;
};

ViewWidgetBinder::Impl::EventHandlerInfo
ViewWidgetBinder::Impl::kEventHandlers[] = {
  { "button-press-event", G_CALLBACK(ButtonPressHandler) },
  { "button-release-event", G_CALLBACK(ButtonReleaseHandler) },
#if GTK_CHECK_VERSION(2,10,0)
  { "composited-changed", G_CALLBACK(CompositedChangedHandler) },
#endif
  { "drag-data-received", G_CALLBACK(DragDataReceivedHandler) },
  { "drag-drop", G_CALLBACK(DragDropHandler) },
  { "drag-leave", G_CALLBACK(DragLeaveHandler) },
  { "drag-motion", G_CALLBACK(DragMotionHandler) },
  { "enter-notify-event", G_CALLBACK(EnterNotifyHandler) },
  { "expose-event", G_CALLBACK(ExposeHandler) },
  { "focus-in-event", G_CALLBACK(FocusInHandler) },
  { "focus-out-event", G_CALLBACK(FocusOutHandler) },
  { "key-press-event", G_CALLBACK(KeyPressHandler) },
  { "key-release-event", G_CALLBACK(KeyReleaseHandler) },
  { "leave-notify-event", G_CALLBACK(LeaveNotifyHandler) },
  { "motion-notify-event", G_CALLBACK(MotionNotifyHandler) },
  { "screen-changed", G_CALLBACK(ScreenChangedHandler) },
  { "scroll-event", G_CALLBACK(ScrollHandler) },
#ifdef GRAB_POINTER_EXPLICITLY
  { "grab-broken-event", G_CALLBACK(GrabBrokenHandler) },
#endif
};

const size_t ViewWidgetBinder::Impl::kEventHandlersNum =
  arraysize(ViewWidgetBinder::Impl::kEventHandlers);

ViewWidgetBinder::ViewWidgetBinder(ViewInterface *view,
                                   ViewHostInterface *host, GtkWidget *widget,
                                   bool no_background)
  : impl_(new Impl(view, host, widget, no_background)) {
}

void ViewWidgetBinder::EnableInputShapeMask(bool enable) {
#if GTK_CHECK_VERSION(2,10,0)
  if (impl_->enable_input_shape_mask_ != enable) {
    impl_->enable_input_shape_mask_ = enable;
    if (impl_->widget_ && impl_->no_background_ &&
        impl_->composited_ && !enable) {
      if (impl_->widget_->window) {
        gdk_window_input_shape_combine_mask(impl_->widget_->window, NULL, 0, 0);
      }
      if (impl_->input_shape_mask_) {
        g_object_unref(G_OBJECT(impl_->input_shape_mask_));
        impl_->input_shape_mask_ = NULL;
      }
    }
    gtk_widget_queue_draw(impl_->widget_);
  }
#endif
}

ViewWidgetBinder::~ViewWidgetBinder() {
  delete impl_;
  impl_ = NULL;
}

void ViewWidgetBinder::QueueDraw() {
  if (!impl_->self_draw_timer_) {
    uint64_t current_time = GetCurrentTime();
    if (current_time - impl_->last_self_draw_time_ >= kSelfDrawInterval) {
      impl_->self_draw_timer_ =
          g_idle_add_full(GDK_PRIORITY_REDRAW, Impl::SelfDrawHandler,
                          impl_, NULL);
    } else {
      impl_->self_draw_timer_ =
          g_timeout_add(kSelfDrawInterval -
                        (current_time - impl_->last_self_draw_time_),
                        Impl::SelfDrawHandler, impl_);
    }
  }
}

void ViewWidgetBinder::DrawImmediately() {
  // Remove pending queue draw, as we don't need it anymore.
  if (impl_->self_draw_timer_) {
    g_source_remove(impl_->self_draw_timer_);
    impl_->self_draw_timer_ = 0;
  }
  impl_->SelfDraw();
}

bool ViewWidgetBinder::DrawQueued() {
  return impl_->self_draw_timer_ != 0;
}

} // namespace gtk
} // namespace ggadget
