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
#include "base/logging.h"
#include "appsensorapi/appsensorapi.h"
#include "appsensorapi/common.h"

using ime_goopy::AppSensorInitFunc;
using ime_goopy::AppSensorHandleCommandFunc;
using ime_goopy::AppSensorHandleMessageFunc;
using ime_goopy::FunctionName;

static const TCHAR *kLibraryPath = TEXT("appsensorapi.dll");

TEST(AppSensorApiTest, LoadLibrary) {
  // Validate the DLL format is valid, and can be loaded into memory.
  HINSTANCE hinst_lib = ::LoadLibrary(kLibraryPath);
  ASSERT_TRUE(hinst_lib != NULL);
  ::FreeLibrary(hinst_lib);
}

TEST(AppSensorApiTest, Init) {
  HINSTANCE hinst_lib = ::LoadLibrary(kLibraryPath);
  ASSERT_TRUE(hinst_lib != NULL);
  // Validate the existence of Init entry point
  AppSensorInitFunc init;
  init = reinterpret_cast<AppSensorInitFunc>(
      GetProcAddress(hinst_lib, FunctionName::kInitFuncName));
  ASSERT_TRUE(init != NULL);
  // Validate the function can be invoked.
  EXPECT_TRUE((init)());
  ::FreeLibrary(hinst_lib);
}

TEST(AppSensorApiTest, HandleMessage) {
  HINSTANCE hinst_lib = ::LoadLibrary(kLibraryPath);
  ASSERT_TRUE(hinst_lib != NULL);
  // Validate the existence of HandleMessage entry point
  AppSensorHandleMessageFunc window_process;
  window_process = reinterpret_cast<AppSensorHandleMessageFunc>(
      GetProcAddress(hinst_lib, FunctionName::kHandleMessageFuncName));
  ASSERT_TRUE(window_process != NULL);
  // Validate the function can be invoked.
  EXPECT_FALSE((window_process)(0, WM_USER, 0, 0));
  ::FreeLibrary(hinst_lib);
}

TEST(AppSensorApiTest, HandleCommand) {
  HINSTANCE hinst_lib = ::LoadLibrary(kLibraryPath);
  ASSERT_TRUE(hinst_lib != NULL);
  // Validate the existence of HandleCommand entry point
  AppSensorHandleCommandFunc invoke;
  invoke = reinterpret_cast<AppSensorHandleCommandFunc>(
      GetProcAddress(hinst_lib, FunctionName::kHandleCommandFuncName));
  ASSERT_TRUE(invoke != NULL);
  // Validate the function can be invoked.
  EXPECT_FALSE((invoke)(0, NULL));
  ::FreeLibrary(hinst_lib);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
