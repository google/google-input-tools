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

#include "sidebar_gtk_host.h"

#include <gtk/gtk.h>
#include <algorithm>
#include <string>
#include <map>
#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/docked_main_view_decorator.h>
#include <ggadget/details_view_decorator.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/floating_main_view_decorator.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/hotkey.h>
#include <ggadget/gtk/menu_builder.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/host_utils.h>
#include <ggadget/locales.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/messages.h>
#include <ggadget/menu_interface.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/popout_main_view_decorator.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/sidebar.h>
#include <ggadget/slot.h>
#include <ggadget/view.h>
#include <ggadget/view_element.h>

#include "gadget_browser_host.h"

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

static const char kOptionAutoHide[]       = "auto_hide";
static const char kOptionAlwaysOnTop[]    = "always_on_top";
static const char kOptionSideBarClosed[]  = "sidebar_closed";
static const char kOptionPosition[]       = "position";
static const char kOptionFontSize[]       = "font_size";
static const char kOptionWidth[]          = "width";
static const char kOptionMonitor[]        = "monitor";
static const char kOptionHotKey[]         = "hotkey";
static const char kOptionGadgetsShown[]   = "gadgets_shown";

static const char kOptionDisplayTarget[]  = "display_target";
static const char kOptionPositionInSideBar[] = "position_in_sidebar";

static const int kAutoHideTimeout         = 200;
static const int kAutoShowTimeout         = 200;
static const int kDefaultSideBarWidth     = 200;
static const int kDefaultMonitor          = 0;

static const int kMinFontSize             = 4;
static const int kMaxFontSize             = 16;

enum SideBarPosition {
  SIDEBAR_POSITION_LEFT,
  SIDEBAR_POSITION_RIGHT,
};

class SideBarGtkHost::Impl {
 public:
  struct GadgetInfo {
    GadgetInfo()
      : gadget(NULL), main_decorator(NULL), details(NULL), floating(NULL),
        popout(NULL), index_in_sidebar(0), undock_by_drag(false),
        old_keep_above(false), details_on_right(false), debug_console(NULL) {
    }

    Gadget *gadget;

    DecoratedViewHost *main_decorator;

    SingleViewHost *details;
    SingleViewHost *floating;
    SingleViewHost *popout;

    size_t index_in_sidebar;
    bool undock_by_drag;
    bool old_keep_above;
    bool details_on_right;

    GtkWidget *debug_console;
  };

  Impl(SideBarGtkHost *owner, const char *options,
       int flags, int view_debug_mode,
       Gadget::DebugConsoleConfig debug_console_config)
    : gadget_browser_host_(owner, view_debug_mode),
      owner_(owner),
      sidebar_shown_(false),
      gadgets_shown_(true),
      flags_(flags),
      view_debug_mode_(view_debug_mode),
      debug_console_config_(debug_console_config),
      sidebar_host_(NULL),
      dragging_gadget_(NULL),
      drag_observer_(NULL),
      dragging_offset_x_(-1),
      dragging_offset_y_(-1),
      sidebar_moving_(false),
      sidebar_resizing_(false),
      has_strut_(false),
      sidebar_(NULL),
      options_(CreateOptions(options)),
      auto_hide_(false),
      always_on_top_(false),
      closed_(false),
      safe_to_exit_(true),
      font_size_(kDefaultFontSize),
      sidebar_monitor_(kDefaultMonitor),
      sidebar_position_(SIDEBAR_POSITION_RIGHT),
      sidebar_width_(kDefaultSideBarWidth),
      auto_hide_source_(0),
      auto_show_source_(0),
      net_wm_strut_(GDK_NONE),
      net_wm_strut_partial_(GDK_NONE),
      gadget_manager_(GetGadgetManager()),
      on_new_gadget_instance_connection_(NULL),
      on_remove_gadget_instance_connection_(NULL),
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
      status_icon_(NULL),
      status_icon_menu_(NULL),
#endif
      sidebar_window_(NULL),
      hotkey_grabber_(NULL) {
    ASSERT(gadget_manager_);
    ASSERT(options_);

    hotkey_grabber_.ConnectOnHotKeyPressed(
        NewSlot(this, &Impl::OnHotKeyPressed));

    workarea_.x = 0;
    workarea_.y = 0;
    workarea_.width = 0;
    workarea_.height = 0;

    int vh_flags = GtkHostBase::FlagsToViewHostFlags(flags_);
    sidebar_host_ = new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN,
                                       1.0, vh_flags, view_debug_mode_);

    sidebar_host_->ConnectOnBeginResizeDrag(
        NewSlot(this, &Impl::OnSideBarBeginResize));
    sidebar_host_->ConnectOnEndResizeDrag(
        NewSlot(this, &Impl::OnSideBarEndResize));
    sidebar_host_->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::OnSideBarBeginMove));
    sidebar_host_->ConnectOnShowHide(
        NewSlot(this, &Impl::OnSideBarShowHide));
    sidebar_host_->ConnectOnResized(
        NewSlot(this, &Impl::OnSideBarResized));

    sidebar_ = new SideBar(sidebar_host_);
    sidebar_->ConnectOnAddGadget(
        NewSlot(this, &Impl::OnSideBarAddGadget));
    sidebar_->ConnectOnMenu(
        NewSlot(this, &Impl::OnSideBarMenu));
    sidebar_->ConnectOnClose(
        NewSlot(this, &Impl::OnSideBarClose));
    sidebar_->ConnectOnUndock(
        NewSlot(this, &Impl::OnSideBarUndock));
    sidebar_->ConnectOnClick(
        NewSlot(this, &Impl::OnSideBarClick));
    sidebar_->ConnectOnViewMoved(
        NewSlot(this, &Impl::OnSideBarChildViewMoved));
    sidebar_->ConnectOnGoogleIconClicked(
        NewSlot(this, &Impl::OnGoogleIconClicked));

    LoadGlobalOptions();

    // Connect gadget manager related signals.
    on_new_gadget_instance_connection_ =
        gadget_manager_->ConnectOnNewGadgetInstance(
            NewSlot(this, &Impl::NewGadgetInstanceCallback));
    on_remove_gadget_instance_connection_ =
        gadget_manager_->ConnectOnRemoveGadgetInstance(
            NewSlot(this, &Impl::RemoveGadgetInstanceCallback));

    // Initializes global permissions.
    // FIXME: Supports customizable global permissions.
    global_permissions_.SetGranted(Permissions::ALL_ACCESS, true);
  }

  ~Impl() {
    SaveGlobalOptions();

    on_new_gadget_instance_connection_->Disconnect();
    on_remove_gadget_instance_connection_->Disconnect();

    if (auto_hide_source_)
      g_source_remove(auto_hide_source_);
    if (auto_show_source_)
      g_source_remove(auto_show_source_);

    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second.debug_console)
        gtk_widget_destroy(it->second.debug_console);
      delete it->second.gadget;
    }

    delete sidebar_;

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    g_object_unref(G_OBJECT(status_icon_));
    if (status_icon_menu_)
      gtk_widget_destroy(status_icon_menu_);
