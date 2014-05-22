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

#ifndef GGADGET_FILE_MANAGER_WRAPPER_H__
#define GGADGET_FILE_MANAGER_WRAPPER_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/file_manager_interface.h>

namespace ggadget {

/**
 * @ingroup FileManager
 * A wrapper FileManager which can manage multiple FileManager instance and dispatch commands
 * to the appropriate FileManager according to file path prefix.
 */
class FileManagerWrapper : public FileManagerInterface {
 public:
  FileManagerWrapper();
  virtual ~FileManagerWrapper();

  /**
   * Registers a new file manager and an associated prefix to be handled
   * by this wrapper. The prefixes associated with this wrapper do not have to
   * be unique. If a prefix is shared by more than one FileManager,
   * the FileManagers will be called in the order in which they were registered.
   * The FileManagers must already be initialized before registration.
   * The registered FileManager instances will be destroyed by the wrapper.
   *
   * One registered prefix must not be the prefix of another (e.g. "abc"
   * and "abcdef").
   *
   * Only one FileManager instance can be associated to the special empty ("")
   * prefix, as the default gadget FileManager.
   */
  bool RegisterFileManager(const char *prefix, FileManagerInterface *fm);

  /**
   * Unregisters a registered file manager.
   * The unregistered file manager will not be deleted.
   */
  bool UnregisterFileManager(const char *prefix, FileManagerInterface *fm);

  virtual bool IsValid();
  /**
   * Initialize the default FileManager instance, if available.
   */
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
  DISALLOW_EVIL_CONSTRUCTORS(FileManagerWrapper);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_WRAPPER_H__
