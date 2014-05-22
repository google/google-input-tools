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

#ifndef GGADGET_FILE_MANAGER_FACTORY_H__
#define GGADGET_FILE_MANAGER_FACTORY_H__

#include <string>
#include <ggadget/file_manager_interface.h>

namespace ggadget {

/**
 * @defgroup FileManager FileManager
 * @ingroup CoreLibrary
 * FileManager related classes.
 * @{
 */

/**
 * Creates a FileManager instance using a proper FileManager implementation.
 * It can only create a FileManager instance against an existent base path.
 *
 * Currently there are two builtin FileManager implementation:
 * 1. ZipFileManager, which can read/write file in a zip archive file.
 * 2. DirFileManager, which can read/write file in a real directory.
 *
 * @param base_path the base path of the newly created FileManager instance,
 *        it must exists before creating the file manager.
 */
FileManagerInterface *
CreateFileManager(const char *base_path);

/**
 * Sets a specified FileManager instance as the global singleton, so that it
 * can be used in any components to access global resources.
 *
 * All global resources, such as global files/images/profiles, shall be managed
 * by global FileManager instance.
 *
 * This function can only be called by the host program at very beginning, and
 * can only be called once.
 *
 * @return @c true if succeeded.
 */
bool SetGlobalFileManager(FileManagerInterface *manager);

/**
 * Gets the global FileManager singleton previously set by calling
 * SetGlobalFileManager() method.
 *
 * The caller shall not delete the returned FileManager instance.
 */
FileManagerInterface *GetGlobalFileManager();

/** @} */

} // namespace ggadget

#endif  // GGADGET_FILE_MANAGER_INTERFACE_H__
