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
  MSG_FOCUS_INPUT_CONTEXT,
  MSG_BLUR_INPUT_CONTEXT,
  MSG_ASSIGN_ACTIVE_CONSUMER,
  MSG_RESIGN_ACTIVE_CONSUMER,
  MSG_REQUEST_CONSUMER,
  MSG_SEND_KEY_EVENT,
  MSG_CANCEL_COMPOSITION,
  MSG_COMPLETE_COMPOSITION,
  MSG_UPDATE_INPUT_CARET,
  MSG_SHOW_COMPOSITION_UI,
  MSG_HIDE_COMPOSITION_UI,
  MSG_SHOW_CANDIDATE_LIST_UI,
  MSG_HIDE_CANDIDATE_LIST_UI,
};

// Messages an application can consume
const uint32 kAppConsumeMessages[] = {
  MSG_COMPOSITION_CHANGED,
  MSG_INSERT_TEXT,
  MSG_GET_DOCUMENT_INFO,
  MSG_GET_DOCUMENT_CONTENT_IN_RANGE,
};

// Messages an input method can produce
const uint32 kIMEProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_SET_COMMAND_LIST,
  MSG_UPDATE_COMMANDS,
  MSG_ADD_HOTKEY_LIST,
  MSG_REMOVE_HOTKEY_LIST,
  MSG_CHECK_HOTKEY_CONFLICT,
  MSG_ACTIVATE_HOTKEY_LIST,
  MSG_DEACTIVATE_HOTKEY_LIST,
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_QUERY_INPUT_CONTEXT,
  MSG_REQUEST_CONSUMER,
  MSG_SET_COMPOSITION,
  MSG_INSERT_TEXT,
  MSG_SET_CANDIDATE_LIST,
  MSG_SET_SELECTED_CANDIDATE,
  MSG_SET_CANDIDATE_LIST_VISIBILITY,
};

// Messages an input method can consume
const uint32 kIMEConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMPONENT_ACTIVATED,
  MSG_COMPONENT_DEACTIVATED,
  MSG_PROCESS_KEY_EVENT,
  MSG_CANCEL_COMPOSITION,
  MSG_COMPLETE_COMPOSITION,
  MSG_CANDIDATE_LIST_SHOWN,
  MSG_CANDIDATE_LIST_HIDDEN,
  MSG_CANDIDATE_LIST_PAGE_DOWN,
  MSG_CANDIDATE_LIST_PAGE_UP,
  MSG_CANDIDATE_LIST_SCROLL_TO,
  MSG_CANDIDATE_LIST_PAGE_RESIZE,
  MSG_SELECT_CANDIDATE,
  MSG_UPDATE_INPUT_CARET,
  MSG_DO_COMMAND,
};

// Messages a candidate window can produce
const uint32 kUIProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_REQUEST_CONSUMER,
  MSG_CANDIDATE_LIST_SHOWN,
  MSG_CANDIDATE_LIST_HIDDEN,
  MSG_CANDIDATE_LIST_PAGE_DOWN,
  MSG_CANDIDATE_LIST_PAGE_UP,
  MSG_CANDIDATE_LIST_SCROLL_TO,
  MSG_CANDIDATE_LIST_PAGE_RESIZE,
  MSG_SELECT_CANDIDATE,
};

// Messages a candidate window can consume
const uint32 kUIConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMPONENT_ACTIVATED,
  MSG_COMPONENT_DEACTIVATED,
  MSG_COMPOSITION_CHANGED,
  MSG_CANDIDATE_LIST_CHANGED,
  MSG_SELECTED_CANDIDATE_CHANGED,
  MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
  MSG_SHOW_COMPOSITION_UI,
  MSG_HIDE_COMPOSITION_UI,
  MSG_SHOW_CANDIDATE_LIST_UI,
  MSG_HIDE_CANDIDATE_LIST_UI,
  MSG_UPDATE_INPUT_CARET,
};

const uint32 kTesterProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_ACTIVATE_COMPONENT,
  MSG_QUERY_ACTIVE_CONSUMER,
  MSG_QUERY_INPUT_CONTEXT,
};

const uint32 kTesterConsumeMessages[] = {
  MSG_COMPONENT_CREATED,
  MSG_COMPONENT_DELETED,
  MSG_INPUT_CONTEXT_CREATED,
  MSG_INPUT_CONTEXT_DELETED,
  MSG_COMPONENT_ATTACHED,
  MSG_COMPONENT_DETACHED,
};

