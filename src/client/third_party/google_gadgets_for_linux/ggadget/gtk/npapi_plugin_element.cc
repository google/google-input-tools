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

#include "npapi_plugin_element.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <ggadget/view.h>
#include <ggadget/element_factory.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/graphics_interface.h>
#include <ggadget/gtk/cairo_canvas.h>
#include <ggadget/npapi/npapi_plugin.h>
#include <ggadget/npapi/npapi_wrapper.h>
#include <ggadget/math_utils.h>
#include <ggadget/small_object.h>

namespace ggadget {
namespace gtk {

class NPAPIPluginElement::Impl : public SmallObject<> {
 public:
  Impl(NPAPIPluginElement *owner, const char *mime_type,
       const StringMap &default_parameters, bool in_object_element)
      : owner_(owner),
        mime_type_(mime_type),
        in_object_element_(in_object_element),
        native_widget_(NULL),
        top_x_window_(None),
        plugin_(NULL),
        scriptable_plugin_(NULL),
        plugin_failed_(false),
        parameters_(default_parameters),
        windowless_(false),
        pixmap_(NULL),
        drawable_(None),
        focused_(false),
        zoom_(1.0),
        socket_(NULL),
        x_(0), y_(0), width_(0), height_(0),
        minimized_(false), popped_out_(false),
        minimized_connection_(owner_->GetView()->ConnectOnMinimizeEvent(
            NewSlot(this, &Impl::OnViewMinimized))),
        restored_connection_(owner_->GetView()->ConnectOnRestoreEvent(
            NewSlot(this, &Impl::OnViewRestored))),
        popout_connection_(owner_->GetView()->ConnectOnPopOutEvent(
            NewSlot(this, &Impl::OnViewPoppedOut))),
        popin_connection_(owner->GetView()->ConnectOnPopInEvent(
            NewSlot(this, &Impl::OnViewPoppedIn))),
        dock_connection_(owner->GetView()->ConnectOnDockEvent(
            NewSlot(this, &Impl::OnViewDockUndock))),
        undock_connection_(owner->GetView()->ConnectOnUndockEvent(
            NewSlot(this, &Impl::OnViewDockUndock))) {
    scriptable_parameters_.SetDynamicPropertyHandler(
        NewSlot(this, &Impl::GetParameter), NewSlot(this, &Impl::SetParameter));
  }

  ~Impl() {
    minimized_connection_->Disconnect();
    restored_connection_->Disconnect();
    popout_connection_->Disconnect();
    popin_connection_->Disconnect();
    dock_connection_->Disconnect();
    undock_connection_->Disconnect();
    if (plugin_)
      plugin_->Destroy();
    if (pixmap_)
      g_object_unref(pixmap_);
    if (GTK_IS_WIDGET(socket_))
      gtk_widget_destroy(socket_);
  }

  bool EnsurePlugin() {
    if (plugin_)
      return true;
    if (plugin_failed_)
      return false;
    CreateSocket();
    UpdateWindow();
    plugin_ = npapi::Plugin::Create(mime_type_.c_str(), owner_,
                                    reinterpret_cast<void *>(top_x_window_),
                                    window_, parameters_);
    // Get the root scriptable object of the plugin.
    if (plugin_) {
      plugin_->SetSrc(src_.c_str());
      scriptable_plugin_ = plugin_->GetScriptablePlugin();
      return true;
    }
    gtk_widget_destroy(socket_);
    socket_ = NULL;
    plugin_failed_ = true;
    return false;
  }

  void UpdateWindowCoords(int width, int height) {
    window_.x = 0;
    window_.y = 0;
    window_.width = static_cast<uint32>(width);
    window_.height = static_cast<uint32>(height);
    window_.clipRect.left = 0;
    window_.clipRect.top = 0;
    window_.clipRect.right = static_cast<uint16>(window_.width);
    window_.clipRect.bottom = static_cast<uint16>(window_.height);
  }

  void ClearPixmap(int x, int y, int w, int h) {
    // For now because the plugin only accepts 24-bit pixmap, we can only
    // clear the pixmap by drawing it in white.
    GdkGC *gc = gdk_gc_new(pixmap_);
    GdkColor white = { 0, 65535, 65535, 65535 };
    gdk_gc_set_rgb_fg_color(gc, &white);
    gdk_draw_rectangle(pixmap_, gc, TRUE, x, y, w, h);
    g_object_unref(gc);
  }

