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
  MSG_CANDIDATE_LIST_CHANGED,
  MSG_SELECTED_CANDIDATE_CHANGED,
  MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
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
const uint32 kCandidateUIProduceMessages[] = {
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
const uint32 kCandidateUIConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMPONENT_ACTIVATED,
  MSG_COMPONENT_DEACTIVATED,
  MSG_CANDIDATE_LIST_CHANGED,
  MSG_SELECTED_CANDIDATE_CHANGED,
  MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
  MSG_SHOW_CANDIDATE_LIST_UI,
  MSG_HIDE_CANDIDATE_LIST_UI,
  MSG_UPDATE_INPUT_CARET,
};

// Messages an off-the-spot compose window can produce
const uint32 kComposeUIProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
};

// Messages an off-the-spot compose window can consume
const uint32 kComposeUIConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMPONENT_ACTIVATED,
  MSG_COMPONENT_DEACTIVATED,
  MSG_COMPOSITION_CHANGED,
  MSG_UPDATE_INPUT_CARET,
  MSG_SHOW_COMPOSITION_UI,
  MSG_HIDE_COMPOSITION_UI,
};

class HubInputContextManagerTest : public HubImplTestBase {
 protected:
  HubInputContextManagerTest() {
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

    SetupComponentInfo("com.google.candidate_ui", "CandidateUI", "",
                       kCandidateUIProduceMessages,
                       arraysize(kCandidateUIProduceMessages),
                       kCandidateUIConsumeMessages,
                       arraysize(kCandidateUIConsumeMessages),
                       &candidate_ui_);

    SetupComponentInfo("com.google.compose_ui", "ComposeUI", "",
                       kComposeUIProduceMessages,
                       arraysize(kComposeUIProduceMessages),
                       kComposeUIConsumeMessages,
                       arraysize(kComposeUIConsumeMessages),
                       &compose_ui_);
  }

  virtual ~HubInputContextManagerTest() {
  }

  proto::ComponentInfo app1_;
  proto::ComponentInfo ime1_;
  proto::ComponentInfo ime2_;
  proto::ComponentInfo candidate_ui_;
  proto::ComponentInfo compose_ui_;
};

TEST_F(HubInputContextManagerTest, CreateDeleteInputContext) {
  MockConnector app_connector;

  app_connector.AddComponent(app1_);
  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();

  std::set<uint32> icids;
  proto::Message* message;
  for (int i = 0; i < 10; ++i) {
    SCOPED_TRACE(testing::Message() << "Creating ic:" << i);

    // Create some input contexts.
    uint32 icid = 0;
    ASSERT_NO_FATAL_FAILURE(CreateInputContext(&app_connector, app_id, &icid));

    // Check the new input context
    EXPECT_FALSE(icids.count(icid));
    icids.insert(icid);

    InputContext* ic = GetInputContext(icid);
    EXPECT_TRUE(ic);
  }

  // Delete an input context.
  message = NewMessageForTest(
      MSG_DELETE_INPUT_CONTEXT, proto::Message::NO_REPLY,
      app_id, kComponentDefault, *icids.begin());

  app_connector.ClearMessages();
  EXPECT_TRUE(hub_->Dispatch(&app_connector, message));
  EXPECT_EQ(NULL, GetInputContext(*icids.begin()));
  EXPECT_EQ(0, app_connector.messages_.size());
  icids.erase(icids.begin());

  // Delete another one expecting reply message.
  message = NewMessageForTest(
      MSG_DELETE_INPUT_CONTEXT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, *icids.begin());
  const uint32 serial = message->serial();

  EXPECT_TRUE(hub_->Dispatch(&app_connector, message));
  EXPECT_EQ(NULL, GetInputContext(*icids.begin()));
  EXPECT_EQ(1U, app_connector.messages_.size());

  message = app_connector.messages_[0];
  EXPECT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_DELETE_INPUT_CONTEXT,
      builtin_consumers_[MSG_DELETE_INPUT_CONTEXT], app_id,
      *icids.begin(), proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  EXPECT_EQ(1U, message->payload().boolean_size());
  EXPECT_TRUE(message->payload().boolean(0));
  app_connector.ClearMessages();
  icids.erase(icids.begin());

  // All input contexts should be deleted when removing the component.
  app_connector.RemoveComponentByIndex(0);
  for (std::set<uint32>::iterator it = icids.begin(); it != icids.end(); ++it) {
    EXPECT_EQ(NULL, GetInputContext(*it));
  }
}

