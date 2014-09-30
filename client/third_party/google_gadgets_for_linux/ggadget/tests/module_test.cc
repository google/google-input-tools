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
#include "ggadget/module.h"
#include "ggadget/slot.h"
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

class EnumerateModuleTester {
 public:
  EnumerateModuleTester() : count_(0) {
    char buf[1024];
    getcwd(buf, 1024);
    LOG("Current dir: %s", buf);

    std::string pwd = std::string(buf);

    paths_.push_back(pwd);
    paths_.push_back(pwd + std::string(kDirSeparatorStr) +
                     std::string("test_modules"));
  }

  // There is a trailing colon to test if Module can handle it correctly.
  std::string GetEnvPath() const {
    std::string result;
    for (std::vector<std::string>::const_iterator it = paths_.begin();
         it != paths_.end(); ++it) {
      result.append(*it);
      result.append(kSearchPathSeparatorStr);
    }

    return result;
  }

  bool EnumeratePathsCallback(const char *path) {
    LOG("Enumerate paths %zu: %s", count_, path);

    std::string value;

    if (count_ < paths_.size()) {
      value = paths_[count_];
    } else {
#ifdef _DEBUG
      if (count_ == paths_.size())
        value = "../modules";
      else
#endif
      value = GGL_MODULE_DIR;
    }

    EXPECT_STREQ(value.c_str(), path);

    count_ ++;
    return true;
  }

  bool EnumerateFilesCallback(const char *file) {
    LOG("Enumerate files %zu: %s", count_, file);
    count_ ++;
    return true;
  }

  size_t count_;
  std::vector<std::string> paths_;
};

class ModuleTest : public testing::Test {
 protected:
  virtual void SetUp() {
    std::string env = tester_.GetEnvPath();
    LOG("Set GGL_MODULE_PATH to %s", env.c_str());
    setenv("GGL_MODULE_PATH", env.c_str(), 1);
  }
  virtual void TearDown() {
    unsetenv("GGL_MODULE_PATH");
  }

  EnumerateModuleTester tester_;
};

TEST_F(ModuleTest, EnumerateModulePaths) {
  EXPECT_TRUE(Module::EnumerateModulePaths(
      NewSlot(&tester_, &EnumerateModuleTester::EnumeratePathsCallback)));
}

TEST_F(ModuleTest, EnumerateModuleFiles) {
  EXPECT_TRUE(Module::EnumerateModuleFiles(NULL,
      NewSlot(&tester_, &EnumerateModuleTester::EnumerateFilesCallback)));

  tester_.count_ = 0;

  EXPECT_TRUE(Module::EnumerateModuleFiles("test_modules",
      NewSlot(&tester_, &EnumerateModuleTester::EnumerateFilesCallback)));

}

typedef std::string (*GetModuleNameFunc)();
typedef void (*WithoutPrefixFunc)(const char*);

TEST_F(ModuleTest, ModuleResident) {
  Module *module = new Module();
  Module *another = new Module();
  ASSERT_TRUE(module->Load(kTestModules[0]));
  ASSERT_TRUE(another->Load(kTestModules[0]));
  ASSERT_FALSE(module->IsResident());
  ASSERT_FALSE(another->IsResident());
  ASSERT_TRUE(module->MakeResident());
  ASSERT_TRUE(module->IsResident());
  ASSERT_TRUE(another->IsResident());
  ASSERT_TRUE(module->GetSymbol("GetModuleName") ==
              another->GetSymbol("GetModuleName"));
  ASSERT_FALSE(module->Unload());
  ASSERT_FALSE(another->Unload());

  delete another;
  delete module;
}

TEST_F(ModuleTest, LoadModule) {
  GetModuleNameFunc get_module_name;
  WithoutPrefixFunc without_prefix;

  Module *module = new Module();

  // Test load multiple modules.
  for (size_t i = 1; kTestModules[i]; ++i) {
    ASSERT_TRUE(module->Load(kTestModules[i]));
    ASSERT_TRUE(module->IsValid());
    ASSERT_FALSE(module->IsResident());
    LOG("Module %s loaded.", module->GetPath().c_str());
    EXPECT_STREQ(kTestModulesNormalized[i], module->GetName().c_str());
    get_module_name =
        reinterpret_cast<GetModuleNameFunc>(module->GetSymbol("GetModuleName"));
    ASSERT_TRUE(get_module_name != NULL);
    ASSERT_STREQ(kTestModules[i], get_module_name().c_str());
    without_prefix =
        reinterpret_cast<WithoutPrefixFunc>(module->GetSymbol("WithoutPrefix"));
    ASSERT_TRUE(without_prefix != NULL);
    without_prefix(kTestModules[i]);
    ASSERT_TRUE(module->Unload());
    ASSERT_FALSE(module->IsValid());
  }

  // Test load one module multiple times.
  ASSERT_TRUE(module->Load(kTestModules[1]));
  Module *another = new Module();
  ASSERT_TRUE(another->Load(kTestModules[1]));
  ASSERT_TRUE(module->GetSymbol("GetModuleName") ==
              another->GetSymbol("GetModuleName"));
  ASSERT_TRUE(module->Unload());
  ASSERT_TRUE(another->Unload());

  delete another;
  delete module;
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