  void UpdateWindow() {
    GdkDrawable *gdk_drawable;
    native_widget_ = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    GdkWindow *gdk_window = gdk_window_get_toplevel(native_widget_->window);
    top_x_window_ = GDK_WINDOW_XID(gdk_window);
    if (windowless_) {
      zoom_ = owner_->GetView()->GetGraphics()->GetZoom();
      window_.window = NULL;
      window_.type = NPWindowTypeDrawable;
      if (pixmap_)
        g_object_unref(pixmap_);
      int width = static_cast<int>(ceil(owner_->GetPixelWidth() * zoom_));
      int height = static_cast<int>(ceil(owner_->GetPixelHeight() * zoom_));
      UpdateWindowCoords(width, height);

      // TODO: If I create the pixmap from native_widget_, the flash 10 plugin
      // will raise XError BadMatch because the pixmap is not compatibile with
      // some Drawable created by the plugin. I don't know how can I create the
      // pixmap, but at least for now depth=24 works.
      pixmap_ = gdk_pixmap_new(NULL, width, height, 24);
      // Must set a colormap for the pixmap otherwise the plugin will raise
      // XError BadColor.
      gdk_drawable_set_colormap(pixmap_, gdk_rgb_get_colormap());
      drawable_ = GDK_PIXMAP_XID(pixmap_);
      gdk_drawable = pixmap_;
      ClearPixmap(0, 0, width, height);
    } else {
      if (gtk_widget_get_parent(socket_) != native_widget_)
        gtk_widget_reparent(socket_, native_widget_);
      window_.window =
          reinterpret_cast<void *>(gtk_socket_get_id(GTK_SOCKET(socket_)));
      window_.type = NPWindowTypeWindow;
      UpdateSocketPosSize(true);
      gdk_drawable = socket_->window;
    }
    window_.ws_info = &ws_info_;
    ws_info_.type = NP_SETWINDOW;
    ws_info_.display =
        GDK_DISPLAY_XDISPLAY(gdk_drawable_get_display(gdk_drawable));
    ws_info_.visual = GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(gdk_drawable));
    ws_info_.colormap =
        GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdk_drawable));
    ws_info_.depth = gdk_drawable_get_depth(gdk_drawable);

