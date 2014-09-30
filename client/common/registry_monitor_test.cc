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

#include "common/registry_monitor.h"

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_handle.h"
#include "base/scoped_ptr.h"
#include "common/string_utils.h"
#include <gtest/gunit.h>

namespace {
using ime_goopy::RegistryMonitor;
HKEY parent = HKEY_CURRENT_USER;
static const wchar_t subkey[] = L"test_monitor";
static const wchar_t value_name[] = L"value";

class RegistryMonitorTest
  : public ::testing::Test,
    public RegistryMonitor::Delegate {
 public:
  void KeyChanged() OVERRIDE {
    changed_ = true;
    SetEvent(wait_event_.Get());
  }

 protected:
  void SetUp() {
    key_.Create(parent,
                subkey,
                REG_NONE,
                REG_OPTION_NON_VOLATILE,
                KEY_READ | KEY_WRITE | KEY_WOW64_64KEY,
                NULL,
                NULL);
    monitor_.reset(new RegistryMonitor(parent, subkey, this));
    wait_event_.Reset(::CreateEvent(NULL, FALSE, FALSE, NULL));
    changed_ = false;
    ASSERT_TRUE(monitor_->Start());
  }

  void TearDown() {
    key_.Close();
    monitor_->Stop();
    CRegKey current_user_(HKEY_CURRENT_USER);
    current_user_.RecurseDeleteKey(subkey);
  }

  void Clear() {
    changed_ = false;
  }

  bool Wait() {
    return ::WaitForSingleObject(wait_event_.Get(), 1000) == WAIT_OBJECT_0;
  }

  bool Changed() {
    return changed_;
  }

  CRegKey& key() {
    return key_;
  }

 private:
  CRegKey key_;
  scoped_ptr<RegistryMonitor> monitor_;
  ScopedHandle wait_event_;
  bool changed_;
};

TEST_F(RegistryMonitorTest, MonitorTest) {
  CRegKey& regkey = key();
  EXPECT_FALSE(Changed());
  regkey.SetDWORDValue(value_name, 0);
  Wait();
  EXPECT_TRUE(Changed());
  Clear();
  EXPECT_FALSE(Changed());
  regkey.DeleteValue(value_name);
  Wait();
  EXPECT_TRUE(Changed());
}
}  // namespace
