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
  MSG_UPDATE_INPUT_CARET,
  MSG_DO_COMMAND,
};

// Messages a toolbar window can produce
const uint32 kToolbarUIProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_QUERY_COMMAND_LIST,
  MSG_DO_COMMAND,
};

// Messages a toolbar window can consume
const uint32 kToolbarUIConsumeMessages[] = {
  MSG_INPUT_CONTEXT_CREATED,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMMAND_LIST_CHANGED,
};

class HubCommandListManagerTest : public HubImplTestBase {
 protected:
  HubCommandListManagerTest() {
    SetupComponentInfo("com.google.app1", "App1", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       &app1_);

    SetupComponentInfo("com.google.ime1", "Ime1", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &ime1_);

    SetupComponentInfo("com.google.ime2", "Ime2", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &ime2_);

    SetupComponentInfo("com.google.toolbar_ui", "ToolbarUI", "",
                       kToolbarUIProduceMessages,
                       arraysize(kToolbarUIProduceMessages),
                       kToolbarUIConsumeMessages,
                       arraysize(kToolbarUIConsumeMessages),
                       &toolbar_ui_);
  }

  virtual ~HubCommandListManagerTest() {
  }

  proto::ComponentInfo app1_;
  proto::ComponentInfo ime1_;
  proto::ComponentInfo ime2_;
  proto::ComponentInfo toolbar_ui_;
};

