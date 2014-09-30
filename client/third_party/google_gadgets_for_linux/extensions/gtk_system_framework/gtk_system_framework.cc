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

#include <unistd.h>
#include <vector>
#include <gtk/gtk.h>
#include <ggadget/common.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/framework_interface.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/xdg/desktop_entry.h>
#include <ggadget/xdg/utilities.h>

#define Initialize gtk_system_framework_LTX_Initialize
#define Finalize gtk_system_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    gtk_system_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace gtk_system_framework {

static const char kFileBrowserFolderOption[] = "file_browser_folder";

class GtkSystemCursor : public CursorInterface {
 public:
  virtual void GetPosition(int *x, int *y)  {
    gint px, py;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &px, &py, NULL);
    if (x) *x = px;
    if (y) *y = py;
  }
};

class GtkSystemScreen : public ScreenInterface {
 public:
  virtual void GetSize(int *width, int *height) {
    GdkScreen *screen = NULL;
    gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);

    if (width) *width = gdk_screen_get_width(screen);
    if (height) *height = gdk_screen_get_height(screen);
  }
};

static const Variant kBrowseForFilesDefaultArgs[] = {
  Variant(),  // filter
  Variant(static_cast<const char *>(NULL)),  // title
  Variant(BROWSE_FILE_MODE_OPEN)  // mode
};

static const Variant kBrowseForFileDefaultArgs[] = {
  Variant(),  // filter
  Variant(static_cast<const char *>(NULL)),  // title
  Variant(BROWSE_FILE_MODE_OPEN),  // mode
  Variant(static_cast<const char *>(NULL))  // default_name
};

class GtkSystemBrowseForFileHelper {
 public:
  GtkSystemBrowseForFileHelper(ScriptableInterface *framework,
                               GadgetInterface *gadget)
    : gadget_(gadget) {
    framework->ConnectOnReferenceChange(
      NewSlot(this, &GtkSystemBrowseForFileHelper::OnFrameworkRefChange));
  }

  // Function to destroy the helper object when framework is destroyed.
  void OnFrameworkRefChange(int ref, int change) {
    GGL_UNUSED(ref);
    if (change == 0) {
      DLOG("Framework destroyed, delete GtkSystemBrowseForFileHelper object.");
      delete this;
    }
  }

  std::string BrowseForFile(const char *filter, const char *title,
                            BrowseForFileMode mode, const char *default_name) {
    std::string result;
    std::vector<std::string> files;
    if (BrowseForFilesImpl(filter, false, title, mode, default_name, &files) &&
        files.size() > 0)
      result = files[0];
    return result;
  }

  ScriptableArray *BrowseForFiles(const char *filter, const char * title,
                                  BrowseForFileMode mode) {
    std::vector<std::string> files;
    BrowseForFilesImpl(filter, true, title, mode, NULL, &files);
    return ScriptableArray::Create(files.begin(), files.end());
  }

  bool BrowseForFilesImpl(const char *filter, bool multiple, const char *title,
                          BrowseForFileMode mode, const char *default_name,
                          std::vector<std::string> *result) {
    ASSERT(result);
    result->clear();

    GtkFileChooserAction action;
    if (mode == BROWSE_FILE_MODE_FOLDER)
      action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    else if (mode == BROWSE_FILE_MODE_SAVEAS)
      action = GTK_FILE_CHOOSER_ACTION_SAVE;
    else
      action = GTK_FILE_CHOOSER_ACTION_OPEN;

    std::string whole_title(gadget_->GetManifestInfo(kManifestName));
    if (title && *title) {
      whole_title += " - ";
      whole_title += title;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        whole_title.c_str(), NULL, action,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
      gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                     TRUE);
    }

    ggadget::gtk::SetGadgetWindowIcon(GTK_WINDOW(dialog), gadget_);

