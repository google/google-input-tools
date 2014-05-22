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

#ifndef HOSTS_QT_HOST_H__
#define HOSTS_QT_HOST_H__

#include <string>
#include <ggadget/decorated_view_host.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_interface.h>

namespace hosts {
namespace qt {

using namespace ggadget;
using ggadget::GadgetInterface;
using ggadget::HostInterface;
using ggadget::ViewHostInterface;
using ggadget::ViewInterface;

class QtHost : public ggadget::HostInterface {
 public:
  QtHost(bool composite, int view_debug_mode,
         Gadget::DebugConsoleConfig debug_console);
  virtual ~QtHost();
  virtual ViewHostInterface *NewViewHost(GadgetInterface *gadget,
                                         ViewHostInterface::Type type);
  virtual GadgetInterface *LoadGadget(const char *path,
                                      const char *options_name,
                                      int instance_id,
                                      bool show_debug_console);
  virtual void RemoveGadget(GadgetInterface *gadget, bool save_data);
  virtual bool LoadFont(const char *filename);
  virtual void Run();
  virtual void ShowGadgetDebugConsole(GadgetInterface *gadget);
  virtual int GetDefaultFontSize();
  virtual bool OpenURL(const GadgetInterface *gadget, const char *url);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(QtHost);
};

} // namespace qt
} // namespace hosts

#endif // HOSTS_QT_HOST_H__