    if (plugin_)
      plugin_->SetWindow(reinterpret_cast<void *>(top_x_window_), window_);
  }

  static gboolean OnPlugRemoved(GtkWidget *widget, gpointer data) {
    GGL_UNUSED(widget);
    GGL_UNUSED(data);
    return TRUE;
  }

  // For windowed plugins.
  void CreateSocket() {
    ASSERT(!socket_);
    ASSERT(!windowless_);
    native_widget_ = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (!GTK_IS_FIXED(native_widget_)) {
      LOG("GtkWindowedFlashElement needs a GTK_FIXED parent. Actual type: %s",
          G_OBJECT_TYPE_NAME(native_widget_));
      return;
    }
    socket_ = gtk_socket_new();
    // Plugins only support RGB surfaces. Passing a RGBA surface to it will
    // cause BadMatch X error.
    gtk_widget_set_colormap(socket_, gdk_rgb_get_colormap());
    // Make sure to handle the plug_removed signal, so that the socket can be
    // reused after the plug is removed.
    g_signal_connect(socket_, "plug_removed",
                     G_CALLBACK(OnPlugRemoved), NULL);
    g_signal_connect(socket_, "destroy",
                     GTK_SIGNAL_FUNC(gtk_widget_destroyed), &socket_);

    gtk_fixed_put(GTK_FIXED(native_widget_), socket_, x_, y_);
    gtk_widget_set_size_request(socket_, width_, height_);
    gtk_widget_realize(socket_);
    gtk_widget_show(socket_);
  }

  // For windowed plugins. Get the position of the socket relative to the
  // parent native widget.
  void GetWidgetExtents(gint *x, gint *y, gint *width, gint *height) {
    ASSERT(!windowless_);
    double widget_x0, widget_y0;
    double widget_x1, widget_y1;
    owner_->SelfCoordToViewCoord(0, 0, &widget_x0, &widget_y0);
    owner_->SelfCoordToViewCoord(owner_->GetPixelWidth(),
                                 owner_->GetPixelHeight(),
                                 &widget_x1, &widget_y1);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x0, widget_y0,
                                                    &widget_x0, &widget_y0);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x1, widget_y1,
                                                    &widget_x1, &widget_y1);
    *x = static_cast<gint>(round(widget_x0));
    *y = static_cast<gint>(round(widget_y0));
    *width = static_cast<gint>(ceil(widget_x1 - widget_x0));
    *height = static_cast<gint>(ceil(widget_y1 - widget_y0));
  }

  // For windowed plugins. Returns true if the window_ structure is changed.
  bool UpdateSocketPosSize(bool force_layout) {
    ASSERT(!windowless_);
    if (!GTK_IS_SOCKET(socket_))
      return false;

    gint x, y, width, height;
    GetWidgetExtents(&x, &y, &width, &height);
    if (x != x_ || y != y_ || force_layout) {
      x_ = x;
      y_ = y;
      gtk_fixed_move(GTK_FIXED(native_widget_), socket_, x, y);
    }
    if (width != width_ || height != height_ || force_layout) {
      width_ = width;
      height_ = height;
      gtk_widget_set_size_request(socket_, width_, height_);

      window_.window =
          reinterpret_cast<void *>(gtk_socket_get_id(GTK_SOCKET(socket_)));
      window_.type = NPWindowTypeWindow;
      UpdateWindowCoords(width_, height_);
      return true;
    }
    return false;
  }

  void Layout() {
    if (!EnsurePlugin())
      return;

    bool new_windowless = plugin_->IsWindowless();
    if (new_windowless != windowless_) {
      windowless_ = new_windowless;
      if (new_windowless) {
        gtk_widget_destroy(socket_);
        socket_ = NULL;
      } else {
        CreateSocket();
      }
      UpdateWindow();
    }

    if (native_widget_ != GTK_WIDGET(owner_->GetView()->GetNativeWidget())) {
      UpdateWindow();
    } else if (windowless_) {
      if (owner_->IsSizeChanged() ||
          owner_->GetView()->GetGraphics()->GetZoom() != zoom_) {
        UpdateWindow();
      }
    } else {
      if (UpdateSocketPosSize(false))
        plugin_->SetWindow(reinterpret_cast<void *>(top_x_window_), window_);
      if (owner_->IsReallyVisible() && (!minimized_ || popped_out_))
        gtk_widget_show(socket_);
      else
        gtk_widget_hide(socket_);
    }
  }

  void DoDraw(CanvasInterface *canvas) {
    if (windowless_ && plugin_) {
      Rectangle rect = plugin_->GetDirtyRect();
      if (rect == npapi::Plugin::kWholePluginRect) {
        rect.Set(0, 0, window_.width, window_.height);
      }
      int x = static_cast<int>(rect.x);
      int y = static_cast<int>(rect.y);
      int w = static_cast<int>(rect.w);
      int h = static_cast<int>(rect.h);
      ClearPixmap(x, y, w, h);
      XEvent expose_event;
      memset(&expose_event, 0, sizeof(expose_event));
      expose_event.type = GraphicsExpose;
      expose_event.xgraphicsexpose.display = ws_info_.display;
      ASSERT(GDK_IS_WINDOW(native_widget_->window));
      expose_event.xgraphicsexpose.drawable = drawable_;
      expose_event.xgraphicsexpose.x = x;
      expose_event.xgraphicsexpose.y = y;
      // In fact, this GraphicsExpose's width and height are the position of
      // the bottom-right corner.
      expose_event.xgraphicsexpose.width = x + w;
      expose_event.xgraphicsexpose.height = y + h;
      plugin_->HandleEvent(&expose_event);
      plugin_->ResetDirtyRect();
#if 0 // Debug dirty area.
      GdkGC *gc = gdk_gc_new(pixmap_);
      GdkColor red = { 0, 65535, 0, 0 };
      gdk_gc_set_rgb_fg_color(gc, &red);
      gdk_draw_rectangle(pixmap_, gc, FALSE, x, y, w - 1, h - 1);
      g_object_unref(gc);
#endif
      if (canvas) {
        cairo_t *cr = down_cast<CairoCanvas*>(canvas)->GetContext();
        if (zoom_ != 1.0) {
          cairo_save(cr);
          cairo_scale(cr, 1.0 / zoom_, 1.0 / zoom_);
        }
        gdk_cairo_set_source_pixmap(cr, pixmap_, 0, 0);
        cairo_paint_with_alpha(cr, owner_->GetOpacity());
        if (zoom_ != 1.0)
          cairo_restore(cr);
      }
    }
  }

  void OnViewMinimized() {
    // The widget must be hidden when the view is minimized.
    if (!windowless_ && GTK_IS_SOCKET(socket_) && !popped_out_)
      gtk_widget_hide(socket_);
    minimized_ = true;
  }

  void OnViewRestored() {
    if (!windowless_ && GTK_IS_SOCKET(socket_) && owner_->IsReallyVisible() &&
        !popped_out_)
      gtk_widget_show(socket_);
    minimized_ = false;
  }

  void OnViewPoppedOut() {
    popped_out_ = true;
    Layout();
  }

  void OnViewPoppedIn() {
    popped_out_ = false;
    Layout();
  }

  void OnViewDockUndock() {
    // The toplevel window might be changed, so it's necessary to reparent the
    // browser widget.
    Layout();
  }

  Variant GetParameter(const std::string &name) {
    // During initialization, allow getting any parameters as strings.
    return Variant(parameters_[name]);
  }

  bool SetParameter(const std::string &name, const Variant &value) {
    // During initialization, allow setting any parameters as strings.
    std::string str;
    value.ConvertToString(&str);
    parameters_[name] = str;
    return true;
  }

  ScriptableInterface *GetObject() {
    return scriptable_plugin_;
  }

  ScriptableInterface *GetParameters() {
    return &scriptable_parameters_;
  }

  // Registered as dynamic property getter when in_object_element_ is true.
  Variant GetProperty(const std::string &name) {
    if (scriptable_plugin_) {
      Variant value;
      ScriptableInterface *scriptable = NULL;
      { // Life scope of ResultVariant result.
        ResultVariant result = scriptable_plugin_->GetProperty(name.c_str());
        value = result.v();
        if (value.type() == Variant::TYPE_SCRIPTABLE) {
          scriptable = VariantValue<ScriptableInterface *>()(value);
          // Add a reference to prevent ResultVariant from deleting the object.
          if (scriptable)
            scriptable->Ref();
        }
      }
      // Release the temporary reference but don't delete the object.
      if (scriptable)
        scriptable->Unref(true);
      return value;
    }
    return GetParameter(name);
  }

  // Registered as dynamic property setter when in_object_element_ is true.
  bool SetProperty(const std::string &name, const Variant &value) {
    if (scriptable_plugin_)
      return scriptable_plugin_->SetProperty(name.c_str(), value);
    return SetParameter(name, value);
  }

  NPAPIPluginElement *owner_;
  View *view_;
  std::string mime_type_;
  bool in_object_element_;

  GtkWidget *native_widget_;
  Window top_x_window_;
  std::string src_;
  npapi::Plugin *plugin_;
  ScriptableInterface *scriptable_plugin_;
  bool plugin_failed_;
  NativeOwnedScriptable<UINT64_C(0x05b7bd1c27924245)> scriptable_parameters_;
  StringMap parameters_;
  bool windowless_;
  NPWindow window_;
  NPSetWindowCallbackStruct ws_info_;
  GdkPixmap *pixmap_;
  Drawable drawable_;
  bool auto_start_;
  bool initialized_;
  bool focused_;
  double zoom_;

  // For windowed plugins.
  GtkWidget *socket_;
  gint x_, y_, width_, height_;
  bool minimized_;
  bool popped_out_;
  Connection *minimized_connection_, *restored_connection_,
             *popout_connection_, *popin_connection_,
             *dock_connection_, *undock_connection_;
};

