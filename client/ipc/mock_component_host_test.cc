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

#include "ipc/mock_component_host.h"

#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/mock_component.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace {

using namespace ipc;

// Messages an application can produce
const uint32 kAppProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_CREATE_INPUT_CONTEXT,
  MSG_DELETE_INPUT_CONTEXT,
  MSG_FOCUS_INPUT_CONTEXT,
  MSG_BLUR_INPUT_CONTEXT,
  MSG_ASSIGN_ACTIVE_CONSUMER,
  MSG_RESIGN_ACTIVE_CONSUMER,
  MSG_REQUEST_CONSUMER,
  MSG_PROCESS_KEY_EVENT,
  MSG_CANCEL_COMPOSITION,
  MSG_COMPLETE_COMPOSITION,
  MSG_UPDATE_INPUT_CARET,
};

// Messages an application can consume
const uint32 kAppConsumeMessages[] = {
  MSG_SET_COMPOSITION,
  MSG_INSERT_TEXT,
  MSG_GET_DOCUMENT_INFO,
  MSG_GET_DOCUMENT_CONTENT_IN_RANGE,
};

// A mock application component
class AppComponent : public MockComponent {
 public:
  explicit AppComponent(const std::string& string_id)
      : MockComponent(string_id) {
  }

  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE {
    SetupComponentInfo("", "Mock Application", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       info);
    MockComponent::GetInfo(info);
  }
};


TEST(MockComponentHostTest, Test) {
  MockComponentHost host;
  AppComponent app("app");

  // Tests AddComponent()
  ASSERT_TRUE(host.AddComponent(&app));
  // We cannot use EXPECT_EQ() here, which will cause compile failure on Linux.
  EXPECT_TRUE(app.id() == MockComponentHost::kMockComponentId);
  EXPECT_TRUE(host.id() == MockComponentHost::kMockComponentId);
  EXPECT_EQ(app.string_id(), host.string_id());
  EXPECT_EQ(static_cast<ComponentHost*>(&host), app.host());

  // Checks if app.OnRegistered() gets called.
  EXPECT_TRUE(app.WaitIncomingMessage(0));
  EXPECT_EQ(NULL, app.PopIncomingMessage());

  for (size_t i = 0; i < arraysize(kAppProduceMessages); ++i)
    EXPECT_TRUE(host.MayProduce(kAppProduceMessages[i]));
  for (size_t i = 0; i < arraysize(kAppConsumeMessages); ++i)
    EXPECT_TRUE(host.CanConsume(kAppConsumeMessages[i]));

  // Tests message handling: No reply.
  scoped_ptr<proto::Message> mptr(new proto::Message());
  mptr->set_type(MSG_UPDATE_INPUT_CARET);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  app.AddOutgoingMessage(mptr.release(), true, 0);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_INSERT_TEXT);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_target(app.id());
  ASSERT_TRUE(host.HandleMessage(mptr.release()));
  ASSERT_TRUE(host.WaitOutgoingMessage(0));
  mptr.reset(host.PopOutgoingMessage());
  EXPECT_EQ(MSG_UPDATE_INPUT_CARET, mptr->type());

  EXPECT_TRUE(app.WaitIncomingMessage(0));
  mptr.reset(app.PopIncomingMessage());
  EXPECT_EQ(MSG_INSERT_TEXT, mptr->type());

  // Tests message handling: Need reply error.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_UPDATE_INPUT_CARET);
  mptr->set_reply_mode(proto::Message::NEED_REPLY);
  app.AddOutgoingMessage(mptr.release(), false, 1);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_INSERT_TEXT);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_target(app.id());
  ASSERT_TRUE(host.HandleMessage(mptr.release()));
  ASSERT_TRUE(host.WaitOutgoingMessage(0));
  mptr.reset(host.PopOutgoingMessage());
  EXPECT_EQ(MSG_UPDATE_INPUT_CARET, mptr->type());

  EXPECT_TRUE(app.WaitIncomingMessage(0));
  mptr.reset(app.PopIncomingMessage());
  EXPECT_EQ(MSG_INSERT_TEXT, mptr->type());

  // Tests message handling: Need reply ok.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_UPDATE_INPUT_CARET);
  mptr->set_reply_mode(proto::Message::NEED_REPLY);
  app.AddOutgoingMessage(mptr.release(), true, 1);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_UPDATE_INPUT_CARET);
  mptr->set_reply_mode(proto::Message::IS_REPLY);
  host.SetNextReplyMessage(mptr.release());

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_INSERT_TEXT);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_target(app.id());
  ASSERT_TRUE(host.HandleMessage(mptr.release()));
  ASSERT_TRUE(host.WaitOutgoingMessage(0));
  mptr.reset(host.PopOutgoingMessage());
  EXPECT_EQ(MSG_UPDATE_INPUT_CARET, mptr->type());

  EXPECT_TRUE(app.WaitIncomingMessage(0));
  mptr.reset(app.PopIncomingMessage());
  EXPECT_EQ(MSG_INSERT_TEXT, mptr->type());

  EXPECT_TRUE(app.WaitIncomingMessage(0));
  mptr.reset(app.PopIncomingMessage());
  EXPECT_EQ(MSG_UPDATE_INPUT_CARET, mptr->type());
  EXPECT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());

  // Checks PauseMessageHandling() and ResumeMessageHandling().
  EXPECT_FALSE(host.IsMessageHandlingPaused());
  host.PauseMessageHandling(&app);
  EXPECT_TRUE(host.IsMessageHandlingPaused());
  host.PauseMessageHandling(&app);
  EXPECT_TRUE(host.IsMessageHandlingPaused());
  host.ResumeMessageHandling(&app);
  EXPECT_TRUE(host.IsMessageHandlingPaused());
  host.ResumeMessageHandling(&app);
  EXPECT_FALSE(host.IsMessageHandlingPaused());

  // Tests RemoveComponent().
  host.RemoveComponent(&app);
  EXPECT_EQ(kComponentDefault, app.id());
  EXPECT_EQ(kComponentDefault, host.id());
  EXPECT_TRUE(host.string_id().empty());
  EXPECT_EQ(NULL, app.host());

  // Checks if app.OnDeregistered() gets called.
  EXPECT_TRUE(app.WaitIncomingMessage(0));
  EXPECT_EQ(NULL, app.PopIncomingMessage());
}

}  // namespace