    OptionsInterface *options = GetGlobalOptions();
    if (options) {
      std::string current_folder;
      options->GetValue(kFileBrowserFolderOption).ConvertToString(
          &current_folder);
      if (current_folder.size()) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            current_folder.c_str());
      }
    }

    if (default_name && *default_name) {
      std::string normalized = NormalizeFilePath(default_name);
      if (normalized.find('/') == normalized.npos) {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),
                                          normalized.c_str());
      } else {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                      normalized.c_str());
      }
    }

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);
    if (filter && *filter) {
      std::string filter_str(filter);
      std::string filter_name, patterns, pattern;
      while (!filter_str.empty()) {
        if (SplitString(filter_str, "|", &filter_name, &filter_str))
          SplitString(filter_str, "|", &patterns, &filter_str);
        else
          patterns = filter_name;

        GtkFileFilter *file_filter = gtk_file_filter_new();
        gtk_file_filter_set_name(file_filter, filter_name.c_str());
        while (!patterns.empty()) {
          SplitString(patterns, ";", &pattern, &patterns);
          gtk_file_filter_add_pattern(file_filter, pattern.c_str());
        }
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
      }
    }

    GSList *selected_files = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
      selected_files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
      if (options) {
        gchar *folder = gtk_file_chooser_get_current_folder(
            GTK_FILE_CHOOSER(dialog));
        if (folder) {
          options->PutValue(kFileBrowserFolderOption, Variant(folder));
          g_free(folder);
        }
      }
    }
    gtk_widget_destroy(dialog);

    if (!selected_files)
      return false;

    while (selected_files) {
      result->push_back(static_cast<const char *>(selected_files->data));
      selected_files = g_slist_next(selected_files);
    }
    return true;
  }

  GadgetInterface *gadget_;
};

// Looks up an icon file with specified name from current gtk icon theme.
std::string LookupIconInIconTheme(const std::vector<std::string> &icon_names,
                                  int size) {
  GtkIconTheme *theme = gtk_icon_theme_get_default();
  std::vector<std::string>::const_iterator it = icon_names.begin();
  for (; it != icon_names.end(); ++it) {
    if (gtk_icon_theme_has_icon(theme, it->c_str())) {
      int flags = 0;
#ifdef HAVE_RSVG_LIBRARY
      flags |= GTK_ICON_LOOKUP_FORCE_SVG;
#endif
      GtkIconInfo *icon = gtk_icon_theme_lookup_icon(
          theme, it->c_str(), size, static_cast<GtkIconLookupFlags>(flags));
      const char *file = gtk_icon_info_get_filename(icon);
      std::string file_str(file && *file ? file : "");
      gtk_icon_info_free(icon);
      if (file_str.length())
        return file_str;
    }
  }
  return std::string("");
}

// Gets the icon file of a DesktopEntry file.
std::string GetDesktopEntryIcon(const char *file, int size) {
  ASSERT(file);
  ggadget::xdg::DesktopEntry entry(file);
  if (entry.IsValid()) {
    std::string icon = entry.GetIcon();

    // Just return if the icon is an absolute path to the icon file.
    if (IsAbsolutePath(icon.c_str()))
      return icon;

    std::vector<std::string> icon_names;
    icon_names.push_back(icon);

    // Remove suffix, icon theme don't like it.
    // The suffix must be at least 3 characters long.
    size_t dot_pos = icon.find_last_of('.');
    if (dot_pos != std::string::npos && dot_pos > 0 &&
        icon.length() - dot_pos > 3)
      icon_names.push_back(icon.substr(0, dot_pos));

    // Try lookup the icon in icon theme.
    std::string icon_file = LookupIconInIconTheme(icon_names, size);

    // Try lookup icon in xdg data dirs.
    if (!icon_file.length())
      icon_file = ggadget::xdg::FindIconFileInXDGDataDirs(icon.c_str());

    if (!icon_file.length()) {
      // Fallback icons.
      icon_names.clear();
      icon_names.push_back("application-x-executable");
      icon_names.push_back("gnome-mime-application-x-executable");
      icon_names.push_back("unknown");
      icon_file = LookupIconInIconTheme(icon_names, size);
    }
    return icon_file;
  }
  return "";
}