NPAPIPluginElement::NPAPIPluginElement(View *view, const char *name,
                                       const char *mime_type,
                                       const StringMap &default_parameters,
                                       bool in_object_element)
    : BasicElement(view, "plugin", name, false),
      impl_(new Impl(this, mime_type, default_parameters, in_object_element)) {
  SetEnabled(true);
  if (in_object_element) {
    SetRelativeX(0);
    SetRelativeY(0);
    SetRelativeWidth(1.0);
    SetRelativeHeight(1.0);
  }
}

NPAPIPluginElement::~NPAPIPluginElement() {
  delete impl_;
}

void NPAPIPluginElement::SetSrc(const char *src) {
  if (AssignIfDiffer(src, &impl_->src_) && impl_->plugin_)
    impl_->plugin_->SetSrc(impl_->src_.c_str());
  // Otherwise wait for the plugin to be initialized.
}

std::string NPAPIPluginElement::GetSrc() const {
  return impl_->src_;
}

void NPAPIPluginElement::DoClassRegister() {
  // As noted in doc of NPAPIPluginElement::NPPluginElement(),
  // in_object_element_ should not differ among objects of the same class,
  // otherwise the integrity of DoClassRegister will be broken.
  if (impl_->in_object_element_) {
    RegisterProperty("movie", NewSlot(&NPAPIPluginElement::GetSrc),
                     NewSlot(&NPAPIPluginElement::SetSrc));
  } else {
    BasicElement::DoClassRegister();
    RegisterProperty("object", NewSlot(&Impl::GetObject,
                                       &NPAPIPluginElement::impl_), NULL);
    RegisterProperty("param", NewSlot(&Impl::GetParameters,
                                      &NPAPIPluginElement::impl_), NULL);
    RegisterProperty("src", NewSlot(&NPAPIPluginElement::GetSrc),
                     NewSlot(&NPAPIPluginElement::SetSrc));
  }
}

