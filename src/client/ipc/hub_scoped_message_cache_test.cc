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

#include <map>
#include <set>

#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/hub_impl.h"
#include "ipc/hub_impl_test_base.h"
#include "ipc/hub_scoped_message_cache.h"
#include "ipc/message_types.h"
#include "ipc/mock_connector.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace {

using namespace ipc;
using namespace hub;

// Messages an application can produce
const uint32 kAppProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_CREATE_INPUT_CONTEXT,
  MSG_DELETE_INPUT_CONTEXT,
  MSG_REQUEST_CONSUMER,
  MSG_DO_COMMAND,
  MSG_BEEP,
};

// Messages an application can consume
const uint32 kAppConsumeMessages[] = {
  MSG_COMPOSITION_CHANGED,
  MSG_INSERT_TEXT,
  MSG_GET_DOCUMENT_INFO,
  MSG_GET_DOCUMENT_CONTENT_IN_RANGE,
};

// Messages an input method can produce
const uint32 kComponentProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
};

// Messages an input method can consume
const uint32 kComponentConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DO_COMMAND,
  MSG_BEEP,
};

class HubScopedMessageCacheTest : public HubImplTestBase {
 protected:
  HubScopedMessageCacheTest() {
    SetupComponentInfo("com.google.app1", "App1", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       &app_);

    SetupComponentInfo("com.google.comp1", "Comp1", "",
                       kComponentProduceMessages,
                       arraysize(kComponentProduceMessages),
                       kComponentConsumeMessages,
                       arraysize(kComponentConsumeMessages),
                       &comp_);
  }

  virtual ~HubScopedMessageCacheTest() {
  }

  proto::ComponentInfo app_;
  proto::ComponentInfo comp_;
};

TEST_F(HubScopedMessageCacheTest, MessageCacheTest) {
  MockConnector app_connector;
  MockConnector comp_connector;

  app_connector.AddComponent(app_);
  comp_connector.AddComponent(comp_);

  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(comp_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();
  uint32 comp_id = comp_connector.components_[0].id();

  // Create an input context.
  uint32 icid = 0;
  ASSERT_NO_FATAL_FAILURE(CreateInputContext(&app_connector, app_id, &icid));

  // app requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &app_connector, app_id, icid, kAppProduceMessages,
      arraysize(kAppProduceMessages)));
  // Attach component
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &comp_connector, comp_id, icid, false));
  // Creates a message cache for MSG_DO_COMMAND in input context icid.
  const uint32 kCacheMessages[] = {
    ipc::MSG_DO_COMMAND,
  };
  scoped_ptr<ipc::hub::HubScopedMessageCache> cache(
      new HubScopedMessageCache(kCacheMessages,
                                arraysize(kCacheMessages),
                                icid,
                                hub_));
  // App sends a MSG_DO_COMMAND to kInputContextNone.
  ipc::proto::Message* message = NewMessageForTest(
      ipc::MSG_DO_COMMAND, proto::Message::NO_REPLY, app_id,
      kComponentDefault, kInputContextNone);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));
  // App sends a MSG_DO_COMMAND to the first ic.
  message = NewMessageForTest(
      ipc::MSG_DO_COMMAND, proto::Message::NO_REPLY, app_id,
      kComponentDefault, icid);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));
  // Component should receive the messages for kInputContextNone.
  ASSERT_EQ(1U, comp_connector.messages_.size());
  message = comp_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_DO_COMMAND,
      app_id, comp_id, kInputContextNone,
      proto::Message::NO_REPLY, false));
  comp_connector.ClearMessages();
  // Apps sends a MSG_BEEP to icid.
  message = NewMessageForTest(
      ipc::MSG_BEEP, proto::Message::NO_REPLY, app_id,
      kComponentDefault, icid);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));
  // Component should receive the messages.
  ASSERT_EQ(1U, comp_connector.messages_.size());
  message = comp_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_BEEP,
      app_id, comp_id, icid,
      proto::Message::NO_REPLY, false));
  comp_connector.ClearMessages();
  // Destroy the cache.
  cache.reset();
  // Component should receive the messages for icid.
  ASSERT_EQ(1U, comp_connector.messages_.size());
  message = comp_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_DO_COMMAND,
      app_id, comp_id, icid,
      proto::Message::NO_REPLY, false));
  comp_connector.ClearMessages();
}
}  // namespace
