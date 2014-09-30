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
  MSG_SWITCH_TO_INPUT_METHOD,
  MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,
  MSG_ADD_HOTKEY_LIST,
  MSG_DO_COMMAND,
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
  MSG_PROCESS_KEY_EVENT,
  MSG_CANCEL_COMPOSITION,
  MSG_COMPLETE_COMPOSITION,
  MSG_DO_COMMAND,
};


class HubInputMethodManagerTest : public HubImplTestBase {
 protected:
  HubInputMethodManagerTest() {
    SetupComponentInfo("com.google.app1", "App", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       &app_);

    SetupComponentInfo("com.google.ime1", "Ime1", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &ime1_);

    // Ime2 don't process MSG_DO_COMMAND so that ime1 will not be detached when
    // switching to ime2.
    SetupComponentInfo("com.google.ime2", "Ime2", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages) - 1,
                       &ime2_);
  }

  virtual ~HubInputMethodManagerTest() {
  }

  proto::ComponentInfo app_;
  proto::ComponentInfo ime1_;
  proto::ComponentInfo ime2_;
};


TEST_F(HubInputMethodManagerTest, SwitchInputMethodTest) {
  MockConnector app_connector;
  MockConnector ime1_connector;
  MockConnector ime2_connector;

  app_connector.AddComponent(app_);
  ime1_connector.AddComponent(ime1_);
  ime2_connector.AddComponent(ime2_);

  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime1_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime2_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();
  uint32 ime_id = ime1_connector.components_[0].id();
  uint32 ime2_id = ime2_connector.components_[0].id();

  // Create an input context.
  uint32 icid = 0;
  ASSERT_NO_FATAL_FAILURE(CreateInputContext(&app_connector, app_id, &icid));

  // app requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &app_connector, app_id, icid, kAppProduceMessages,
      arraysize(kAppProduceMessages)));

  // Attach IME1
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ime1_connector, ime_id, icid, false));
  ime1_connector.ClearMessages();

  // Switch to ime2_.
  ipc::proto::Message* message = NewMessageForTest(
      MSG_SWITCH_TO_INPUT_METHOD, proto::Message::NO_REPLY, app_id,
      kComponentDefault, icid);
  message->mutable_payload()->add_uint32(ime2_id);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));
  const int kFirstKeyCode = 123;
  const int kSecondKeyCode = 124;
  // Send a keyboard event to the input context.
  message = NewMessageForTest(
      MSG_SEND_KEY_EVENT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, icid);
  // We need a KeyEvent object as payload to satisfy HubHotkeyManager.
  message->mutable_payload()->mutable_key_event()->set_keycode(kFirstKeyCode);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));

  // Switch back to ime1_.
  message = NewMessageForTest(
      MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD, proto::Message::NO_REPLY, app_id,
      kComponentDefault, icid);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));

  // Send another keyboard event to the input context.
  message = NewMessageForTest(
      MSG_SEND_KEY_EVENT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, icid);
  // We need a KeyEvent object as payload to satisfy HubHotkeyManager.
  message->mutable_payload()->mutable_key_event()->set_keycode(kSecondKeyCode);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));

  ASSERT_EQ(1U, ime1_connector.messages_.size());
  // ime1 should only receive MSG_CANCEL_COMPOSITION.
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      ime1_connector.messages_[0], MSG_CANCEL_COMPOSITION,
      builtin_consumers_[MSG_SWITCH_TO_INPUT_METHOD], ime_id, icid,
      proto::Message::NEED_REPLY, false));
  // Reply MSG_CANCEL_COMPOSITION
  message = NewMessageForTest(
      MSG_CANCEL_COMPOSITION, proto::Message::IS_REPLY,
      ime_id, builtin_consumers_[MSG_SWITCH_TO_INPUT_METHOD], icid);
  ASSERT_TRUE(hub_->Dispatch(&ime1_connector, message));
  ime1_connector.ClearMessages();
  // ime2 is not attached yet, so it should only received message
  // ATTACH_TO_INPUT_CONTEXT.
  ASSERT_EQ(1U, ime2_connector.messages_.size());
  // Attach IME2.
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ime2_connector, ime2_id, icid, true));
  // The ime2 component should receive:
  // MSG_PROCESS_KEY_EVENT, and MSG_CANCEL_COMPOSITION
  ASSERT_EQ(2U, ime2_connector.messages_.size());
  message = ime2_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_PROCESS_KEY_EVENT,
      builtin_consumers_[MSG_SEND_KEY_EVENT], ime2_id, icid,
      proto::Message::NEED_REPLY, true));
  ASSERT_TRUE(message->payload().has_key_event());
  ASSERT_EQ(kFirstKeyCode, message->payload().key_event().keycode());
  message = ime2_connector.messages_[1];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CANCEL_COMPOSITION,
      builtin_consumers_[MSG_SWITCH_TO_INPUT_METHOD], ime2_id, icid,
      proto::Message::NEED_REPLY, false));
  ime2_connector.ClearMessages();
  // Reply MSG_CANCEL_COMPOSITION
  message = NewMessageForTest(
      MSG_CANCEL_COMPOSITION, proto::Message::IS_REPLY,
      ime2_id, builtin_consumers_[MSG_SWITCH_TO_INPUT_METHOD], icid);
  ASSERT_TRUE(hub_->Dispatch(&ime2_connector, message));
  // The ime2 component should receive: MSG_DETACHED_FROM_INPUT_CONTEXT
  ASSERT_EQ(1U, ime2_connector.messages_.size());
  message = ime2_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_DETACHED_FROM_INPUT_CONTEXT,
      kComponentDefault, ime2_id, icid,
      proto::Message::NO_REPLY, false));

  ime2_connector.ClearMessages();
  // The ime1 component should receive MSG_PROCESS_KEY_EVENT.
  ASSERT_EQ(1U, ime1_connector.messages_.size());
  message = ime1_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_PROCESS_KEY_EVENT,
      builtin_consumers_[MSG_SEND_KEY_EVENT], ime_id, icid,
      proto::Message::NEED_REPLY, true));
  ASSERT_TRUE(message->payload().has_key_event());
  ASSERT_EQ(kSecondKeyCode, message->payload().key_event().keycode());
  ime1_connector.ClearMessages();
  // Switch to ime2_.
  message = NewMessageForTest(
      MSG_SWITCH_TO_INPUT_METHOD, proto::Message::NO_REPLY, app_id,
      kComponentDefault, icid);
  message->mutable_payload()->add_uint32(ime2_id);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));

  // Send a keyboard event to the input context.
  message = NewMessageForTest(
      MSG_SEND_KEY_EVENT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, icid);
  // We need a KeyEvent object as payload to satisfy HubHotkeyManager.
  message->mutable_payload()->mutable_key_event()->set_keycode(kFirstKeyCode);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));

  // ime1 should only receive MSG_CANCEL_COMPOSITION.
  ASSERT_EQ(1U, ime1_connector.messages_.size());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      ime1_connector.messages_[0], MSG_CANCEL_COMPOSITION,
      builtin_consumers_[MSG_SWITCH_TO_INPUT_METHOD], ime_id, icid,
      proto::Message::NEED_REPLY, false));
  ime1_connector.ClearMessages();
  // Reply MSG_CANCEL_COMPOSITION
  message = NewMessageForTest(
      MSG_CANCEL_COMPOSITION, proto::Message::IS_REPLY,
      ime_id, builtin_consumers_[MSG_SWITCH_TO_INPUT_METHOD], icid);
  ASSERT_TRUE(hub_->Dispatch(&ime1_connector, message));
  // Remove ime2.
  ime2_connector.Detach();
  // ime1 should receive the key event;
  ASSERT_EQ(1U, ime1_connector.messages_.size());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      ime1_connector.messages_[0], MSG_PROCESS_KEY_EVENT,
      builtin_consumers_[MSG_SEND_KEY_EVENT], ime_id, icid,
      proto::Message::NEED_REPLY, true));
  ime1_connector.ClearMessages();

  // Connect ime2 again.
  ASSERT_NO_FATAL_FAILURE(ime2_connector.Attach(hub_));
  // Trying switching to ime2.
  message = NewMessageForTest(
      MSG_SWITCH_TO_INPUT_METHOD, proto::Message::NO_REPLY, app_id,
      kComponentDefault, icid);
  ime2_id = ime2_connector.components_[0].id();
  message->mutable_payload()->add_uint32(ime2_id);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));
  // Send a keyboard event to the input context.
  message = NewMessageForTest(
      MSG_SEND_KEY_EVENT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, icid);
  // We need a KeyEvent object as payload to satisfy HubHotkeyManager.
  message->mutable_payload()->mutable_key_event()->set_keycode(kFirstKeyCode);
  ASSERT_TRUE(hub_->Dispatch(&app_connector, message));
  // Delete the input context.
  // The input method manager should discard all caching messages and delete the
  // input context related data.
  ASSERT_TRUE(hub_->DeleteInputContext(hub_->GetComponent(app_id), icid));
  // The program should not crash.
}
}  // namespace
