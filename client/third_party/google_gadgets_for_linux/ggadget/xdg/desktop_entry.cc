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

#include "desktop_entry.h"
#include <map>
#include <string>
#include <stdio.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/system_utils.h>
#include <ggadget/string_utils.h>
#include <ggadget/locales.h>

namespace ggadget {
namespace xdg {

class DesktopEntry::Impl {
 public:
  Impl() : type_(DesktopEntry::UNKNOWN) {
  }

  bool Load(const char *file) {
    ASSERT(file);

    // Clear current content.
    type_ = DesktopEntry::UNKNOWN;
    file_.clear();
    entries_.clear();

    FILE *fp = fopen(file, "r");
    if (!fp) return false;

    char buf[1024];
    bool in_group = false;;
    // Read entries.
    while(!feof(fp) && !ferror(fp)) {
      if (!fgets(buf, 1024, fp))
        break;

      std::string entry = TrimString(buf);
      if (entry.length() == 0 || entry[0] == '#') {
        // Blank line or Comment.
        continue;
      } else if (!in_group && entry == "[Desktop Entry]") {
        // Start of desktop entry group
        in_group = true;
        continue;
      } else if (entry[0] == '[') {
        // End of desktop entry group. No matter if in_group is true or not,
        // a group header other than Desktop Entry will finish the process.
        break;
      }

      // Not in desktop entry group yet.
      if (!in_group) continue;

      std::string key, value;
      if (!SplitString(entry, "=", &key, &value))
        continue;

      key = TrimString(key);
      value = TrimString(value);
      if (!key.length() || !value.length())
        continue;

      entries_[key] = UnescapeString(value.c_str());
    }

    fclose(fp);

    if (in_group) {
      // Check required entries.
      std::string value;
      if (ReadStringEntry("Type", &value)) {
        bool result = false;
        if (value == "Application") {
          type_ = DesktopEntry::APPLICATION;
          std::string try_exec;
          bool has_try_exec = ReadStringEntry("TryExec", &try_exec);
          if (has_try_exec)
            try_exec = GetFullPathOfSystemCommand(try_exec.c_str());
          // Exec is required for Application.
          // If there is a TryExec key, then the command must be available.
          result = (!has_try_exec || try_exec.length()) &&
              ReadStringEntry("Exec", NULL);
        } else if (value == "Link") {
          type_ = DesktopEntry::LINK;
          // URL is required for Link.
          result = ReadStringEntry("URL", NULL);
        }
        // Name is required for all types.
        if (result && ReadLocaleStringEntry("Name", NULL)) {
          file_ = file;
          return true;
        }
      }
    }

    type_ = DesktopEntry::UNKNOWN;
    file_.clear();
    entries_.clear();
    return false;
  }

  bool ReadBoolEntry(const char *key, bool *result) {
    ASSERT(key);
    LightMap<std::string, std::string>::const_iterator it = entries_.find(key);
    if (it != entries_.end()) {
      if (result)
        *result = (it->second == "1" || it->second == "true");
      return true;
    }
    return false;
  }

  bool ReadStringEntry(const char *key, std::string *result) {
    ASSERT(key);
    LightMap<std::string, std::string>::const_iterator it = entries_.find(key);
    if (it != entries_.end()) {
      if (result)
        *result = it->second;
      return true;
    }
    return false;
  }

  bool ReadLocaleStringEntry(const char *key, std::string *result) {
    ASSERT(key);
    std::string language, territory;
    if (!GetSystemLocaleInfo(&language, &territory))
      return ReadStringEntry(key, result);
    LightMap<std::string, std::string>::const_iterator it;
    std::string locale_key =
        StringPrintf("%s[%s_%s]", key, language.c_str(), territory.c_str());
    it = entries_.find(locale_key);
    if (it == entries_.end()) {
      locale_key =
          StringPrintf("%s[%s-%s]", key, language.c_str(), territory.c_str());
      it = entries_.find(locale_key);
      if (it == entries_.end()) {
        locale_key = StringPrintf("%s[%s]", key, language.c_str());
        it = entries_.find(locale_key);
        if (it == entries_.end()) {
          it = entries_.find(key);
        }
      }
    }
    if (it != entries_.end()) {
      if (result) *result = it->second;
      return true;
    }
    return false;
  }

  bool IsApplication() { return type_ == DesktopEntry::APPLICATION; }

  bool IsLink() { return type_ == DesktopEntry::LINK; }

  static std::string UnescapeString(const char *str) {
    std::string result;
    for (const char *p = str; *p; ++p) {
      if (*p == '\\' && *(p+1)) {
        ++p;
        switch(*p) {
          case 's': result.push_back(' '); break;
          case 'n': result.push_back('\n'); break;
          case 'r': result.push_back('\r'); break;
          case 't': result.push_back('\t'); break;
          default: result.push_back(*p); break;
        }
      } else {
        result.push_back(*p);
      }
    }
    return result;
  }

  static std::string ShellQuote(const char *str) {
    std::string result("'");
    for (const char *p = str; *p; ++p) {
      if (*p == '\'')
        result.append("'\\''");
      else
        result.push_back(*p);
    }
    result.push_back('\'');
    return result;
  }

