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

#include "gtk_host_base.h"

#include <ggadget/common.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/locales.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/permissions.h>
#include <ggadget/string_utils.h>

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

static bool GetPermissionsDescriptionCallback(int permission,
                                              std::string *msg) {
  if (msg->length())
    msg->append("\n");
  msg->append("  ");
  msg->append(Permissions::GetDescription(permission));
  return true;
}

static bool ShowPermissionsConfirmDialog(const std::string &download_url,
                                         const std::string &title,
                                         const std::string &description,
                                         Permissions *permissions) {
  // Get required permissions description.
  std::string permissions_msg;
  permissions->EnumerateAllRequired(
      NewSlot(GetPermissionsDescriptionCallback, &permissions_msg));

  GtkWidget *dialog = gtk_message_dialog_new(
      NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
      "%s\n\n%s\n%s\n\n%s%s\n\n%s\n%s",
      GM_("GADGET_CONFIRM_MESSAGE"), title.c_str(), download_url.c_str(),
      GM_("GADGET_DESCRIPTION"), description.c_str(),
      GM_("GADGET_REQUIRED_PERMISSIONS"), permissions_msg.c_str());

  GdkScreen *screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen,
                          NULL, NULL, NULL);
  gtk_window_set_screen(GTK_WINDOW(dialog), screen);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(dialog), GM_("GADGET_CONFIRM_TITLE"));
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), false);
  gtk_window_present(GTK_WINDOW(dialog));
  gtk_window_set_urgency_hint(GTK_WINDOW(dialog), true);
  gint result = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return result == GTK_RESPONSE_YES;
}

bool GtkHostBase::ConfirmGadget(const std::string &path,
                                const std::string &options_name,
                                const std::string &download_url,
                                const std::string &title,
                                const std::string &description,
                                Permissions *permissions) {
  GGL_UNUSED(path);
  // If the permissions was already saved and there is no ungranted permission,
  // just returns true.
  if (Gadget::LoadGadgetInitialPermissions(options_name.c_str(), permissions) &&
      !permissions->HasUngranted()) {
    return true;
  }

  if (permissions->HasUngranted()) {
    if (!ShowPermissionsConfirmDialog(download_url, title,
                                      description, permissions)) {
      return false;
    }
    // TODO: Is it necessary to let user grant individual permissions
    // separately?
    permissions->GrantAllRequired();
  }

  Gadget::SaveGadgetInitialPermissions(options_name.c_str(), *permissions);
  return true;
}

bool GtkHostBase::ConfirmManagedGadget(int id, bool grant) {
  GadgetManagerInterface *gadget_manager = GetGadgetManager();
  std::string options_name = gadget_manager->GetGadgetInstanceOptionsName(id);
  Permissions permissions;

  // If the permissions was already saved and there is no ungranted permission,
  // just returns true.
  if (Gadget::LoadGadgetInitialPermissions(options_name.c_str(),
                                           &permissions) &&
      !permissions.HasUngranted()) {
    return true;
  }

  std::string path = gadget_manager->GetGadgetInstancePath(id);
  std::string download_url, title, description;
  if (!gadget_manager->GetGadgetInstanceInfo(id, GetSystemLocaleName().c_str(),
                                             NULL, &download_url,
                                             &title, &description) ||
      !gadget_manager->GetGadgetDefaultPermissions(id, &permissions)) {
    ShowAlertDialog(GM_("GOOGLE_GADGETS"), StringPrintf(
        GM_("GADGET_LOAD_FAILURE"), path.c_str()).c_str());
    return false;
  }

  if (grant) {
    permissions.GrantAllRequired();
  }

  if (permissions.HasUngranted()) {
    if (!ShowPermissionsConfirmDialog(download_url, title,
                                      description, &permissions)) {
      // User didn't grant the permissions.
      return false;
    }
    permissions.GrantAllRequired();
  }

  Gadget::SaveGadgetInitialPermissions(options_name.c_str(), permissions);
  return true;
}

int GtkHostBase::FlagsToViewHostFlags(int flags) {
  int vh_flags = SingleViewHost::DEFAULT;

  if (flags & WINDOW_MANAGER_BORDER)
    vh_flags |= SingleViewHost::DECORATED;
  if (flags & MATCHBOX_WORKAROUND)
    vh_flags |= SingleViewHost::DIALOG_TYPE_HINT;
  if (flags & NO_TRANSPARENT)
    vh_flags |= SingleViewHost::OPAQUE_BACKGROUND;
  return vh_flags;
}

} // namespace gtk
} // namespace hosts