TEST_F(HubInputContextManagerTest, ComponentActivation) {
  MockConnector app_connector;
  MockConnector ime1_connector;
  MockConnector ime2_connector;
  MockConnector candidate_ui_connector;
  MockConnector compose_ui_connector;

  // Let's monitor MSG_COMPONENT_ACTIVATED and MSG_COMPONENT_DEACTIVATED
  app1_.add_consume_message(MSG_COMPONENT_ACTIVATED);
  app1_.add_consume_message(MSG_COMPONENT_DEACTIVATED);

  app_connector.AddComponent(app1_);
  ime1_connector.AddComponent(ime1_);
  ime2_connector.AddComponent(ime2_);
  candidate_ui_connector.AddComponent(candidate_ui_);
  compose_ui_connector.AddComponent(compose_ui_);

  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime1_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime2_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(candidate_ui_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(compose_ui_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();
  uint32 ime1_id = ime1_connector.components_[0].id();
  uint32 ime2_id = ime2_connector.components_[0].id();
  uint32 candidate_ui_id = candidate_ui_connector.components_[0].id();
  uint32 compose_ui_id = compose_ui_connector.components_[0].id();

  // Check default active consumers.
  const uint32 kMessagesToCheckActiveConsumers[] = {
    MSG_CREATE_INPUT_CONTEXT,
    MSG_SET_COMPOSITION,
    MSG_INSERT_TEXT,
    MSG_PROCESS_KEY_EVENT,
    MSG_CANCEL_COMPOSITION,
    MSG_COMPLETE_COMPOSITION,
    MSG_SET_CANDIDATE_LIST,
    MSG_SET_SELECTED_CANDIDATE,
    MSG_SET_CANDIDATE_LIST_VISIBILITY,
    MSG_SHOW_COMPOSITION_UI,
    MSG_HIDE_COMPOSITION_UI,
    MSG_SHOW_CANDIDATE_LIST_UI,
    MSG_HIDE_CANDIDATE_LIST_UI,
  };

  const uint32 kDefaultActiveConsumers[] = {
    builtin_consumers_[MSG_CREATE_INPUT_CONTEXT],
    builtin_consumers_[MSG_SET_COMPOSITION],
    app_id,
    ime1_id,
    ime1_id,
    ime1_id,
    builtin_consumers_[MSG_SET_CANDIDATE_LIST],
    builtin_consumers_[MSG_SET_SELECTED_CANDIDATE],
    builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY],
    compose_ui_id,
    compose_ui_id,
    candidate_ui_id,
    candidate_ui_id,
  };

  ASSERT_NO_FATAL_FAILURE(CheckActiveConsumers(
      kInputContextNone,
      kMessagesToCheckActiveConsumers,
      arraysize(kMessagesToCheckActiveConsumers),
      kDefaultActiveConsumers,
      arraysize(kDefaultActiveConsumers)));

  // Create an input context.
  uint32 icid = 0;
  ASSERT_NO_FATAL_FAILURE(CreateInputContext(&app_connector, app_id, &icid));

  // No other component should be attached, until the application requests
  // consumers explicitly.
  ASSERT_EQ(0, ime1_connector.messages_.size());
  ASSERT_EQ(0, candidate_ui_connector.messages_.size());
  ASSERT_EQ(0, compose_ui_connector.messages_.size());

  // app requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &app_connector, app_id, icid, kAppProduceMessages,
      arraysize(kAppProduceMessages)));

  const uint32 kIMEMandatoryActiveMessages[] = {
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
  };

  // Check MSG_ATTACH_TO_INPUT_CONTEXT messages received by ime1_ and actually
  // attach it.
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ime1_connector, ime1_id, icid, false));
  ASSERT_NO_FATAL_FAILURE(CheckMsgComponentActivated(
      &ime1_connector, ime1_id, icid,
      kIMEMandatoryActiveMessages, arraysize(kIMEMandatoryActiveMessages)));

  const uint32 kCandidateUIMandatoryActiveMessages[] = {
    MSG_SHOW_CANDIDATE_LIST_UI,
    MSG_HIDE_CANDIDATE_LIST_UI,
  };

  // UI components should be attached already.
  ASSERT_EQ(1, candidate_ui_connector.messages_.size());
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &candidate_ui_connector, candidate_ui_id, icid, false));
  ASSERT_NO_FATAL_FAILURE(CheckMsgComponentActivated(
      &candidate_ui_connector, candidate_ui_id, icid,
      kCandidateUIMandatoryActiveMessages,
      arraysize(kCandidateUIMandatoryActiveMessages)));

  const uint32 kComposeUIMandatoryActiveMessages[] = {
    MSG_SHOW_COMPOSITION_UI,
    MSG_HIDE_COMPOSITION_UI,
  };

  ASSERT_EQ(1, compose_ui_connector.messages_.size());
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &compose_ui_connector, compose_ui_id, icid, false));
  ASSERT_NO_FATAL_FAILURE(CheckMsgComponentActivated(
      &compose_ui_connector, compose_ui_id, icid,
      kComposeUIMandatoryActiveMessages,
      arraysize(kComposeUIMandatoryActiveMessages)));

  // ime1 requests message consumers. Nothing should happen.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &ime1_connector, ime1_id, icid, kIMEProduceMessages,
      arraysize(kIMEProduceMessages)));

  // candidate_ui requests messages consumers. Nothing should happen.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &candidate_ui_connector, candidate_ui_id, icid,
      kCandidateUIProduceMessages,
      arraysize(kCandidateUIProduceMessages)));

  // ime1 should not be requested again.
  ASSERT_EQ(0, ime1_connector.messages_.size());

  // ime2_ should not be attached.
  ASSERT_EQ(0, ime2_connector.messages_.size());

  // The active consumers should be same as the default input context.
  ASSERT_NO_FATAL_FAILURE(CheckActiveConsumers(
      icid,
      kMessagesToCheckActiveConsumers,
      arraysize(kMessagesToCheckActiveConsumers),
      kDefaultActiveConsumers,
      arraysize(kDefaultActiveConsumers)));

  // Switch to ime2_
  ASSERT_NO_FATAL_FAILURE(ActivateComponent(icid, ime2_id));

  // Check MSG_ATTACH_TO_INPUT_CONTEXT messages received by ime2_.
  ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
      &ime2_connector, ime2_id, icid, true));
  ASSERT_NO_FATAL_FAILURE(CheckMsgComponentActivated(
      &ime2_connector, ime2_id, icid,
      kIMEMandatoryActiveMessages, arraysize(kIMEMandatoryActiveMessages)));

  // ime2 requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &ime2_connector, ime2_id, icid, kIMEProduceMessages,
      arraysize(kIMEProduceMessages)));

  const uint32 kActiveConsumersWithIME2[] = {
    builtin_consumers_[MSG_CREATE_INPUT_CONTEXT],
    builtin_consumers_[MSG_SET_COMPOSITION],
    app_id,
    ime2_id,
    ime2_id,
    ime2_id,
    builtin_consumers_[MSG_SET_CANDIDATE_LIST],
    builtin_consumers_[MSG_SET_SELECTED_CANDIDATE],
    builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY],
    compose_ui_id,
    compose_ui_id,
    candidate_ui_id,
    candidate_ui_id,
  };

  // ime2_ should replace ime1_.
  ASSERT_NO_FATAL_FAILURE(CheckActiveConsumers(
      icid,
      kMessagesToCheckActiveConsumers,
      arraysize(kMessagesToCheckActiveConsumers),
      kActiveConsumersWithIME2,
      arraysize(kActiveConsumersWithIME2)));

  // ime1_ should be deactivated and detached.
  ASSERT_EQ(2U, ime1_connector.messages_.size());
  proto::Message* message = ime1_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_DEACTIVATED, kComponentDefault, ime1_id,
      icid, proto::Message::NO_REPLY, true));
  EXPECT_NO_FATAL_FAILURE(CheckUnorderedUint32Payload(
      message, kIMEMandatoryActiveMessages,
      arraysize(kIMEMandatoryActiveMessages), false));

  message = ime1_connector.messages_[1];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_DETACHED_FROM_INPUT_CONTEXT, kComponentDefault, ime1_id,
      icid, proto::Message::NO_REPLY, false));
  ime1_connector.ClearMessages();

  // Attach ime1_ again.
  message = NewMessageForTest(
      MSG_ATTACH_TO_INPUT_CONTEXT, proto::Message::NEED_REPLY,
      ime1_id, kComponentDefault, icid);
  uint32 serial = message->serial();
  ASSERT_TRUE(hub_->Dispatch(&ime1_connector, message));

  // ime1_ should not be activated, but should be attached correctly.
  ASSERT_EQ(1U, ime1_connector.messages_.size());
  message = ime1_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_ATTACH_TO_INPUT_CONTEXT,
      builtin_consumers_[MSG_ATTACH_TO_INPUT_CONTEXT], ime1_id,
      icid, proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  ASSERT_TRUE(message->has_payload());
  ASSERT_EQ(1, message->payload().boolean_size());
  ASSERT_TRUE(message->payload().boolean(0));
  ime1_connector.ClearMessages();

  // ime1 requests message consumers.
  ASSERT_NO_FATAL_FAILURE(RequestConsumers(
      &ime1_connector, ime1_id, icid, kIMEProduceMessages,
      arraysize(kIMEProduceMessages)));

  // Switch back to ime1_.
  ASSERT_NO_FATAL_FAILURE(ActivateComponent(icid, ime1_id));
  // ime1_ should receive MSG_COMPONENT_ACTIVATED message, but no
  // MSG_ATTACH_TO_INPUT_CONTEXT anymore.
  ASSERT_NO_FATAL_FAILURE(CheckMsgComponentActivated(
      &ime1_connector, ime1_id, icid,
      kIMEMandatoryActiveMessages, arraysize(kIMEMandatoryActiveMessages)));
  ime1_connector.ClearMessages();

  // ime1_ should replace ime2_.
  ASSERT_NO_FATAL_FAILURE(CheckActiveConsumers(
      icid,
      kMessagesToCheckActiveConsumers,
      arraysize(kMessagesToCheckActiveConsumers),
      kDefaultActiveConsumers,
      arraysize(kDefaultActiveConsumers)));

  // ime2_ should be deactivated and detached.
  ASSERT_EQ(2U, ime2_connector.messages_.size());
  message = ime2_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_DEACTIVATED, kComponentDefault, ime2_id,
      icid, proto::Message::NO_REPLY, true));
  EXPECT_NO_FATAL_FAILURE(CheckUnorderedUint32Payload(
      message, kIMEMandatoryActiveMessages,
      arraysize(kIMEMandatoryActiveMessages), false));

  message = ime2_connector.messages_[1];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_DETACHED_FROM_INPUT_CONTEXT, kComponentDefault, ime2_id,
      icid, proto::Message::NO_REPLY, false));
  ime2_connector.ClearMessages();
}

