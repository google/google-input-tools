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

#include <gtk/gtk.h>
#include <string>

#include "standalone_gtk_host.h"

#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/floating_main_view_decorator.h>
#include <ggadget/popout_main_view_decorator.h>
#include <ggadget/details_view_decorator.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/menu_builder.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/gtk/hotkey.h>
#include <ggadget/locales.h>
#include <ggadget/messages.h>
#include <ggadget/logger.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/view.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/options_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/permissions.h>
#include <ggadget/digest_utils.h>

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

static const char kPermissionConfirmedOption[] = "permission_confirmed";

class StandaloneGtkHost::Impl {
 public:
  Impl(StandaloneGtkHost *owner, int flags, int view_debug_mode,
       Gadget::DebugConsoleConfig debug_console_config)
    : owner_(owner),
      gadget_(NULL),
      main_view_host_(NULL),
      details_view_host_(NULL),
      debug_console_(NULL),
      details_on_right_(false),
      safe_to_exit_(true),
      flags_(flags),
      view_debug_mode_(view_debug_mode),
      debug_console_config_(debug_console_config) {
  }

  ~Impl() {
    if (debug_console_)
      gtk_widget_destroy(debug_console_);
    delete gadget_;
  }

  bool InitFailed(const std::string &gadget_path) {
    safe_to_exit_ = false;
    ShowAlertDialog(GM_("GOOGLE_GADGETS"),
                    StringPrintf(GM_("GADGET_LOAD_FAILURE"),
                                 gadget_path.c_str()).c_str());
    safe_to_exit_ = true;
    owner_->Exit();
    return false;
  }

  bool Init(const std::string &gadget_path) {
    StringMap manifest;
    if (!Gadget::GetGadgetManifest(gadget_path.c_str(), &manifest)) {
      return InitFailed(gadget_path);
    }

    StringMap::const_iterator id_it = manifest.find(kManifestId);
    if (id_it == manifest.end() || id_it->second.empty()) {
      return InitFailed(gadget_path);
    }

    std::string options_name, options_name_sha1;
    GenerateSHA1(gadget_path + "-" + id_it->second, &options_name_sha1);
    WebSafeEncodeBase64(options_name_sha1, false, &options_name);
    options_name = "standalone-" + options_name;

    Permissions permissions;
    Gadget::GetGadgetRequiredPermissions(&manifest, &permissions);

    if (flags_ & GRANT_PERMISSIONS)
      permissions.GrantAllRequired();

    safe_to_exit_ = false;
    if (!owner_->ConfirmGadget(gadget_path, options_name, gadget_path,
                               manifest[kManifestName],
                               manifest[kManifestDescription], &permissions)) {
      safe_to_exit_ = true;
      return false;
    }
    safe_to_exit_ = true;

    if (!LoadGadget(gadget_path.c_str(), options_name.c_str(), 0, false)) {
      return InitFailed(gadget_path);
    }
    return true;
  }

  GadgetInterface *LoadGadget(const char *path,
                              const char *options_name,
                              int instance_id,
                              bool show_debug_console) {
    if (gadget_) {
      DLOG("Standalone gadget has been loaded. "
           "Load gadget %s in managed host.", path);
      return on_load_gadget_signal_(path, options_name, instance_id,
                                    show_debug_console);
    }

    Permissions global_permissions;
    global_permissions.SetGranted(Permissions::ALL_ACCESS, true);

    Gadget::DebugConsoleConfig dcc = show_debug_console ?
        Gadget::DEBUG_CONSOLE_INITIAL : debug_console_config_;

    safe_to_exit_ = false;
    gadget_ = new Gadget(owner_, path, options_name, instance_id,
                         global_permissions, dcc);
    safe_to_exit_ = true;

    if (!gadget_->IsValid()) {
      LOG("Failed to load standalone gadget %s", path);
      if (debug_console_)
        gtk_widget_destroy(debug_console_);
      delete gadget_;
      gadget_ = NULL;
      main_view_host_ = NULL;
      details_view_host_ = NULL;
      debug_console_ = NULL;
      return NULL;
    }

    gadget_->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    gadget_->GetMainView()->OnOtherEvent(SimpleEvent(Event::EVENT_UNDOCK));
    gadget_->ShowMainView();

    // If debug console is opened during view host creation, the title is
    // not set then because main view is not available. Set the title now.
    if (debug_console_) {
      gtk_window_set_title(GTK_WINDOW(debug_console_),
                           gadget_->GetMainView()->GetCaption().c_str());
    }
    return gadget_;
  }

