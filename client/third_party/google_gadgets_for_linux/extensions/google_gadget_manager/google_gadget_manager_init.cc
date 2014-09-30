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

#include "google_gadget_manager.h"

static ggadget::google::GoogleGadgetManager *g_gadget_manager = NULL;

#define Initialize google_gadget_manager_LTX_Initialize
#define Finalize google_gadget_manager_LTX_Finalize

extern "C" {
  bool Initialize() {
    LOGI("Initialize google_gadget_manager extension.");
    g_gadget_manager = new ggadget::google::GoogleGadgetManager();
    return ggadget::SetGadgetManager(g_gadget_manager);
  }

  void Finalize() {
    LOGI("Finalize google_gadget_manager extension.");
  }
}
