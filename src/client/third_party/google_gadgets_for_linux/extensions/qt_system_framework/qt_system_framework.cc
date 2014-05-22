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

#include <vector>
#include <QtGui/QCursor>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopWidget>
#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/framework_interface.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/xdg/desktop_entry.h>
#include <ggadget/xdg/icon_theme.h>
#include <ggadget/xdg/utilities.h>

#define Initialize qt_system_framework_LTX_Initialize
#define Finalize qt_system_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    qt_system_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace qt_system_framework {

class QtSystemCursor : public CursorInterface {
 public:
  virtual void GetPosition(int *x, int *y)  {
    QPoint p = QCursor::pos();
    if (x) *x = p.x();
    if (y) *y = p.y();
  }
};

class QtSystemScreen : public ScreenInterface {
 public:
  virtual void GetSize(int *width, int *height) {
    QDesktopWidget w;
    QRect r = w.screenGeometry();
    if (width) *width = r.width();
    if (height) *height = r.height();
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

class QtSystemBrowseForFileHelper {
 public:
  QtSystemBrowseForFileHelper(ScriptableInterface *framework,
                              GadgetInterface *gadget)
    : gadget_(gadget) {
    framework->ConnectOnReferenceChange(
      NewSlot(this, &QtSystemBrowseForFileHelper::OnFrameworkRefChange));
  }

  // Function to destroy the helper object when framework is destroyed.
  void OnFrameworkRefChange(int ref, int change) {
    GGL_UNUSED(ref);
    if (change == 0) {
      DLOG("Framework destroyed, delete QtSystemBrowseForFileHelper object.");
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

  ScriptableArray *BrowseForFiles(const char *filter, const char *title,
                                  BrowseForFileMode mode) {
    std::vector<std::string> files;
    BrowseForFilesImpl(filter, true, title, mode, NULL, &files);
    return ScriptableArray::Create(files.begin(), files.end());
  }

  bool BrowseForFilesImpl(const char *filter, bool multiple, const char *title,
                          BrowseForFileMode mode, const char *default_name,
                          std::vector<std::string> *result) {
    GGL_UNUSED(title);
    GGL_UNUSED(mode);
    GGL_UNUSED(default_name);
    ASSERT(result);
    result->clear();

    QStringList filters;
    QFileDialog dialog;
    // TODO: support title(including gadget name), gadget icon, mode, default_name.
    if (multiple) dialog.setFileMode(QFileDialog::ExistingFiles);
    if (filter && *filter) {
      size_t len = strlen(filter);
      char *copy = new char[len + 2];
      memcpy(copy, filter, len + 1);
      copy[len] = '|';
      copy[len + 1] = '\0';
      char *str = copy;
      int i = 0;
      int t = 0;
      while (str[i] != '\0') {
        if (str[i] == '|') {
          t++;
          if (t == 1) str[i] = '(';
          if (t == 2) {
            str[i] = ')';
            char bak = str[i + 1];
            str[i + 1] = '\0';
            filters << QString::fromUtf8(str);
            str[i + 1] = bak;
            str = &str[i+1];
            i = 0;
            t = 0;
            continue;
          }
        } else if (str[i] == ';' && t == 1) {
          str[i] = ' ';
        }
        i++;
      }
      delete [] copy;
      dialog.setFilters(filters);
    }
    if (dialog.exec()) {
      QStringList fnames = dialog.selectedFiles();
      for (int i = 0; i < fnames.size(); ++i)
        result->push_back(fnames.at(i).toUtf8().data());
      return true;
    }
    return false;
  }

  GadgetInterface *gadget_;
};

// Gets the icon file of a DesktopEntry file.
static std::string GetDesktopEntryIcon(const char *file) {
  ASSERT(file);
  ggadget::xdg::DesktopEntry entry(file);
  if (entry.IsValid()) {
    return entry.GetIcon();
  } else {
    return "";
  }
}

static std::string GetDirectorySpecialIcon(const std::string &file) {
  GGL_UNUSED(file);
  // TODO: check .directory file to show the right icon
  return "";
}

std::string GetFileIcon(const char *file) {
  static const int kDefaultIconSize = 128;
  std::vector<std::string> icon_names;
  std::string type = ggadget::xdg::GetFileMimeType(file);
  std::string icon_name, icon_file;

  DLOG("GetFileIcon:%s, %s", file, type.c_str());

  if (type == ggadget::xdg::kDesktopEntryMimeType) {
    icon_name = GetDesktopEntryIcon(file);
    if (icon_name.length())
      icon_names.push_back(icon_name);
  } else if (type == ggadget::xdg::kDirectoryMimeType) {
    icon_name = GetDirectorySpecialIcon(file);
    if (icon_name.length())
      icon_names.push_back(icon_name);
    icon_names.push_back("folder");
  } else {
    icon_name = ggadget::xdg::GetMimeTypeXDGIcon(type.c_str());
    if (icon_name.length())
      icon_names.push_back(icon_name);

    // Try icon name similar than: text-plain
    icon_name = type;
    for (size_t i = icon_name.find('/', 0); i != std::string::npos;
         i = icon_name.find('/', i + 1))
      icon_name[i] = '-';
    icon_names.push_back(icon_name);

    // Try generic name like text-x-generic.
    icon_name = type.substr(0, type.find('/')) + "-x-generic";
    icon_names.push_back(icon_name);

    icon_names.push_back("gnome-mime-" + icon_name);

    // Last resort
    icon_names.push_back("unknown");
  }
  for (size_t i = 0; i < icon_names.size(); i++) {
    if (icon_names[i][0] == '/') {
      if (!access(icon_names[i].c_str(), R_OK))
        return icon_names[i];
    } else {
      icon_file = ggadget::xdg::LookupIconInDefaultTheme(icon_names[i], kDefaultIconSize);
      if (icon_file.length()) {
        DLOG("Found Icon: %s", icon_file.c_str());
        return icon_file;
      }
    }
  }
  return icon_file;
}

static QtSystemCursor g_cursor_;
static QtSystemScreen g_screen_;
static ScriptableCursor g_script_cursor_(&g_cursor_);
static ScriptableScreen g_script_screen_(&g_screen_);

} // namespace qt_system_framework
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::qt_system_framework;

extern "C" {
  bool Initialize() {
    LOGI("Initialize qt_system_framework extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize qt_system_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  GadgetInterface *gadget) {
    LOGI("Register qt_system_framework extension.");
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
      QtSystemBrowseForFileHelper *helper =
          new QtSystemBrowseForFileHelper(framework, gadget);

      reg_framework->RegisterMethod("BrowseForFile",
          NewSlotWithDefaultArgs(
              NewSlot(helper, &QtSystemBrowseForFileHelper::BrowseForFile),
              kBrowseForFileDefaultArgs));
      reg_framework->RegisterMethod("BrowseForFiles",
          NewSlotWithDefaultArgs(
              NewSlot(helper, &QtSystemBrowseForFileHelper::BrowseForFiles),
              kBrowseForFilesDefaultArgs));

      reg_system->RegisterMethod("getFileIcon",
          NewSlot(ggadget::framework::qt_system_framework::GetFileIcon));
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