  ViewHostInterface *NewViewHost(GadgetInterface *gadget,
                                 ViewHostInterface::Type type) {
    GGL_UNUSED(gadget);
    int vh_flags = GtkHostBase::FlagsToViewHostFlags(flags_);
    if (type == ViewHostInterface::VIEW_HOST_OPTIONS) {
      vh_flags |= (SingleViewHost::DECORATED | SingleViewHost::WM_MANAGEABLE);
    } else if (type == ViewHostInterface::VIEW_HOST_DETAILS) {
      vh_flags &= ~SingleViewHost::DECORATED;
    } else if (type == ViewHostInterface::VIEW_HOST_MAIN) {
      // Standalone gadget will always be removed if the window is closed.
      vh_flags |= SingleViewHost::REMOVE_ON_CLOSE;
      // Standalone gadget will always be managed by window manager.
      vh_flags |= SingleViewHost::WM_MANAGEABLE;
      vh_flags |= SingleViewHost::RECORD_STATES;
    }

    SingleViewHost *svh =
        new SingleViewHost(type, 1.0, vh_flags, view_debug_mode_);

    if (type == ViewHostInterface::VIEW_HOST_OPTIONS)
      return svh;

    ViewHostInterface *view_host = NULL;
    if (type == ViewHostInterface::VIEW_HOST_MAIN) {
      main_view_host_ = svh;
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnMainViewResizedHandler));
      svh->ConnectOnMoved(
          NewSlot(this, &Impl::OnMainViewMovedHandler));

      if (flags_ & NO_MAIN_VIEW_DECORATOR) {
        view_host = svh;
      } else {
        FloatingMainViewDecorator *view_decorator =
            new FloatingMainViewDecorator(svh, !(flags_ & NO_TRANSPARENT));
        view_host = new DecoratedViewHost(view_decorator);
        view_decorator->SetButtonVisible(
            MainViewDecoratorBase::POP_IN_OUT_BUTTON, false);
        view_decorator->ConnectOnClose(
            NewSlot(this, &Impl::OnMainViewCloseHandler));
      }
    } else {
      details_view_host_ = svh;
      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnDetailsViewShowHideHandler));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::OnDetailsViewBeginResizeHandler));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnDetailsViewResizedHandler));
      svh->ConnectOnBeginMoveDrag(
          NewSlot(this, &Impl::OnDetailsViewBeginMoveHandler));

      DetailsViewDecorator *view_decorator = new DetailsViewDecorator(svh);
      view_host = new DecoratedViewHost(view_decorator);
      view_decorator->ConnectOnClose(
          NewSlot(this, &Impl::OnDetailsViewCloseHandler));
    }

    return view_host;
  }

  void RemoveGadget(GadgetInterface *gadget, bool save_data) {
    GGL_UNUSED(gadget);
    GGL_UNUSED(save_data);
    ASSERT(gadget && gadget == gadget_);
    owner_->Exit();
  }

  void OnMainViewCloseHandler() {
    ASSERT(gadget_);
    gadget_->RemoveMe(true);
  }

  void OnDetailsViewCloseHandler() {
    ASSERT(gadget_);
    gadget_->CloseDetailsView();
  }

  void AdjustViewHostPosition() {
    ASSERT(gadget_ && main_view_host_);

    int x, y;
    int width, height;
    main_view_host_->GetWindowPosition(&x, &y);
    main_view_host_->GetWindowSize(&width, &height);

    int screen_width = gdk_screen_get_width(
        gtk_widget_get_screen(main_view_host_->GetWindow()));
    int screen_height = gdk_screen_get_height(
        gtk_widget_get_screen(main_view_host_->GetWindow()));

    if (details_view_host_ && details_view_host_->IsVisible()) {
      int details_width, details_height;
      details_view_host_->GetWindowSize(&details_width, &details_height);
      if (details_on_right_ && details_width < x &&
          x + width + details_width > screen_width)
        details_on_right_ = false;
      else if (!details_on_right_ && details_width > x &&
               x + width + details_width < screen_width)
        details_on_right_ = true;

      if (y + details_height > screen_height)
        y = screen_height - details_height;

      if (details_on_right_) {
        details_view_host_->SetWindowPosition(x + width, y);
      } else {
        details_view_host_->SetWindowPosition(x - details_width, y);
      }
    }
  }

  void OnMainViewResizedHandler(int width, int height) {
    GGL_UNUSED(width);
    GGL_UNUSED(height);
    AdjustViewHostPosition();
  }

  void OnMainViewMovedHandler(int x, int y) {
    GGL_UNUSED(x);
    GGL_UNUSED(y);
    AdjustViewHostPosition();
  }

  void OnDetailsViewShowHideHandler(bool show) {
    if (show)
      AdjustViewHostPosition();
    else
      details_view_host_ = NULL;
  }

  bool OnDetailsViewBeginResizeHandler(int button, int hittest) {
    GGL_UNUSED(button);
    if (details_on_right_) {
      return hittest == ViewInterface::HT_LEFT ||
             hittest == ViewInterface::HT_TOPLEFT ||
             hittest == ViewInterface::HT_BOTTOMLEFT ||
             hittest == ViewInterface::HT_TOP ||
             hittest == ViewInterface::HT_TOPRIGHT;
    } else {
      return hittest == ViewInterface::HT_RIGHT ||
             hittest == ViewInterface::HT_TOPRIGHT ||
             hittest == ViewInterface::HT_BOTTOMRIGHT ||
             hittest == ViewInterface::HT_TOP ||
             hittest == ViewInterface::HT_TOPLEFT;
    }
  }

  void OnDetailsViewResizedHandler(int width, int height) {
    GGL_UNUSED(width);
    GGL_UNUSED(height);
    AdjustViewHostPosition();
  }

  bool OnDetailsViewBeginMoveHandler(int button) {
    GGL_UNUSED(button);
    // User can't move popout view window.
    return true;
  }

  void ShowGadgetDebugConsole(GadgetInterface *gadget) {
    ASSERT(gadget && (!gadget_ || gadget == gadget_));

    if (!debug_console_) {
      debug_console_ = NewGadgetDebugConsole(gadget);
      g_signal_connect(debug_console_, "destroy",
                       G_CALLBACK(gtk_widget_destroyed), &debug_console_);
    }
  }

  StandaloneGtkHost *owner_;
  Gadget *gadget_;
  SingleViewHost *main_view_host_;
  SingleViewHost *details_view_host_;
  GtkWidget *debug_console_;

  bool details_on_right_;
  bool safe_to_exit_;
  int flags_;
  int view_debug_mode_;
  Gadget::DebugConsoleConfig debug_console_config_;
  std::string options_name_;

  Signal4<GadgetInterface *, const char *, const char *, int, bool>
      on_load_gadget_signal_;
};

