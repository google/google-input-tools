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

#ifndef GGADGET_XDG_DESKTOP_ENTRY_H__
#define GGADGET_XDG_DESKTOP_ENTRY_H__

#include <string>
#include <ggadget/common.h>

namespace ggadget {
namespace xdg {

/**
 * @defgroup XDGLibrary libggadget-xdg - Utilities to support freedesktop specs.
 * @ingroup SharedLibraries
 *
 * This shared library contains some utilities to support freedesktop specs,
 * such as Desktop Entry spec, etc.
 * @{
 */

const char kDesktopEntryFileExtension[] = ".desktop";
const char kDesktopEntryMimeType[] = "application/x-desktop";

/**
 * A class to hold the information of a desktop entry.
 * See freedesktop Desktop Entry specification:
 * http://www.freedesktop.org/wiki/Specifications/desktop-entry-spec
 *
 * Only supports Application and Link types.
 */
class DesktopEntry {
 public:
  enum Type {
    UNKNOWN,
    APPLICATION,
    LINK
  };

  DesktopEntry();

  /** Constructs a DesktopEntry from a specified desktop file. */
  explicit DesktopEntry(const char *desktop_file);

  ~DesktopEntry();

  /**
   * Loads a specified desktop file.
   * @return true if succeed.
   */
  bool Load(const char *desktop_file);

  /** Checks if this DesktopEntry is valid. */
  bool IsValid() const;

  /** Gets the type of this DesktopEntry. */
  Type GetType() const;

  /** Checks if it needs run in a terminal window. */
  bool NeedTerminal() const;

  /** Checks if it supports startup notify. */
  bool SupportStartupNotify() const;

  /** Checks if it supports a specified mime type. */
  bool SupportMimeType(const char *mime) const;

  /**
   * Checks if the command specified in this desktop file accepts one or
   * multiple urls as command arguments.
   *
   * @param multiple if it's true then check if it accepts multiple urls.
   * @return true if it accepts one or multiple urls.
   */
  bool AcceptURL(bool multiple) const;

  /**
   * Checks if the command specified in this desktop file accepts one or
   * multiple files as command arguments.
   *
   * @param multiple if it's true then check if it accepts multiple files.
   * @return true if it accepts one or multiple files.
   */
  bool AcceptFile(bool multiple) const;

  /** Gets the human readable localized name of this DesktopEntry. */
  std::string GetName() const;

  /** Gets the human readable localized generic name of this DesktopEntry. */
  std::string GetGenericName() const;

  /** Gets the human readable localized comment of this DesktopEntry. */
  std::string GetComment() const;

  /**
   * Gets the icon of this DesktopEntry.
   * The returned icon might be an icon file, or an icon name.
   * Caller needs to interpret it by itself.
   */
  std::string GetIcon() const;

  /** Gets the working directory of this DesktopEntry. */
  std::string GetWorkingDirectory() const;

  /** Gets the mime types supported by this DesktopEntry. */
  std::string GetMimeTypes() const;

  /** Gets the startup wmclass name of this DesktopEntry. */
  std::string GetStartupWMClass() const;

  /** Gets the url of this DesktopEntry. Only valid for Link type entry. */
  std::string GetURL() const;

  /** Gets TryExec value. Only valid for Application. */
  std::string GetTryExec() const;

  /**
   * Gets the command line for executing, with optional files or urls as the
   * arguments.
   *
   * If the DesktopEntry doesn't accept urls or files, then the arguments will
   * be discarded.
   *
   * This method only valid for Application type entry.
   *
   * @param argc The number of arguments specified in argv.
   * @param argv The arguments to be added to the command line.
   */
  std::string GetExecCommand(int argc, const char *argv[]) const;

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DesktopEntry);
};

/** @} */

} // namespace xdg
} // namespace ggadget

#endif // GGADGET_XDG_DESKTOP_ENTRY_H__
