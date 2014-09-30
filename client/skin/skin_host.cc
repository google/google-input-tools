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

#include "skin/skin_host.h"

#include <string>
#include "base/scoped_ptr.h"
#include "skin/skin.h"
#include "third_party/google_gadgets_for_linux/ggadget/gadget_consts.h"
#include "third_party/google_gadgets_for_linux/ggadget/permissions.h"

using ggadget::GadgetInterface;
using ggadget::Permissions;
using ggadget::ViewHostInterface;


namespace ime_goopy {
namespace skin {

SkinHost::~SkinHost() {
}

int SkinHost::GetDefaultFontSize() {
  return ggadget::kDefaultFontSize;
}

void SkinHost::ShowGadgetDebugConsole(GadgetInterface* gadget) {
  // This function is not needed.
  return;
}

GadgetInterface* SkinHost::LoadGadget(const char* path,
                                      const char* options_name,
                                      int instance_id,
                                      bool show_debug_console) {
  // This function is not needed.
  return LoadSkin(path, options_name, NULL, instance_id, false, false, false);
}

void SkinHost::RemoveGadget(GadgetInterface* gadget, bool save_data) {
  // This function is not needed.
  return;
}

Skin* SkinHost::LoadSkin(const char* base_path,
                         const char* options_name,
                         const char* ui_locale,
                         int instance_id,
                         bool is_system_account,
                         bool vertical_candidate_layout,
                         bool right_to_left_layout) {
  Permissions global_permissions;
  if (!is_system_account)
    global_permissions.SetGranted(Permissions::NETWORK, true);
  scoped_ptr<Skin> new_skin(new Skin(this, base_path, options_name, ui_locale,
                            instance_id, global_permissions,
                            vertical_candidate_layout, right_to_left_layout));
  return new_skin.release();
}

}  // namespace skin
}  // namespace ime_goopy
