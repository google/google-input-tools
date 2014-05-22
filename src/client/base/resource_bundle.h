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

#ifndef GOOPY_BASE_RESOURCE_BUNDLE_H_
#define GOOPY_BASE_RESOURCE_BUNDLE_H_
#pragma once

#include <set>
#include <string>
#include <vector>

#include "base/synchronization/lock.h"
#include "base/basictypes.h"
#include "base/scoped_ptr.h"

class StringPiece;

namespace ime_shared {
class DataPack;
}  // namespace ime_shared

namespace ime_goopy {
// ResourceBundle is a central facility to load strings and other resources.
// Every resource is loaded only once.
class ResourceBundle {
 public:
  typedef std::string FilePath;
  static const char kLocalePlaceHolder[];  // = "[LANG]"

  // Initialize the ResourceBundle for this process.  Returns the language
  // selected.
  // NOTE: Mac ignores this and always loads up resources for the language
  // defined by the Cocoa UI (ie-NSBundle does the langange work).
  static void InitSharedInstanceWithLocale(const std::string& pref_locale);

  // Convenient function for InitSharedInstanceWithLocale with system locale.
  static void InitSharedInstanceWithSystemLocale();

  // Initialize the ResourceBundle for testing other codes using ResourceBundle.
  // The initialized ResourceBundle will return mocked string for
  // GetLocalizedString.
  static void InitSharedInstanceForTest();

  // Registers data pack files specified by the |path_pattern| and current local
  // with the global ResourceBundle. This function should be called after
  // calling InitSharedInstanceWithLocale.
  // The |path_pattern| will be something like "/dir/foo_[LANG]_bar", the
  // pattern "[LANG]" (kLocalePlaceHolder) will be replaced with the locale name
  // (with underscore instead of dash). The pattern "[LANG]" can be in any
  // position in the |path_pattern|, and if it does not present in the
  // |path_pattern|, it will be automatically append to |path_pattern|.
  // This function will load the file with path:
  //     path_pattern.replace("[LANG]", locale)
  // And locale is:
  //   1. |pref_locale_| of shared instance
  //   2. parent locales of 1 if the file with |pref_locale_| is not available.
  //   3. "en" if 1,2 are both unavailable.
  // Returns the locale of the data pack loaded.
  static std::string AddDataPackToSharedInstance(const FilePath& path_pattern);

  // Deletes the ResourceBundle for this process if it exists.
  static void CleanupSharedInstance();

  // Returns true after the global resource loader instance has been created.
  static bool HasSharedInstance();

  // Return the global resource loader instance.
  static ResourceBundle& GetSharedInstance();

  // Changes the locale for an already-initialized ResourceBundle. All currently
  // registered datapaks will be reloaded in the new |pref_local|, Future calls
  // to get strings will return the strings for this new locale.
  void ReloadLocaleResources(const std::string& pref_locale);

  // Get a localized string given a message id.  Returns false if the
  // |message_id| is not found.
  virtual std::wstring GetLocalizedString(int message_id);

  // Registers data pack files with the ResourceBundle object.
  // Non-static version of AddDataPackToSharedInstance.
  virtual std::string AddDataPack(const FilePath& path_pattern);

 protected:
  ResourceBundle();
  virtual ~ResourceBundle();

 private:
  ime_shared::DataPack* LoadResourcesDataPak(const FilePath& path_pattern,
                                             std::string* locale);

  base::Lock data_lock_;
  // References to extra data packs loaded via AddDataPackToSharedInstance.
  std::vector<ime_shared::DataPack*> data_packs_;
  // The path patterns of all registered data packs.
  std::set<std::string> path_patterns_;
  static ResourceBundle* g_shared_instance_;
  // The current prefered locale.
  std::string pref_locale_;

  DISALLOW_COPY_AND_ASSIGN(ResourceBundle);
};

}  // namespace ime_shared

#endif  // GOOPY_BASE_RESOURCE_RESOURCE_BUNDLE_H_