  DesktopEntry::Type type_;
  std::string file_;
  LightMap<std::string, std::string> entries_;
};

DesktopEntry::DesktopEntry() : impl_(new Impl()) {
}

DesktopEntry::DesktopEntry(const char *desktop_file)
  : impl_(new Impl()) {
  impl_->Load(desktop_file);
}

DesktopEntry::~DesktopEntry() {
  delete impl_;
  impl_ = NULL;
}

bool DesktopEntry::Load(const char *desktop_file) {
  return impl_->Load(desktop_file);
}

bool DesktopEntry::IsValid() const {
  return impl_->type_ != UNKNOWN;
}

DesktopEntry::Type DesktopEntry::GetType() const {
  return impl_->type_;
}

bool DesktopEntry::NeedTerminal() const {
  bool value = false;
  if (impl_->IsApplication())
    impl_->ReadBoolEntry("Terminal", &value);
  return value;
}

bool DesktopEntry::SupportStartupNotify() const {
  bool value = false;
  if (impl_->IsApplication())
    impl_->ReadBoolEntry("StartupNotify", &value);
  return value;
}

bool DesktopEntry::SupportMimeType(const char *mime) const {
  if (!impl_->IsApplication()) return false;

  std::string mimetypes;
  if (mime && *mime && impl_->ReadStringEntry("MimeType", &mimetypes)) {
    size_t pos = mimetypes.find(mime);
    if (pos != std::string::npos) {
      size_t end = pos + strlen(mime);
      if ((pos == 0 || mimetypes[pos - 1] == ';') &&
          (end == mimetypes.length() || mimetypes[end] == ';'))
        return true;
    }
  }
  return false;
}

bool DesktopEntry::AcceptURL(bool multiple) const {
  if (!impl_->IsApplication()) return false;

  std::string exec;
  if (impl_->ReadStringEntry("Exec", &exec)) {
    bool accept_single = (exec.find("%u") != std::string::npos);
    bool accept_multiple = (exec.find("%U") != std::string::npos);
    return multiple ? accept_multiple : (accept_single || accept_multiple);
  }
  return false;
}

bool DesktopEntry::AcceptFile(bool multiple) const {
  if (!impl_->IsApplication()) return false;

  std::string exec;
  if (impl_->ReadStringEntry("Exec", &exec)) {
    bool accept_single = (exec.find("%f") != std::string::npos);
    bool accept_multiple = (exec.find("%F") != std::string::npos);
    return multiple ? accept_multiple : (accept_single || accept_multiple);
  }
  return false;
}

std::string DesktopEntry::GetName() const {
  std::string value;
  impl_->ReadLocaleStringEntry("Name", &value);
  return value;
}

std::string DesktopEntry::GetGenericName() const {
  std::string value;
  impl_->ReadLocaleStringEntry("GenericName", &value);
  return value;
}

std::string DesktopEntry::GetComment() const {
  std::string value;
  impl_->ReadLocaleStringEntry("Comment", &value);
  return value;
}

std::string DesktopEntry::GetIcon() const {
  std::string value;
  impl_->ReadLocaleStringEntry("Icon", &value);
  return value;
}

std::string DesktopEntry::GetWorkingDirectory() const {
  std::string value;
  impl_->ReadStringEntry("Path", &value);
  return value;
}

std::string DesktopEntry::GetMimeTypes() const {
  std::string value;
  impl_->ReadStringEntry("MimeType", &value);
  return value;
}

std::string DesktopEntry::GetStartupWMClass() const {
  std::string value;
  impl_->ReadStringEntry("StartupWMClass", &value);
  return value;
}

std::string DesktopEntry::GetURL() const {
  std::string value;
  impl_->ReadStringEntry("URL", &value);
  return value;
}

std::string DesktopEntry::GetTryExec() const {
  std::string value;
  impl_->ReadStringEntry("TryExec", &value);
  return value;
}

std::string DesktopEntry::GetExecCommand(int argc, const char *argv[]) const {
  std::string exec;
  if (!impl_->ReadStringEntry("Exec", &exec))
    return "";

  std::string result;
  size_t start = 0;
  size_t end = exec.length();
  size_t pos = 0;
  while (start < end) {
    pos = exec.find('%', start);
    result.append(exec, start, (pos == std::string::npos ? end : pos) - start);
    if (pos == std::string::npos)
      break;
    if (pos + 1 < end) {
      ++pos;
      char field = exec[pos];
      if ((field == 'U' || field == 'u') && argc > 0 && argv) {
        int num = (field == 'U' ? argc : 1);
        for (int i = 0; i < argc && num > 0; ++i) {
          if (argv && argv[i]) {
            if (IsValidURL(argv[i])) {
              result.append(Impl::ShellQuote(argv[i]));
              result.push_back(' ');
              --num;
            } else if (IsAbsolutePath(argv[i])) {
              std::string file_url(kFileUrlPrefix);
              file_url.append(argv[i]);
              result.append(Impl::ShellQuote(EncodeURL(file_url).c_str()));
              result.push_back(' ');
              --num;
            }
          }
        }
      } else if ((field == 'F' || field == 'f') && argc > 0 && argv) {
        int num = (field == 'U' ? argc : 1);
        for (int i = 0; i < argc && num > 0; ++i) {
          if (argv && argv[i]) {
            if (IsValidFileURL(argv[i])) {
              std::string filename = GetPathFromFileURL(argv[i]);
              if (filename.length()) {
                result.append(Impl::ShellQuote(filename.c_str()));
                result.push_back(' ');
                --num;
              } else if (IsAbsolutePath(argv[i])) {
                result.append(Impl::ShellQuote(argv[i]));
                result.push_back(' ');
                --num;
              }
            }
          }
        }
      } else if (field == 'i') {
        std::string icon = GetIcon();
        if (icon.length()) {
          result.append("--icon ");
          result.append(Impl::ShellQuote(icon.c_str()));
        }
      } else if (field == 'c') {
        std::string name = GetName();
        if (name.length()) {
          result.append(Impl::ShellQuote(name.c_str()));
        }
      } else if (field == 'k') {
        result.append(Impl::ShellQuote(impl_->file_.c_str()));
      } else if (field == '%') {
        result.push_back(field);
      }
    }
    ++pos;
    start = pos;
  }

  return TrimString(result);
}

} // namespace xdg
} // namespace ggadget
