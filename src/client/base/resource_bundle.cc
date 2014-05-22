/*
  Copyright 2014 Google Inc.

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

#include "base/resource_bundle.h"

#include "strings/stringpiece.h"
#include "ime/shared/base/resource/data_pack.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/stringprintf.h"
#include "base/synchronization/lock.h"
#include "locale/locale_utils.h"
#include "common/string_utils.h"

namespace ime_goopy {
using ime_shared::DataPack;
namespace {
class MockedResourceBundle : public ResourceBundle {
 public:
  MockedResourceBundle() {
  }
  virtual ~MockedResourceBundle() {
  }
  virtual std::wstring GetLocalizedString(int message_id) {
    return WideStringPrintf(L"%d", message_id);
  }
  virtual std::string AddDataPack(const FilePath& path) {
    return "";
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(MockedResourceBundle);
};

const char kDefaultLocale[] = "en";

inline ResourceBundle::FilePath GetFilePathFromPathPattern(
    const ResourceBundle::FilePath& path_pattern,
    const std::string& locale) {
  const char* str = path_pattern.c_str();
  ResourceBundle::FilePath path;
  while (true) {
    const char* remain = strstr(str, ResourceBundle::kLocalePlaceHolder);
    if (remain) {
      path.append(str, remain - str);
      path.append(locale);
      str = remain + strlen(ResourceBundle::kLocalePlaceHolder);
    } else {
      if (*str)
        path.append(str);
      break;
    }
  }
  return path;
}

}  // namespace

using ime_shared::DataPack;
const char ResourceBundle::kLocalePlaceHolder[] = "[LANG]";
ResourceBundle* ResourceBundle::g_shared_instance_ = NULL;

// static
void ResourceBundle::InitSharedInstanceWithLocale(
    const std::string& pref_locale) {
  DCHECK(g_shared_instance_ == NULL) << "ResourceBundle initialized twice";
  g_shared_instance_ = new ResourceBundle();
  g_shared_instance_->pref_locale_ = pref_locale;
}

// static
void ResourceBundle::InitSharedInstanceWithSystemLocale() {
  InitSharedInstanceWithLocale(LocaleUtils::GetUserUILanguage());
}

// static
void ResourceBundle::InitSharedInstanceForTest() {
  DCHECK(g_shared_instance_ == NULL) << "ResourceBundle initialized twice";
  g_shared_instance_ = new MockedResourceBundle();
}

// static
std::string ResourceBundle::AddDataPackToSharedInstance(
      const FilePath& path_pattern) {
  CHECK(g_shared_instance_ != NULL);
  return g_shared_instance_->AddDataPack(path_pattern);
}

// static
void ResourceBundle::CleanupSharedInstance() {
  if (g_shared_instance_) {
    delete g_shared_instance_;
    g_shared_instance_ = NULL;
  }
}

// static
bool ResourceBundle::HasSharedInstance() {
  return g_shared_instance_ != NULL;
}

// static
ResourceBundle& ResourceBundle::GetSharedInstance() {
  // Must call InitSharedInstance before this function.
  CHECK(g_shared_instance_ != NULL);
  return *g_shared_instance_;
}

std::wstring ResourceBundle::GetLocalizedString(int message_id) {
  base::AutoLock lock_scope(data_lock_);

  StringPiece data;
  DataPack::TextEncodingType encoding = DataPack::BINARY;
  for (int i = 0; i < data_packs_.size(); i++) {
    if (data_packs_[i]->GetStringPiece(message_id, &data)) {
      encoding = data_packs_[i]->GetTextEncodingType();
      break;
    }
  }
  if (data.empty()) {
    NOTREACHED() << "unable to find resource: " << message_id;
    return std::wstring();
  }

  // Strings should not be loaded from a data pack that contains binary data.
  DCHECK(encoding == DataPack::UTF16 || encoding == DataPack::UTF8)
      << "requested localized string from binary pack file";

  // Data pack encodes strings as either UTF8 or UTF16.
  std::wstring msg;
  if (encoding == DataPack::UTF16) {
    msg = std::wstring(reinterpret_cast<const wchar_t*>(data.data()),
                       data.length() / 2);
  } else if (encoding == DataPack::UTF8) {
    msg = Utf8ToWide(data.ToString());
  }
  return msg;
}

std::string ResourceBundle::AddDataPack(const FilePath& path_pattern) {
  base::AutoLock lock_scope(data_lock_);
  if (path_patterns_.count(path_pattern))
    DLOG(ERROR) << "Duplicated data pak: " << path_pattern;
  std::string locale;
  scoped_ptr<DataPack> data_pack(
      g_shared_instance_->LoadResourcesDataPak(path_pattern, &locale));
  path_patterns_.insert(path_pattern);
  if (data_pack.get())
    g_shared_instance_->data_packs_.push_back(data_pack.release());
  return locale;
}

void ResourceBundle::ReloadLocaleResources(const std::string& pref_locale) {
  base::AutoLock lock_scope(data_lock_);
  pref_locale_ = pref_locale;
  STLDeleteContainerPointers(data_packs_.begin(), data_packs_.end());
  data_packs_.clear();
  for (std::set<std::string>::const_iterator i = path_patterns_.begin();
       i != path_patterns_.end();
       ++i) {
    DataPack* pack = g_shared_instance_->LoadResourcesDataPak(*i, NULL);
    if (pack)
      data_packs_.push_back(pack);
  }
}

ResourceBundle::ResourceBundle() {
}

ResourceBundle::~ResourceBundle() {
  base::AutoLock lock_scope(data_lock_);
  STLDeleteContainerPointers(data_packs_.begin(), data_packs_.end());
}

DataPack* ResourceBundle::LoadResourcesDataPak(const FilePath& path_pattern,
                                               std::string* locale) {
  FilePath path = path_pattern.find(kLocalePlaceHolder) == path_pattern.npos ?
      path_pattern + kLocalePlaceHolder : path_pattern;
  std::vector<std::string> locales;
  LocaleUtils::GetParentLocales(pref_locale_, &locales);
  scoped_ptr<DataPack> pack(new DataPack);
  for (size_t i = 0; i < locales.size(); ++i) {
    if (pack->Load(GetFilePathFromPathPattern(path, locales[i]))) {
      if (locale) *locale = locales[i];
      return pack.release();
    }
  }
  DLOG(ERROR) << "Unable to find data pack for " << path_pattern
              << "with locale " << pref_locale_ << ", fallback to en";
  if (pack->Load(GetFilePathFromPathPattern(path, kDefaultLocale))) {
    if (locale) *locale = kDefaultLocale;
    return pack.release();
  }
  NOTREACHED() << "Unable to find data pack" << path_pattern;
  if (locale) locale->clear();
  return NULL;
}

}  // namespace ime_goopy
