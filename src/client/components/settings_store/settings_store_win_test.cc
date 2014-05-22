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

#include "components/settings_store/settings_store_win.h"
#include "base/logging.h"
#include "common/app_utils.h"
#include "common/registry.h"
#include "components/settings_store/settings_store_test_common.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace ime_goopy {
namespace components {

class SettingsStoreWinTest : public ::testing::Test {
 protected:
  SettingsStoreWinTest() : registry_(AppUtils::OpenUserRegistry()),
                           store_(AppUtils::OpenUserRegistry()) {
  }
  void SetUp() {
    registry_->DeleteValue(L"value1");
    registry_->DeleteValue(L"value2");
    registry_->DeleteValue(L"array1");
    registry_->DeleteValue(L"array2");
    registry_->DeleteValue(L"nonexistent");
  }
  void TearDown() {
    registry_->DeleteValue(L"value1");
    registry_->DeleteValue(L"value2");
    registry_->DeleteValue(L"array1");
    registry_->DeleteValue(L"array2");
    registry_->DeleteValue(L"nonexistent");
  }
  scoped_ptr<RegistryKey> registry_;
  SettingsStoreWin store_;
};

TEST_F(SettingsStoreWinTest, Value) {
  SettingsStoreTestCommon::TestValue(&store_);
}

TEST_F(SettingsStoreWinTest, Array) {
  SettingsStoreTestCommon::TestArray(&store_);
}

}  // namespace components
}  // namespace ime_goopy
