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

#ifndef GGADGET_LOCALIZED_FILE_MANAGER_H__
#define GGADGET_LOCALIZED_FILE_MANAGER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/file_manager_interface.h>

namespace ggadget {

/**
 * @ingroup FileManager
 * A wrapper FileManager that supports localized files.
 *
 * It dispatches all requests to a real FileManager implementation, but does
 * additional file searching rules. It searches a file in the paths in the
 * following order:
 *  - @c file (in the @c base_path);
 *  - @c lang/file (e.g. @c zh/myfile);
 *    (Here @c lang is the short name of current user's locale got from
 *     @c GetLocaleShortName(), or the original two-segment locale name
 *     if the locale has no short name.
 *  - @c windows_locale_id/file (for Windows compatibility, e.g. 2052/myfile);
 *    (Here @c windows_locale_id is the corresponding Windows LCID of current
 *     user's locale got from @c GetLocaleWindowIDString().)
 *  - @c en/file;
 *  - @c 1033/file (for Windows compatibility).
 */
class LocalizedFileManager : public FileManagerInterface {
 public:
  LocalizedFileManager();

  /**
   * Constructor.
   *
   * @param file_manager a FileManager instance to do the real task, it'll be
   *        destroyed, when this LocalizedFileManager instance is destroyed.
   */
  explicit LocalizedFileManager(FileManagerInterface *file_manager);
  LocalizedFileManager(FileManagerInterface *file_manager, const char *locale);
  virtual ~LocalizedFileManager();

  /**
   * Attaches a FileManager instance to do the real tasks, it'll be destroyed
   * when this LocalizedFileManager instance is destroyed.
   *
   * @return true if succeeded.
   */
  bool Attach(FileManagerInterface *file_manager);

  virtual bool IsValid();
  virtual bool Init(const char *base_path, bool create);
  virtual bool ReadFile(const char *file, std::string *data);
  virtual bool WriteFile(const char *file, const std::string &data,
                         bool overwrite);
  virtual bool RemoveFile(const char *file);
  virtual bool ExtractFile(const char *file, std::string *into_file);
  virtual bool FileExists(const char *file, std::string *path);
  virtual bool IsDirectlyAccessible(const char *file, std::string *path);
  virtual std::string GetFullPath(const char *file);
  virtual uint64_t GetLastModifiedTime(const char *file);
  virtual bool EnumerateFiles(const char *dir,
                              Slot1<bool, const char *> *callback);

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(LocalizedFileManager);
};

} // namespace ggadget

#endif  // GGADGET_LOCALIZED_FILE_MANAGER_H__
