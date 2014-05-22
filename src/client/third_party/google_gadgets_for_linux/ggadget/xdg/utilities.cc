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

#include "utilities.h"
#include "desktop_entry.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/string_utils.h>
#include <ggadget/permissions.h>
#include <ggadget/system_utils.h>

#ifdef GGL_ENABLE_XDGMIME
#include <third_party/xdgmime/xdgmime.h>
#endif

namespace ggadget {
namespace xdg {

static const char kUnknownMimeType[] = "application/octet-stream";

enum WmType {
  WM_UNKNOWN,
  WM_KDE,
  WM_GNOME,
  WM_XFCE4
};

// Determines current WM is KDE or GNOME according to the env variables.
// Copied from xdg-open, a standard script provided by freedesktop.org.
static WmType DetermineWindowManager() {
  char *value;
  value = getenv("KDE_FULL_SESSION");

  if (value && strcmp (value, "true") == 0)
    return WM_KDE;

  value = getenv("GNOME_DESKTOP_SESSION_ID");

  if (value && *value)
    return WM_GNOME;

  int ret;

  ret = system("xprop -root _DT_SAVE_MODE | grep ' = \"xfce4\"$' > /dev/null 2>&1");
  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
    return WM_XFCE4;

  return WM_UNKNOWN;
}

static bool OpenURLWithSystemCommand(const char *url) {
  char *argv[4] = { NULL, NULL, NULL, NULL };

  // xdg-open or desktop-launch is our first choice, if it's not available,
  // then use window manager specific commands.
  std::string open_command = GetFullPathOfSystemCommand("xdg-open");
  if (!open_command.length())
    open_command = GetFullPathOfSystemCommand("desktop-launch");

  if (open_command.length()) {
    argv[0] = strdup(open_command.c_str());
    argv[1] = strdup(url);
  } else {
    WmType wm_type = DetermineWindowManager();
    if (wm_type == WM_GNOME) {
      open_command = GetFullPathOfSystemCommand("gnome-open");
      if (open_command.length()) {
        argv[0] = strdup(open_command.c_str());
        argv[1] = strdup(url);
      }
    } else if (wm_type == WM_KDE) {
      open_command = GetFullPathOfSystemCommand("kfmclient");
      if (open_command.length()) {
        argv[0] = strdup(open_command.c_str());
        argv[1] = strdup("exec");
        argv[2] = strdup(url);
      }
    } else if (wm_type == WM_XFCE4) {
      open_command = GetFullPathOfSystemCommand("exo-open");
      if (open_command.length()) {
        argv[0] = strdup(open_command.c_str());
        argv[1] = strdup(url);
      }
    }
  }

  if (argv[0] == NULL) {
    LOG("Can't find a suitable command to open the url.\n"
        "You probably need to install xdg-utils package.");
    return false;
  }

  pid_t pid;
  // fork and run the command.
  if ((pid = fork()) == 0) {
    if (fork() != 0)
      _exit(0);

    execv(argv[0], argv);

    DLOG("Failed to exec command: %s", argv[0]);
    _exit(-1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  for (size_t i = 0; argv[i]; ++i)
    free(argv[i]);

  // Assume open command will always succeed.
  return true;
}

bool OpenURL(const Permissions &permissions, const char *url) {
  if (!url || !*url) {
    LOG("Invalid URL!");
    return false;
  }

  if (IsAbsolutePath(url)) {
    // Allow to open a file directly.
    std::string new_url(kFileUrlPrefix);
    new_url.append(url);
    new_url = EncodeURL(new_url);
    return OpenURL(permissions, new_url.c_str());
  } else if (GetURLScheme(url).length() == 0) {
    // URI without prefix, will be treated as http://
    // Allow mailto:xxx.
    std::string new_url(kHttpUrlPrefix);
    new_url.append(url);
    return OpenURL(permissions, new_url.c_str());
  }

  std::string new_url(url);
  if (!IsValidURL(url))
    new_url = EncodeURL(new_url);

  if (IsValidWebURL(new_url.c_str())) {
    if (!permissions.IsRequiredAndGranted(Permissions::NETWORK)) {
      LOG("No permission to open a remote url: %s", url);
      return false;
    }
    return OpenURLWithSystemCommand(new_url.c_str());
  } else if (IsValidURL(new_url.c_str())) {
    // Support file or other special urls if allaccess is granted, including
    // things like mailto:xxx, trash:/, sysinfo:/, etc.
    if (!permissions.IsRequiredAndGranted(Permissions::ALL_ACCESS)) {
      LOG("No permission to open url: %s", url);
      return false;
    }
    return OpenURLWithSystemCommand(new_url.c_str());
  } else {
    LOG("Unsupported or malformed URL: %s", url);
  }

  return false;
}

std::string GetFileMimeType(const char *file) {
  static struct {
    const char *ext;
    const char *mime;
  } kDefaultMimeTypes[] = {
    { kDesktopEntryFileExtension, kDesktopEntryMimeType, },
    { kGadgetFileSuffix, kGoogleGadgetsMimeType, },
    { NULL, NULL }
  };

  std::string mime(kUnknownMimeType);
  if (file && *file) {
    struct stat statbuf;
    if (stat(file, &statbuf) == 0) {
      if (S_ISDIR(statbuf.st_mode)) {
        mime = kDirectoryMimeType;
      } else if (strcasecmp(file, kGadgetGManifest) == 0) {
        mime = kGoogleGadgetsMimeType;
      } else {
        for (size_t i = 0; kDefaultMimeTypes[i].ext; ++i) {
          if (EndWithNoCase(file, kDefaultMimeTypes[i].ext)) {
            mime = kDefaultMimeTypes[i].mime;
            break;
          }
        }
      }

#ifdef GGL_ENABLE_XDGMIME
      if (mime == kUnknownMimeType)
        mime = xdg_mime_get_mime_type_for_file(file, &statbuf);
#endif
    }
  }
  return mime;
}

std::string GetMimeTypeXDGIcon(const char *mimetype) {
#ifdef GGL_ENABLE_XDGMIME
  const char *icon = xdg_mime_get_icon(mimetype);
  return icon ? std::string(icon) : "";
#else
  return "";
#endif
}

void GetXDGDataDirs(std::vector<std::string> *dirs) {
  ASSERT(dirs);
  std::string home = GetHomeDirectory();

  // data dir for current user.
  const char *xdg_data_home = getenv("XDG_DATA_HOME");
  if (xdg_data_home && *xdg_data_home) {
    dirs->push_back(xdg_data_home);
  } else {
    dirs->push_back(BuildFilePath(home.c_str(), ".local", "share", NULL));
  }

  // system wide data dir.
  const char *xdg_data_dirs = getenv("XDG_DATA_DIRS");
  if (!xdg_data_dirs || !*xdg_data_dirs)
    xdg_data_dirs = "/usr/local/share:/usr/share";
  std::vector<std::string> result;
  SplitStringList(xdg_data_dirs, ":", &result);

  for (std::vector<std::string>::iterator i = result.begin();
       i != result.end(); ++i) {
    std::string trimmed = TrimString(*i);
    if (trimmed.length())
      dirs->push_back(trimmed);
  }
}

std::string FindIconFileInXDGDataDirs(const char *icon) {
  ASSERT(icon);
  // If it's an icon file, then just return it.
  if (IsAbsolutePath(icon))
    return icon;

  // System icon directories
  std::vector<std::string> xdg_dirs;
  GetXDGDataDirs(&xdg_dirs);
  std::vector<std::string>::iterator it = xdg_dirs.begin();
  for (; it != xdg_dirs.end(); ++it)
    *it = BuildFilePath(it->c_str(), "pixmaps", NULL);

  // For backwards compatibility.
  xdg_dirs.insert(xdg_dirs.begin(),
                  BuildFilePath(GetHomeDirectory().c_str(), ".icons", NULL));

  std::string icon_str(icon);
  size_t dot_pos = icon_str.find_last_of('.');
  // If it's an icon file name, without absolute path, then try to find it.
  if (dot_pos != std::string::npos) {
    for (it = xdg_dirs.begin(); it != xdg_dirs.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), icon_str.c_str(), NULL);
      if (access(path.c_str(), R_OK) == 0)
        return path;
    }
  }

  // Try with standard extensions.
  static const char *kStandardIconExtensions[] = {
    ".png", ".PNG", ".svg", ".SVG", ".xpm", ".XPM", NULL
  };
  for (it = xdg_dirs.begin(); it != xdg_dirs.end(); ++it) {
    for (size_t ext = 0; kStandardIconExtensions[ext]; ++ext) {
      std::string path =
          BuildFilePath(it->c_str(),
                        (icon_str + kStandardIconExtensions[ext]).c_str(),
                        NULL);
      if (access(path.c_str(), R_OK) == 0)
        return path;
    }
  }
  return "";
}

} // namespace xdg
} // namespace ggadget
