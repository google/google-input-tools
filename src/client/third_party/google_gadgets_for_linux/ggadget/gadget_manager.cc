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

#include "logger.h"
#include "gadget_manager_interface.h"

namespace ggadget {

static GadgetManagerInterface *g_gadget_manager = NULL;

bool SetGadgetManager(GadgetManagerInterface *gadget_manager) {
  ASSERT(!g_gadget_manager && gadget_manager);
  if (!g_gadget_manager && gadget_manager) {
    g_gadget_manager = gadget_manager;
    return true;
  }
  return false;
}

GadgetManagerInterface *GetGadgetManager() {
  EXPECT_M(g_gadget_manager,
           ("The global gadget manager has not been set yet."));
  return g_gadget_manager;
}

} // namespace ggadget