#endif
    delete options_;
  }

  void OnHotKeyPressed() {
    if (!gadgets_shown_ || (!closed_ && sidebar_->IsMinimized()))
      ShowOrHideAll(true);
    else
      ShowOrHideAll(false);
  }

  void OnWorkAreaChange() {
    GdkRectangle old = workarea_;
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(sidebar_window_));
    int screen_width = gdk_screen_get_width(screen);
    GetWorkAreaGeometry(sidebar_window_, &workarea_);
    // Remove the portion that occupied by sidebar itself.
    if (has_strut_) {
      if (sidebar_position_ == SIDEBAR_POSITION_LEFT &&
          workarea_.x >= sidebar_width_) {
        workarea_.x -= sidebar_width_;
        workarea_.width += sidebar_width_;
      } else if (sidebar_position_ == SIDEBAR_POSITION_RIGHT &&
                 workarea_.x + workarea_.width + sidebar_width_ <=
                 screen_width) {
        workarea_.width += sidebar_width_;
      }
    }
    DLOG("New work area: x:%d y:%d w:%d h:%d",
         workarea_.x, workarea_.y, workarea_.width, workarea_.height);

    if (old.x != workarea_.x || old.y != workarea_.y ||
        old.width != workarea_.width || old.height != workarea_.height)
      AdjustSideBar();
  }

  // SideBar handlers
  bool OnSideBarBeginResize(int button, int hittest) {
    CloseAllPopOutWindowsOfSideBar(-1);
    if (button == MouseEvent::BUTTON_LEFT &&
        ((hittest == ViewInterface::HT_LEFT &&
          sidebar_position_ == SIDEBAR_POSITION_RIGHT) ||
         (hittest == ViewInterface::HT_RIGHT &&
          sidebar_position_ == SIDEBAR_POSITION_LEFT))) {
      sidebar_resizing_ = true;
      return false;
    }

    // Don't allow resize drag in any other situation.
    return true;
  }

  void OnSideBarEndResize() {
    sidebar_resizing_ = false;
    if (has_strut_)
      AdjustSideBar();
  }

  bool OnSideBarBeginMove(int button) {
    if (button != MouseEvent::BUTTON_LEFT || dragging_gadget_ ||
        sidebar_->IsMinimized())
      return true;
    CloseAllPopOutWindowsOfSideBar(-1);
    if (gdk_pointer_grab(drag_observer_->window, FALSE,
                         (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK),
                         NULL, NULL, gtk_get_current_event_time()) ==
        GDK_GRAB_SUCCESS) {
      DLOG("OnSideBarBeginMove");
      int x, y;
      gtk_widget_get_pointer(sidebar_window_, &x, &y);
      sidebar_host_->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);
      dragging_offset_x_ = x;
      dragging_offset_y_ = y;
      sidebar_moving_ = true;
    }
    return true;
  }

  void OnSideBarMove() {
    int px, py;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &px, &py, NULL);
    sidebar_host_->SetWindowPosition(px - static_cast<int>(dragging_offset_x_),
                                     py - static_cast<int>(dragging_offset_y_));
  }

  void OnSideBarEndMove() {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(sidebar_window_));
    sidebar_monitor_ =
        gdk_screen_get_monitor_at_window(screen, sidebar_window_->window);
    GdkRectangle rect;
    gdk_screen_get_monitor_geometry(screen, sidebar_monitor_, &rect);
    int px, py;
    sidebar_host_->GetWindowPosition(&px, &py);
    if (px >= rect.x + (rect.width - sidebar_width_) / 2)
      sidebar_position_ = SIDEBAR_POSITION_RIGHT;
    else
      sidebar_position_ = SIDEBAR_POSITION_LEFT;
    sidebar_moving_ = false;
    AdjustSideBar();
  }

  void OnSideBarShowHide(bool show) {
    sidebar_shown_ = show;
    AdjustSideBar();
  }

  void OnSideBarAddGadget() {
    ShowOrHideAll(true);
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  // Returns true if any menu item is added.
  bool AddFloatingGadgetToMenu(MenuInterface *menu, int priority) {
    bool result = false;
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      Gadget *gadget = it->second.gadget;
      if (gadget->GetDisplayTarget() != Gadget::TARGET_SIDEBAR) {
        std::string caption = gadget->GetMainView()->GetCaption();
        menu->AddItem(caption.c_str(), 0, 0,
                      NewSlot(this, &Impl::FloatingGadgetMenuHandler, gadget),
                      priority);
        result = true;
      }
    }
    return result;
  }

  void OnSideBarMenu(MenuInterface *menu) {
    const int priority = MenuInterface::MENU_ITEM_PRI_HOST;
    menu->AddItem(GM_("MENU_ITEM_ADD_GADGETS"), 0,
                  MenuInterface::MENU_ITEM_ICON_ADD,
                  NewSlot(this, &Impl::AddGadgetMenuHandler), priority);
    menu->AddItem(GM_("MENU_ITEM_ADD_IGOOGLE_GADGET"), 0,
                  MenuInterface::MENU_ITEM_ICON_ADD,
                  NewSlot(this, &Impl::AddIGoogleGadgetMenuHandler), priority);
    menu->AddItem(NULL, 0, 0, NULL, priority);
    menu->AddItem(GM_("MENU_ITEM_SIDEBAR"),
                  (closed_ ? 0 : MenuInterface::MENU_ITEM_FLAG_CHECKED), 0,
                  NewSlot(this, &Impl::OpenCloseSidebarMenuHandler),
                  priority);
    if (!gadgets_shown_)
      menu->AddItem(GM_("MENU_ITEM_SHOW_ALL"), 0, 0,
                    NewSlot(this, &Impl::ShowAllMenuHandler), priority);
    else
      menu->AddItem(GM_("MENU_ITEM_HIDE_ALL"), 0, 0,
                    NewSlot(this, &Impl::HideAllMenuHandler), priority);

    if (!closed_) {
      menu->AddItem(GM_("MENU_ITEM_AUTO_HIDE"),
                    auto_hide_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
                    NewSlot(this, &Impl::AutoHideMenuHandler), priority);
      menu->AddItem(GM_("MENU_ITEM_ALWAYS_ON_TOP"), always_on_top_ ?
                    MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
                    NewSlot(this, &Impl::AlwaysOnTopMenuHandler), priority);

      MenuInterface *dock_submenu =
          menu->AddPopup(GM_("MENU_ITEM_DOCK_SIDEBAR"), priority);
      dock_submenu->AddItem(GM_("MENU_ITEM_LEFT"),
                            sidebar_position_ == SIDEBAR_POSITION_LEFT ?
                            MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
                            NewSlot(this, &Impl::SideBarPositionMenuHandler,
                                    static_cast<int>(SIDEBAR_POSITION_LEFT)),
                            priority);
      dock_submenu->AddItem(GM_("MENU_ITEM_RIGHT"),
                            sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
                            MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
                            NewSlot(this, &Impl::SideBarPositionMenuHandler,
                                    static_cast<int>(SIDEBAR_POSITION_RIGHT)),
                            priority);
    }
    {
      MenuInterface *sub = menu->AddPopup(GM_("MENU_ITEM_FONT_SIZE"),
                                          priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_LARGER"),
                   font_size_ >= kMaxFontSize ?
                   MenuInterface::MENU_ITEM_FLAG_GRAYED : 0,
                   MenuInterface::MENU_ITEM_ICON_ZOOM_IN,
                   NewSlot(this, &Impl::FontSizeMenuHandler, 1), priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_DEFAULT"), 0,
                   MenuInterface::MENU_ITEM_ICON_ZOOM_100,
                   NewSlot(this, &Impl::FontSizeMenuHandler, 0), priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_SMALLER"),
                   font_size_ <= kMinFontSize ?
                   MenuInterface::MENU_ITEM_FLAG_GRAYED : 0,
                   MenuInterface::MENU_ITEM_ICON_ZOOM_OUT,
                   NewSlot(this, &Impl::FontSizeMenuHandler, -1), priority);
    }
    menu->AddItem(GM_("MENU_ITEM_CHANGE_HOTKEY"), 0, 0,
                  NewSlot(this, &Impl::ChangeHotKeyMenuHandler), priority);

    menu->AddItem(NULL, 0, 0, NULL, priority);
    if (AddFloatingGadgetToMenu(menu, priority)) {
      menu->AddItem(NULL, 0, 0, NULL, priority);
    }
    menu->AddItem(GM_("MENU_ITEM_ABOUT"), 0,
                  MenuInterface::MENU_ITEM_ICON_ABOUT,
                  NewSlot(this, &Impl::AboutMenuHandler), priority);
    menu->AddItem(GM_("MENU_ITEM_EXIT"),
                  IsSafeToExit() ? 0 : MenuInterface::MENU_ITEM_FLAG_GRAYED,
                  MenuInterface::MENU_ITEM_ICON_QUIT,
                  NewSlot(this, &Impl::ExitMenuHandler), priority);
  }

  void OnSideBarClose() {
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    closed_ = true;
    ShowOrHideSideBar(false);
#else
    if (!gadgets_shown_ || sidebar_->IsMinimized())
      ShowOrHideAll(true);
    else
      ShowOrHideAll(false);
#endif
  }

  void OnSideBarResized(int width, int height) {
    GGL_UNUSED(height);
    // ignore width changes when the sidebar is minimized.
    if (!sidebar_->IsMinimized()) {
      sidebar_width_ = width;
      DLOG("set sidebar_width_ to %d", sidebar_width_);
    }

    // Call AdjustSideBar() if it's not in resize mode, otherwise it'll be
    // called by OnSideBarEndResize().
    if (!sidebar_resizing_) {
      AdjustSideBar();
    }
  }

  // Handle gadget undock by dragging out of sidebar
  void OnSideBarUndock(View *view, size_t index,
                       double offset_x, double offset_y) {
    ASSERT(view);
    int gadget_id = view->GetGadget()->GetInstanceID();
    GadgetInfo *info = &gadgets_[gadget_id];
    info->index_in_sidebar = index;

    // Close details view and popout view before undocking.
    CloseDetailsView(gadget_id);
    OnMainViewPopIn(gadget_id);

    // We need the height of decorated view.
    double height = view->GetHeight();
    // Convert offset_x, offset_y to main view's coords if main view is not
    // collapsed.
    // Note: the view passed to this method is the decorated view instead of
    // the main view.
    view->ViewCoordToNativeWidgetCoord(offset_x, offset_y,
                                       &offset_x, &offset_y);

    View *main_view = info->gadget->GetMainView();
    if (info->main_decorator->GetViewDecorator()->IsChildViewVisible())
      view = main_view;

    double width = view->GetWidth();
    view->NativeWidgetCoordToViewCoord(offset_x, offset_y,
                                       &offset_x, &offset_y);

    DecoratedViewHost *new_host = NewFloatingMainViewHost(gadget_id);
    new_host->SetAutoLoadChildViewSize(false);
    ViewHostInterface *old = main_view->SwitchViewHost(new_host);
    // DisplayTarget and undock event will be set in OnMainViewEndMove();
    // FIXME: How to make sure the browser element can reparent correctly?
    if (old) {
      CopyMinimizedState(old, new_host);
      old->Destroy();
    }

    if (gdk_pointer_grab(drag_observer_->window, FALSE,
                         (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK),
                         NULL, NULL, gtk_get_current_event_time()) ==
        GDK_GRAB_SUCCESS) {
      dragging_gadget_ = info->gadget;
      sidebar_->InsertPlaceholder(index, height);

      if (!info->main_decorator->GetViewDecorator()->IsChildViewVisible()) {
        view = info->main_decorator->GetViewDecorator();
        view->SetSize(width, view->GetHeight());
      } else {
        view = main_view;
      }

      view->ViewCoordToNativeWidgetCoord(offset_x, offset_y,
                                         &dragging_offset_x_,
                                         &dragging_offset_y_);
      info->undock_by_drag = true;

      // move window to the cursor position.
      int x, y;
      gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
      info->floating->SetWindowPosition(
          x - static_cast<int>(dragging_offset_x_),
          y - static_cast<int>(dragging_offset_y_));
      info->floating->ShowView(false, 0, NULL);
      // make sure that the floating window can move on to the sidebar.
      info->floating->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);
      info->old_keep_above = info->floating->IsKeepAbove();
      info->floating->SetKeepAbove(true);
      gdk_window_raise(info->floating->GetWindow()->window);
    } else {
      info->floating->ShowView(false, 0, NULL);
      info->gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    }
  }

  void OnSideBarClick(View *) {
    if (auto_hide_source_) {
      g_source_remove(auto_hide_source_);
      auto_hide_source_ = 0;
    }
    if (auto_show_source_) {
      g_source_remove(auto_show_source_);
      auto_show_source_ = 0;
    }
    if (auto_hide_ && sidebar_->IsMinimized())
      ShowOrHideSideBar(true);
  }

  void OnSideBarChildViewMoved(View *view) {
    if (view) {
      GadgetInterface *gadget = view->GetGadget();
      if (gadget) {
        int gadget_id = gadget->GetInstanceID();
        SetPopOutViewPosition(gadget_id);
        SetDetailsViewPosition(gadget_id);
      }
    }
  }

  // Close pop out window for all gadgets in sidebar except for the
  // specified one.
  // If the specified gadget_id is -1, then close all pop out windows.
  void CloseAllPopOutWindowsOfSideBar(int gadget_id) {
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->first != gadget_id && !it->second.floating) {
        CloseDetailsView(it->first);
        OnMainViewPopIn(it->first);
      }
    }
  }

  // option load / save methods
  void LoadGlobalOptions() {
    Variant value;
    value = options_->GetInternalValue(kOptionAutoHide);
    value.ConvertToBool(&auto_hide_);
    value = options_->GetInternalValue(kOptionAlwaysOnTop);
    value.ConvertToBool(&always_on_top_);
    value = options_->GetInternalValue(kOptionSideBarClosed);
    value.ConvertToBool(&closed_);
    value = options_->GetInternalValue(kOptionPosition);
    value.ConvertToInt(&sidebar_position_);
    value = options_->GetInternalValue(kOptionWidth);
    value.ConvertToInt(&sidebar_width_);
    value = options_->GetInternalValue(kOptionMonitor);
    value.ConvertToInt(&sidebar_monitor_);
    value = options_->GetInternalValue(kOptionFontSize);
    value.ConvertToInt(&font_size_);
    font_size_ = std::min(kMaxFontSize, std::max(kMinFontSize, font_size_));

    // Auto hide can't work correctly without always on top.
    if (auto_hide_)
      always_on_top_ = true;

    std::string hotkey;
    if (options_->GetInternalValue(kOptionHotKey).ConvertToString(&hotkey) &&
        hotkey.length()) {
      hotkey_grabber_.SetHotKey(hotkey);
      hotkey_grabber_.SetEnableGrabbing(true);
    }

    // The default value of gadgets_shown_ is true.
    value = options_->GetInternalValue(kOptionGadgetsShown);
    if (value.type() == Variant::TYPE_BOOL)
      gadgets_shown_ = VariantValue<bool>()(value);
  }

  bool SaveGadgetOrder(size_t index, View *view) {
    GadgetInterface *gadget = view->GetGadget();
    OptionsInterface *opt = gadget->GetOptions();
    opt->PutInternalValue(kOptionPositionInSideBar, Variant(index));
    return true;
  }

  void SaveGlobalOptions() {
    // save gadgets' information
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      OptionsInterface *opt = it->second.gadget->GetOptions();
      opt->PutInternalValue(kOptionDisplayTarget,
                            Variant(it->second.gadget->GetDisplayTarget()));
    }
    sidebar_->EnumerateViews(NewSlot(this, &Impl::SaveGadgetOrder));

    // save sidebar's information
    options_->PutInternalValue(kOptionAutoHide, Variant(auto_hide_));
    options_->PutInternalValue(kOptionAlwaysOnTop,
                               Variant(always_on_top_));
    options_->PutInternalValue(kOptionSideBarClosed, Variant(closed_));
    options_->PutInternalValue(kOptionPosition,
                               Variant(sidebar_position_));
    options_->PutInternalValue(kOptionWidth, Variant(sidebar_width_));
    options_->PutInternalValue(kOptionMonitor,
                               Variant(sidebar_monitor_));
    options_->PutInternalValue(kOptionFontSize, Variant(font_size_));
    options_->PutInternalValue(kOptionGadgetsShown, Variant(gadgets_shown_));
    options_->PutInternalValue(kOptionHotKey,
                               Variant(hotkey_grabber_.GetHotKey()));
    options_->Flush();
  }

  void SetupUI() {
    sidebar_window_ = sidebar_host_->GetWindow();

    g_signal_connect_after(G_OBJECT(sidebar_window_), "focus-out-event",
                           G_CALLBACK(ToplevelWindowFocusOutHandler), this);
    g_signal_connect_after(G_OBJECT(sidebar_window_), "focus-in-event",
                           G_CALLBACK(ToplevelWindowFocusInHandler), this);
    g_signal_connect_after(G_OBJECT(sidebar_window_), "enter-notify-event",
                           G_CALLBACK(SideBarEnterNotifyHandler), this);
    g_signal_connect_after(G_OBJECT(sidebar_window_), "leave-notify-event",
                           G_CALLBACK(SideBarLeaveNotifyHandler), this);

    MonitorWorkAreaChange(sidebar_window_,
                          NewSlot(this, &Impl::OnWorkAreaChange));

    // AdjustSideBar() will be called by this function.
    OnWorkAreaChange();

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      GdkPixbuf *icon_pixbuf = LoadPixbufFromData(icon_data);
      status_icon_ = gtk_status_icon_new_from_pixbuf(icon_pixbuf);
      g_object_unref(icon_pixbuf);
    } else {
      status_icon_ = gtk_status_icon_new_from_stock(GTK_STOCK_ABOUT);
    }
    gtk_status_icon_set_tooltip(status_icon_, GM_("GOOGLE_GADGETS"));
    g_signal_connect(G_OBJECT(status_icon_), "activate",
                     G_CALLBACK(StatusIconActivateHandler), this);
    g_signal_connect(G_OBJECT(status_icon_), "popup-menu",
                     G_CALLBACK(StatusIconPopupMenuHandler), this);