StandaloneGtkHost::StandaloneGtkHost(
    int flags, int view_debug_mode,
    Gadget::DebugConsoleConfig debug_console_config)
  : impl_(new Impl(this, flags, view_debug_mode, debug_console_config)) {
}

StandaloneGtkHost::~StandaloneGtkHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *StandaloneGtkHost::NewViewHost(
    GadgetInterface *gadget, ViewHostInterface::Type type) {
  return impl_->NewViewHost(gadget, type);
}

GadgetInterface *StandaloneGtkHost::LoadGadget(
    const char *path, const char *options_name,
    int instance_id, bool show_debug_console) {
  return impl_->LoadGadget(path, options_name, instance_id, show_debug_console);
}

void StandaloneGtkHost::RemoveGadget(GadgetInterface *gadget, bool save_data) {
  return impl_->RemoveGadget(gadget, save_data);
}

void StandaloneGtkHost::ShowGadgetDebugConsole(GadgetInterface *gadget) {
  impl_->ShowGadgetDebugConsole(gadget);
}

int StandaloneGtkHost::GetDefaultFontSize() {
  return kDefaultFontSize;
}

bool StandaloneGtkHost::Init(const std::string &gadget_path) {
  return impl_->Init(gadget_path);
}

void StandaloneGtkHost::Present() {
  if (impl_->main_view_host_)
    gtk_window_present(GTK_WINDOW(impl_->main_view_host_->GetWindow()));
}

Connection *StandaloneGtkHost::ConnectOnLoadGadget(
    ggadget::Slot4<GadgetInterface *, const char *, const char *, int, bool> *
    slot) {
  return impl_->on_load_gadget_signal_.Connect(slot);
}

bool StandaloneGtkHost::IsSafeToExit() const {
  return impl_->safe_to_exit_ &&
      (!impl_->gadget_ || impl_->gadget_->IsSafeToRemove());
}

} // namespace gtk
} // namespace hosts
