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

#include "appsensorapi/handlermanager.h"
#include <gtest/gunit.h>
#include "appsensorapi/handler.h"
#include "appsensorapi/versionreader.h"

using ime_goopy::HandlerManager;
using ime_goopy::VersionInfo;
using ime_goopy::Handler;

namespace {

const int kNumHandlers = 20;

// Customize the handler with special rules to validate if the rules are
// invoked.
class CustomHandler : public Handler {
 public:
  explicit CustomHandler(size_t size) : Handler() {
    version_info_.file_size = size;
  }
  // Validate if the method is invoked by whether data is null.
  BOOL HandleCommand(uint32 command, void *data) const {
    return data != NULL;
  }
  // Validate if the method is invoked by whether wparam is zero.
  LRESULT HandleMessage(HWND hwnd,
                        UINT message,
                        WPARAM wparam,
                        LPARAM lparam) const {
    return wparam != 0;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(CustomHandler);
};

class HandlerManagerTest : public testing::Test {
 protected:
  void SetUp() {
    handler_manager_.reset(new HandlerManager());
    for (int i = 0; i < arraysize(handler_); i++) {
      handler_[i] = NULL;
    }
  }

  void TearDown() {
    for (int i = 0; i < arraysize(handler_); i++) {
      delete handler_[i];
      handler_[i] = NULL;
    }
  }

  // Create an array of handlers with specified size. They are used to
  // validate the logic of HandlerManager.
  void PrepareData() {
    for (int i = 0; i < arraysize(handler_); i++) {
      handler_[i] = new CustomHandler(1000 * (i + 1));
      ASSERT_TRUE(handler_manager_->AddHandler(handler_[i]));
    }
    ASSERT_EQ(arraysize(handler_), handler_manager_->GetCount());
  }

  scoped_ptr<HandlerManager> handler_manager_;
  CustomHandler *handler_[kNumHandlers];
};

TEST_F(HandlerManagerTest, AddHandler) {
  // Prepare handler instances.
  for (int i = 0; i < arraysize(handler_); i++) {
    handler_[i] = new CustomHandler(1000 * (i + 1));
  }

  // Add the handlers and validate its success.
  for (int i = 0; i < arraysize(handler_); i++) {
    EXPECT_TRUE(handler_manager_->AddHandler(handler_[i]));
    EXPECT_EQ(i + 1, handler_manager_->GetCount());
  }

  // Add the same handlers, should be failed and the number of handlers
  // should not be changed.
  for (int i = 0; i < arraysize(handler_); i++) {
    EXPECT_FALSE(handler_manager_->AddHandler(handler_[i]));
    EXPECT_EQ(arraysize(handler_), handler_manager_->GetCount());
  }
}

TEST_F(HandlerManagerTest, RemoveHandler) {
  // Prepare the handlers array.
  PrepareData();

  // Remove each handler and validate the number of handlers is
  // decreasing.
  for (int i = 0; i < arraysize(handler_); i++) {
    EXPECT_TRUE(handler_manager_->RemoveHandler(handler_[i]));
    EXPECT_EQ(arraysize(handler_) - i - 1,
              handler_manager_->GetCount());
  }

  // Try to remove again, should not be able to removed.
  for (int i = 0; i < arraysize(handler_); i++) {
    EXPECT_FALSE(handler_manager_->RemoveHandler(handler_[i]));
  }
  EXPECT_EQ(0, handler_manager_->GetCount());
}


TEST_F(HandlerManagerTest, GetHandlerBySize) {
  // Prepare the handlers array.
  PrepareData();
  // Validate all handlers are obtained by their sizes.
  for (int i = 0; i < arraysize(handler_); i++) {
    const Handler *handler = handler_manager_->GetHandlerBySize(
        handler_[i]->version_info()->file_size);
    ASSERT_TRUE(handler != NULL);
    EXPECT_EQ(handler_[i]->version_info()->file_size,
              handler->version_info()->file_size);
  }

  // Try to remove each handler, then validate the failure of the method.
  for (int i = 0; i < arraysize(handler_); i++) {
    ASSERT_TRUE(handler_manager_->RemoveHandler(handler_[i]));
    EXPECT_TRUE(handler_manager_->GetHandlerBySize(
        handler_[i]->version_info()->file_size) == NULL);
  }
}

TEST_F(HandlerManagerTest, GetHandlerByInfo) {
  // Prepare the handlers array.
  PrepareData();
  // Validate all handlers are obtained by their info.
  for (int i = 0; i < arraysize(handler_); i++) {
    const Handler *handler = handler_manager_->GetHandlerByInfo(
        *handler_[i]->version_info());
    ASSERT_TRUE(handler != NULL);
    EXPECT_EQ(handler_[i]->version_info()->file_size,
              handler->version_info()->file_size);
  }

  // Try to remove each handler, then validate the failure of the method.
  for (int i = 0; i < arraysize(handler_); i++) {
    ASSERT_TRUE(handler_manager_->RemoveHandler(handler_[i]));
    EXPECT_TRUE(handler_manager_->GetHandlerByInfo(
        *handler_[i]->version_info()) == NULL);
  }
}

TEST_F(HandlerManagerTest, HandleCommand) {
  PrepareData();

  // Validate the customized rule is invoked.
  // The rule is based on the assume:
  // data = non-null -> return true
  //            null -> return false
  EXPECT_TRUE(handler_manager_->HandleCommand(
        *handler_[0]->version_info(), 1, "Not Null"));
  EXPECT_FALSE(handler_manager_->HandleCommand(
        *handler_[0]->version_info(), 1, NULL));
}

TEST_F(HandlerManagerTest, HandleMessage) {
  PrepareData();

  // Validate the customized rule is invoked.
  // The rule is based on the assume:
  // wparam = non-zero -> return true
  //              zero -> return false
  EXPECT_TRUE(handler_manager_->HandleMessage(
      *handler_[0]->version_info(), NULL, WM_USER, 1, 0));
  EXPECT_FALSE(handler_manager_->HandleMessage(
      *handler_[0]->version_info(), NULL, WM_USER, 0, 0));
}

}  // namespace

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