#else
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(sidebar_window_), FALSE);
#endif

    gtk_window_set_title(GTK_WINDOW(sidebar_window_), GM_("GOOGLE_GADGETS"));

    // create drag observer
    drag_observer_ = gtk_invisible_new();
    gtk_widget_show(drag_observer_);
    g_signal_connect(G_OBJECT(drag_observer_), "motion-notify-event",
                     G_CALLBACK(DragObserverMotionNotifyHandler), this);
    g_signal_connect(G_OBJECT(drag_observer_), "button-release-event",
                     G_CALLBACK(DragObserverButtonReleaseHandler), this);
  }

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  void UpdateStatusIconTooltip() {
    if (hotkey_grabber_.IsGrabbing()) {
      gtk_status_icon_set_tooltip(status_icon_,
          StringPrintf(GM_("STATUS_ICON_TOOLTIP_WITH_HOTKEY"),
                       hotkey_grabber_.GetHotKey().c_str()).c_str());
    } else {
      gtk_status_icon_set_tooltip(status_icon_, GM_("STATUS_ICON_TOOLTIP"));
    }
  }
#endif

  bool EnumerateGadgetInstancesCallback(int id) {
    if (!LoadGadgetInstance(id))
      gadget_manager_->RemoveGadgetInstance(id);
    // Return true to continue the enumeration.
    return true;
  }

  bool NewGadgetInstanceCallback(int id) {
    return LoadGadgetInstance(id);
  }

  bool LoadGadgetInstance(int id) {
    bool result = false;
    safe_to_exit_ = false;
    if (owner_->ConfirmManagedGadget(id, flags_ & GRANT_PERMISSIONS)) {
      std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
      std::string path = gadget_manager_->GetGadgetInstancePath(id);
      if (options.length() && path.length()) {
        result = (LoadGadget(path.c_str(), options.c_str(), id, false) != NULL);
        DLOG("SideBarGtkHost: Load gadget %s, with option %s, %s",
             path.c_str(), options.c_str(), result ? "succeeded" : "failed");
      }
    }
    safe_to_exit_ = true;
    return result;
  }

  void AdjustSideBar() {
    // Don't adjust sidebar if it's not visible yet.
    if (!sidebar_shown_)
      return;

    GdkRectangle monitor_geometry;
    // got monitor information
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(sidebar_window_));
    int screen_width = gdk_screen_get_width(screen);
    int monitor_number = gdk_screen_get_n_monitors(screen);
    if (sidebar_monitor_ >= monitor_number) {
      DLOG("want to put sidebar in %d monitor, but this screen(%p) has "
           "only %d monitor(s), put to last monitor.",
           sidebar_monitor_, screen, monitor_number);
      sidebar_monitor_ = monitor_number - 1;
    }
    gdk_screen_get_monitor_geometry(screen, sidebar_monitor_,
                                    &monitor_geometry);
    DLOG("monitor %d's rect: %d %d %d %d", sidebar_monitor_,
         monitor_geometry.x, monitor_geometry.y,
         monitor_geometry.width, monitor_geometry.height);

    DLOG("Set SideBar size: %dx%d", sidebar_width_, workarea_.height);
    sidebar_->SetSize(sidebar_width_, workarea_.height);

    int x = 0;
    if (sidebar_position_ == SIDEBAR_POSITION_LEFT) {
      x = std::max(monitor_geometry.x, workarea_.x);
    } else {
      x = std::min(monitor_geometry.x + monitor_geometry.width,
                   workarea_.x + workarea_.width) -
          static_cast<int>(sidebar_->GetWidth());
    }

    // if sidebar is on the edge, do strut
    if (always_on_top_ && !sidebar_->IsMinimized() && !auto_hide_ &&
        ((monitor_geometry.x <= 0 &&
          sidebar_position_ == SIDEBAR_POSITION_LEFT) ||
         (monitor_geometry.x + monitor_geometry.width >= screen_width &&
          sidebar_position_ == SIDEBAR_POSITION_RIGHT))) {
      has_strut_ = true;
      sidebar_host_->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);

      // lazy initial gdk atoms
      if (net_wm_strut_ == GDK_NONE)
        net_wm_strut_ = gdk_atom_intern("_NET_WM_STRUT", FALSE);
      if (net_wm_strut_partial_ == GDK_NONE)
        net_wm_strut_partial_ = gdk_atom_intern("_NET_WM_STRUT_PARTIAL", FALSE);

      // change strut property now
      gulong struts[12];
      memset(struts, 0, sizeof(struts));
      if (sidebar_position_ == SIDEBAR_POSITION_LEFT) {
        struts[0] = x + static_cast<int>(sidebar_->GetWidth());
        struts[4] = workarea_.y;
        struts[5] = workarea_.y + workarea_.height;
      } else {
        struts[1] = screen_width - x;
        struts[6] = workarea_.y;
        struts[7] = workarea_.y + workarea_.height;
      }
      gdk_property_change(sidebar_window_->window, net_wm_strut_,
                          gdk_atom_intern("CARDINAL", FALSE),
                          32, GDK_PROP_MODE_REPLACE,
                          reinterpret_cast<guchar *>(&struts), 4);
      gdk_property_change(sidebar_window_->window, net_wm_strut_partial_,
                          gdk_atom_intern("CARDINAL", FALSE),
                          32, GDK_PROP_MODE_REPLACE,
                          reinterpret_cast<guchar *>(&struts), 12);
    } else {
      has_strut_ = false;
      if (net_wm_strut_ != GDK_NONE)
        gdk_property_delete(sidebar_window_->window, net_wm_strut_);
      if (net_wm_strut_partial_ != GDK_NONE)
        gdk_property_delete(sidebar_window_->window, net_wm_strut_partial_);

      sidebar_host_->SetWindowType((flags_ & MATCHBOX_WORKAROUND) ?
                                   GDK_WINDOW_TYPE_HINT_DIALOG :
                                   GDK_WINDOW_TYPE_HINT_NORMAL);
    }

    DLOG("move sidebar to %dx%d", x, workarea_.y);
    sidebar_host_->SetWindowPosition(x, workarea_.y);

    sidebar_host_->SetKeepAbove(always_on_top_ || sidebar_->IsMinimized());

    // adjust the orientation of the arrow of each gadget in the sidebar
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second.gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
        MainViewDecoratorBase *view_decorator =
            down_cast<MainViewDecoratorBase *>(
                it->second.main_decorator->GetViewDecorator());
        if (view_decorator) {
          view_decorator->SetPopOutDirection(
              sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
              MainViewDecoratorBase::POPOUT_TO_LEFT :
              MainViewDecoratorBase::POPOUT_TO_RIGHT);
        }
      }
    }
  }

  // view host related methods.
  void CloseDetailsView(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    if (info->details) {
      info->gadget->CloseDetailsView();
      info->details = NULL;
    }
  }
  void CopyMinimizedState(ViewHostInterface *from, ViewHostInterface *to) {
    DecoratedViewHost *from_dvh = static_cast<DecoratedViewHost*>(from);
    DecoratedViewHost *to_dvh = static_cast<DecoratedViewHost*>(to);
    MainViewDecoratorBase *from_vd =
        static_cast<MainViewDecoratorBase*>(from_dvh->GetViewDecorator());
    MainViewDecoratorBase *to_vd =
        static_cast<MainViewDecoratorBase*>(to_dvh->GetViewDecorator());
    DLOG("From is %s", from_vd->IsMinimized()? "false":"true");
    DLOG("To is %s", to_vd->IsMinimized()? "false":"true");
    to_vd->SetMinimized(from_vd->IsMinimized());
  }

  // Handle undock event triggered by clicking undock menu item.
  // Only for docked main view.
  void OnMainViewUndock(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(!info->floating);
    // Close details view and popout view before undocking.
    CloseDetailsView(gadget_id);
    OnMainViewPopIn(gadget_id);

    // Record gadget's position in sidebar for later use.
    info->index_in_sidebar =
        sidebar_->GetIndexOfView(info->main_decorator->GetViewDecorator());

    View *view = info->gadget->GetMainView();
    DecoratedViewHost *new_host = NewFloatingMainViewHost(gadget_id);
    ViewHostInterface *old = view->SwitchViewHost(new_host);
    // Send undock event before destroying the old view host.
    // Browser element relies on it to reparent the browser widget.
    // Otherwise the browser widget might be destroyed along with the old view
    // host.
    view->OnOtherEvent(SimpleEvent(Event::EVENT_UNDOCK));
    info->gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    if (old) {
      CopyMinimizedState(old, new_host);
      old->Destroy();
    }

    info->floating->ShowView(false, 0, NULL);
    // Move the floating gadget to the center of the monitor, if the gadget
    // window overlaps with sidebar window.
    if (IsOverlapWithSideBar(gadget_id, NULL)) {
      GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(sidebar_window_));
      GdkRectangle rect;
      gdk_screen_get_monitor_geometry(screen, sidebar_monitor_, &rect);
      int width, height;
      info->floating->GetWindowSize(&width, &height);
      int x = (rect.x + (rect.width - width) / 2);
      int y = (rect.y + (rect.height - height) / 2);
      info->floating->SetWindowPosition(x, y);
    }
  }

  // Only for floating main view.
  void OnMainViewDock(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(info->floating);
    // Close details view before docking. floating main view won't have popout
    // view.
    CloseDetailsView(gadget_id);

    View *view = info->gadget->GetMainView();
    DecoratedViewHost *new_host = NewDockedMainViewHost(gadget_id);
    ViewHostInterface *old = view->SwitchViewHost(new_host);
    // Send dock event before destroying the old view host.
    // Browser element relies on it to reparent the browser widget.
    // Otherwise the browser widget might be destroyed along with the old view
    // host.
    view->OnOtherEvent(SimpleEvent(Event::EVENT_DOCK));
    info->gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    if (old) {
      CopyMinimizedState(old, new_host);
      old->Destroy();
    }
    new_host->ShowView(false, 0, NULL);
    info->floating = NULL;
  }

  // Only for floating main view.
  bool OnMainViewBeginMove(int button, int gadget_id) {
    GGL_UNUSED(button);
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(info->floating);
    if (gdk_pointer_grab(drag_observer_->window, FALSE,
                         (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK),
                         NULL, NULL, gtk_get_current_event_time()) ==
        GDK_GRAB_SUCCESS) {
      dragging_gadget_ = info->gadget;
      int x, y;
      GtkWidget *window = info->floating->GetWindow();
      gtk_widget_get_pointer(window, &x, &y);
      dragging_offset_x_ = x;
      dragging_offset_y_ = y;
      // make sure that the floating window can move on to the sidebar.
      info->floating->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);
      info->old_keep_above = info->floating->IsKeepAbove();
      info->floating->SetKeepAbove(true);

      // Raise the sidebar window to make sure that there is no other window on
      // top of sidebar window.
      gdk_window_raise(sidebar_window_->window);

      // Raise gadget window after raising sidebar window, to make sure it's on
      // top of sidebar window.
      gdk_window_raise(window->window);
      return true;
    }
    return false;
  }

  // Only for floating main view.
  void OnMainViewMove(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(info->floating);
    int h, x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    info->floating->SetWindowPosition(
        x - static_cast<int>(dragging_offset_x_),
        y - static_cast<int>(dragging_offset_y_));
    SetDetailsViewPosition(gadget_id);
    if (IsOverlapWithSideBar(gadget_id, &h)) {
      // show sidebar first if it is auto hiden
      // note that we don't use flag gadgets_shown_ to judge if sidebar is
      // shown, since resize action is async in GTK, so the status of the flag
      // may not be right.
      if (sidebar_->IsMinimized()) {
        ShowOrHideSideBar(true);
        info->floating->SetKeepAbove(true);
        gdk_window_raise(info->floating->GetWindow()->window);
      }

      size_t index = sidebar_->GetIndexOfPosition(h);
      int width, height;
      info->floating->GetWindowSize(&width, &height);
      sidebar_->InsertPlaceholder(index, static_cast<double>(height));
    } else {
      sidebar_->ClearPlaceholder();
    }
  }

  void OnMainViewEndMove(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(info->floating);
    int h, x, y;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    // The floating window must be normal window when not dragging,
    // otherwise it'll always on top.
    info->floating->SetWindowType((flags_ & MATCHBOX_WORKAROUND) ?
                                  GDK_WINDOW_TYPE_HINT_DIALOG :
                                  GDK_WINDOW_TYPE_HINT_NORMAL);
    info->floating->SetKeepAbove(info->old_keep_above);
    if (IsOverlapWithSideBar(gadget_id, &h)) {
      info->index_in_sidebar = sidebar_->GetIndexOfPosition(h);
      OnMainViewDock(gadget_id);
    } else if (info->undock_by_drag) {
      // In drag undock mode, OnSideBarUndock() will not set the display
      // target and send undock event.
      SimpleEvent event(Event::EVENT_UNDOCK);
      info->gadget->GetMainView()->OnOtherEvent(event);
      info->gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
      info->main_decorator->SetAutoLoadChildViewSize(true);
      info->main_decorator->LoadChildViewSize();
      info->undock_by_drag = false;
    }
    sidebar_->ClearPlaceholder();
    dragging_gadget_ = NULL;
  }

  // Only for floating main view.
  void OnMainViewResized(int, int, int gadget_id) {
    SetDetailsViewPosition(gadget_id);
  }

  void OnMainViewClose(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    CloseDetailsView(gadget_id);
    OnMainViewPopIn(gadget_id);
    info->gadget->RemoveMe(true);
  }

  void OnMainViewPopOut(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(!info->popout);
    ASSERT(!info->floating);
    CloseDetailsView(gadget_id);

    View *view = info->gadget->GetMainView();
    DecoratedViewHost *new_host = NewPopOutViewHost(gadget_id);
    // Send popout event to decorator before switching the view host.
    // View decorator requires it to work properly.
    SimpleEvent event(Event::EVENT_POPOUT);
    info->main_decorator->GetViewDecorator()->OnOtherEvent(event);
    view->SwitchViewHost(new_host);
    SetPopOutViewPosition(gadget_id);
    new_host->ShowView(false, 0, NULL);
  }

  void OnMainViewPopIn(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    if (info->popout) {
      CloseDetailsView(gadget_id);

      View *view = info->gadget->GetMainView();
      info->popout->CloseView();
      ViewHostInterface *old_host = view->SwitchViewHost(info->main_decorator);
      // Send popin event to decorator after switching the view host.
      // View decorator requires it to work properly.
      SimpleEvent event(Event::EVENT_POPIN);
      info->main_decorator->GetViewDecorator()->OnOtherEvent(event);
      // The old host must be destroyed after sending onpopin event.
      old_host->Destroy();
      info->popout = NULL;
    }
  }

  // Handlers for details view signals.
  void OnDetailsViewShowHide(bool show, int gadget_id) {
    if (show)
      SetDetailsViewPosition(gadget_id);
    else
      gadgets_[gadget_id].details = NULL;
  }

  void OnDetailsViewResized(int, int, int gadget_id) {
    SetDetailsViewPosition(gadget_id);
  }

  bool OnDetailsViewBeginResize(int button, int hittest, int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    if (button != MouseEvent::BUTTON_LEFT || hittest == ViewInterface::HT_TOP)
      return true;

    if ((info->details_on_right &&
         (hittest == ViewInterface::HT_LEFT ||
          hittest == ViewInterface::HT_TOPLEFT ||
          hittest == ViewInterface::HT_BOTTOMLEFT)) ||
        (!info->details_on_right &&
         (hittest == ViewInterface::HT_RIGHT ||
          hittest == ViewInterface::HT_TOPRIGHT ||
          hittest == ViewInterface::HT_BOTTOMRIGHT)))
      return true;

    return false;
  }

  bool OnDetailsViewBeginMove(int button, int gadget_id) {
    GGL_UNUSED(button);
    GGL_UNUSED(gadget_id);
    // details window is not allowed to move, just return true
    return true;
  }

  void OnDetailsViewClose(int gadget_id) {
    CloseDetailsView(gadget_id);
  }

  void SetDetailsViewPosition(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    ASSERT(info->floating == NULL || info->popout == NULL);
    if (info->details) {
      int main_x, main_y;
      int main_width, main_height;
      // Use the view's size directly, in case the window's size hasn't been
      // updated.
      int details_width =
          static_cast<int>(info->details->GetView()->GetWidth());
      int details_height =
          static_cast<int>(info->details->GetView()->GetHeight());
      GdkScreen *screen = gtk_widget_get_screen(info->details->GetWindow());
      int screen_width = gdk_screen_get_width(screen);
      int screen_height = gdk_screen_get_height(screen);

      double mx, my;
      info->main_decorator->ViewCoordToNativeWidgetCoord(0, 0, &mx, &my);
      if (info->floating) {
        info->floating->GetWindowPosition(&main_x, &main_y);
        info->floating->GetWindowSize(&main_width, &main_height);
        main_y += static_cast<int>(my);
      } else if (info->popout) {
        info->popout->GetWindowPosition(&main_x, &main_y);
        info->popout->GetWindowSize(&main_width, &main_height);
      } else {
        sidebar_host_->GetWindowPosition(&main_x, &main_y);
        sidebar_host_->GetWindowSize(&main_width, &main_height);
        main_y += static_cast<int>(my);
      }

      bool on_right = info->details_on_right;
      if (info->floating) {
        if (on_right && details_width + main_width + main_x > screen_width)
          on_right = false;
        else if (!on_right && details_width > main_x)
          on_right = true;
      } else {
        on_right = (sidebar_position_ == SIDEBAR_POSITION_LEFT);
      }
      info->details_on_right = on_right;

      int x;
      if (on_right)
        x = main_x + main_width;
      else
        x = main_x - details_width;
      int y = main_y;
      if (y + details_height > screen_height)
        y = screen_height - details_height;
      info->details->SetWindowPosition(x, y);
    }
  }

  // handlers for popout view signals.
  void OnPopOutViewShowHide(bool show, int gadget_id) {
    if (show)
      SetPopOutViewPosition(gadget_id);
    else
      gadgets_[gadget_id].popout = NULL;
  }

  void OnPopOutViewResized(int, int, int gadget_id) {
    SetPopOutViewPosition(gadget_id);
    SetDetailsViewPosition(gadget_id);
  }

  bool OnPopOutViewBeginResize(int button, int hittest, int gadget_id) {
    GGL_UNUSED(gadget_id);
    if (button != MouseEvent::BUTTON_LEFT || hittest == ViewInterface::HT_TOP)
      return true;

    if ((sidebar_position_ == SIDEBAR_POSITION_LEFT &&
         (hittest == ViewInterface::HT_LEFT ||
          hittest == ViewInterface::HT_TOPLEFT ||
          hittest == ViewInterface::HT_BOTTOMLEFT)) ||
        (sidebar_position_ == SIDEBAR_POSITION_RIGHT &&
         (hittest == ViewInterface::HT_RIGHT ||
          hittest == ViewInterface::HT_TOPRIGHT ||
          hittest == ViewInterface::HT_BOTTOMRIGHT)))
      return true;

    return false;
  }

  bool OnPopOutViewBeginMove(int button, int gadget_id) {
    GGL_UNUSED(gadget_id);
    GGL_UNUSED(button);
    // popout window is not allowed to move, just return true
    return true;
  }

  void OnPopOutViewClose(int gadget_id) {
    OnMainViewPopIn(gadget_id);
  }

  void SetPopOutViewPosition(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget);
    if (info->popout) {
      int main_x, main_y;
      int main_width, main_height;
      // Use the view's size directly, in case the window's size hasn't been
      // updated.
      int popout_width =
          static_cast<int>(info->popout->GetView()->GetWidth());
      int popout_height =
          static_cast<int>(info->popout->GetView()->GetHeight());
      GdkScreen *screen = gtk_widget_get_screen(info->popout->GetWindow());
      int screen_height = gdk_screen_get_height(screen);
      sidebar_host_->GetWindowPosition(&main_x, &main_y);
      sidebar_host_->GetWindowSize(&main_width, &main_height);
      double mx, my;
      info->main_decorator->ViewCoordToNativeWidgetCoord(0, 0, &mx, &my);
      main_y += static_cast<int>(my);

      bool on_right = (sidebar_position_ == SIDEBAR_POSITION_LEFT);
      int x;
      if (on_right)
        x = main_x + main_width;
      else
        x = main_x - popout_width;
      int y = main_y;
      if (y + popout_height > screen_height)
        y = screen_height - popout_height;
      info->popout->SetWindowPosition(x, y);
    }
  }

  bool IsOverlapWithSideBar(int gadget_id, int *height) {
    GadgetInfo *info = &gadgets_[gadget_id];
    if (info->floating && !closed_) {
      int w, h, x, y;
      info->floating->GetWindowSize(&w, &h);
      info->floating->GetWindowPosition(&x, &y);
      int sx, sy, sw, sh;
      sidebar_host_->GetWindowPosition(&sx, &sy);
      sidebar_host_->GetWindowSize(&sw, &sh);
      if ((x + w >= sx) && (sx + sw >= x) && (y + h >= sy) && (sy + sh >= y)) {
        if (height) {
          int dummy;
          gtk_widget_get_pointer(sidebar_window_, &dummy, height);
        }
        return true;
      }
    }
    return false;
  }

  void ShowOrHideAll(bool show) {
    DLOG("ShowOrHideAll(%d)", show);
    ShowOrHideSideBar(show);
    ShowOrHideAllGadgets(show);
    gadgets_shown_ = show;
  }

  void ShowOrHideAllGadgets(bool show) {
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second.gadget->GetDisplayTarget() != Gadget::TARGET_SIDEBAR) {
        if (show)
          it->second.gadget->ShowMainView();
        else
          it->second.gadget->CloseMainView();
      }
      if (!show) {
        CloseDetailsView(it->first);
        OnMainViewPopIn(it->first);
      }
    }
  }

  void ShowOrHideSideBar(bool show) {
    DLOG("ShowOrHideSideBar(%d)", show);
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    if (show && !closed_) {
      sidebar_->Restore();
      // AdjustSideBar() will be called by OnSideBarResized().
      sidebar_->Show();
    } else {
      CloseAllPopOutWindowsOfSideBar(-1);
      if (auto_hide_ && !closed_) {
        // Minimize sidebar vertically.
        sidebar_->Minimize(true);
        // AdjustSideBar() will be called by OnSideBarResized().
      } else {
        sidebar_->Hide();
      }
    }
#else
    if (show) {
      sidebar_->Restore();
      // AdjustSideBar() will be called by OnSideBarResized().
      sidebar_->Show();
    } else {
      CloseAllPopOutWindowsOfSideBar(-1);
      // Minimize sidebar horizontally.
      sidebar_->Minimize(false);
    }
#endif
  }

  GadgetInterface *LoadGadget(const char *path,
                              const char *options_name,
                              int instance_id,
                              bool show_debug_console) {
    if (gadgets_.find(instance_id) != gadgets_.end()) {
      // Gadget is already loaded.
      return gadgets_[instance_id].gadget;
    }

    Gadget::DebugConsoleConfig dcc = show_debug_console ?
        Gadget::DEBUG_CONSOLE_INITIAL : debug_console_config_;

    safe_to_exit_ = false;
    Gadget *gadget = new Gadget(owner_, path, options_name, instance_id,
                                global_permissions_, dcc);
    safe_to_exit_ = true;

    GadgetInfoMap::iterator it = gadgets_.find(instance_id);
    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      if (it != gadgets_.end()) {
        if (it->second.debug_console)
          gtk_widget_destroy(it->second.debug_console);
        gadgets_.erase(it);
      }
      delete gadget;
      return NULL;
    }

    SetupGadgetGetFeedbackURLHandler(gadget);

    if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
      MainViewDecoratorBase *view_decorator =
          down_cast<MainViewDecoratorBase *>(
              it->second.main_decorator->GetViewDecorator());
      if (view_decorator) {
        view_decorator->SetPopOutDirection(
            sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
            MainViewDecoratorBase::POPOUT_TO_LEFT :
            MainViewDecoratorBase::POPOUT_TO_RIGHT);
      }
      gadget->GetMainView()->OnOtherEvent(SimpleEvent(Event::EVENT_DOCK));
    } else {
      gadget->GetMainView()->OnOtherEvent(SimpleEvent(Event::EVENT_UNDOCK));
    }

    if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR || gadgets_shown_)
      gadget->ShowMainView();

    // If debug console is opened during view host creation, the title is
    // not set then because main view is not available. Set the title now.
    if (it->second.debug_console) {
      gtk_window_set_title(GTK_WINDOW(it->second.debug_console),
                           gadget->GetMainView()->GetCaption().c_str());
    }
    return gadget;
  }

  DecoratedViewHost *NewDockedMainViewHost(int gadget_id) {
    GadgetInfo *info = &gadgets_[gadget_id];
    ViewHostInterface *view_host =
        sidebar_->NewViewHost(info->index_in_sidebar);
    DockedMainViewDecorator *view_decorator =
        new DockedMainViewDecorator(view_host);
    DecoratedViewHost *decorated_view_host =
        new DecoratedViewHost(view_decorator);

    view_decorator->ConnectOnUndock(
        NewSlot(this, &Impl::OnMainViewUndock, gadget_id));
    view_decorator->ConnectOnPopOut(
        NewSlot(this, &Impl::OnMainViewPopOut, gadget_id));
    view_decorator->ConnectOnPopIn(
        NewSlot(this, &Impl::OnMainViewPopIn, gadget_id));
    view_decorator->ConnectOnClose(
        NewSlot(this, &Impl::OnMainViewClose, gadget_id));
    view_decorator->SetPopOutDirection(
        sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
        MainViewDecoratorBase::POPOUT_TO_LEFT :
        MainViewDecoratorBase::POPOUT_TO_RIGHT);
    info->main_decorator = decorated_view_host;
    return decorated_view_host;
  }

  DecoratedViewHost *NewFloatingMainViewHost(int gadget_id) {
    int vh_flags = GtkHostBase::FlagsToViewHostFlags(flags_);
    vh_flags |= SingleViewHost::RECORD_STATES;

    SingleViewHost *view_host =
        new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0, vh_flags,
                           view_debug_mode_);
    view_host->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::OnMainViewBeginMove, gadget_id));
    view_host->ConnectOnResized(
        NewSlot(this, &Impl::OnMainViewResized, gadget_id));
    FloatingMainViewDecorator *view_decorator =
        new FloatingMainViewDecorator(view_host, !(flags_ & NO_TRANSPARENT));
    DecoratedViewHost *decorated_view_host =
        new DecoratedViewHost(view_decorator);

    view_decorator->ConnectOnDock(
        NewSlot(this, &Impl::OnMainViewDock, gadget_id));
    view_decorator->ConnectOnClose(
        NewSlot(this, &Impl::OnMainViewClose, gadget_id));
    view_decorator->SetButtonVisible(MainViewDecoratorBase::POP_IN_OUT_BUTTON,
                                     false);

    GadgetInfo *info = &gadgets_[gadget_id];
    info->main_decorator = decorated_view_host;
    info->floating = view_host;

    // It's ok to get the toplevel window, because decorated view host will set
    // decorated view into single view host.
    GtkWidget *toplevel = view_host->GetWindow();

    // To auto show/hide sidebar.
    g_signal_connect_after(G_OBJECT(toplevel), "focus-out-event",
                           G_CALLBACK(ToplevelWindowFocusOutHandler), this);
    g_signal_connect_after(G_OBJECT(toplevel), "focus-in-event",
                           G_CALLBACK(ToplevelWindowFocusInHandler), this);
    return decorated_view_host;
  }

  DecoratedViewHost *NewDetailsViewHost(int gadget_id) {
    int vh_flags = GtkHostBase::FlagsToViewHostFlags(flags_);
    // don't show window manager's border for details view.
    vh_flags &= ~SingleViewHost::DECORATED;
    SingleViewHost *view_host =
        new SingleViewHost(ViewHostInterface::VIEW_HOST_DETAILS, 1.0, vh_flags,
                           view_debug_mode_);
    view_host->ConnectOnShowHide(
        NewSlot(this, &Impl::OnDetailsViewShowHide, gadget_id));
    view_host->ConnectOnResized(
        NewSlot(this, &Impl::OnDetailsViewResized, gadget_id));
    view_host->ConnectOnBeginResizeDrag(
        NewSlot(this, &Impl::OnDetailsViewBeginResize, gadget_id));
    view_host->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::OnDetailsViewBeginMove, gadget_id));
    DetailsViewDecorator *view_decorator =
        new DetailsViewDecorator(view_host);
    DecoratedViewHost *decorated_view_host =
        new DecoratedViewHost(view_decorator);
    view_decorator->ConnectOnClose(
        NewSlot(this, &Impl::OnDetailsViewClose, gadget_id));

    // It's ok to get the toplevel window, because decorated view host will set
    // decorated view into single view host.
    GtkWidget *toplevel = view_host->GetWindow();

    // To auto show/hide sidebar.
    g_signal_connect_after(G_OBJECT(toplevel), "focus-out-event",
                           G_CALLBACK(ToplevelWindowFocusOutHandler), this);
    g_signal_connect_after(G_OBJECT(toplevel), "focus-in-event",
                           G_CALLBACK(ToplevelWindowFocusInHandler), this);

    GadgetInfo *info = &gadgets_[gadget_id];
    if (info->floating) {
      view_host->SetKeepAbove(info->floating->IsKeepAbove());
      gtk_window_set_transient_for(GTK_WINDOW(toplevel),
                                   GTK_WINDOW(info->floating->GetWindow()));
    } else {
      // Close all other details/popout view of sidebar.
      CloseAllPopOutWindowsOfSideBar(gadget_id);
      view_host->SetKeepAbove(always_on_top_);
      gtk_window_set_transient_for(GTK_WINDOW(toplevel),
                                   GTK_WINDOW(sidebar_window_));
    }
    info->details = view_host;

    // Set initial window position to reduce flicker when showing the view.
    SetDetailsViewPosition(gadget_id);
    return decorated_view_host;
  }

  DecoratedViewHost *NewPopOutViewHost(int gadget_id) {
    int vh_flags = GtkHostBase::FlagsToViewHostFlags(flags_);
    // don't show window manager's border for popout view.
    vh_flags &= ~SingleViewHost::DECORATED;
    SingleViewHost *view_host =
        new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0, vh_flags,
                           view_debug_mode_);
    //view_host->ConnectOnShowHide(
    //    NewSlot(this, &Impl::OnPopOutViewShowHide, id));
    view_host->ConnectOnResized(
        NewSlot(this, &Impl::OnPopOutViewResized, gadget_id));
    view_host->ConnectOnBeginResizeDrag(
        NewSlot(this, &Impl::OnPopOutViewBeginResize, gadget_id));
    view_host->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::OnPopOutViewBeginMove, gadget_id));
    PopOutMainViewDecorator *view_decorator =
        new PopOutMainViewDecorator(view_host);
    DecoratedViewHost *decorated_view_host =
        new DecoratedViewHost(view_decorator);
    view_decorator->ConnectOnClose(
        NewSlot(this, &Impl::OnPopOutViewClose, gadget_id));

    // It's ok to get the toplevel window, because decorated view host will set
    // decorated view into single view host.
    GtkWidget *toplevel = view_host->GetWindow();

    // To auto show/hide sidebar.
    g_signal_connect_after(G_OBJECT(toplevel), "focus-out-event",
                           G_CALLBACK(ToplevelWindowFocusOutHandler), this);
    g_signal_connect_after(G_OBJECT(toplevel), "focus-in-event",
                           G_CALLBACK(ToplevelWindowFocusInHandler), this);

    GadgetInfo *info = &gadgets_[gadget_id];
    if (info->floating) {
      view_host->SetKeepAbove(info->floating->IsKeepAbove());
      gtk_window_set_transient_for(GTK_WINDOW(toplevel),
                                   GTK_WINDOW(info->floating->GetWindow()));
    } else {
      // Close all other details/popout view of sidebar.
      CloseAllPopOutWindowsOfSideBar(gadget_id);
      view_host->SetKeepAbove(always_on_top_);
      gtk_window_set_transient_for(GTK_WINDOW(toplevel),
                                   GTK_WINDOW(sidebar_window_));
    }
    info->popout = view_host;

    return decorated_view_host;
  }

  void LoadGadgetOptions(Gadget *gadget) {
    OptionsInterface *opt = gadget->GetOptions();
    Variant value = opt->GetInternalValue(kOptionDisplayTarget);
    int target;
    // Load gadget into a floating view host if sidebar is closed.
    if (closed_ ||
        (value.ConvertToInt(&target) && target == Gadget::TARGET_FLOATING_VIEW))
      gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    else  // default value is TARGET_SIDEBAR
      gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    value = opt->GetInternalValue(kOptionPositionInSideBar);
    int temp_int = 0;
    if (value.ConvertToInt(&temp_int)) {
      gadgets_[gadget->GetInstanceID()].index_in_sidebar =
          static_cast<size_t>(std::max(0, temp_int));
    }
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    // Options view host can be created without a gadget.
    if (type == ViewHostInterface::VIEW_HOST_OPTIONS) {
      // No decorator for options view.
      int vh_flags = SingleViewHost::DECORATED | SingleViewHost::WM_MANAGEABLE;
      return new SingleViewHost(type, 1.0, vh_flags, view_debug_mode_);
    }

    if (!gadget) return NULL;
    int gadget_id = gadget->GetInstanceID();
    GadgetInfo *info = &gadgets_[gadget_id];
    ASSERT(info->gadget == NULL || info->gadget == gadget);
    info->gadget = gadget;

    if (type == ViewHostInterface::VIEW_HOST_MAIN) {
      // Make sure it's the initial loading.
      ASSERT(info->main_decorator == NULL);
      ASSERT(info->details == NULL);
      ASSERT(info->floating == NULL);
      ASSERT(info->popout == NULL);
      LoadGadgetOptions(gadget);
      if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
        return NewDockedMainViewHost(gadget_id);
      } else {
        return NewFloatingMainViewHost(gadget_id);
      }
    } else if (type == ViewHostInterface::VIEW_HOST_DETAILS) {
      return NewDetailsViewHost(gadget_id);
    }
    return NULL;
  }

  void RemoveGadget(GadgetInterface *gadget, bool save_data) {
    GGL_UNUSED(save_data);
    ASSERT(gadget);
    int id = gadget->GetInstanceID();
    // If RemoveGadgetInstance() returns false, then means this instance is not
    // installed by gadget manager.
    if (!gadget_manager_->RemoveGadgetInstance(id))
      RemoveGadgetInstanceCallback(id);
  }

  void SaveGadgetDisplayTargetInfo(const GadgetInfo &info) {
    OptionsInterface *opt = info.gadget->GetOptions();
    opt->PutInternalValue(kOptionDisplayTarget,
                          Variant(info.gadget->GetDisplayTarget()));
    opt->PutInternalValue(kOptionPositionInSideBar,
                          Variant(info.index_in_sidebar));
  }

  void RemoveGadgetInstanceCallback(int instance_id) {
    GadgetInfoMap::iterator it = gadgets_.find(instance_id);
    if (it != gadgets_.end()) {
      CloseDetailsView(instance_id);
      OnMainViewPopIn(instance_id);
      SaveGadgetDisplayTargetInfo(it->second);
      if (it->second.debug_console)
        gtk_widget_destroy(it->second.debug_console);
      delete it->second.gadget;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }
  }

  // handlers for menu items
  void FloatingGadgetMenuHandler(const char *str, Gadget *gadget) {
    GGL_UNUSED(str);
    gadget->ShowMainView();
  }

  void AddGadgetMenuHandler(const char *str) {
    GGL_UNUSED(str);
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  void AddIGoogleGadgetMenuHandler(const char *str) {
    GGL_UNUSED(str);
    gadget_manager_->NewGadgetInstanceFromFile(kIGoogleGadgetName);
  }

  void ShowAllMenuHandler(const char *str) {
    GGL_UNUSED(str);
    ShowOrHideAll(true);
  }

  void HideAllMenuHandler(const char *str) {
    GGL_UNUSED(str);
    ShowOrHideAll(false);
  }

  void AutoHideMenuHandler(const char *str) {
    GGL_UNUSED(str);
    auto_hide_ = !auto_hide_;
    options_->PutInternalValue(kOptionAutoHide, Variant(auto_hide_));

    // FIXME:
    // always on top if auto hide is chosen. Since the sidebar could not
    // "autoshow" if it is not always on top
    if (auto_hide_) {
      always_on_top_ = true;
      options_->PutInternalValue(kOptionAlwaysOnTop, Variant(always_on_top_));
    }

    ShowOrHideSideBar(true);
    AdjustSideBar();
  }

  void AlwaysOnTopMenuHandler(const char *str) {
    GGL_UNUSED(str);
    always_on_top_ = !always_on_top_;
    options_->PutInternalValue(kOptionAlwaysOnTop,
                               Variant(always_on_top_));

    // FIXME:
    // uncheck auto hide too if "always on top" is unchecked.
    if (!always_on_top_) {
      auto_hide_ = false;
      options_->PutInternalValue(kOptionAutoHide, Variant(auto_hide_));
    }

    ShowOrHideSideBar(true);
    AdjustSideBar();
  }

  void ChangeHotKeyMenuHandler(const char *) {
    safe_to_exit_ = false;
    HotKeyDialog dialog;
    dialog.SetHotKey(hotkey_grabber_.GetHotKey());
    hotkey_grabber_.SetEnableGrabbing(false);
    if (dialog.Show()) {
      std::string hotkey = dialog.GetHotKey();
      hotkey_grabber_.SetHotKey(hotkey);
      // The hotkey will not be enabled if it's invalid.
      hotkey_grabber_.SetEnableGrabbing(true);
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
      UpdateStatusIconTooltip();
#endif
    }
    safe_to_exit_ = true;
  }

  void SideBarPositionMenuHandler(const char *str, int position) {
    GGL_UNUSED(str);
    CloseAllPopOutWindowsOfSideBar(-1);
    sidebar_position_ = position;
    options_->PutInternalValue(kOptionPosition,
                               Variant(sidebar_position_));
    if (!sidebar_shown_)
      ShowOrHideSideBar(true);
    else
      AdjustSideBar();
  }

  void OpenCloseSidebarMenuHandler(const char *str) {
    GGL_UNUSED(str);
    closed_ = !closed_;
    ShowOrHideSideBar(!closed_);
  }

  void OnThemeChanged() {
    SimpleEvent event(Event::EVENT_THEME_CHANGED);
    sidebar_->GetSideBarViewHost()->GetView()->OnOtherEvent(event);
    for (GadgetInfoMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      it->second.main_decorator->GetView()->OnOtherEvent(event);
      if (it->second.details)
        it->second.details->GetView()->OnOtherEvent(event);
      if (it->second.popout)
        it->second.popout->GetView()->OnOtherEvent(event);
    }
  }

  void FontSizeMenuHandler(const char *str, int delta) {
    GGL_UNUSED(str);
    int new_font_size;
    if (delta == 0) {
      new_font_size = kDefaultFontSize;
    } else {
      new_font_size = std::min(std::max(font_size_ + delta, kMinFontSize),
                               kMaxFontSize);
    }
    if (new_font_size != font_size_) {
      font_size_ = new_font_size;
      options_->PutInternalValue(kOptionFontSize, Variant(font_size_));
      OnThemeChanged();
    }
  }

  void AboutMenuHandler(const char *str) {
    GGL_UNUSED(str);
    safe_to_exit_ = false;
    ShowAboutDialog(owner_);
    safe_to_exit_ = true;
  }

  void ExitMenuHandler(const char *str) {
    GGL_UNUSED(str);
    owner_->Exit();
  }

  void LoadGadgets() {
    sidebar_->SetInitializing(true);
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::EnumerateGadgetInstancesCallback));
    sidebar_->SetInitializing(false);
  }

  bool ShouldHideSideBar() const {
    // first check if the cursor is in sidebar
    int size_x, size_y, x, y;
    gtk_widget_get_pointer(sidebar_window_, &x, &y);
    sidebar_host_->GetWindowSize(&size_x, &size_y);
    if (x >= 0 && y >= 0 && x <= size_x && y <= size_y)
      return false;

    // second check if the focus is given to the popout window
    bool result = true;
    GList *toplevels = gtk_window_list_toplevels();
    for (GList *i = toplevels; i; i = i->next) {
      if (gtk_window_is_active(GTK_WINDOW(i->data))) {
        result = false;
        break;
      }
    }
    g_list_free(toplevels);
    return result;
  }

  // gtk call-backs
  static gboolean ToplevelWindowFocusOutHandler(
      GtkWidget *widget, GdkEventFocus *event, Impl *impl) {
    GGL_UNUSED(event);
    DLOG("ToplevelWindowFocusOutHandler %d", widget == impl->sidebar_window_);
    if (impl->auto_hide_ && !impl->sidebar_->IsMinimized() &&
        impl->auto_hide_source_ == 0) {
      impl->auto_hide_source_ =
        g_timeout_add(kAutoHideTimeout, SideBarAutoHideTimeoutHandler, impl);
    }
    return FALSE;
  }

  static gboolean SideBarAutoHideTimeoutHandler(gpointer user_data) {
    DLOG("SideBarAutoHideTimeoutHandler");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->auto_hide_source_ = 0;
    if (impl->auto_hide_ && !impl->sidebar_->IsMinimized() &&
        impl->ShouldHideSideBar())
      impl->ShowOrHideSideBar(false);
    return FALSE;
  }

  static gboolean ToplevelWindowFocusInHandler(
      GtkWidget *widget, GdkEventFocus *event, Impl *impl) {
    GGL_UNUSED(event);
    DLOG("ToplevelWindowFocusInHandler %d", widget == impl->sidebar_window_);
    if (impl->auto_hide_source_) {
      g_source_remove(impl->auto_hide_source_);
      impl->auto_hide_source_ = 0;
    }
    return FALSE;
  }

  static gboolean SideBarEnterNotifyHandler(
      GtkWidget *widget, GdkEventCrossing *event, Impl *impl) {
    GGL_UNUSED(widget);
    GGL_UNUSED(event);
    DLOG("SideBarEnterNotifyHandler");
    if (impl->auto_hide_source_) {
      g_source_remove(impl->auto_hide_source_);
      impl->auto_hide_source_ = 0;
    }
    if (impl->auto_hide_ && impl->sidebar_->IsMinimized() &&
        impl->auto_show_source_ == 0)
      impl->auto_show_source_ =
          g_timeout_add(kAutoShowTimeout, SideBarAutoShowTimeoutHandler, impl);
    return FALSE;
  }

  static gboolean SideBarAutoShowTimeoutHandler(gpointer user_data) {
    DLOG("SideBarAutoShowTimeoutHandler");
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->auto_show_source_ = 0;
    impl->ShowOrHideSideBar(true);
    return FALSE;
  }

  static gboolean SideBarLeaveNotifyHandler(
      GtkWidget *widget, GdkEventCrossing *event, Impl *impl) {
    GGL_UNUSED(widget);
    GGL_UNUSED(event);
    DLOG("SideBarLeaveNotifyHandler");
    if (impl->auto_hide_ && !impl->sidebar_->IsMinimized() &&
        !gtk_window_is_active(GTK_WINDOW(impl->sidebar_window_)) &&
        impl->auto_hide_source_ == 0 && impl->auto_show_source_ == 0) {
      impl->auto_hide_source_ =
        g_timeout_add(kAutoHideTimeout, SideBarAutoHideTimeoutHandler, impl);
    }
    return FALSE;
  }

  static gboolean DragObserverMotionNotifyHandler(
      GtkWidget *widget, GdkEventMotion *event, Impl *impl) {
    GGL_UNUSED(widget);
    GGL_UNUSED(event);
    if (impl->sidebar_moving_) {
      impl->OnSideBarMove();
    } else if (impl->dragging_gadget_) {
      impl->OnMainViewMove(impl->dragging_gadget_->GetInstanceID());
    }
    return FALSE;
  }

  static gboolean DragObserverButtonReleaseHandler(
      GtkWidget *widget, GdkEventMotion *event, Impl *impl) {
    GGL_UNUSED(widget);
    gdk_pointer_ungrab(event->time);
    if (impl->sidebar_moving_) {
      impl->OnSideBarEndMove();
    } else {
      ASSERT(impl->dragging_gadget_);
      impl->OnMainViewEndMove(impl->dragging_gadget_->GetInstanceID());
    }
    return FALSE;
  }

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  static void StatusIconActivateHandler(GtkWidget *widget, Impl *impl) {
    GGL_UNUSED(widget);
    if (!impl->gadgets_shown_ ||
        (!impl->closed_ && impl->sidebar_->IsMinimized()))
      impl->ShowOrHideAll(true);
    else
      impl->ShowOrHideAll(false);
  }

  static void StatusIconPopupMenuHandler(GtkWidget *widget, guint button,
                                         guint activate_time, Impl *impl) {
    GGL_UNUSED(widget);
    if (impl->status_icon_menu_)
      gtk_widget_destroy(impl->status_icon_menu_);

    impl->status_icon_menu_ = gtk_menu_new();
    MenuBuilder menu_builder(NULL, GTK_MENU_SHELL(impl->status_icon_menu_));

    impl->OnSideBarMenu(&menu_builder);
    gtk_menu_popup(GTK_MENU(impl->status_icon_menu_), NULL, NULL,
                   gtk_status_icon_position_menu, impl->status_icon_,
                   button, activate_time);
  }
