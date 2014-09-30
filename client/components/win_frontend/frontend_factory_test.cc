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
#include "components/win_frontend/frontend_factory.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "common/framework_interface.h"
#include "ipc/testing.h"

namespace ime_goopy {

class FrontendFactoryTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
  }
  virtual void TearDown() {
  }
};

class MockContext : public ContextInterface {
 public:
  explicit MockContext(int id) : id_(id) {
  }

  virtual Platform GetPlatform() OVERRIDE {
    return PLATFORM_WINDOWS_IMM;
  }
  virtual EngineInterface* GetEngine() OVERRIDE {
    return NULL;
  }
  virtual bool ShouldShow(UIComponent ui_type) OVERRIDE {
    return false;
  }
  // Returns a context unique id.
  virtual ContextInterface::ContextId GetId() OVERRIDE {
    return reinterpret_cast<ContextInterface::ContextId>(id_);
  }

 private:
  int id_;
  DISALLOW_COPY_AND_ASSIGN(MockContext);
};

TEST_F(FrontendFactoryTest, BaseTest) {
  FrontendFactory* factory = FrontendFactory::GetThreadLocalInstance();
  ASSERT_TRUE(factory != NULL);
  ASSERT_EQ(0, factory->frontends_.size());

  EngineInterface* frontend = factory->CreateFrontend();
  ASSERT_EQ(1, factory->frontends_.size());

  // Should always create successful.
  ASSERT_TRUE(frontend != NULL);
#if defined(OS_WIN)
  HWND var = ::CreateWindow(L"MESSAGE", NULL, WS_DISABLED, 0, 0, 0, 0, NULL,
                            NULL, NULL, NULL);
  ASSERT_TRUE(var);
#else
  // Should return NULL on unknown id.
  int int_val = 0;
  ContextInterface::ContextId var =
      reinterpret_cast<ContextInterface::ContextId>(&int_val);
  ASSERT_EQ(NULL, factory->UnshelveFrontend(var));
#endif

  // Should shelve nothing on unknown frontend.
  EngineInterface* fake_frontend = reinterpret_cast<EngineInterface*>(0x1);
  ASSERT_FALSE(factory->ShelveFrontend(var, fake_frontend));
  ASSERT_TRUE(factory->shelved_frontends_.empty());

  // Shelve a frontend with valid id will success.
  ASSERT_TRUE(factory->ShelveFrontend(var, frontend));
  ASSERT_EQ(1, factory->shelved_frontends_.size());

  // Shelve a frontend without old id will do nothing.
  ASSERT_FALSE(factory->ShelveFrontend(var, frontend));
  ASSERT_EQ(1, factory->shelved_frontends_.size());

  // Unshelve with invalid id will do nothing.
  // Unshelve with valid id will success.
  int16 short_val = 0;
  ContextInterface::ContextId fake_var =
      reinterpret_cast<ContextInterface::ContextId>(&short_val);
  ASSERT_EQ(NULL, factory->UnshelveFrontend(fake_var));

  // UnshelveOrCreate with id equals to NULL will return a new frontend.
  ASSERT_TRUE(factory->UnshelveOrCreateFrontend(NULL) != NULL);
  ASSERT_EQ(1, factory->shelved_frontends_.size());
  ASSERT_EQ(2, factory->frontends_.size());

  // UnshelveOrCreate always return non-null and given unknown id will cause to
  // create a new one.
  int16 new_short_val = 0;
  ContextInterface::ContextId new_id =
      reinterpret_cast<ContextInterface::ContextId>(&new_short_val);
  ASSERT_EQ(NULL, factory->UnshelveFrontend(new_id));
  EngineInterface* new_frontend = factory->UnshelveOrCreateFrontend(new_id);
  ASSERT_TRUE(new_frontend != NULL);
  ASSERT_EQ(3, factory->frontends_.size());

  ASSERT_TRUE(factory->ShelveFrontend(new_id, new_frontend));
  factory->DestroyFrontend(new_frontend);
  ASSERT_EQ(2, factory->frontends_.size());
  ASSERT_EQ(1, factory->shelved_frontends_.size());

  EngineInterface* old_frontend = factory->UnshelveFrontend(var);
  ASSERT_EQ(old_frontend, frontend);
  ASSERT_TRUE(factory->shelved_frontends_.empty());
  ASSERT_EQ(2, factory->frontends_.size());
#if defined(OS_WIN)
  ::DestroyWindow(var);
#endif
}

}  // namespace ime_goopy