void NPAPIPluginElement::DoRegister() {
  if (impl_->in_object_element_) {
    SetDynamicPropertyHandler(NewSlot(impl_, &Impl::GetProperty),
                              NewSlot(impl_, &Impl::SetProperty));
  } else {
    BasicElement::DoRegister();
  }
}

void NPAPIPluginElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void NPAPIPluginElement::DoDraw(CanvasInterface *canvas) {
  impl_->DoDraw(canvas);
}

EventResult NPAPIPluginElement::HandleMouseEvent(const MouseEvent &event) {
  if (!impl_->plugin_ || !impl_->windowless_)
    return EVENT_RESULT_UNHANDLED;
  XEvent x_event;
  memset(&x_event, 0, sizeof(x_event));
  Event::Type type = event.GetType();
  x_event.xany.display = impl_->ws_info_.display;
  int x = static_cast<int>(event.GetX() * impl_->zoom_);
  int y = static_cast<int>(event.GetY() * impl_->zoom_);
  if (type == Event::EVENT_MOUSE_OVER || type == Event::EVENT_MOUSE_OUT) {
    x_event.xcrossing.type =
        type == Event::EVENT_MOUSE_OVER ? EnterNotify : LeaveNotify;
    x_event.xcrossing.x = x;
    x_event.xcrossing.y = y;
    x_event.xcrossing.mode = NotifyNormal;
    x_event.xcrossing.detail = NotifyVirtual;
    x_event.xcrossing.focus = impl_->focused_;
    x_event.xcrossing.same_screen = True;
  } else if (type == Event::EVENT_MOUSE_MOVE) {
    GdkEventMotion *motion =
        reinterpret_cast<GdkEventMotion *>(event.GetOriginalEvent());
    if (!motion) {
      // This event is a test event sent by view on mouse over.
      return EVENT_RESULT_HANDLED;
    }
    x_event.xmotion.type = MotionNotify;
    x_event.xmotion.time = motion->time;
    x_event.xmotion.state = static_cast<unsigned int>(motion->state);
    x_event.xmotion.x = x;
    x_event.xmotion.y = y;
    x_event.xmotion.x_root = static_cast<int>(round(motion->x_root));
    x_event.xmotion.y_root = static_cast<int>(round(motion->y_root));
    x_event.xmotion.is_hint = static_cast<char>(motion->is_hint ?
                                                NotifyHint : NotifyNormal);
    x_event.xmotion.same_screen = True;
  } else {
    GdkEventButton *button =
        reinterpret_cast<GdkEventButton *>(event.GetOriginalEvent());
    if (!button) {
      // This event is a synthesized event, such as EVENT_CLICK or EVENT_RCLICK.
      // Return HANDLED to disable default actions.
      return EVENT_RESULT_HANDLED;
    }
    // These two types events are added by gdk. For a double-click, the order
    // of gdk events will be: GDK_BUTTON_PRESS, GDK_BUTTON_RELEASE,
    // GDK_BUTTON_PRESS, GDK_2BUTTON_PRESS, GDK_BUTTON_RELEASE. But, we should
    // pass the raw X button events to the plugin, i.e. press->release->
    // press->release. So simply discards those button types added by gdk, and
    // let the plugin to deal with double-clicks or triple-clicks itself,
    // returns HANDLED here.
    if (button->type == GDK_2BUTTON_PRESS ||
        button->type == GDK_3BUTTON_PRESS) {
      return EVENT_RESULT_HANDLED;
    }
    // Translate GdkEvent to the original XEvent.
    x_event.xbutton.root = GDK_ROOT_WINDOW();
    x_event.xbutton.type =
        button->type == GDK_BUTTON_PRESS ? ButtonPress : ButtonRelease;
    x_event.xbutton.time = button->time;
    x_event.xbutton.state = static_cast<unsigned int>(button->state);
    x_event.xbutton.button = button->button;
    // Use the coordinates in MouseEvent structure, but not those in GdkEvent.
    x_event.xbutton.x = x;
    x_event.xbutton.y = y;
    x_event.xbutton.x_root = static_cast<int>(round(button->x_root));
    x_event.xbutton.y_root = static_cast<int>(round(button->y_root));
    x_event.xbutton.same_screen = True;

    // It's weird that 64-bit Flash 10 plugin will ignore the mouse press event
    // following a mouse motion event with the same (x,y) position. Here send
    // a fake mouse motion event with position slightly shifted to avoid this
    // issue.
    XEvent x_event1;
    memset(&x_event1, 0, sizeof(x_event1));
    x_event1.xany.display = impl_->ws_info_.display;
    x_event1.xmotion.type = MotionNotify;
    x_event1.xmotion.time = x_event.xbutton.time;
    x_event1.xmotion.state = x_event.xbutton.state;
    x_event1.xmotion.x = x + 1;
    x_event1.xmotion.y = y;
    x_event1.xmotion.x_root = x_event.xbutton.x_root;
    x_event1.xmotion.y_root = x_event.xbutton.y_root;
    x_event1.xmotion.is_hint = NotifyNormal;
    x_event1.xmotion.same_screen = True;
    impl_->plugin_->HandleEvent(&x_event1);
  }
  // Some plugins always return 0 for mouse/keyboard events even if they
  // have handled them. Here returns HANDLED to prevent the container from
  // doing extra things.
  impl_->plugin_->HandleEvent(&x_event);
  return EVENT_RESULT_HANDLED;
}

