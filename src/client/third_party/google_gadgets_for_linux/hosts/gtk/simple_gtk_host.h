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

#ifndef HOSTS_GTK_SIMPLE_GTK_HOST_H__
#define HOSTS_GTK_SIMPLE_GTK_HOST_H__

#include <string>
#include <ggadget/gadget.h>
#include <ggadget/host_interface.h>
#include <ggadget/options_interface.h>

#include "gtk_host_base.h"

namespace hosts {
namespace gtk {

using ggadget::Gadget;
using ggadget::GadgetInterface;
using ggadget::ViewHostInterface;;

class SimpleGtkHost : public GtkHostBase {
 public:
  SimpleGtkHost(const char *options, int flags, int view_debug_mode,
                Gadget::DebugConsoleConfig debug_console_config);
  virtual ~SimpleGtkHost();
  virtual ViewHostInterface *NewViewHost(GadgetInterface *gadget,
                                         ViewHostInterface::Type type);
  virtual GadgetInterface *LoadGadget(const char *path,
                                      const char *options_name,
                                      int instance_id,
                                      bool show_debug_console);
  virtual void RemoveGadget(GadgetInterface *gadget, bool save_data);
  virtual void ShowGadgetDebugConsole(GadgetInterface *gadget);
  virtual int GetDefaultFontSize();

 public:
  virtual bool IsSafeToExit() const;

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(SimpleGtkHost);
};

} // namespace gtk
} // namespace hosts

#endif // HOSTS_GTK_SIMPLE_GTK_HOST_H__
