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

#ifndef HOSTS_GTK_GADGET_BROWSER_HOST_H__
#define HOSTS_GTK_GADGET_BROWSER_HOST_H__

#include <ggadget/gadget_consts.h>
#include <ggadget/host_interface.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>

namespace hosts {
namespace gtk {

// A special Host for Gadget browser to show browser in a decorated window.
class GadgetBrowserHost : public ggadget::HostInterface {
 public:
  GadgetBrowserHost(ggadget::HostInterface *owner, int view_debug_mode)
    : owner_(owner), view_debug_mode_(view_debug_mode) {
  }
  virtual ViewHostInterface *NewViewHost(GadgetInterface *,
                                         ViewHostInterface::Type type) {
    int flags = ggadget::gtk::SingleViewHost::WM_MANAGEABLE |
                ggadget::gtk::SingleViewHost::REMOVE_ON_CLOSE |
                ggadget::gtk::SingleViewHost::RECORD_STATES;
    return new ggadget::gtk::SingleViewHost(type, 1.0, flags, view_debug_mode_);
  }
  virtual GadgetInterface *LoadGadget(const char *path,
                                      const char *options_name,
                                      int instance_id,
                                      bool show_debug_console) {
    return owner_->LoadGadget(path, options_name, instance_id,
                              show_debug_console);
  }
  virtual void RemoveGadget(GadgetInterface *gadget, bool save_data) {
    GGL_UNUSED(save_data);
    ggadget::GetGadgetManager()->RemoveGadgetInstance(gadget->GetInstanceID());
  }
  virtual bool LoadFont(const char *filename) {
    return owner_->LoadFont(filename);
  }
  virtual void ShowGadgetDebugConsole(GadgetInterface *) {}
  virtual int GetDefaultFontSize() { return ggadget::kDefaultFontSize; }
  virtual bool OpenURL(const GadgetInterface *gadget, const char *url) {
    return owner_->OpenURL(gadget, url);
  }
 private:
  ggadget::HostInterface *owner_;
  int view_debug_mode_;
};

} // namespace gtk
} // namespace hosts

#endif // HOSTS_GTK_GADGET_BROWSER_HOST_H__
