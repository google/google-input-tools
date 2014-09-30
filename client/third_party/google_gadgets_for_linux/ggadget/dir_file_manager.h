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

#ifndef GGADGET_DIR_FILE_MANAGER_H__
#define GGADGET_DIR_FILE_MANAGER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/file_manager_interface.h>

namespace ggadget {

/**
 * @ingroup FileManager
 * Handles files in a real directory.
 */
class DirFileManager : public FileManagerInterface {
 public:
  DirFileManager();
  virtual ~DirFileManager();

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

 public:
  /**
   * Handy function to new a ZipFileManager instance, and initialize it.
   *
   * @return the newly created and initialized ZipFileManager instance.
   *         Returns NULL when fail.
   */
  static FileManagerInterface *Create(const char *base_path, bool create);

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(DirFileManager);
};

} // namespace ggadget

#endif  // GGADGET_DIR_FILE_MANAGER_H__
