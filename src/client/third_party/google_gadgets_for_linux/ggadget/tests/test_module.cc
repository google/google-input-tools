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

#include <string>
#include "unittest/gtest.h"
#include "ggadget/module.h"
#include "ggadget/logger.h"
#include "ggadget/common.h"
#include "ggadget/element_factory.h"
#include "ggadget/script_context_interface.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/gadget_interface.h"

#define FUNC_NAME_INTERNAL2(a,b)  a##_LTX_##b
#define FUNC_NAME_INTERNAL1(prefix,name)  FUNC_NAME_INTERNAL2(prefix,name)
#define FUNC_NAME(name) FUNC_NAME_INTERNAL1(MODULE_NAME_UNDERSCORE,name)

static int refcount = 0;

extern "C" {
  bool FUNC_NAME(Initialize)() {
    LOG("Initialize module %s", AS_STRING(MODULE_NAME));
    refcount ++;
    EXPECT_EQ(1, refcount);
    return true;
  }

  void FUNC_NAME(Finalize)() {
    LOG("Finalize module %s", AS_STRING(MODULE_NAME));
    refcount --;
    EXPECT_EQ(0, refcount);
  }

  std::string FUNC_NAME(GetModuleName)() {
    LOG("Get module name %s", AS_STRING(MODULE_NAME));
    return std::string(AS_STRING(MODULE_NAME));
  }

  void WithoutPrefix(const char *module_name) {
    LOG("WithoutPrefix() of module %s was called.",
        AS_STRING(MODULE_NAME));
    EXPECT_STREQ(AS_STRING(MODULE_NAME), module_name);
  }

#ifdef ELEMENT_EXTENSION
  bool FUNC_NAME(RegisterElementExtension)(
      ggadget::ElementFactory *factory) {
    LOG("Register Element extension %s, factory=%p",
        AS_STRING(MODULE_NAME), factory);
    return true;
  }
#endif

#ifdef SCRIPT_EXTENSION
  bool FUNC_NAME(RegisterScriptExtension)(
      ggadget::ScriptContextInterface *context,
      ggadget::GadgetInterface *gadget) {
    GGL_UNUSED(gadget);
    LOG("Register Script extension %s, context=%p",
        AS_STRING(MODULE_NAME), context);
    return true;
  }
#endif

#ifdef FRAMEWORK_EXTENSION
  bool FUNC_NAME(RegisterFrameworkExtension)(
      ggadget::ScriptableInterface *framework,
      ggadget::GadgetInterface *gadget) {
    LOG("Register Framework extension %s, framework=%p gadget=%p",
        AS_STRING(MODULE_NAME), framework, gadget);
    return true;
  }
#endif

}
