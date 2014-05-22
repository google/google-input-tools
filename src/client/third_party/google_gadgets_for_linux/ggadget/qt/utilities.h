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

#ifndef GGADGET_QT_UTILITIES_H__
#define GGADGET_QT_UTILITIES_H__

#include <string>
#include <QtGui/QCursor>

#include <ggadget/gadget_manager_interface.h>
#include <ggadget/permissions.h>

namespace ggadget {

class GadgetInterface;
class MainLoopInterface;

namespace qt {

/**
 * @ingroup QtLibrary
 * @{
 */

Qt::CursorShape GetQtCursorShape(int type);

int GetMouseButtons(const Qt::MouseButtons buttons);

int GetMouseButton(const Qt::MouseButton button);

int GetModifiers(Qt::KeyboardModifiers state);

unsigned int GetKeyCode(int qt_key);

QWidget *NewGadgetDebugConsole(GadgetInterface *gadget, QWidget **widget);

bool OpenURL(const GadgetInterface *gadget, const char *url);

QPixmap GetGadgetIcon(const GadgetInterface *gadget);

void SetGadgetWindowIcon(QWidget *widget, const GadgetInterface *gadget);

/* Get the proper popup position to show a rectangle of @param size for an existing
 * rectangle with geometry @param rect
 */
QPoint GetPopupPosition(const QRect &rect, const QSize &size);

enum GGLInitFlag {
  GGL_INIT_FLAG_NONE = 0,
  GGL_INIT_FLAG_LONG_LOG = 0x1,
  GGL_INIT_FLAG_COLLECTOR = 0x2
};
Q_DECLARE_FLAGS(GGLInitFlags, GGLInitFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(GGLInitFlags)

/* Initialize the GGL runtime system.
 *
 * @main_loop If null, InitGGL will create main_loop itself
 * @user_agent Application name
 * @profile_dir If null, kDefaultProfileDirectory will be used
 * @extensions Extensions to be loaded.
 */
bool InitGGL(
    MainLoopInterface *main_loop,
    const char *user_agent,
    const char *profile_dir,
    const char *extensions[],
    int log_level,
    GGLInitFlags flags,
    QString *error_msg
    );
/*
 * Show a dialog asking user confirm gadget installation
 */
bool ConfirmGadget(GadgetManagerInterface *gadget_manager,
                   int gadget_id);

/** @} */

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_UTILITIES_H__