TEST_F(HubCommandListManagerTest, CommandList) {
  MockConnector app_connector;
  MockConnector ime_connector;
  MockConnector ui_connector;

  app_connector.AddComponent(app1_);
  ime_connector.AddComponent(ime1_);
  ime_connector.AddComponent(ime2_);
  ui_connector.AddComponent(toolbar_ui_);

  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ui_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();
  uint32 ime1_id = ime_connector.components_[0].id();
  uint32 ime2_id = ime_connector.components_[1].id();
  uint32 toolbar_id = ui_connector.components_[0].id();

  // ime2 registers some global commands.
  proto::Message* message =
      NewMessageForTest(MSG_SET_COMMAND_LIST, proto::Message::NO_REPLY,
                        ime2_id, kComponentDefault, kInputContextNone);

  // Constructs a cascaded command list
  // 1
  // 2 -> 4
  // 3    5 -> 6
  proto::CommandList* command_list =
      message->mutable_payload()->add_command_list();

  proto::Command* command = command_list->add_command();
  command->set_id("1");
  command->mutable_title()->set_text("1");
  command = command_list->add_command();
  command->set_id("2");
  command->mutable_title()->set_text("2");

  proto::CommandList* sub_commands = command->mutable_sub_commands();
  command = command_list->add_command();
  command->set_id("3");
  command->mutable_title()->set_text("3");

  command = sub_commands->add_command();
  command->set_id("4");
  command->mutable_title()->set_text("4");
  command = sub_commands->add_command();
  command->set_id("5");
  command->mutable_title()->set_text("5");

  sub_commands = command->mutable_sub_commands();
  command = sub_commands->add_command();
  command->set_id("6");
  command->mutable_title()->set_text("6");

  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // toolbar should receive MSG_COMMAND_LIST_CHANGED message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMMAND_LIST_CHANGED,
      builtin_consumers_[MSG_SET_COMMAND_LIST], toolbar_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(1, message->payload().command_list_size());
  EXPECT_EQ(ime2_id, message->payload().command_list(0).owner());
  ASSERT_EQ(3, message->payload().command_list(0).command_size());
  EXPECT_EQ("1", message->payload().command_list(0).command(0).id());
  EXPECT_EQ("1", message->payload().command_list(0).command(0).title().text());
  EXPECT_EQ("2", message->payload().command_list(0).command(1).id());
  EXPECT_EQ("2", message->payload().command_list(0).command(1).title().text());
  ASSERT_TRUE(message->payload().command_list(0).command(1).has_sub_commands());
  EXPECT_EQ(ime2_id, message->payload().command_list(0).command(1).
            sub_commands().owner());
  ASSERT_EQ(1, message->payload().boolean_size());
  ASSERT_TRUE(message->payload().boolean(0));

  ui_connector.ClearMessages();

  // ime1 registers some global commands.
  message = NewMessageForTest(MSG_SET_COMMAND_LIST, proto::Message::NO_REPLY,
                              ime1_id, kComponentDefault, kInputContextNone);

  command_list = message->mutable_payload()->add_command_list();

  command = command_list->add_command();
  command->set_id("7");
  command->mutable_title()->set_text("7");

  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // toolbar should receive MSG_COMMAND_LIST_CHANGED message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMMAND_LIST_CHANGED,
      builtin_consumers_[MSG_SET_COMMAND_LIST], toolbar_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(2, message->payload().command_list_size());
  ASSERT_EQ(2, message->payload().boolean_size());
  EXPECT_EQ(ime1_id, message->payload().command_list(0).owner());
  ASSERT_EQ(1, message->payload().command_list(0).command_size());
  EXPECT_EQ("7", message->payload().command_list(0).command(0).id());
  EXPECT_EQ("7", message->payload().command_list(0).command(0).title().text());
  EXPECT_TRUE(message->payload().boolean(0));

  EXPECT_EQ(ime2_id, message->payload().command_list(1).owner());
  EXPECT_EQ(3, message->payload().command_list(1).command_size());
  EXPECT_FALSE(message->payload().boolean(1));

  ui_connector.ClearMessages();
  ime_connector.ClearMessages();

  // ime2 updates some global commands, and checks reply message.
  message = NewMessageForTest(MSG_UPDATE_COMMANDS, proto::Message::NEED_REPLY,
                              ime2_id, kComponentDefault, kInputContextNone);

  command_list = message->mutable_payload()->add_command_list();

  command = command_list->add_command();
  command->set_id("1");
  command->mutable_title()->set_text("1n");
  command = command_list->add_command();
  command->set_id("6");
  command->mutable_title()->set_text("6n");

  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // Check reply message.
  ASSERT_EQ(1U, ime_connector.messages_.size());
  message = ime_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_UPDATE_COMMANDS,
      builtin_consumers_[MSG_SET_COMMAND_LIST], ime2_id,
      kInputContextNone, proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(1, message->payload().boolean_size());
  EXPECT_TRUE(message->payload().boolean(0));

  // toolbar should receive MSG_COMMAND_LIST_CHANGED message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMMAND_LIST_CHANGED,
      builtin_consumers_[MSG_SET_COMMAND_LIST], toolbar_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(2, message->payload().command_list_size());
  ASSERT_EQ(2, message->payload().boolean_size());
  EXPECT_EQ(ime1_id, message->payload().command_list(0).owner());
  ASSERT_EQ(1, message->payload().command_list(0).command_size());
  EXPECT_EQ("7", message->payload().command_list(0).command(0).id());
  EXPECT_EQ("7", message->payload().command_list(0).command(0).title().text());
  EXPECT_FALSE(message->payload().boolean(0));

  EXPECT_EQ(ime2_id, message->payload().command_list(1).owner());
  ASSERT_EQ(3, message->payload().command_list(1).command_size());
  EXPECT_EQ("1", message->payload().command_list(1).command(0).id());
  EXPECT_EQ("1n", message->payload().command_list(1).command(0).title().text());
  ASSERT_TRUE(message->payload().command_list(1).command(1).has_sub_commands());
  ASSERT_EQ(2, message->payload().command_list(1).command(1).sub_commands().
            command_size());
  ASSERT_TRUE(message->payload().command_list(1).command(1).sub_commands().
              command(1).has_sub_commands());
  ASSERT_EQ("6", message->payload().command_list(1).command(1).sub_commands().
            command(1).sub_commands().command(0).id());
  ASSERT_EQ("6n", message->payload().command_list(1).command(1).sub_commands().
            command(1).sub_commands().command(0).title().text());
  EXPECT_TRUE(message->payload().boolean(1));

  ui_connector.ClearMessages();
  ime_connector.ClearMessages();

  // FAIL case: ime1 updates some global commands, and checks reply message.
  message = NewMessageForTest(MSG_UPDATE_COMMANDS, proto::Message::NEED_REPLY,
                              ime1_id, kComponentDefault, kInputContextNone);

  command_list = message->mutable_payload()->add_command_list();

  command = command_list->add_command();
  command->set_id("10");

  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // Check reply message.
  ASSERT_EQ(1U, ime_connector.messages_.size());
  message = ime_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_UPDATE_COMMANDS,
      builtin_consumers_[MSG_SET_COMMAND_LIST], ime1_id,
      kInputContextNone, proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(1, message->payload().boolean_size());
  EXPECT_FALSE(message->payload().boolean(0));

  ime_connector.ClearMessages();

  // toolbar should receive nothing.
  ASSERT_EQ(0, ui_connector.messages_.size());


  // toolbar queries command list, and checks reply message.
  message = NewMessageForTest(
      MSG_QUERY_COMMAND_LIST, proto::Message::NEED_REPLY,
      toolbar_id, kComponentDefault, kInputContextNone);
  ASSERT_TRUE(hub_->Dispatch(&ui_connector, message));

  // Check reply message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_QUERY_COMMAND_LIST,
      builtin_consumers_[MSG_SET_COMMAND_LIST], toolbar_id,
      kInputContextNone, proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(2, message->payload().command_list_size());
  EXPECT_EQ(0, message->payload().boolean_size());
  EXPECT_EQ(ime1_id, message->payload().command_list(0).owner());
  EXPECT_EQ(1, message->payload().command_list(0).command_size());
  EXPECT_EQ(ime2_id, message->payload().command_list(1).owner());
  EXPECT_EQ(3, message->payload().command_list(1).command_size());

  ui_connector.ClearMessages();

  // ime1 deletes global commands.
  message = NewMessageForTest(MSG_SET_COMMAND_LIST, proto::Message::NO_REPLY,
                              ime1_id, kComponentDefault, kInputContextNone);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // toolbar should receive MSG_COMMAND_LIST_CHANGED message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMMAND_LIST_CHANGED,
      builtin_consumers_[MSG_SET_COMMAND_LIST], toolbar_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(2, message->payload().command_list_size());
  ASSERT_EQ(2, message->payload().boolean_size());
  EXPECT_EQ(ime1_id, message->payload().command_list(0).owner());
  EXPECT_EQ(0, message->payload().command_list(0).command_size());
  EXPECT_TRUE(message->payload().boolean(0));
  EXPECT_EQ(ime2_id, message->payload().command_list(1).owner());
  EXPECT_EQ(3, message->payload().command_list(1).command_size());
  EXPECT_FALSE(message->payload().boolean(1));

  ui_connector.ClearMessages();
  ime_connector.ClearMessages();

  // ime1 deletes global commands again.
  message = NewMessageForTest(MSG_SET_COMMAND_LIST, proto::Message::NO_REPLY,
                              ime1_id, kComponentDefault, kInputContextNone);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // toolbar should receive nothing.
  ASSERT_EQ(0, ui_connector.messages_.size());


  // Create an input context.
  uint32 icid = 0;
  ASSERT_NO_FATAL_FAILURE(
      CreateInputContext(&app_connector, app_id, &icid));

  // app requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &app_connector, app_id, icid, kAppProduceMessages,
      arraysize(kAppProduceMessages)));

  // Attach IME
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ime_connector, ime1_id, icid, false));
  ime_connector.ClearMessages();

  // toolbar should receive MSG_INPUT_CONTEXT_CREATED message.
  ASSERT_EQ(1U, ui_connector.messages_.size());
  message = ui_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_INPUT_CONTEXT_CREATED,
      kComponentDefault, toolbar_id,
      kInputContextNone, proto::Message::NO_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_TRUE(message->payload().has_input_context_info());
  ASSERT_EQ(icid, message->payload().input_context_info().id());
  ASSERT_EQ(app_id, message->payload().input_context_info().owner());

  // Attach toolbar UI
  message = NewMessageForTest(
      MSG_ATTACH_TO_INPUT_CONTEXT, proto::Message::NO_REPLY,
      toolbar_id, kComponentDefault, icid);
  ASSERT_TRUE(hub_->Dispatch(&ui_connector, message));
  ui_connector.ClearMessages();

  // FAIL case: ime2 registers some commands to the new input context.

  message = NewMessageForTest(MSG_SET_COMMAND_LIST, proto::Message::NEED_REPLY,
                              ime2_id, kComponentDefault, icid);

  command_list = message->mutable_payload()->add_command_list();
  command = command_list->add_command();
  command->set_id("8");
  command->mutable_title()->set_text("8");

  ASSERT_TRUE(hub_->Dispatch(&ime_connector, message));

  // Check reply message.
  ASSERT_EQ(1U, ime_connector.messages_.size());
  message = ime_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_COMMAND_LIST,
      builtin_consumers_[MSG_SET_COMMAND_LIST], ime2_id,
      icid, proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->has_payload());
  ASSERT_TRUE(message->payload().has_error());
  EXPECT_EQ(proto::Error::COMPONENT_NOT_ATTACHED,
            message->payload().error().code());

  ime_connector.ClearMessages();

  // toolbar should receive nothing.
  ASSERT_EQ(0, ui_connector.messages_.size());
}

}  // namespace
