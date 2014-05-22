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

// As we don't compile extension manager and module support in goopy,
// we need to static link to the extensions.
#if defined(GGL_FOR_GOOPY)

#include "extensions/extensions.h"

#define INIT_EXTENSIONS(argc, argv, kExtensions) \
  ggadget::extensions::Initialize();

#else // GGL_FOR_GOOPY

#include <unistd.h>
#include <cstdlib>
#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "ggadget/system_utils.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/extension_manager.h"

#ifndef GGADGET_TESTS_INIT_EXTENSIONS_H__
#define GGADGET_TESTS_INIT_EXTENSIONS_H__

#define INIT_EXTENSIONS(argc, argv, extensions) \
    InitExtensions( \
        (argc) > 1 ? (argc) - 1 : static_cast<int>(arraysize(extensions)), \
        (argc) > 1 ? const_cast<const char **>(argv) + 1 : (extensions))


inline void InitExtensions(int argc, const char **argv) {
  // Setup GGL_MODULE_PATH env.
  char buf[1024];
  getcwd(buf, 1024);
  LOG("Current dir: %s", buf);

  std::string path = ggadget::BuildPath(
      ggadget::kSearchPathSeparatorStr, buf,
      ggadget::BuildFilePath(buf, "../../extensions", NULL).c_str(),
      ggadget::BuildFilePath(buf, "../../../extensions", NULL).c_str(),
      NULL);

  LOG("Set GGL_MODULE_PATH to %s", path.c_str());
  setenv("GGL_MODULE_PATH", path.c_str(), 1);

  // Load XMLHttpRequest module.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();
  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  for (int i = 0; i < argc; i++)
    ext_manager->LoadExtension(argv[i], false);
  ext_manager->SetReadonly();
}

#endif // GGL_FOR_GOOPY

#endif // GGADGET_TESTS_INIT_EXTENSIONS_H__