std::string GetFileIcon(const char *file) {
  static const int kDefaultIconSize = 256;

  std::vector<std::string> icon_names;
  std::string type = ggadget::xdg::GetFileMimeType(file);

  if (type == ggadget::xdg::kDesktopEntryMimeType) {
    return GetDesktopEntryIcon(file, kDefaultIconSize);
  } else if (type == ggadget::xdg::kDirectoryMimeType) {
    icon_names.push_back("gnome-fs-directory");
    icon_names.push_back("gtk-directory");
  } else {
    std::string icon = ggadget::xdg::GetMimeTypeXDGIcon(type.c_str());

    // XDG icon is the best choice, if it's available.
    if (icon.length())
      icon_names.push_back(icon);

    icon = type;
    for (size_t i = icon.find('/', 0); i != std::string::npos;
         i = icon.find('/', i + 1))
      icon[i] = '-';

    // Try icon name similar than: text-plain
    icon_names.push_back(icon);

    // For backwards compatibility.
    icon_names.push_back("gnome-mime-" + icon);

    // Try generic name like text-x-generic.
    icon = type.substr(0, type.find('/')) + "-x-generic";
    icon_names.push_back(icon);
    icon_names.push_back("gnome-mime-" + icon);

    // Last resort
    icon_names.push_back("unknown");
  }
  return LookupIconInIconTheme(icon_names, kDefaultIconSize);
}

static GtkSystemCursor g_cursor_;
static GtkSystemScreen g_screen_;
static ScriptableCursor g_script_cursor_(&g_cursor_);
static ScriptableScreen g_script_screen_(&g_screen_);

} // namespace gtk_system_framework
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::gtk_system_framework;

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtk_system_framework extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtk_system_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  GadgetInterface *gadget) {
    LOGI("Register gtk_system_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();
    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    ResultVariant prop = framework->GetProperty("system");
    if (prop.v().type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable<UINT64_C(0xdf78c12fc974489c)>();
      reg_framework->RegisterVariantConstant("system", Variant(system));
    } else {
      system = VariantValue<ScriptableInterface *>()(prop.v());
    }

    if (!system) {
      LOG("Failed to retrieve or add framework.system object.");
      return false;
    }

    RegisterableInterface *reg_system = system->GetRegisterable();
    if (!reg_system) {
      LOG("framework.system object is not registerable.");
      return false;
    }

    // Check permissions.
    const Permissions *perm = gadget->GetPermissions();
    if (perm && perm->IsRequiredAndGranted(Permissions::FILE_READ)) {
      GtkSystemBrowseForFileHelper *helper =
          new GtkSystemBrowseForFileHelper(framework, gadget);
      reg_framework->RegisterMethod("BrowseForFile",
          NewSlotWithDefaultArgs(
              NewSlot(helper, &GtkSystemBrowseForFileHelper::BrowseForFile),
              kBrowseForFileDefaultArgs));
      reg_framework->RegisterMethod("BrowseForFiles",
          NewSlotWithDefaultArgs(
              NewSlot(helper, &GtkSystemBrowseForFileHelper::BrowseForFiles),
              kBrowseForFilesDefaultArgs));

      reg_system->RegisterMethod("getFileIcon",
          NewSlot(ggadget::framework::gtk_system_framework::GetFileIcon));
    } else {
      DLOG("No permission to read file.");
    }

    if (perm && perm->IsRequiredAndGranted(Permissions::DEVICE_STATUS)) {
      reg_system->RegisterVariantConstant("cursor",
                                          Variant(&g_script_cursor_));
      reg_system->RegisterVariantConstant("screen",
                                          Variant(&g_script_screen_));
    } else {
      DLOG("No permission to access device status.");
    }
    return true;
  }
}
