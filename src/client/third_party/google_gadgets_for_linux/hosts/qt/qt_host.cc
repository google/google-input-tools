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

#include <limits.h>
#include <string>
#include <map>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtGui/QIcon>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QFontDatabase>
#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/floating_main_view_decorator.h>
#include <ggadget/docked_main_view_decorator.h>
#include <ggadget/popout_main_view_decorator.h>
#include <ggadget/details_view_decorator.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/options_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_utils.h>
#include <ggadget/locales.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/view.h>
#include "qt_host.h"
#include "gadget_browser_host.h"
#include "qt_host_internal.h"

using namespace ggadget;
using namespace ggadget::qt;

namespace hosts {
namespace qt {

QtHost::QtHost(bool composite, int view_debug_mode,
               Gadget::DebugConsoleConfig debug_console)
  : impl_(new Impl(this, composite, view_debug_mode,
                   debug_console)) {
  impl_->InitGadgets();
}

QtHost::~QtHost() {
  DLOG("Removing QtHost");
  delete impl_;
  DLOG("QtHost removed");
}

ViewHostInterface *QtHost::NewViewHost(GadgetInterface *gadget,
                                       ViewHostInterface::Type type) {
  return impl_->NewViewHost(gadget, type);
}

GadgetInterface *QtHost::LoadGadget(const char *path,
                                    const char *options_name,
                                    int instance_id,
                                    bool show_debug_console) {
  return impl_->LoadGadget(path, options_name, instance_id, show_debug_console);
}

void QtHost::RemoveGadget(GadgetInterface *gadget, bool save_data) {
  impl_->RemoveGadget(gadget, save_data);
}

bool QtHost::LoadFont(const char *filename) {
 if (QFontDatabase::addApplicationFont(filename) != -1)
   return true;
 else
   return false;
}

void QtHost::Run() {
}

void QtHost::ShowGadgetDebugConsole(ggadget::GadgetInterface *gadget) {
  impl_->ShowGadgetDebugConsole(gadget);
}

int QtHost::GetDefaultFontSize() {
  return kDefaultFontSize;
}

bool QtHost::OpenURL(const ggadget::GadgetInterface *gadget, const char *url) {
  return ggadget::qt::OpenURL(gadget, url);
}

} // namespace qt
} // namespace hosts
#include "qt_host_internal.moc"
