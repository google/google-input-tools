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

#ifndef HOSTS_GTK_GTK_HOST_BASE_H__
#define HOSTS_GTK_GTK_HOST_BASE_H__

#include <string>
#include <ggadget/gadget.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/host_interface.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/signals.h>

namespace hosts {
namespace gtk {

using ggadget::Connection;
using ggadget::Gadget;
using ggadget::GadgetInterface;
using ggadget::HostInterface;
using ggadget::Permissions;

class GtkHostBase : public ggadget::HostInterface {
 public:
  enum Flags {
    NONE                   = 0,
    WINDOW_MANAGER_BORDER  = 0x01,  // Enables window manager's decoration.
    NO_MAIN_VIEW_DECORATOR = 0x02,  // Disables builtin main view decoration.
    MATCHBOX_WORKAROUND    = 0x04,  // Workaround matchbox compatibility issue.
    NO_TRANSPARENT         = 0x08,  // Does not use transparent background.
    GRANT_PERMISSIONS      = 0x10   // Grants permissions to gadgets by default.
  };

  GtkHostBase() {
  }

  virtual bool LoadFont(const char *filename) {
    return ggadget::gtk::LoadFont(filename);
  }
  virtual bool OpenURL(const GadgetInterface *gadget, const char *url) {
    return ggadget::gtk::OpenURL(gadget, url);
  }

 public:
  virtual bool IsSafeToExit() const {
    return true;
  }

  void Exit() {
    if (IsSafeToExit())
      on_exit_signal_();
  }

  Connection *ConnectOnExit(ggadget::Slot0<void> *callback) {
    return on_exit_signal_.Connect(callback);
  }

  static int FlagsToViewHostFlags(int flags);

 protected:
  bool ConfirmGadget(const std::string &path, const std::string &options_name,
                     const std::string &download_url, const std::string &title,
                     const std::string &description, Permissions *permissions);

  bool ConfirmManagedGadget(int id, bool grant);

 private:
  ggadget::Signal0<void> on_exit_signal_;

  DISALLOW_EVIL_CONSTRUCTORS(GtkHostBase);
};

} // namespace gtk
} // namespace hosts

#endif // HOSTS_GTK_GTK_HOST_BASE_H__
