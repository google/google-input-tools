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
#include <sys/time.h>
#include <time.h>
#include <cstdlib>

#include <map>
#include <vector>
#include <list>

#include <ggadget/string_utils.h>
#include <ggadget/logger.h>

#include "icon_theme.h"

namespace ggadget {
namespace xdg {

static const int kUpdateInterval = 5;

// Store icon dirs and their mtime
static LightMap<std::string, int> *icon_dirs = NULL;

// Store loaded icon_theme
class IconTheme;
static LightMap<std::string, IconTheme*> *icon_themes = NULL;

// Store icon formats
static std::vector<std::string> *icon_formats = NULL;

// The default theme
static const int kDefaultThemeNum = 5;
static IconTheme *default_themes[kDefaultThemeNum];

static time_t last_check_time = 0;
static void EnsureUpdated();

class IconTheme {
 public:
  class SubDirInfo {
   public:
    enum Type {
      Fixed,
      Scalable,
      Threshold
    };
    SubDirInfo() : size(0), type(Threshold), max_size(0), min_size(0), threshold(2) {
    }
    int size;
    Type type;
    int max_size;
    int min_size;
    int threshold; // as specified by icon-theme-spec, default 2
  };

  IconTheme(const std::string &name)
      : index_theme_loaded_(false),
        info_array_(NULL),
        current_info_(NULL) {
    // Iterate on all icon directories to find directories of the specified
    // theme and load the first encountered index.theme
    LightMap<std::string, int>::iterator iter;
    std::string theme_path;
    for (iter = icon_dirs->begin(); iter != icon_dirs->end(); ++iter) {
      theme_path = iter->first + name;
      DLOG("Trying find theme in %s", theme_path.c_str());
      if (access(theme_path.c_str(), R_OK|X_OK))
        continue;
      if (!index_theme_loaded_ &&
          !access((theme_path + "/index.theme").c_str(), R_OK)) {
        DLOG("Trying loading %s/index.theme", theme_path.c_str());
        if (!LoadIndexTheme(theme_path + "/index.theme")) return;
        DLOG("index.theme Loaded");
        index_theme_loaded_ = true;
      }
      dirs_.push_back(theme_path);
    }
  }

  ~IconTheme() {
    delete [] info_array_;
  }

  static std::string ReadLine(FILE *fp) {
    if (!fp)
      return "";

    int ch = 0;
    std::string result = "";
    while ((ch = fgetc(fp)) != EOF) {
      result.append(1, ch);
      if ('\n' == ch)
        break;
    }

    return result;
  }

  bool LoadIndexTheme(const std::string &file) {
    FILE *fp = fopen(file.c_str(), "r");
    if (!fp) return false;

    // Read entries.
    while(!feof(fp) && !ferror(fp)) {
      std::string buf = ReadLine(fp);
      if (buf == "")
        break;

      std::string entry = TrimString(buf);
      if (entry.length() == 0 || entry[0] == '#') {
        // Blank line or Comment.
        continue;
      } else if (entry[0] == '[' && info_array_) {
        current_info_ = NULL;
        std::string subdir = entry.substr(1, entry.length() - 2);
        DLOG("Set Subdir:%s", subdir.c_str());
        if (subdirs_.find(subdir) != subdirs_.end())
          current_info_ = &info_array_[subdirs_[subdir]];
      }

      std::string key, value;
      if (!SplitString(entry, "=", &key, &value))
        continue;

      key = TrimString(key);
      value = TrimString(value);

      if (current_info_) {
        if (key == "Size") {
          current_info_->size = atoi(value.c_str());
          DLOG("Size:%d", current_info_->size);
        } else if (key == "Type") {
          if (value == "Fixed")
            current_info_->type = SubDirInfo::Fixed;
          else if (value == "Scalable")
            current_info_->type = SubDirInfo::Scalable;
          else if (value == "Threshold")
            current_info_->type = SubDirInfo::Threshold;
        } else if (key == "MaxSize") {
          current_info_->max_size = atoi(value.c_str());
          DLOG("MaxSize:%d", current_info_->max_size);
        } else if (key == "MinSize") {
          current_info_->min_size = atoi(value.c_str());
          DLOG("MinSize:%d", current_info_->min_size);
        } else if (key == "Threshold") {
          current_info_->threshold = atoi(value.c_str());
          DLOG("Threshold:%d", current_info_->threshold);
        }
      } else {
        if (key.compare("Directories") == 0 && !info_array_) {
          if (!SetDirectories(value)) break;
        } else if (key.compare("Inherits") == 0) {
          if (value != "hicolor")
            inherits_ = value;
        }
      }
    }

    fclose(fp);
    return info_array_ != NULL;
  }

