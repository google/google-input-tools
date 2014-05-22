/*
  Copyright 2014 Google Inc.

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

#ifndef GOOPY_SKIN_SKIN_HOST_H_
#define GOOPY_SKIN_SKIN_HOST_H_

#include "base/basictypes.h"
#include "third_party/google_gadgets_for_linux/ggadget/host_interface.h"

namespace ggadget {

class GadgetInterface;
class ViewHostInterface;

}  // namespace ggadget

namespace ime_goopy {
namespace skin {

class Skin;

class SkinHost : public ggadget::HostInterface {
 public:
  // Create an instance of SkinHost.
  static SkinHost* NewSkinHost();
  virtual ~SkinHost();

 public:
  virtual ggadget::ViewHostInterface* NewViewHost(
      ggadget::GadgetInterface* gadget,
      ggadget::ViewHostInterface::Type type) = 0;

  virtual bool LoadFont(const char* filename) = 0;

  virtual bool OpenURL(const ggadget::GadgetInterface* gadget,
                       const char* url) = 0;

 public:
  virtual ggadget::GadgetInterface* LoadGadget(const char* path,
                                               const char* options_name,
                                               int instance_id,
                                               bool show_debug_console);

  virtual void RemoveGadget(ggadget::GadgetInterface* gadget, bool save_data);

  virtual void ShowGadgetDebugConsole(ggadget::GadgetInterface* gadget);

  virtual int GetDefaultFontSize();

 public:
  // Loads a skin from a the file in |base_path|.
  Skin* LoadSkin(const char* base_path,
                 const char* options_name,
                 const char* ui_locale,
                 int instance_id,
                 bool is_system_account,
                 bool vertical_candidate_layout,
                 bool right_to_left_layout);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_HOST_H_
