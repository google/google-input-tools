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

#include <atlbase.h>
#include <atlcom.h>
#include <atlcomcli.h>
#include <msctf.h>

#include "base/scoped_ptr.h"
#include "tsf/compartment.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace tsf {
TEST(Compartment, Test) {
  SmartComPtr<ITfThreadMgr> thread_manager;
  ASSERT_TRUE(SUCCEEDED(thread_manager.CoCreateInstance(CLSID_TF_ThreadMgr)));

  TfClientId client_id = TF_CLIENTID_NULL;
  ASSERT_TRUE(SUCCEEDED(thread_manager->Activate(&client_id)));
  ASSERT_TRUE(client_id != TF_CLIENTID_NULL);

  Compartment compartment(client_id,
                          thread_manager,
                          GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                          NULL);
  ASSERT_TRUE(compartment.Ready());

  DWORD old_value = 0;
  ASSERT_TRUE(SUCCEEDED(compartment.GetInteger(&old_value)));

  DWORD new_value = !old_value;
  ASSERT_TRUE(SUCCEEDED(compartment.SetInteger(new_value)));

  DWORD changed_value = 0;
  ASSERT_TRUE(SUCCEEDED(compartment.GetInteger(&changed_value)));
  ASSERT_EQ(new_value, changed_value);

  ASSERT_TRUE(SUCCEEDED(compartment.SetInteger(old_value)));

  thread_manager->Deactivate();
}
}  // namespace tsf
}  // namespace ime_goopy