class HubImplTest : public HubImplTestBase {
 protected:
  HubImplTest() {
    SetupComponentInfo("com.google.app1", "App1", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       &app1_);

    SetupComponentInfo("com.google.app2", "App2", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       &app2_);

    SetupComponentInfo("com.google.ime1", "Ime1", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &ime1_);

    SetupComponentInfo("com.google.ime2", "Ime2", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &ime2_);

    SetupComponentInfo("com.google.ime3", "Ime3", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &ime3_);

    SetupComponentInfo("com.google.ui", "UI", "",
                       kUIProduceMessages,
                       arraysize(kUIProduceMessages),
                       kUIConsumeMessages,
                       arraysize(kUIConsumeMessages),
                       &ui_);

    SetupComponentInfo("com.google.tester", "Tester", "",
                       kTesterProduceMessages,
                       arraysize(kTesterProduceMessages),
                       kTesterConsumeMessages,
                       arraysize(kTesterConsumeMessages),
                       &tester_);

  }

  virtual ~HubImplTest() {
  }

  proto::ComponentInfo app1_;
  proto::ComponentInfo app2_;
  proto::ComponentInfo ime1_;
  proto::ComponentInfo ime2_;
  proto::ComponentInfo ime3_;
  proto::ComponentInfo ui_;
  proto::ComponentInfo tester_;
};

TEST_F(HubImplTest, Default) {
  ASSERT_NO_FATAL_FAILURE(VerifyDefaultComponent());
  ASSERT_NO_FATAL_FAILURE(VerifyDefaultInputContext());
}

TEST_F(HubImplTest, AttachDetach) {
  // Test HubImpl::Attach() and HubImpl::Detach().
  MockConnector connector1;
  MockConnector connector2;

  connector1.Attach(hub_);
  ASSERT_TRUE(IsConnectorAttached(&connector1));

  connector2.Attach(hub_);
  ASSERT_TRUE(IsConnectorAttached(&connector2));

  connector2.Detach();
  ASSERT_FALSE(IsConnectorAttached(&connector2));

  ASSERT_TRUE(IsConnectorAttached(&connector1));
  connector1.Detach();
  ASSERT_FALSE(IsConnectorAttached(&connector1));
}