EventResult NPAPIPluginElement::HandleKeyEvent(const KeyboardEvent &event) {
  if (!impl_->plugin_ || !impl_->windowless_ ||
      // EVENT_KEY_PRESS is the char generated by EVENT_KEY_DOWN.
      event.GetType() == Event::EVENT_KEY_PRESS)
    return EVENT_RESULT_UNHANDLED;

  GdkEventKey *key = reinterpret_cast<GdkEventKey *>(event.GetOriginalEvent());
  if (!key)
    return EVENT_RESULT_UNHANDLED;
  // Translate GdkEvent to the original XEvent.
  XEvent x_event;
  memset(&x_event, 0, sizeof(x_event));
  x_event.xkey.type = key->type == GDK_KEY_PRESS ? KeyPress : KeyRelease;
  x_event.xkey.display = impl_->ws_info_.display;
  x_event.xkey.time = key->time;
  x_event.xkey.state = static_cast<unsigned int>(key->state);
  x_event.xkey.keycode = key->hardware_keycode;
  // Some plugins always return 0 for mouse/keyboard events even if they
  // have handled them. Here returns HANDLED to prevent the container from
  // doing extra things.
  impl_->plugin_->HandleEvent(&x_event);
  return EVENT_RESULT_HANDLED;
}

EventResult NPAPIPluginElement::HandleOtherEvent(const Event &event) {
  if (!impl_->plugin_ || !impl_->windowless_)
    return EVENT_RESULT_UNHANDLED;
  Event::Type type = event.GetType();
  XEvent x_event;
  memset(&x_event, 0, sizeof(x_event));
  x_event.xany.display = impl_->ws_info_.display;
  if (type == Event::EVENT_FOCUS_IN || type == Event::EVENT_FOCUS_OUT) {
    x_event.xfocus.type = type == Event::EVENT_FOCUS_IN ? FocusIn : FocusOut;
    x_event.xfocus.mode = NotifyNormal;
    x_event.xfocus.detail = NotifyDetailNone;
    impl_->focused_ = type == Event::EVENT_FOCUS_IN ? true : false;
  } else {
    return EVENT_RESULT_UNHANDLED;
  }
  return impl_->plugin_->HandleEvent(&x_event) ?
         EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
}

} // namespace gtk
} // namespace ggadget