TEST_F(HubInputContextManagerTest, FocusInputContext) {
  MockConnector app_connector;
  MockConnector ime_connector;
  MockConnector ui_connector;

  app_connector.AddComponent(app1_);
  ime_connector.AddComponent(ime1_);
  ui_connector.AddComponent(candidate_ui_);

  ASSERT_NO_FATAL_FAILURE(app_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ime_connector.Attach(hub_));
  ASSERT_NO_FATAL_FAILURE(ui_connector.Attach(hub_));

  uint32 app_id = app_connector.components_[0].id();
  uint32 ime_id = ime_connector.components_[0].id();
  uint32 candidate_ui_id = ui_connector.components_[0].id();

  // Create some input contexts.
  uint32 icid[3];
  for (size_t i = 0; i < 3; ++i) {
    ASSERT_NO_FATAL_FAILURE(
        CreateInputContext(&app_connector, app_id, &icid[i]));

    // app requests message consumers.
    ASSERT_NO_FATAL_FAILURE(RequestConsumers(
        &app_connector, app_id, icid[i], kAppProduceMessages,
        arraysize(kAppProduceMessages)));

    // Attach IME
    ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
        &ime_connector, ime_id, icid[i], false));
    ime_connector.ClearMessages();

    // ime requests message consumers.
    ASSERT_NO_FATAL_FAILURE(RequestConsumers(
        &ime_connector, ime_id, icid[i], kIMEProduceMessages,
        arraysize(kIMEProduceMessages)));

    // Attach UI
    ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
        &ui_connector, candidate_ui_id, icid[i], false));
    ui_connector.ClearMessages();
  }

  // Focus an input context
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, icid[0], true));

  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ime_connector, ime_id, kInputContextNone, icid[0]));
  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ui_connector, candidate_ui_id, kInputContextNone, icid[0]));

  // Focus another input context
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, icid[1], true));

  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ime_connector, ime_id, icid[0], icid[1]));
  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ui_connector, candidate_ui_id, icid[0], icid[1]));

  // Focus an already focused input context
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, icid[1], true));
  ASSERT_EQ(0, ime_connector.messages_.size());
  ASSERT_EQ(0, ui_connector.messages_.size());

  // Focusing kInputContextFocused is ok, as long as the input context is owned
  // by the application.
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, kInputContextFocused, true));
  ASSERT_EQ(0, ime_connector.messages_.size());
  ASSERT_EQ(0, ui_connector.messages_.size());

  // Blur the focused input context
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, icid[1], false));

  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ime_connector, ime_id, icid[1], kInputContextNone));
  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ui_connector, candidate_ui_id, icid[1], kInputContextNone));

  // Focus another input context
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, icid[2], true));

  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ime_connector, ime_id, kInputContextNone, icid[2]));
  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ui_connector, candidate_ui_id, kInputContextNone, icid[2]));

  // Blurring kInputContextFocused is ok, as long as the current focused input
  // context is owned by this application.
  ASSERT_NO_FATAL_FAILURE(FocusOrBlurInputContext(
      &app_connector, app_id, kInputContextFocused, false));

  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ime_connector, ime_id, icid[2], kInputContextNone));
  ASSERT_NO_FATAL_FAILURE(CheckFocusChangeMessages(
      &ui_connector, candidate_ui_id, icid[2], kInputContextNone));
}

}  // namespace