TEST_F(HubImplTest, RegisterComponents) {
  MockConnector tester_connector;
  MockConnector app_connector1;
  MockConnector app_connector2;
  MockConnector ime_connector;
  MockConnector ui_connector;

  // Register a tester component first, so that we can test
  // MSG_COMPONENT_CREATED, MSG_COMPONENT_DELETED, MSG_COMPONENT_ATTACHED and
  // MSG_COMPONENT_DETACHED messages.
  tester_connector.AddComponent(tester_);
  ASSERT_NO_FATAL_FAILURE(tester_connector.Attach(hub_));
  uint32 tester_id = tester_connector.components_[0].id();

  app_connector1.Attach(hub_);
  // Register single component.
  ASSERT_NO_FATAL_FAILURE(app_connector1.AddComponent(app1_));
  ASSERT_NE(kComponentDefault, app_connector1.components_[0].id());
  ASSERT_NO_FATAL_FAILURE(VerifyComponent(app_connector1.components_[0]));
  // Check MSG_COMPONENT_CREATED and MSG_COMPONENT_ATTACHED messages
  ASSERT_EQ(2U, tester_connector.messages_.size());

  // MSG_COMPONENT_CREATED
  proto::Message* message = tester_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_CREATED, kComponentDefault, tester_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().component_info_size());
  ASSERT_EQ(app_connector1.components_[0].id(),
            message->payload().component_info(0).id());

  // MSG_COMPONENT_ATTACHED (to kInputContextNone)
  message = tester_connector.messages_[1];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_ATTACHED, kComponentDefault, tester_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_EQ(2, message->payload().uint32_size());
  ASSERT_EQ(kInputContextNone, message->payload().uint32(0));
  ASSERT_EQ(app_connector1.components_[0].id(),
            message->payload().uint32(1));
  tester_connector.ClearMessages();

  app_connector2.Attach(hub_);
  // Register single component.
  ASSERT_NO_FATAL_FAILURE(app_connector2.AddComponent(app2_));
  ASSERT_NE(kComponentDefault, app_connector2.components_[0].id());
  ASSERT_NO_FATAL_FAILURE(VerifyComponent(app_connector2.components_[0]));

  ime_connector.AddComponent(ime1_);
  ime_connector.AddComponent(ime2_);
  // Register two components at the same time.
  ASSERT_NO_FATAL_FAILURE(ime_connector.Attach(hub_));
  // Register another component.
  ASSERT_NO_FATAL_FAILURE(ime_connector.AddComponent(ime3_));

  ASSERT_EQ(3U, ime_connector.components_.size());
  ASSERT_NE(kComponentDefault, ime_connector.components_[0].id());
  ASSERT_NE(kComponentDefault, ime_connector.components_[1].id());
  ASSERT_NE(kComponentDefault, ime_connector.components_[2].id());
  ASSERT_NO_FATAL_FAILURE(VerifyComponent(ime_connector.components_[0]));
  ASSERT_NO_FATAL_FAILURE(VerifyComponent(ime_connector.components_[1]));
  ASSERT_NO_FATAL_FAILURE(VerifyComponent(ime_connector.components_[2]));

  ui_connector.Attach(hub_);
  ASSERT_NO_FATAL_FAILURE(ui_connector.AddComponent(ui_));
  ASSERT_EQ(1U, ui_connector.components_.size());
  ASSERT_NE(kComponentDefault, ui_connector.components_[0].id());
  ASSERT_NO_FATAL_FAILURE(VerifyComponent(ui_connector.components_[0]));

  // Test some error conditions.
  tester_connector.ClearMessages();

  // Try to register a component with duplicated string id.
  MockConnector error_connector;
  ASSERT_NO_FATAL_FAILURE(error_connector.Attach(hub_));
  message = NewMessageForTest(
      MSG_REGISTER_COMPONENT, proto::Message::NEED_REPLY,
      kComponentDefault, kComponentDefault, kInputContextNone);
  uint32 serial = message->serial();
  message->mutable_payload()->add_component_info()->CopyFrom(ime1_);
  EXPECT_TRUE(hub_->Dispatch(&error_connector, message));

  // Check the reply message, whose target should be kComponentDefault.
  ASSERT_EQ(1U, error_connector.messages_.size());
  message = error_connector.messages_[0];
  EXPECT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_REGISTER_COMPONENT, kComponentDefault, kComponentDefault,
      kInputContextNone, proto::Message::IS_REPLY, true));
  ASSERT_EQ(1, message->payload().component_info_size());
  EXPECT_EQ(kComponentDefault, message->payload().component_info(0).id());
  ASSERT_EQ(serial, message->serial());
  error_connector.ClearMessages();

  // No MSG_COMPONENT_CREATED should be broadcasted.
  ASSERT_EQ(0, tester_connector.messages_.size());

  // Try to register two components with duplicated string ids.
  message = NewMessageForTest(
      MSG_REGISTER_COMPONENT, proto::Message::NEED_REPLY,
      kComponentDefault, kComponentDefault, kInputContextNone);
  serial = message->serial();
  message->mutable_payload()->add_component_info()->CopyFrom(ime1_);
  message->mutable_payload()->add_component_info()->CopyFrom(ime2_);
  EXPECT_TRUE(hub_->Dispatch(&error_connector, message));

  // Check the reply message, whose target should be kComponentDefault.
  ASSERT_EQ(1U, error_connector.messages_.size());
  message = error_connector.messages_[0];
  EXPECT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_REGISTER_COMPONENT, kComponentDefault, kComponentDefault,
      kInputContextNone, proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  ASSERT_EQ(2, message->payload().component_info_size());
  EXPECT_EQ(kComponentDefault, message->payload().component_info(0).id());
  EXPECT_EQ(kComponentDefault, message->payload().component_info(1).id());
  error_connector.ClearMessages();

  // No MSG_COMPONENT_CREATED should be broadcasted.
  ASSERT_EQ(0, tester_connector.messages_.size());

  // MSG_REGISTER_COMPONENT's reply message is mandatory.
  message = NewMessageForTest(
      MSG_REGISTER_COMPONENT, proto::Message::NO_REPLY,
      kComponentDefault, kComponentDefault, kInputContextNone);
  message->mutable_payload()->add_component_info()->CopyFrom(ime1_);
  EXPECT_FALSE(hub_->Dispatch(&error_connector, message));
  ASSERT_EQ(0, error_connector.messages_.size());

  // No MSG_COMPONENT_CREATED should be broadcasted.
  ASSERT_EQ(0, tester_connector.messages_.size());

  // MSG_REGISTER_COMPONENT without payload.
  message = NewMessageForTest(
      MSG_REGISTER_COMPONENT, proto::Message::NEED_REPLY,
      kComponentDefault, kComponentDefault, kInputContextNone);
  serial = message->serial();
  EXPECT_TRUE(hub_->Dispatch(&error_connector, message));
  ASSERT_EQ(1U, error_connector.messages_.size());
  message = error_connector.messages_[0];
  EXPECT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_REGISTER_COMPONENT, kComponentDefault, kComponentDefault,
      kInputContextNone, proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  ASSERT_TRUE(message->payload().has_error());
  EXPECT_EQ(proto::Error::INVALID_PAYLOAD, message->payload().error().code());
  error_connector.ClearMessages();

  // No MSG_COMPONENT_CREATED should be broadcasted.
  ASSERT_EQ(0, tester_connector.messages_.size());

  // Try to remove an input method component.
  uint32 id = ime_connector.components_[0].id();
  ASSERT_NO_FATAL_FAILURE(ime_connector.RemoveComponent(id));
  ASSERT_EQ(NULL, GetComponent(id));
  // Check MSG_COMPONENT_DELETED and MSG_COMPONENT_DETACHED message.
  ASSERT_EQ(2U, tester_connector.messages_.size());

  // MSG_COMPONENT_DETACHED.
  message = tester_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_DETACHED, kComponentDefault, tester_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_EQ(2, message->payload().uint32_size());
  ASSERT_EQ(kInputContextNone, message->payload().uint32(0));
  ASSERT_EQ(id, message->payload().uint32(1));

  // MSG_COMPONENT_DELETED.
  message = tester_connector.messages_[1];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_DELETED, kComponentDefault, tester_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().uint32_size());
  ASSERT_EQ(id, message->payload().uint32(0));
  tester_connector.ClearMessages();

  // Try to detach a connector, all its components should be deleted.
  ASSERT_NO_FATAL_FAILURE(hub_->Detach(&ui_connector));
  ASSERT_EQ(NULL, GetComponent(ui_connector.components_[0].id()));

  // Check MSG_COMPONENT_DELETED and MSG_COMPONENT_DETACHED messages.
  ASSERT_EQ(2U, tester_connector.messages_.size());

  // UI component.
  // MSG_COMPONENT_DETACHED.
  message = tester_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_DETACHED, kComponentDefault, tester_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_EQ(2, message->payload().uint32_size());
  ASSERT_EQ(kInputContextNone, message->payload().uint32(0));
  ASSERT_EQ(ui_connector.components_[0].id(), message->payload().uint32(1));

  // MSG_COMPONENT_DELETED.
  message = tester_connector.messages_[1];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_DELETED, kComponentDefault, tester_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().uint32_size());
  ASSERT_EQ(ui_connector.components_[0].id(), message->payload().uint32(0));
  tester_connector.ClearMessages();

  // Remove an app component then detach its connector.
  id = app_connector1.components_[0].id();
  ASSERT_NO_FATAL_FAILURE(app_connector1.RemoveComponent(id));
  ASSERT_EQ(NULL, GetComponent(id));
  tester_connector.ClearMessages();

  // Detaching app_connector1, which doesn't have any component, should not
  // trigger MSG_COMPONENT_DELETED.
  ASSERT_NO_FATAL_FAILURE(hub_->Detach(&app_connector1));
  // No MSG_COMPONENT_DELETED should be broadcasted.
  ASSERT_EQ(0, tester_connector.messages_.size());
}