#endif

  void ShowGadgetDebugConsole(GadgetInterface *gadget) {
    if (!gadget)
      return;
    GadgetInfoMap::iterator it = gadgets_.find(gadget->GetInstanceID());
    if (it == gadgets_.end())
      return;
    if (it->second.debug_console) {
      DLOG("Gadget has already opened a debug console: %p",
           it->second.debug_console);
      return;
    }
    it->second.debug_console = NewGadgetDebugConsole(gadget);
    g_signal_connect(it->second.debug_console, "destroy",
                     G_CALLBACK(gtk_widget_destroyed),
                     &it->second.debug_console);
  }

  bool IsSafeToExit() {
    if (!safe_to_exit_)
      return false;
    GadgetInfoMap::const_iterator it = gadgets_.begin();
    GadgetInfoMap::const_iterator end = gadgets_.end();
    for (; it != end; ++it) {
      if (!it->second.gadget->IsSafeToRemove())
        return false;
    }
    return true;
  }

  void OnGoogleIconClicked() {
    owner_->OpenURL(NULL, GM_("GOOGLE_HOMEPAGE_URL"));
  }

 public:  // members
  GadgetBrowserHost gadget_browser_host_;

  typedef LightMap<int, GadgetInfo> GadgetInfoMap;
  GadgetInfoMap gadgets_;

  SideBarGtkHost *owner_;

  bool sidebar_shown_;
  bool gadgets_shown_;

  int flags_;
  int view_debug_mode_;
  Gadget::DebugConsoleConfig debug_console_config_;

  SingleViewHost *sidebar_host_;
  Gadget *dragging_gadget_;
  GtkWidget *drag_observer_;
  GdkRectangle workarea_;

  double dragging_offset_x_;
  double dragging_offset_y_;
  bool   sidebar_moving_;
  bool   sidebar_resizing_;

  bool has_strut_;

  SideBar *sidebar_;

  OptionsInterface *options_;
  bool auto_hide_;
  bool always_on_top_;
  bool closed_;
  bool safe_to_exit_;
  int  font_size_;
  int  sidebar_monitor_;
  int  sidebar_position_;
  int  sidebar_width_;

  int auto_hide_source_;
  int auto_show_source_;

  GdkAtom net_wm_strut_;
  GdkAtom net_wm_strut_partial_;

  GadgetManagerInterface *gadget_manager_;
  Connection *on_new_gadget_instance_connection_;
  Connection *on_remove_gadget_instance_connection_;

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  GtkStatusIcon *status_icon_;
  GtkWidget *status_icon_menu_;
