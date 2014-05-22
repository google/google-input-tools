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

#include "ipc/hub_component.h"

#include <vector>

#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/hub.h"
#include "ipc/hub_hotkey_list.h"
#include "ipc/message_types.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace {

const char kComponentStringID[] = "com.google.imp.TestComponent";
const uint32 kComponentID = 1;
const char kComponentName[] = "Test Component";
const char kComponentDesc[] = "A test component";

const uint32 kComponentProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_QUERY_COMPONENT,
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_DELETE_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_PROCESS_KEY_EVENT,
};

const uint32 kComponentConsumeMessages[] = {
  ipc::MSG_SET_COMPOSITION,
  ipc::MSG_INSERT_TEXT,
};

class HubComponentTest : public ::testing::Test,
                         public ipc::Hub::Connector {
 protected:
  HubComponentTest() {
    component_.reset(CreateTestComponent(
        kComponentID,
        this,
        kComponentStringID,
        kComponentName,
        kComponentDesc,
        kComponentProduceMessages,
        arraysize(kComponentProduceMessages),
        kComponentConsumeMessages,
        arraysize(kComponentConsumeMessages)));
  }

  virtual ~HubComponentTest() {
  }

  virtual bool Send(ipc::proto::Message*) {
    return true;
  }

  scoped_ptr<ipc::hub::Component> component_;
};

TEST_F(HubComponentTest, Properties) {
  EXPECT_TRUE(static_cast<ipc::Hub::Connector*>(this) == component_->connector());
  EXPECT_TRUE(kComponentID == component_->id());
}

TEST_F(HubComponentTest, Messages) {
  std::vector<uint32> produce;
  std::vector<uint32> consume;

  for (uint32 i = 0; i < ipc::MSG_END_OF_PREDEFINED_MESSAGE; ++i) {
    if (component_->MayProduce(i))
      produce.push_back(i);
    if (component_->CanConsume(i))
      consume.push_back(i);
  }

  ASSERT_EQ(arraysize(kComponentProduceMessages), produce.size());
  for (size_t i = 0; i < arraysize(kComponentProduceMessages); ++i)
    EXPECT_EQ(kComponentProduceMessages[i], produce[i]) << i;

  ASSERT_EQ(arraysize(kComponentConsumeMessages), consume.size());
  for (size_t i = 0; i < arraysize(kComponentConsumeMessages); ++i)
    EXPECT_EQ(kComponentConsumeMessages[i], consume[i]) << i;
}

TEST_F(HubComponentTest, HotkeyList) {
  ipc::proto::HotkeyList hotkey_list;
  hotkey_list.set_id(1);
  hotkey_list.add_hotkey();
  component_->AddHotkeyList(hotkey_list);
  hotkey_list.set_id(2);
  hotkey_list.add_hotkey();
  component_->AddHotkeyList(hotkey_list);

  const ipc::hub::HotkeyList* result_list = component_->GetHotkeyList(1);
  ASSERT_TRUE(result_list);
  ASSERT_EQ(1, result_list->id());
  ASSERT_EQ(1, result_list->hotkeys().hotkey_size());

  result_list = component_->GetHotkeyList(2);
  ASSERT_TRUE(result_list);
  ASSERT_EQ(2, result_list->id());
  ASSERT_EQ(2, result_list->hotkeys().hotkey_size());

  component_->RemoveHotkeyList(1);
  ASSERT_FALSE(component_->GetHotkeyList(1));

  component_->RemoveHotkeyList(2);
  ASSERT_FALSE(component_->GetHotkeyList(2));
}

}  // namespace