  bool SetDirectories(const std::string &dirs) {
    DLOG("SetDirectories:%s", dirs.c_str());
    int num = 0;
    std::string::size_type pos = 0, epos;
    std::string dir;
    while ((epos = dirs.find(',', pos)) != std::string::npos) {
      dir = TrimString(dirs.substr(pos, epos - pos));
      DLOG("Add Subdir:%s, %d, %d", dir.c_str(), (int)pos, (int)epos);
      if (dir.length() == 0) {
        LOG("Invalid index.theme: blank subdir");
        return false;
      }
      subdirs_[dir] = num++;
      pos = epos + 1;
    }
    dir = TrimString(dirs.substr(pos));
    if (dir.length() == 0) {
      LOG("Invalid index.theme: blank subdir");
      return false;
    }
    subdirs_[dir] = num++;
    info_array_ = new SubDirInfo[num];
    return true;
  }

  static int MatchesSize(SubDirInfo* info, int size) {
    if (info->type == SubDirInfo::Fixed) {
      return size - info->size;
    } else if (info->type == SubDirInfo::Scalable) {
      if (size >= info->min_size && size <= info->max_size)
        return 0;
      else
        return abs(size - info->min_size) < abs(size - info->max_size)
            ? (size - info->min_size) : (size - info->max_size);
    } else {
      if (size >= info->size - info->threshold && size <= info->size + info->threshold)
        return 0;
      else
        return abs(size - info->size - info->threshold) <
            abs(size - info->size + info->threshold)
            ? size - info->size - info->threshold
            : size - info->size + info->threshold;
    }
  }

  std::string GetIconPathUnderSubdir(const std::string &icon_name,
                                     const std::string &subdir) {
    std::string icon_path;
    std::list<std::string>::iterator dir_iter;
    for (dir_iter = dirs_.begin(); dir_iter != dirs_.end(); ++dir_iter) {
      for (size_t i = 0; i < icon_formats->size(); ++i) {
        icon_path = (*dir_iter) + "/" + subdir + "/" + icon_name
            + (*icon_formats)[i];
        // DLOG("Searching %s", icon_path.c_str());
        if (!access(icon_path.c_str(), R_OK)) {
          DLOG("Found %s", icon_path.c_str());
          return icon_path;
        }
      }
    }
    return "";
  }

  std::string GetIconPath(const std::string &icon_name, int size, bool inherits) {
    DLOG("GetIconPath:%s, %d", icon_name.c_str(), size);
    LightMap<std::string, int>::iterator subdir_iter;
    std::list<std::string>::iterator dir_iter;
    std::string icon_path;

    for (subdir_iter = subdirs_.begin(); subdir_iter != subdirs_.end(); ++subdir_iter) {
      SubDirInfo *info = &info_array_[subdir_iter->second];
      if (MatchesSize(info, size) == 0) {
        icon_path = GetIconPathUnderSubdir(icon_name, subdir_iter->first);
        if (icon_path != "") return icon_path;
      }
    }
    // Now looking for the mostly matched
    int delta = 9999;

    for (subdir_iter = subdirs_.begin(); subdir_iter != subdirs_.end(); ++subdir_iter) {
      SubDirInfo *info = &info_array_[subdir_iter->second];
      int d = abs(MatchesSize(info, size));
      if (d < delta) {
        std::string path = GetIconPathUnderSubdir(icon_name, subdir_iter->first);
        if (path != "") {
          delta = d;
          icon_path = path;
        }
      }
    }

    if (icon_path != "" || !inherits || inherits_ == "") return icon_path;

    IconTheme *theme = LoadTheme(inherits_);
    if (theme)
      return theme->GetIconPath(icon_name, size, inherits);
    else
      return "";
  }

  bool IsValid() {
    return index_theme_loaded_;
  }

  bool index_theme_loaded_;
  // store the scattered directories of this theme
  std::list<std::string> dirs_;

  // store the subdirs of this theme and array index of info_array_
  LightMap<std::string, int> subdirs_;
  SubDirInfo *info_array_;
  SubDirInfo *current_info_;
  std::string inherits_;

