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

#ifndef GGADGET_XDG_UTILITIES_H__
#define GGADGET_XDG_UTILITIES_H__

#include <string>
#include <vector>
namespace ggadget {

class Permissions;

namespace xdg {

/**
 * @ingroup XDGLibrary
 * @{
 */

const char kDirectoryMimeType[] = "inode/directory";

/**
 * Opens a specified URL by system default application.
 *
 * @param permissions Permissions to check if the operation is allowed.
 * @param url The url to open.
 */
bool OpenURL(const Permissions &permissions, const char *url);

/** Gets the mimetype of a specified file. */
std::string GetFileMimeType(const char *file);

/** Gets xdg standard icon name of a specified mime type. */
std::string GetMimeTypeXDGIcon(const char *mimetype);

/**
 * Gets xdg data dirs for current user, which include the user specific data
 * dirs and the system wide data dirs.
 */
void GetXDGDataDirs(std::vector<std::string> *dirs);

/**
 * Finds an icon file in xdg data dirs.
 * @param icon Name of the icon.
 * @return path to the icon file, or empty string if can't find the icon.
 */
std::string FindIconFileInXDGDataDirs(const char *icon);

/** @} */

} // namespace xdg
} // namespace ggadget

#endif // GGADGET_XDG_UTILITIES_H__
