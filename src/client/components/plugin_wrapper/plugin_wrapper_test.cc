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

#include "base/logging.h"

#include "components/plugin_wrapper/mocked_plugin_component.h"
#include "components/plugin_wrapper/plugin_component_stub.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/mock_component_host.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"
#include "ipc/test_util.h"

namespace {

using ime_goopy::components::MockedPluginComponent;
using ipc::NewMessageForTest;
static const char kPluginName[] = PLUGIN_NAME;
static const int kComponentID = ipc::MockComponentHost::kMockComponentId;

class PluginWrapperTest : public ::testing::Test {
 protected:
  PluginWrapperTest() { }
  virtual ~PluginWrapperTest() { }
  virtual void SetUp() {
    scoped_ptr<ime_goopy::components::PluginInstance> dll(
        new ime_goopy::components::PluginInstance(kPluginName));
    ipc::proto::MessagePayload payload;
    int component_count = dll->ListComponents(&payload);
    ASSERT_GT(component_count, 0);
    ASSERT_EQ(component_count, payload.component_info_size());
    component_.reset(new ime_goopy::components::PluginComponentStub(
        kPluginName, payload.component_info(0).string_id().c_str()));
    ASSERT_TRUE(component_.get());
    host_.reset(new ipc::MockComponentHost);
    host_->AddComponent(component_.get());
  }

  virtual void TearDown() {
    host_->RemoveComponent(component_.get());
  }

  scoped_ptr<ime_goopy::components::PluginComponentStub> component_;
  scoped_ptr<ipc::MockComponentHost> host_;
};

TEST_F(PluginWrapperTest, ListComponentsTest) {
  scoped_ptr<ime_goopy::components::PluginInstance> dll(
      new ime_goopy::components::PluginInstance(kPluginName));
  ipc::proto::MessagePayload payload;
  int component_count = dll->ListComponents(&payload);
  ASSERT_GT(component_count, 0);
  ASSERT_EQ(component_count, payload.component_info_size());
  scoped_ptr<ime_goopy::components::PluginComponentStub> component;
  for (int i = 0; i < component_count; ++i) {
    component.reset(
        new ime_goopy::components::PluginComponentStub(
            kPluginName, payload.component_info(i).string_id().c_str()));
    ipc::proto::ComponentInfo info;
    component->GetInfo(&info);
    std::string info_expected;
    std::string info_actual;
    info.SerializeToString(&info_actual);
    payload.component_info(i).SerializeToString(&info_expected);
    EXPECT_EQ(info_expected.size(),
              info_actual.size());
    if (info_expected.size() == info_actual.size()) {
      EXPECT_EQ(0, memcmp(info_expected.c_str(),
                          info_actual.c_str(),
                          info_expected.size()));
    }
  }
}

TEST_F(PluginWrapperTest, MessageTest) {
  scoped_ptr<ipc::proto::Message> mptr;
  mptr.reset(
      NewMessageForTest(MockedPluginComponent::MSG_REQUEST_SEND,
                        ipc::proto::Message::NEED_REPLY,
                        ipc::kComponentDefault,
                        kComponentID,
                        ipc::kInputContextNone));
  component_->Handle(mptr.release());
  mptr.reset(host_->PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(kComponentID, mptr->source());
  EXPECT_EQ(MockedPluginComponent::MSG_TEST_MESSAGE, mptr->type());
  EXPECT_NE(ipc::proto::Message::IS_REPLY, mptr->reply_mode());

  mptr.reset(host_->PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(kComponentID, mptr->source());
  EXPECT_EQ(MockedPluginComponent::MSG_REQUEST_SEND, mptr->type());
  EXPECT_EQ(ipc::proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->has_payload());
  ASSERT_GT(mptr->payload().boolean_size(), 0);
  ASSERT_TRUE(mptr->payload().boolean(0));
}

TEST_F(PluginWrapperTest, SendWithReplyTest) {
  scoped_ptr<ipc::proto::Message> mptr;
  mptr.reset(
      NewMessageForTest(MockedPluginComponent::MSG_TEST_SEND_WITH_REPLY,
                        ipc::proto::Message::IS_REPLY,
                        ipc::kComponentDefault,
                        kComponentID,
                        ipc::kInputContextNone));
  host_->SetNextReplyMessage(mptr.release());
  mptr.reset(
      NewMessageForTest(MockedPluginComponent::MSG_REQUEST_SEND_WITH_REPLY,
                        ipc::proto::Message::NEED_REPLY,
                        ipc::kComponentDefault,
                        kComponentID,
                        ipc::kInputContextNone));
  component_->Handle(mptr.release());
  mptr.reset(host_->PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(kComponentID, mptr->source());
  EXPECT_EQ(MockedPluginComponent::MSG_TEST_SEND_WITH_REPLY, mptr->type());
  EXPECT_NE(ipc::proto::Message::IS_REPLY, mptr->reply_mode());

  mptr.reset(host_->PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(kComponentID, mptr->source());
  EXPECT_EQ(MockedPluginComponent::MSG_REQUEST_SEND_WITH_REPLY, mptr->type());
  EXPECT_EQ(ipc::proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->has_payload());
  ASSERT_GT(mptr->payload().boolean_size(), 0);
  ASSERT_TRUE(mptr->payload().boolean(0));
}

TEST_F(PluginWrapperTest, PauseResumeMessageHandlingTest) {
  scoped_ptr<ipc::proto::Message> mptr;
  host_->PauseMessageHandling(component_.get());
  ASSERT_TRUE(host_->IsMessageHandlingPaused());
  mptr.reset(
      NewMessageForTest(
          MockedPluginComponent::MSG_REQUEST_RESUME_MESSAGE_HANDLING,
          ipc::proto::Message::NO_REPLY,
          ipc::kComponentDefault,
          kComponentID,
          ipc::kInputContextNone));
  component_->Handle(mptr.release());
  ASSERT_FALSE(host_->IsMessageHandlingPaused());
  mptr.reset(
      NewMessageForTest(
          MockedPluginComponent::MSG_REQUEST_PAUSE_MESSAGE_HANDLING,
          ipc::proto::Message::NO_REPLY,
          ipc::kComponentDefault,
          kComponentID,
          ipc::kInputContextNone));
  component_->Handle(mptr.release());
  ASSERT_TRUE(host_->IsMessageHandlingPaused());
}

}  // namespace

int main(int argc, char* argv[]) {
#if defined(OS_WIN)
  testing::InitGoogleTest(&argc, argv);
  logging::InitLogging("", logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                       logging::DONT_LOCK_LOG_FILE,
                       logging::APPEND_TO_OLD_LOG_FILE);
#elif defined(OS_LINUX)
  testing::InitGoogleTest(&argc, argv);
#endif
  return RUN_ALL_TESTS();
}
