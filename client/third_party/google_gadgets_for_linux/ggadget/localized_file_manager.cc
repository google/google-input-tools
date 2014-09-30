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

#include "localized_file_manager.h"

#include <cstring>
#include <vector>
#include <string>
#include <cstdio>

#include "common.h"
#include "locales.h"
#include "gadget_consts.h"
#include "logger.h"
#include "slot.h"
#include "system_utils.h"
#include "string_utils.h"
#include "small_object.h"

namespace ggadget {

class LocalizedFileManager::Impl : public SmallObject<> {
 public:
  Impl(FileManagerInterface *file_manager, const char *locale)
    : file_manager_(file_manager) {
    std::string locale_name;
    if (locale && *locale)
      locale_name = locale;
    else
      locale_name = GetSystemLocaleName();

    prefixes_.push_back(locale_name);
    std::string lower_locale_name = ToLower(locale_name);
    if (lower_locale_name != locale_name)
      prefixes_.push_back(lower_locale_name);

    std::string locale_name_temp(locale_name);
    size_t pos = locale_name_temp.find('-');
    if (pos != locale_name_temp.npos) {
      locale_name_temp.replace(pos, 1, 1, '_');
      lower_locale_name.replace(pos, 1, 1, '_');
      prefixes_.push_back(locale_name_temp);
      if (lower_locale_name != locale_name)
        prefixes_.push_back(lower_locale_name);
    }

    // for windows compatibility.
    std::string windows_locale_id;
    if (GetLocaleWindowsIDString(locale_name.c_str(), &windows_locale_id))
      prefixes_.push_back(windows_locale_id);

    if (locale_name != "en") {
      prefixes_.push_back("en");
      prefixes_.push_back("1033"); // for windows compatibility.
    }
  }

  ~Impl() {
    delete file_manager_;
    file_manager_ = NULL;
  }

  StringVector prefixes_;
  FileManagerInterface *file_manager_;
};


LocalizedFileManager::LocalizedFileManager()
  : impl_(new Impl(NULL, NULL)){
}

LocalizedFileManager::~LocalizedFileManager() {
  delete impl_;
}

LocalizedFileManager::LocalizedFileManager(FileManagerInterface *file_manager)
  : impl_(new Impl(file_manager, NULL)){
}

LocalizedFileManager::LocalizedFileManager(FileManagerInterface *file_manager,
                                           const char *locale)
  : impl_(new Impl(file_manager, locale)){
}

bool LocalizedFileManager::Attach(FileManagerInterface *file_manager) {
  if (file_manager) {
    delete impl_->file_manager_;
    impl_->file_manager_ = file_manager;
    return true;
  }
  return false;
}

bool LocalizedFileManager::IsValid() {
  return impl_->file_manager_ && impl_->file_manager_->IsValid();
}

bool LocalizedFileManager::Init(const char *base_path, bool create) {
  return impl_->file_manager_ ?
         impl_->file_manager_->Init(base_path, create) :
         false;
}

bool LocalizedFileManager::ReadFile(const char *file, std::string *data) {
  ASSERT(file && data);

  if (!file || !*file || !data)
    return false;

  if (impl_->file_manager_) {
    // Try non-localized file first.
    if (impl_->file_manager_->ReadFile(file, data))
      return true;

    // Then try localized file.
    for (StringVector::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->ReadFile(path.c_str(), data))
        return true;
    }
  }
  return false;
}

bool LocalizedFileManager::WriteFile(const char *file, const std::string &data,
                                     bool overwrite) {
  // It makes no sense to support writting to localized file.
  return impl_->file_manager_ ?
         impl_->file_manager_->WriteFile(file, data, overwrite) :
         false;
}

bool LocalizedFileManager::RemoveFile(const char *file) {
  ASSERT(file);

  if (!file || !*file)
    return false;

  bool result = false;
  if (impl_->file_manager_) {
    // Remove all localized and non-localized versions.
    if (impl_->file_manager_->RemoveFile(file))
      result = true;

    for (StringVector::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->RemoveFile(path.c_str()))
        result = true;
    }
  }
  return result;
}

bool LocalizedFileManager::ExtractFile(const char *file, std::string *into_file) {
  ASSERT(file && into_file);

  if (!file || !*file || !into_file)
    return false;

  if (impl_->file_manager_) {
    // Try non-localized file first.
    if (impl_->file_manager_->ExtractFile(file, into_file))
      return true;

    // Then try localized file.
    for (StringVector::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->ExtractFile(path.c_str(), into_file))
        return true;
    }
  }
  return false;
}

bool LocalizedFileManager::FileExists(const char *file, std::string *path) {
  ASSERT(file);

  if (!file || !*file)
    return false;

  if (impl_->file_manager_) {
    // Try non-localized file first, so that we can get the non-localized path.
    if (impl_->file_manager_->FileExists(file, path))
      return true;

    for (StringVector::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->FileExists(path.c_str(), NULL))
        return true;
    }
  }
  return false;
}

bool LocalizedFileManager::IsDirectlyAccessible(const char *file,
                                          std::string *path) {
  // This file manager doesn't support localized feature of this function.
  return impl_->file_manager_ ?
         impl_->file_manager_->IsDirectlyAccessible(file, path) :
         false;
}

std::string LocalizedFileManager::GetFullPath(const char *file) {
  // This file manager doesn't support localized feature of this function.
  if (impl_->file_manager_)
    return impl_->file_manager_->GetFullPath(file);
  return std::string("");
}

uint64_t LocalizedFileManager::GetLastModifiedTime(const char *file) {
  // This file manager doesn't support localized feature of this function.
  return impl_->file_manager_ ?
         impl_->file_manager_->GetLastModifiedTime(file) : 0;
}

bool LocalizedFileManager::EnumerateFiles(const char *dir,
                                          Slot1<bool, const char *> *callback) {
  return impl_->file_manager_ &&
         impl_->file_manager_->EnumerateFiles(dir, callback);
}

} // namespace ggadget
