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

#ifndef GGADGET_GTK_TOOLTIP_H__
#define GGADGET_GTK_TOOLTIP_H__

#include <gtk/gtk.h>

namespace ggadget {
namespace gtk {

/**
 * @ingroup GtkLibrary
 * A simple class to show a tooltip at a specified screen and position.
 */
class Tooltip {
 public:
  /**
   * @param show_timeout timeout in milliseconds before actually show a
   *        tooltip, <= 0 means show immediately.
   * @param hide_timeout timeout in milliseconds before hide a tooltip,
   *        <= 0 means no auto hide.
   */
  Tooltip(int show_timeout, int hide_timeout);
  ~Tooltip();

  /**
   * Shows a tooltip. The tooltip will be actually shown after @c show_timeout,
   * near the position of the mouse cursor.
   */
  void Show(const char *tooltip);

  /**
   * Shows a tooltip at specific screen and position.
   * The tooltip will be shown immediately.
   */
  void ShowAtPosition(const char *tooltip,
                      GdkScreen *screen, int x, int y);

  /**
   * Hide tooltip window immediately.
   */
  void Hide();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Tooltip);
  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget


#endif // GGADGET_GTK_TOOLTIP_H__