TEST_F(HubImplTest, BasicMessageDispatch) {
  MockConnector app_connector;
  MockConnector ime_connector;
  MockConnector ui_connector;

  app_connector.AddComponent(app1_);
  ime_connector.AddComponent(ime1_);
  ui_connector.AddComponent(ui_);

  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ui_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();
  uint32 ime_id = ime_connector.components_[0].id();
  uint32 ui_id = ui_connector.components_[0].id();

  // Create an input context.
  uint32 icid = 0;
  ASSERT_NO_FATAL_FAILURE(CreateInputContext(&app_connector, app_id, &icid));

  // app requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &app_connector, app_id, icid, kAppProduceMessages,
      arraysize(kAppProduceMessages)));

  // Attach IME
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ime_connector, ime_id, icid, false));
  ime_connector.ClearMessages();

  // ime1 requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &ime_connector, ime_id, icid, kIMEProduceMessages,
      arraysize(kIMEProduceMessages)));

  // Attach UI
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ui_connector, ui_id, icid, false));
  ui_connector.ClearMessages();

  // Focus the input context
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, icid, true));

  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ime_connector, ime_id, kInputContextNone, icid));
  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ui_connector, ui_id, kInputContextNone, icid));

  // Send a keyboard event to kInputContextFocused.
  proto::Message* message = NewMessageForTest(
      MSG_SEND_KEY_EVENT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, kInputContextFocused);
  // We need a KeyEvent object as payload to satisfy HubHotkeyManager.
  message->mutable_payload()->mutable_key_event()->set_keycode(123);
  uint32 app_key_event_serial = message->serial();
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));

  // The IME component should receive this message.
  ASSERT_EQ(1U, ime_connector.messages_.size());
  message = ime_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_PROCESS_KEY_EVENT,
      builtin_consumers_[MSG_SEND_KEY_EVENT], ime_id, icid,
      proto::Message::NEED_REPLY, true));
  uint32 ime_key_event_serial = message->serial();
  ime_connector.ClearMessages();

  // Update composition text
  message = NewMessageForTest(MSG_SET_COMPOSITION, proto::Message::NO_REPLY,
                              ime_id, kComponentDefault, icid);
  message->mutable_payload()->mutable_composition()->mutable_text()->set_text(
      "Hello world");
  uint32 serial = message->serial();
  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // The application and compose ui should receive MSG_COMPOSITION_CHANGED
  // message.
  ASSERT_EQ(1U, app_connector.messages_.size());
  message = app_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPOSITION_CHANGED,
      builtin_consumers_[MSG_SET_COMPOSITION], app_id, icid,
      proto::Message::NO_REPLY, true));
  ASSERT_STREQ("Hello world",
               message->payload().composition().text().text().c_str());
  app_connector.ClearMessages();

  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPOSITION_CHANGED,
      builtin_consumers_[MSG_SET_COMPOSITION], ui_id, icid,
      proto::Message::NO_REPLY, true));
  ASSERT_STREQ("Hello world",
               message->payload().composition().text().text().c_str());
  ui_connector.ClearMessages();

  // Update candidate list
  message = NewMessageForTest(MSG_SET_CANDIDATE_LIST, proto::Message::NO_REPLY,
                              ime_id, kComponentDefault, icid);
  message->mutable_payload()->mutable_candidate_list()->set_id(1);
  serial = message->serial();
  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // The candidate ui should receive this message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CANDIDATE_LIST_CHANGED,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST], ui_id, icid,
      proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().candidate_list().id());
  ASSERT_EQ(ime_id, message->payload().candidate_list().owner());
  ui_connector.ClearMessages();

  // Select a candidate by mouse clicking
  message = NewMessageForTest(MSG_SELECT_CANDIDATE, proto::Message::NO_REPLY,
                              ui_id, kComponentDefault, icid);
  serial = message->serial();
  ASSERT_TRUE(hub_->Dispatch(&ui_connector, message));

  // The IME component should receive this message.
  ASSERT_EQ(1U, ime_connector.messages_.size());
  message = ime_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SELECT_CANDIDATE, ui_id, ime_id, icid,
      proto::Message::NO_REPLY, false));
  ASSERT_EQ(serial, message->serial());
  ime_connector.ClearMessages();

  // Confirm the composition text.
  message = NewMessageForTest(MSG_INSERT_TEXT, proto::Message::NO_REPLY,
                              ime_id, kComponentDefault, icid);
  serial = message->serial();
  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // The application should receive this message.
  ASSERT_EQ(1U, app_connector.messages_.size());
  message = app_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_INSERT_TEXT, ime_id, app_id, icid,
      proto::Message::NO_REPLY, false));
  ASSERT_EQ(serial, message->serial());
  app_connector.ClearMessages();

  // Reply the key event.
  // The reply message must be sent to the application component directly.
  message = NewMessageForTest(
      MSG_PROCESS_KEY_EVENT, proto::Message::IS_REPLY,
      ime_id, builtin_consumers_[MSG_SEND_KEY_EVENT], icid);
  message->set_serial(ime_key_event_serial);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // The application should receive this message.
  ASSERT_EQ(1U, app_connector.messages_.size());
  message = app_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SEND_KEY_EVENT,
      builtin_consumers_[MSG_SEND_KEY_EVENT], app_id, icid,
      proto::Message::IS_REPLY, false));
  ASSERT_EQ(app_key_event_serial, message->serial());
  app_connector.ClearMessages();
}

}  // namespace