#endif
  GtkWidget *sidebar_window_;

  HotKeyGrabber hotkey_grabber_;
  Permissions global_permissions_;
};

SideBarGtkHost::SideBarGtkHost(const char *options,
                               int flags, int view_debug_mode,
                               Gadget::DebugConsoleConfig debug_console_config)
  : impl_(new Impl(this, options, flags, view_debug_mode,
                   debug_console_config)) {
  impl_->SetupUI();
  impl_->LoadGadgets();
#if !GTK_CHECK_VERSION(2,10,0) || !defined(GGL_HOST_LINUX)
  impl_->sidebar_host_->ShowView(false, 0, NULL);
#endif
  impl_->ShowOrHideSideBar(impl_->gadgets_shown_ && !impl_->closed_);
}

SideBarGtkHost::~SideBarGtkHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SideBarGtkHost::NewViewHost(GadgetInterface *gadget,
                                               ViewHostInterface::Type type) {
  ASSERT(!gadget || gadget->IsInstanceOf(Gadget::TYPE_ID));
  return impl_->NewViewHost(down_cast<Gadget*>(gadget), type);
}

GadgetInterface *SideBarGtkHost::LoadGadget(const char *path,
                                            const char *options_name,
                                            int instance_id,
                                            bool show_debug_console) {
  return impl_->LoadGadget(path, options_name, instance_id, show_debug_console);
}

void SideBarGtkHost::RemoveGadget(GadgetInterface *gadget, bool save_data) {
  return impl_->RemoveGadget(gadget, save_data);
}

void SideBarGtkHost::ShowGadgetDebugConsole(GadgetInterface *gadget) {
  impl_->ShowGadgetDebugConsole(gadget);
}

int SideBarGtkHost::GetDefaultFontSize() {
  return impl_->font_size_;
}

bool SideBarGtkHost::IsSafeToExit() const {
  return impl_->IsSafeToExit();
}

} // namespace gtk
} // namespace hosts
