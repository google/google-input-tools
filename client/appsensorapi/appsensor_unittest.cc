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
#include <gtest/gunit.h>
#include "base/scoped_ptr.h"
#include "base/logging.h"
#include "appsensorapi/appsensor.h"
#include "appsensorapi/handler.h"
#include "appsensorapi/handlermanager.h"

static const void *kNotNullDummyPointer = "Not Null";
namespace ime_goopy {
// Customize the handler with special rules to validate if the rules are
// invoked.
class CustomHandler : public Handler {
 public:
  CustomHandler() : Handler() {
    TCHAR thisFilename[MAX_PATH];
    // Obtain the version info of the current process. A match should be
    // emitted and the custom rules would be invoked.
    ::GetModuleFileName(NULL, thisFilename, MAX_PATH);
    VersionReader::GetVersionInfo(thisFilename, &version_info_);
  }
  // Validate if the method is invoked by whether data is null.
  virtual BOOL HandleCommand(uint32 command, void *data) const {
    return data != NULL;
  }
  // Validate if the method is invoked by whether wparam is zero.
  virtual LRESULT HandleMessage(HWND hwnd, UINT message,
                                WPARAM wparam, LPARAM lparam) const {
    return wparam != 0;
  }
  const VersionInfo *version_info() const { return &version_info_; }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(CustomHandler);
};

class AppSensorTest : public testing::Test {
 protected:
  virtual void SetUp() {
    app_sensor_.reset(new AppSensor());
  }

  virtual void TearDown() {
  }

  // Import the customized handler into the manager class.
  void ImportHandler() {
    HandlerManager *handler_manager = app_sensor_->handler_manager();
    handler_.reset(new CustomHandler);
    ASSERT_TRUE(handler_manager->AddHandler(handler_.get()));
  }

  scoped_ptr<AppSensor> app_sensor_;
  VersionInfo version_info_;
  scoped_ptr<CustomHandler> handler_;
};

TEST_F(AppSensorTest, Constructor) {
  // Create the AppSensor and validate the instance to be created.
  AppSensor *app_sensor = new AppSensor();
  const HandlerManager *handler_manager = app_sensor->handler_manager();
  // Validate the manager is assigned in the constructor.
  EXPECT_EQ(handler_manager, app_sensor->handler_manager());
  delete app_sensor;
}

TEST_F(AppSensorTest, Init) {
  // Should always return true to indicate a success initialization.
  EXPECT_TRUE(app_sensor_->Init());
}

TEST_F(AppSensorTest, HandleMessage) {
  EXPECT_TRUE(app_sensor_->Init());
  ImportHandler();

  // Validate the customized HandleMessage rule is invoked.
  // We assume the following condition should be met:
  // wparam is non-zero -> return true
  //               zero -> return false
  EXPECT_TRUE(app_sensor_->HandleMessage(NULL, WM_USER, 1, 0));
  EXPECT_FALSE(app_sensor_->HandleMessage(NULL, WM_USER, 0, 0));
}

TEST_F(AppSensorTest, HandleMessageNoAction) {
  EXPECT_TRUE(app_sensor_->Init());

  // When the customized rule is not added, the default behavior should be
  // different. It always get a false value.
  EXPECT_FALSE(app_sensor_->HandleMessage(NULL, WM_USER, 1, 0));
  EXPECT_FALSE(app_sensor_->HandleMessage(NULL, WM_USER, 0, 0));
}

TEST_F(AppSensorTest, HandleCommand) {
  EXPECT_TRUE(app_sensor_->Init());
  ImportHandler();

  // Validate the customized HandleCommand rule is invoked.
  // We assume the following condition should be met:
  // data is non-null -> return true
  //             null -> return false
  EXPECT_TRUE(
      app_sensor_->HandleCommand(1, const_cast<void *>(kNotNullDummyPointer)));
  EXPECT_FALSE(app_sensor_->HandleCommand(0, NULL));
}

TEST_F(AppSensorTest, HandleCommandNoAction) {
  EXPECT_TRUE(app_sensor_->Init());

  // When the customized rule is not added, the default behavior should be
  // different. It always get a false value.
  EXPECT_FALSE(
      app_sensor_->HandleCommand(1, const_cast<void *>(kNotNullDummyPointer)));
  EXPECT_FALSE(app_sensor_->HandleCommand(0, NULL));
}

TEST_F(AppSensorTest, RegisterHandler) {
  CustomHandler *handler = new CustomHandler;
  Handler *result = NULL;
  app_sensor_->Init();

  app_sensor_->RegisterHandler(handler);
  HandlerManager *handler_manager = app_sensor_->handler_manager();
  ASSERT_TRUE(
      handler_manager->GetHandlerByInfo(*handler->version_info()) != NULL);
}

}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
