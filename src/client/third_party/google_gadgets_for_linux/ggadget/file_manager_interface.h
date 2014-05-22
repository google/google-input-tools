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

#ifndef GGADGET_FILE_MANAGER_INTERFACE_H__
#define GGADGET_FILE_MANAGER_INTERFACE_H__

#include <string>
#include <ggadget/common.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;

/**
 * @ingroup Interfaces
 * @ingroup FileManager
 * Handles all file resources and file access used by a gadget.
 */
class FileManagerInterface {
 public:
  virtual ~FileManagerInterface() { }

  /**
   * Returns true if the FileManager instance has been initialized correctly.
   */
  virtual bool IsValid() = 0;

  /**
   * Initialize the @c FileManagerInterface instance.
   *
   * A @c FileManager instance must be initialized before use.
   * @param base_path the base path of this @c FileManager.  All file names
   *     in subsequent operations are relative to this base path.
   * @param create if it's true, then if the target is not available, a new one
   *     will be created.
   * @return @c true if succeeded.
   */
  virtual bool Init(const char *base_path, bool create) = 0;

  /**
   * Reads the contents of a file into a specified string buffer.
   *
   * @param file the file name relative to the base path.
   * @param[out] data returns the file contents.
   * @return @c true if succeeded.
   */
  virtual bool ReadFile(const char *file, std::string *data) = 0;

  /**
   * Writes the specified contents into a specified file.
   * Not all FileManager implementations supports this method.
   *
   * An existing file can't be overwritten, it must be removed first.
   *
   * @param file the file name relative to the base path.
   * @param data the contents to be written.
   * @param overwrite if overwriting allowed if the file exists.
   * @return @c true if succeeded.
   */
  virtual bool WriteFile(const char *file, const std::string &data,
                         bool overwrite) = 0;

  /**
   * Removes a specified file.
   * Not all FileManager implementations supports this method.
   *
   * @param file the file name relative to the base path.
   * @return @c true if succeeded.
   */
  virtual bool RemoveFile(const char *file) = 0;

  /**
   * Extracts the contents of a file into a given file name or into a
   * temporary file.
   *
   * The temporary file will be deleted when the FileManager instance is
   * destroyed.
   *
   * The temporary file will have the same file name as the orginial file, but
   * under a temporary directory.
   *
   * @param file the file name relative to the base path.
   * @param[in, out] into_file if the input value is empty, the file manager
   *     generates a unique temporary file name and save the contents into
   *     that file and return the filename in this parameter on return;
   *     Otherwise the file manager will save it into the given file name.
   * @return @c true if succeeded.
   */
  virtual bool ExtractFile(const char *file, std::string *into_file) = 0;

  /**
   * Check if a file with the given name exists under the base_path of this
   * file manager. Returns @c false if the filename is absolute.
   *
   * @param file the file name relative to the base path.
   * @param[out] path (optional) the actual path name of the file. For a file
   *     in a zip, the value is only for logging purposes. This parameter will
   *     be set even if the file doesn't exist.
   */
  virtual bool FileExists(const char *file, std::string *path) = 0;

  /**
   * Checks if a file name can be accessible directly by the full path returned
   * by GetFullPath() method.
   *
   * If a file name is accessible directly, then functions like fopen can be
   * used directly against the full path returned by GetFullPath(file), or the
   * second parameter of this function.
   *
   * @param file the file name relative to the base path.
   * @param[out] path (optional) the actual path name of the file. For a file
   *     in a zip, the value is only for logging purposes. This parameter will
   *     be set even if the file doesn't exist.
   * @return @c true if the specified file can be accessed directly by the full
   *         path returned by GetFullPath() method.
   */
  virtual bool IsDirectlyAccessible(const char *file, std::string *path) = 0;

  /**
   * Gets the full path of a specified file.
   * For some FileManager implementation (such as ZipFileManager), the returned
   * full path might not be accessible directly. In most case it's only for
   * logging purpose.
   *
   * @param file the file name relative to the base path, can be empty.
   * @return the full path of the specified file name, if the specified file
   *         name is empty then the base path of this file manager will be
   *         returned.
   */
  virtual std::string GetFullPath(const char *file) = 0;

  /**
   * @param file the file name relative to the base path.
   * @return the last modified time of the specified file, or 0 on any error.
   *     The time value is number of milliseconds since EPOCH.
   */
  virtual uint64_t GetLastModifiedTime(const char *file) = 0;

  /**
   * Enumerates all files recursively in a directory.
   * @param dir relative directory name.
   * @param callback its prototype is like:<code>
   *     bool Callback(const char *relative_name);
   *     </code>. The callback can return false to break the enumeration loop.
   *     @c relative_name is the part of file path under @c dir.
   * @return true if the enumration is not canceled by callback.
   */
  virtual bool EnumerateFiles(const char *dir,
                              Slot1<bool, const char *> *callback) = 0;

};

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_INTERFACE_H__
