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

#include <ggadget/element_factory.h>
#include <ggadget/view.h>
#include <ggadget/gtk/npapi_plugin_element.h>

namespace ggadget {
namespace gtk {

static StringMap g_default_flash_params;

class FlashElement : public NPAPIPluginElement {
 public:
  DEFINE_CLASS_ID(0xb74637c33c404a37, NPAPIPluginElement);
  FlashElement(View *view, const char *name)
      : NPAPIPluginElement(view, name, "application/x-shockwave-flash",
                           g_default_flash_params, false) {
  }

  static BasicElement *CreateInstance(View *view, const char *name) {
    return new FlashElement(view, name);
  }
};

class FlashObjectElement : public NPAPIPluginElement {
 public:
  DEFINE_CLASS_ID(0x69ea255b890d4fc9, NPAPIPluginElement);
  FlashObjectElement(View *view, const char *name)
      : NPAPIPluginElement(view, name, "application/x-shockwave-flash",
                           g_default_flash_params, true) {
  }

  static BasicElement *CreateInstance(View *view, const char *name) {
    return new FlashObjectElement(view, name);
  }
};

} // namespace gtk
} // namespace ggadget

#define Initialize gtk_flash_element_LTX_Initialize
#define Finalize gtk_flash_element_LTX_Finalize
#define RegisterElementExtension gtk_flash_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtk_flash_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtk_flash_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    // For now we prefer windowed mode. See nsapi_plugins.cc for details.
    // ggadget::gtk::g_default_flash_params["wmode"] = "transparent";
    if (factory) {
      LOGI("Register gtk_flash_element extension, using name \"flash\".");
      factory->RegisterElementClass(
          "clsid:D27CDB6E-AE6D-11CF-96B8-444553540000",
          &ggadget::gtk::FlashObjectElement::CreateInstance);
      factory->RegisterElementClass(
          "progid:ShockwaveFlash.ShockwaveFlash.9",
          &ggadget::gtk::FlashObjectElement::CreateInstance);
      factory->RegisterElementClass(
          "progid:ShockwaveFlash.ShockwaveFlash",
          &ggadget::gtk::FlashObjectElement::CreateInstance);
      factory->RegisterElementClass(
          "flash", &ggadget::gtk::FlashElement::CreateInstance);
    }
    return true;
  }
}