  static IconTheme* LoadTheme(const std::string &theme_name) {
    IconTheme *theme;
    if (icon_themes->find(theme_name) != icon_themes->end()) {
      theme = (*icon_themes)[theme_name];
    } else {
      theme = new IconTheme(theme_name);
      if (!theme->IsValid()) {
        delete theme;
        theme = NULL;
      }
      (*icon_themes)[theme_name] = theme;
    }
    return theme;
  }
};

static void TryAddIconDir(const std::string &dir) {
  if (access(dir.c_str(), R_OK)) return;
  if (dir[dir.length() - 1] != '/')
    (*icon_dirs)[dir + '/'] = 0;
  else
    (*icon_dirs)[dir] = 0;
}

void AddIconDir(const std::string &dir) {
  EnsureUpdated();
  TryAddIconDir(dir);
}

static std::string LookupFallbackIcon(const std::string &icon_name) {
  std::string icon;
  LightMap<std::string, int>::iterator iter;
  for (iter = icon_dirs->begin(); iter != icon_dirs->end(); ++iter) {
    for (size_t i = 0; i < icon_formats->size(); ++i) {
      icon = iter->first + icon_name + (*icon_formats)[i];
      if (!access(icon.c_str(), R_OK)) {
        DLOG("Found %s", icon.c_str());
        return icon;
      }
    }
  }
  return "";
}

std::string LookupIcon(const std::string &icon_name,
                       const std::string &theme_name,
                       int size) {
  EnsureUpdated();

  IconTheme *theme = IconTheme::LoadTheme(theme_name);

  std::string icon;
  if (theme) {
    icon = theme->GetIconPath(icon_name, size, true);
  }

  if (icon.length()) return icon;

  if (theme_name != "hicolor")
    return LookupIcon(icon_name, "hicolor", size);
  else
    return LookupFallbackIcon(icon_name);
}

static void InitDefaultThemes() {
  // FIXME: There is no standard way to know about the current icon theme. So
  // I just make a guess.
  char *env = getenv("GGL_ICON_THEME");
  if (env)
    default_themes[0] = IconTheme::LoadTheme(env);

  env = getenv("KDE_FULL_SESSION");
  if (env) {
    env = getenv("KDE_SESSION_VERSION");
    if (!env || env[0] != '4') {
      default_themes[1] = IconTheme::LoadTheme("crystalsvg");   // KDE3
      default_themes[2] = IconTheme::LoadTheme("oxygen");       // KDE4
    } else {
      default_themes[1] = IconTheme::LoadTheme("oxygen");       // KDE4
      default_themes[2] = IconTheme::LoadTheme("crystalsvg");   // KDE3
    }
    default_themes[3] = IconTheme::LoadTheme("gnome");
  } else {   // Assume it's gnome.
    default_themes[1] = IconTheme::LoadTheme("gnome");
    default_themes[2] = IconTheme::LoadTheme("crystalsvg");   // KDE3
    default_themes[3] = IconTheme::LoadTheme("oxygen");       // KDE4
  }
  default_themes[4] = IconTheme::LoadTheme("hicolor");
}

std::string LookupIconInDefaultTheme(const std::string &icon_name,
                                     int size) {
  EnsureUpdated();
  if (icon_themes->size() == 0) InitDefaultThemes();

  std::string icon_path;
  for (int i = 0; i < kDefaultThemeNum; i++) {
    if (default_themes[i]) {
      icon_path = default_themes[i]->GetIconPath(icon_name, size, true);
      if (icon_path != "") return icon_path;
    }
  }
  return LookupFallbackIcon(icon_name);
}

void EnableSvgIcon(bool enable) {
  icon_formats->clear();
  icon_formats->push_back(".png");
  if (enable) {
    icon_formats->push_back(".svg");
    icon_formats->push_back(".svgz");
  }
  icon_formats->push_back(".xpm");
}

static void AddXDGDataDir(const std::string &dir) {
  DLOG("AddXDGDataDir:%s", dir.c_str());
  if (access(dir.c_str(), R_OK)) return;
  TryAddIconDir(dir + "/icons");
  TryAddIconDir(dir + "/pixmaps");
}

static void InitIconDir() {
  icon_dirs->clear();
  std::string xdg_data_dirs;
  char *env = getenv("XDG_DATA_HOME");
  if (!env) {
    env = getenv("HOME");
    if (env) {
      std::string local_data_dir = env;
      local_data_dir += "/.local/share";
      AddXDGDataDir(local_data_dir);
    }
  } else {
    AddXDGDataDir(env);
  }

  env = getenv("XDG_DATA_DIRS");
  if (!env) {
    AddXDGDataDir("/usr/local/share/");
    AddXDGDataDir("/usr/share/");
  } else {
    std::string xdg_data_dirs = env;
    std::string::size_type pos = 0, epos;
    while ((epos = xdg_data_dirs.find(':', pos)) != std::string::npos) {
      AddXDGDataDir(xdg_data_dirs.substr(pos, epos - pos));
      pos = epos + 1;
    }
    AddXDGDataDir(xdg_data_dirs.substr(pos));
  }
}

static void EnsureUpdated() {
  struct timeval t;
  gettimeofday(&t, NULL);
  time_t now = t.tv_sec;

  if (last_check_time == 0) {
    icon_dirs = new LightMap<std::string, int>;
    icon_themes = new LightMap<std::string, IconTheme*>;
    icon_formats = new std::vector<std::string>;
    EnableSvgIcon(false);
    InitIconDir();
    last_check_time = now;
  } else {
    // TODO: something changed. start over
    if (now > last_check_time + kUpdateInterval) {

    }
  }
}

// typedef bool (*ThemeVisitFunc)(const std::string &theme);
// static bool EnumerateThemes(ThemeVisitFunc func);

} // namespace xdg
} // namespace ggadget
