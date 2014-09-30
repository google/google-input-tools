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

#ifndef GGADGET_GTK_HOTKEY_H__
#define GGADGET_GTK_HOTKEY_H__

#include <string>
#include <ggadget/slot.h>
#include <ggadget/signals.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

namespace ggadget {
namespace gtk {

/**
 * @ingroup GtkLibrary
 * @{
 */

/** A class to grab a specified hotkey. */
class HotKeyGrabber {
 public:
  /** Creates a HotKeyGrabber for the default screen */
  HotKeyGrabber();

  /** Creates a HotKeyGrabber for a specified screen */
  explicit HotKeyGrabber(GdkScreen *screen);
  ~HotKeyGrabber();

  /** Sets the screen to be grabbed. */
  void SetScreen(GdkScreen *screen);

  /**
   * Sets the hotkey to be grabbed.
   *
   * @param hotkey The string representing the hotkey, which can be get by
   *        using @c HotKeyDialog.
   * @return true if success.
   */
  bool SetHotKey(const std::string &hotkey);

  /** Gets the hotkey to be grabbed. */
  std::string GetHotKey() const;

  /**
   * Enables or disables hotkey grabbing.
   *
   * This method must be called after setting a valid hotkey.
   */
  void SetEnableGrabbing(bool grabbing);

  /** Checks if it's grabbing. */
  bool IsGrabbing() const;

  /**
   * Connects a slot to the hotkey pressed signal.
   *
   * The slot will be called when the hotkey is pressed.
   */
  Connection *ConnectOnHotKeyPressed(Slot0<void> *slot);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(HotKeyGrabber);
  class Impl;
  Impl *impl_;
};

/** A class to show a dialog to let user input a hotkey. */
class HotKeyDialog {
 public:
  HotKeyDialog();
  ~HotKeyDialog();

  /**
   * Sets the title of the dialog.
   * A default title will be used if it's not set.
   */
  void SetTitle(const char *title);

  /**
   * Sets the prompt displayed in the dialog.
   * A default prompt will be used if it's not set.
   */
  void SetPrompt(const char *prompt);

  /**
   * Shows the hotkey dialog.
   *
   * This method will block until user inputs a valid hotkey or cancel the
   * dialog.
   *
   * @return true if a valid hotkey is inputted, an empty hotkey is also valid.
   */
  bool Show();

  /** Sets the hotkey to be displayed initially. */
  void SetHotKey(const std::string &hotkey);

  /** Gets the hotkey set by user. */
  std::string GetHotKey() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(HotKeyDialog);
  class Impl;
  Impl *impl_;
};

/** @} */

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_HOTKEY_H__
