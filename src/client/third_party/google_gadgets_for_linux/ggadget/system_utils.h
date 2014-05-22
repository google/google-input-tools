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

/**
 * This file includes common utils related to system level functionalities,
 * such as path, directory and file operations.
 */

#ifndef GGADGET_SYSTEM_UTILS_H__
#define GGADGET_SYSTEM_UTILS_H__

#include <cstdarg>
#include <string>
#include <ggadget/common.h>

namespace ggadget {

/**
 * @defgroup SystemUtilities System utilities
 * @ingroup Utilities
 * @{
 */

/**
 * Build a path using specified separator.
 *
 * @param separator The separator to be used, or if it's NULL, the system
 * default directory separator will be used.
 * @param element The first path element followed by one or more elements,
 * terminated by NULL.
 */
std::string BuildPath(const char *separator, const char *element, ...);

/**
 * Same as BuildPath, but accepting a va_list.
 */
std::string BuildPathV(const char *separator, const char *element, va_list ap);

/**
 * Build a file path using system default dir separator.
 * On Unix systems, it's identical to BuildPath(kDirSeparatorStr, ...);
 */
std::string BuildFilePath(const char *element, ...);

/**
 * Same as BuildFilePath, but accepting a va_list.
 */
std::string BuildFilePathV(const char *element, va_list ap);

/**
 * Split file path into directory and filename.
 *
 * @param[in] path The full file path to be splitted.
 * @param[out] dir The directory part of the specified path will be stored in
 * it, can be NULL to discard this part.
 * @param[out] filename The filename part of the specified path will be stored
 * in it, can be NULL to discard this part.
 * @return true if both directory and filename are available.
 */
bool SplitFilePath(const char *path, std::string *dir, std::string *filename);

/**
 * Normalize a file path.
 *
 * - Remove redundant dir separators.
 * - Replace '\' with '/' on non-Windows platforms.
 * - Remove all "." and ".." components.
 *
 * The file path to be normalized shall be an absolute path, otherwise the
 * result might be incorrect.
 *
 * @param path the path to be normalized.
 * @return the normalized result.
 */
std::string NormalizeFilePath(const char *path);

/**
 * Ensure each directory in the directory path exists in the file system,
 * or if not, create it.
 *
 * @param path the path of a directory. Normally it should be a absolute path,
 *     but relative path is also supported.
 * @return true if success.
 */
bool EnsureDirectories(const char *path);

/**
 * Read the contents of a file into memory.
 *
 * The maximum size of loaded content will be no more than 20M bytes.
 *
 * @param path the path of a directory. Normally it should be a absolute path,
 *     but relative path is also supported.
 * @param[out] data returns the file contents.
 * @return true if success.
 */
bool ReadFileContents(const char *path, std::string *content);

/**
 * Write memory data into a file.
 *
 * @param path the path of a directory. Normally it should be a absolute path,
 *     but relative path is also supported.
 * @param data the data to be written.
 * @return true if success.
 */
bool WriteFileContents(const char *path, const std::string &content);

/**
 * Gets the current working directory.
 *
 * @return the absolute path of the current working directory.
 */
std::string GetCurrentDirectory();

/**
 * Gets current user's home directory.
 *
 * @return the absolute path of the current user's home directory.
 */
std::string GetHomeDirectory();

/**
 * Gets the absolute path of a given relative or absolute path.
 */
std::string GetAbsolutePath(const char *path);

/** Checks if the specified file path is an absolute path. */
bool IsAbsolutePath(const char *path);

/**
 * Creates an unique temporary directory that can be used to store temporary
 * files.
 *
 * The created temporary directory shall be removed by caller when it's not
 * used anymore.
 *
 * @param prefix A prefix used in the directory name.
 * @param[out] path returns the path of the newly created temporary directory.
 * @return true if the directory is created successfully.
 */
bool CreateTempDirectory(const char *prefix, std::string *path);

/**
 * Removes a directory tree, all files under the specified directory will be
 * removed.
 *
 * @param path the path of the directory tree.
 * @param remove_readonly_files @c true if also force to remove readonly files,
 *     otherwise readonly files cause error.
 * @return true if succeeded.
 */
bool RemoveDirectory(const char *path, bool remove_readonly_files);

/**
 * Returns the current system locale information. In most cases,
 * GetSystemLocaleName() defined in locales.h is more useful than this.
 *
 * @param[out] language returns the system language id, such as "en", "zh".
 * @param[out] territory returns the system territory id, such as "US", "CN",
 *             it's in uppercase.
 * @return true if succeeded.
 */
bool GetSystemLocaleInfo(std::string *language, std::string *territory);

/**
 * Puts the process into background.
 */
void Daemonize();

/**
 * Copies source file to the dest.
 * @return true if succeeded.
 */
bool CopyFile(const char *src, const char *dest);

/**
 * Gets full path of a specified system command.
 *
 * If the specified command is an absolute path and it's executable, then just
 * return it.
 *
 * Otherwise search the command in PATH and return the full patch if it's
 * available and executable.
 */
std::string GetFullPathOfSystemCommand(const char *command);

/**
 * Get the path of a gadget which was pre-installed into the system.
 * Such as the path of rss_gadget.gg, etc.
 *
 * @param basename of the gadget, without ".gg" suffix.
 * @return the full path of the gadget, or an empty string if the gadget is
 *     not available.
 */
std::string GetSystemGadgetPath(const char *basename);

/**
 * Gets the real name of the current user.
 */
std::string GetUserRealName();

/**
 * Gets the login name of the current user.
 */
std::string GetUserLoginName();

/** @} */

} // namespace ggadget

#endif // GGADGET_SYSTEM_UTILS_H__
