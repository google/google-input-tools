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

#include "options_interface.h"

namespace ggadget {

static OptionsFactory g_options_factory = NULL;
static OptionsInterface *g_global_options = NULL;

bool SetOptionsFactory(OptionsFactory options_factory) {
  ASSERT(!g_options_factory && options_factory);
  if (!g_options_factory && options_factory) {
    g_options_factory = options_factory;
    return true;
  }
  return false;
}

OptionsInterface *CreateOptions(const char *name) {
  EXPECT_M(g_options_factory, ("The options factory has not been set yet."));
  return g_options_factory ? g_options_factory(name) : NULL;
}

bool SetGlobalOptions(OptionsInterface *global_options) {
  ASSERT(!g_global_options && global_options);
  if (!g_global_options && global_options) {
    g_global_options = global_options;
    return true;
  }
  return false;
}

OptionsInterface *GetGlobalOptions() {
  EXPECT_M(g_global_options, ("The global options has not been set yet."));
  return g_global_options;
}

} // namespace ggadget
