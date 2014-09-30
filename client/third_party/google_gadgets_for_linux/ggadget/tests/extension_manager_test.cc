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

#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "ggadget/logger.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/extension_manager.h"
#include "ggadget/slot.h"
#include "ggadget/system_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

static const char *kTestModules[] = {
  "foo-module",
  "bar-module",
  "fake-module",
  "tux-module",
  NULL
};

static const char *kTestModulesNormalized[] = {
  "foo_module",
  "bar_module",
  "fake_module",
  "tux_module",
  NULL
};

static const bool kTestModulesIsExtension[] = {
  true,
  true,
  false,
  true,
};

static ExtensionManager *g_manager = NULL;

class ExtensionManagerTest : public testing::Test {
 protected:
  virtual void SetUp() {
    char buf[1024];
    getcwd(buf, 1024);
    LOG("Current dir: %s", buf);

    std::string path =
        BuildPath(kSearchPathSeparatorStr, buf,
                  BuildFilePath(buf, "test_modules", NULL).c_str(),
                  NULL);

    LOG("Set GGL_MODULE_PATH to %s", path.c_str());
    setenv("GGL_MODULE_PATH", path.c_str(), 1);
  }

  virtual void TearDown() {
    unsetenv("GGL_MODULE_PATH");
  }
};


class EnumerateExtensionCallback {
 public:
  EnumerateExtensionCallback(ExtensionManager *manager)
    : manager_(manager) {
  }

  bool Callback(const char *name, const char *norm_name) {
    MultipleExtensionRegisterWrapper reg_wrapper;
    ElementExtensionRegister element_reg(NULL);
    ScriptExtensionRegister script_reg(NULL, NULL);
    FrameworkExtensionRegister framework_reg(NULL, NULL);
    reg_wrapper.AddExtensionRegister(&element_reg);
    reg_wrapper.AddExtensionRegister(&script_reg);
    reg_wrapper.AddExtensionRegister(&framework_reg);

    LOG("Enumerate Extension: %s - %s", name, norm_name);
    bool result = false;
    for (size_t i = 0; kTestModules[i]; ++i) {
      if (strcmp(name, kTestModules[i]) == 0) {
        result = true;
        EXPECT_STREQ(kTestModulesNormalized[i], norm_name);
        if (kTestModulesIsExtension[i])
          EXPECT_TRUE(manager_->RegisterExtension(name, &reg_wrapper));
        else
          EXPECT_FALSE(manager_->RegisterExtension(name, &reg_wrapper));
        break;
      }
    }
    EXPECT_TRUE(result);

    return true;
  }

  ExtensionManager *manager_;
};

TEST_F(ExtensionManagerTest, LoadAndEnumerateAndRegister) {
  MultipleExtensionRegisterWrapper reg_wrapper;
  ElementExtensionRegister element_reg(NULL);
  ScriptExtensionRegister script_reg(NULL, NULL);
  FrameworkExtensionRegister framework_reg(NULL, NULL);
  reg_wrapper.AddExtensionRegister(&element_reg);
  reg_wrapper.AddExtensionRegister(&script_reg);
  reg_wrapper.AddExtensionRegister(&framework_reg);

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      ASSERT_TRUE(g_manager->LoadExtension(kTestModules[i], false));
  }

  EXPECT_TRUE(g_manager->RegisterLoadedExtensions(&reg_wrapper));

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (!kTestModulesIsExtension[i])
      ASSERT_TRUE(g_manager->LoadExtension(kTestModules[i], false));
  }

  EXPECT_FALSE(g_manager->RegisterLoadedExtensions(&reg_wrapper));

  EnumerateExtensionCallback callback(g_manager);
  ASSERT_TRUE(g_manager->EnumerateLoadedExtensions(
      NewSlot(&callback, &EnumerateExtensionCallback::Callback)));

  // Same extension can be loaded twice.
  for (size_t i = 0; kTestModules[i]; ++i)
    ASSERT_TRUE(g_manager->LoadExtension(kTestModules[i], false));
}

TEST_F(ExtensionManagerTest, Resident) {
  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      ASSERT_TRUE(g_manager->LoadExtension(kTestModules[i], true));
  }

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      EXPECT_FALSE(g_manager->UnloadExtension(kTestModules[i]));
    else
      EXPECT_TRUE(g_manager->UnloadExtension(kTestModules[i]));
  }
}

TEST_F(ExtensionManagerTest, GlobalManager) {
  MultipleExtensionRegisterWrapper reg_wrapper;
  ElementExtensionRegister element_reg(NULL);
  ScriptExtensionRegister script_reg(NULL, NULL);
  FrameworkExtensionRegister framework_reg(NULL, NULL);
  reg_wrapper.AddExtensionRegister(&element_reg);
  reg_wrapper.AddExtensionRegister(&script_reg);
  reg_wrapper.AddExtensionRegister(&framework_reg);

  ASSERT_TRUE(NULL == ExtensionManager::GetGlobalExtensionManager());

  ASSERT_TRUE(ExtensionManager::SetGlobalExtensionManager(g_manager));
  ASSERT_EQ(g_manager, ExtensionManager::GetGlobalExtensionManager());
  EXPECT_FALSE(ExtensionManager::SetGlobalExtensionManager(g_manager));
  g_manager->SetReadonly();

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      ASSERT_FALSE(g_manager->LoadExtension(kTestModules[i], false));
  }

  for (size_t i = 0; kTestModules[i]; ++i) {
    if (kTestModulesIsExtension[i])
      EXPECT_FALSE(g_manager->UnloadExtension(kTestModules[i]));
  }

  EXPECT_TRUE(g_manager->RegisterLoadedExtensions(&reg_wrapper));
  EXPECT_FALSE(g_manager->Destroy());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  g_manager = ExtensionManager::CreateExtensionManager();

  return RUN_ALL_TESTS();
}
