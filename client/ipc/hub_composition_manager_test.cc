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
  MSG_QUERY_COMPOSITION,
  MSG_QUERY_CANDIDATE_LIST,
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

class HubCompositionManagerTest : public HubImplTestBase {
 protected:
  HubCompositionManagerTest() {
    proto::ComponentInfo info;
    SetupComponentInfo("com.google.app", "App", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       &info);
    app_connector_.AddComponent(info);
    info.Clear();

    SetupComponentInfo("com.google.ime", "Ime", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       &info);
    ime_connector_.AddComponent(info);
    info.Clear();

    SetupComponentInfo("com.google.ui", "UI", "",
                       kUIProduceMessages,
                       arraysize(kUIProduceMessages),
                       kUIConsumeMessages,
                       arraysize(kUIConsumeMessages),
                       &info);
    ui_connector_.AddComponent(info);
  }

  virtual ~HubCompositionManagerTest() {
  }

  virtual void SetUp() {
    HubImplTestBase::SetUp();

    ASSERT_NO_FATAL_FAILURE(app_connector_.Attach(hub_));
    ASSERT_NO_FATAL_FAILURE(ime_connector_.Attach(hub_));
    ASSERT_NO_FATAL_FAILURE(ui_connector_.Attach(hub_));

    app_id_ = app_connector_.components_[0].id();
    ime_id_ = ime_connector_.components_[0].id();
    ui_id_ = ui_connector_.components_[0].id();

    // Create an input context.
    icid_ = 0;
    ASSERT_NO_FATAL_FAILURE(
        CreateInputContext(&app_connector_, app_id_, &icid_));

    // app requests message consumers.
    ASSERT_NO_FATAL_FAILURE(RequestConsumers(
        &app_connector_, app_id_, icid_, kAppProduceMessages,
        arraysize(kAppProduceMessages)));

    // Attach IME
    ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
        &ime_connector_, ime_id_, icid_, false));
    ime_connector_.ClearMessages();

    // ime requests message consumers.
    ASSERT_NO_FATAL_FAILURE(RequestConsumers(
        &ime_connector_, ime_id_, icid_, kIMEProduceMessages,
        arraysize(kIMEProduceMessages)));

    // Attach UI
    ASSERT_NO_FATAL_FAILURE(CheckAndReplyMsgAttachToInputContext(
        &ui_connector_, ui_id_, icid_, false));
    ui_connector_.ClearMessages();
  }

  MockConnector app_connector_;
  MockConnector ime_connector_;
  MockConnector ui_connector_;

  uint32 app_id_;
  uint32 ime_id_;
  uint32 ui_id_;
  uint32 icid_;
};

TEST_F(HubCompositionManagerTest, CompositionText) {
  // Update composition text
  proto::Message* message = NewMessageForTest(
      MSG_SET_COMPOSITION, proto::Message::NO_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->mutable_composition()->mutable_text()->set_text(
      "Hello world");
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));

  // The application and compose ui should receive MSG_COMPOSITION_CHANGED
  // message.
  ASSERT_EQ(1U, app_connector_.messages_.size());
  message = app_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPOSITION_CHANGED,
      builtin_consumers_[MSG_SET_COMPOSITION], app_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_STREQ("Hello world",
               message->payload().composition().text().text().c_str());
  app_connector_.ClearMessages();

  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPOSITION_CHANGED,
      builtin_consumers_[MSG_SET_COMPOSITION], ui_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_STREQ("Hello world",
               message->payload().composition().text().text().c_str());
  ui_connector_.ClearMessages();

  // Query composition text
  message = NewMessageForTest(
      MSG_QUERY_COMPOSITION, proto::Message::NEED_REPLY,
      ui_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ui_connector_, message));

  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_QUERY_COMPOSITION,
      builtin_consumers_[MSG_QUERY_COMPOSITION], ui_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_STREQ("Hello world",
               message->payload().composition().text().text().c_str());
  ui_connector_.ClearMessages();


  // Clear composition text
  message = NewMessageForTest(MSG_SET_COMPOSITION, proto::Message::NO_REPLY,
                              ime_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));

  // The application and compose ui should receive a MSG_COMPOSITION_CHANGED
  // message without payload, indicating the composition text was cleared.
  ASSERT_EQ(1U, app_connector_.messages_.size());
  message = app_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPOSITION_CHANGED,
      builtin_consumers_[MSG_SET_COMPOSITION], app_id_, icid_,
      proto::Message::NO_REPLY, false));
  app_connector_.ClearMessages();

  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPOSITION_CHANGED,
      builtin_consumers_[MSG_SET_COMPOSITION], ui_id_, icid_,
      proto::Message::NO_REPLY, false));
  ui_connector_.ClearMessages();

  // Clear composition text again, nothing should happen.
  message = NewMessageForTest(MSG_SET_COMPOSITION, proto::Message::NO_REPLY,
                              ime_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(0U, app_connector_.messages_.size());
  ASSERT_EQ(0U, ui_connector_.messages_.size());
}

TEST_F(HubCompositionManagerTest, CandidateList) {
  // Update candidate list
  proto::Message* message = NewMessageForTest(
      MSG_SET_CANDIDATE_LIST, proto::Message::NO_REPLY,
      ime_id_, kComponentDefault, icid_);
  proto::CandidateList* cand_list =
      message->mutable_payload()->mutable_candidate_list();
  cand_list->set_id(1);

  // Add some candidates and second-level candidate lists.
  proto::Candidate* cand;
  for (int i = 0; i < 5; ++i) {
    cand = cand_list->add_candidate();
    cand->mutable_sub_candidates()->set_id(10 + i);
  }

  // Add a third-level candidate list.
  cand_list = cand_list->mutable_candidate(2)->mutable_sub_candidates();
  cand = cand_list->add_candidate();
  cand->mutable_sub_candidates()->set_id(15);

  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));

  // The candidate ui should receive this message, but the application
  // shouldn't.
  ASSERT_EQ(0U, app_connector_.messages_.size());
  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CANDIDATE_LIST_CHANGED,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST], ui_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().candidate_list().id());
  ASSERT_EQ(5, message->payload().candidate_list().candidate_size());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().owner());
  ASSERT_EQ(12, message->payload().candidate_list().candidate(2).
            sub_candidates().id());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().candidate(2).
            sub_candidates().owner());
  ASSERT_EQ(15, message->payload().candidate_list().candidate(2).
            sub_candidates().candidate(0).sub_candidates().id());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().candidate(2).
            sub_candidates().candidate(0).sub_candidates().owner());
  ASSERT_FALSE(message->payload().candidate_list().has_selected_candidate());
  ASSERT_FALSE(message->payload().candidate_list().has_visible());
  ASSERT_FALSE(message->payload().candidate_list().candidate(2).
               sub_candidates().has_selected_candidate());
  ASSERT_FALSE(message->payload().candidate_list().candidate(2).
               sub_candidates().has_visible());
  ui_connector_.ClearMessages();

  // Query candidate list
  message = NewMessageForTest(
      MSG_QUERY_CANDIDATE_LIST, proto::Message::NEED_REPLY,
      ui_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ui_connector_, message));

  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_QUERY_CANDIDATE_LIST,
      builtin_consumers_[MSG_QUERY_CANDIDATE_LIST], ui_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_EQ(1, message->payload().candidate_list().id());
  ASSERT_EQ(5, message->payload().candidate_list().candidate_size());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().owner());
  ASSERT_EQ(12, message->payload().candidate_list().candidate(2).
            sub_candidates().id());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().candidate(2).
            sub_candidates().owner());
  ASSERT_EQ(15, message->payload().candidate_list().candidate(2).
            sub_candidates().candidate(0).sub_candidates().id());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().candidate(2).
            sub_candidates().candidate(0).sub_candidates().owner());
  ASSERT_FALSE(message->payload().candidate_list().has_selected_candidate());
  ASSERT_FALSE(message->payload().candidate_list().has_visible());
  ASSERT_FALSE(message->payload().candidate_list().candidate(2).
               sub_candidates().has_selected_candidate());
  ASSERT_FALSE(message->payload().candidate_list().candidate(2).
               sub_candidates().has_visible());
  ui_connector_.ClearMessages();

  // Set selected candidate
  message = NewMessageForTest(
      MSG_SET_SELECTED_CANDIDATE, proto::Message::NEED_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->add_uint32(1);
  message->mutable_payload()->add_uint32(2);

  // We should receive a true reply.
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(1U, ime_connector_.messages_.size());
  message = ime_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_SELECTED_CANDIDATE,
      builtin_consumers_[MSG_SET_SELECTED_CANDIDATE], ime_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->payload().boolean(0));
  ime_connector_.ClearMessages();

  // UI should receive MSG_SELECTED_CANDIDATE_CHANGED.
  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SELECTED_CANDIDATE_CHANGED,
      builtin_consumers_[MSG_SET_SELECTED_CANDIDATE], ui_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_EQ(2, message->payload().uint32_size());
  ASSERT_EQ(1, message->payload().uint32(0));
  ASSERT_EQ(2, message->payload().uint32(1));
  ui_connector_.ClearMessages();

  // Select a candidate in a nonexistent candidate list.
  message = NewMessageForTest(
      MSG_SET_SELECTED_CANDIDATE, proto::Message::NEED_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->add_uint32(100);
  message->mutable_payload()->add_uint32(2);

  // We should receive a false reply.
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(1U, ime_connector_.messages_.size());
  message = ime_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_SELECTED_CANDIDATE,
      builtin_consumers_[MSG_SET_SELECTED_CANDIDATE], ime_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_FALSE(message->payload().boolean(0));
  ime_connector_.ClearMessages();

  ASSERT_EQ(0U, ui_connector_.messages_.size());

  // Select a candidate in a sub candidate list.
  message = NewMessageForTest(
      MSG_SET_SELECTED_CANDIDATE, proto::Message::NEED_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->add_uint32(12);
  message->mutable_payload()->add_uint32(0);

  // We should receive a true reply.
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(1U, ime_connector_.messages_.size());
  message = ime_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_SELECTED_CANDIDATE,
      builtin_consumers_[MSG_SET_SELECTED_CANDIDATE], ime_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->payload().boolean(0));
  ime_connector_.ClearMessages();

  // UI should receive MSG_SELECTED_CANDIDATE_CHANGED.
  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SELECTED_CANDIDATE_CHANGED,
      builtin_consumers_[MSG_SET_SELECTED_CANDIDATE], ui_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_EQ(2, message->payload().uint32_size());
  ASSERT_EQ(12, message->payload().uint32(0));
  ASSERT_EQ(0, message->payload().uint32(1));
  ui_connector_.ClearMessages();

  // Set candidate list visibility
  message = NewMessageForTest(
      MSG_SET_CANDIDATE_LIST_VISIBILITY, proto::Message::NEED_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->add_uint32(1);
  message->mutable_payload()->add_boolean(true);

  // We should receive a true reply.
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(1U, ime_connector_.messages_.size());
  message = ime_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_CANDIDATE_LIST_VISIBILITY,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY], ime_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->payload().boolean(0));
  ime_connector_.ClearMessages();

  // UI should receive MSG_CANDIDATE_LIST_VISIBILITY_CHANGED.
  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY], ui_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().uint32_size());
  ASSERT_EQ(1, message->payload().uint32(0));
  ASSERT_EQ(1, message->payload().boolean_size());
  ASSERT_TRUE(message->payload().boolean(0));
  ui_connector_.ClearMessages();

  // Set visibility of a nonexistent candidate list.
  message = NewMessageForTest(
      MSG_SET_CANDIDATE_LIST_VISIBILITY, proto::Message::NEED_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->add_uint32(100);
  message->mutable_payload()->add_boolean(true);

  // We should receive a false reply.
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(1U, ime_connector_.messages_.size());
  message = ime_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_CANDIDATE_LIST_VISIBILITY,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY], ime_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_FALSE(message->payload().boolean(0));
  ime_connector_.ClearMessages();

  ASSERT_EQ(0U, ui_connector_.messages_.size());

  // Set visibility of a sub candidate list.
  message = NewMessageForTest(
      MSG_SET_CANDIDATE_LIST_VISIBILITY, proto::Message::NEED_REPLY,
      ime_id_, kComponentDefault, icid_);
  message->mutable_payload()->add_uint32(15);
  message->mutable_payload()->add_boolean(true);

  // We should receive a true reply.
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));
  ASSERT_EQ(1U, ime_connector_.messages_.size());
  message = ime_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_SET_CANDIDATE_LIST_VISIBILITY,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY], ime_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_TRUE(message->payload().boolean(0));
  ime_connector_.ClearMessages();

  // UI should receive MSG_CANDIDATE_LIST_VISIBILITY_CHANGED.
  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST_VISIBILITY], ui_id_, icid_,
      proto::Message::NO_REPLY, true));
  ASSERT_EQ(1, message->payload().uint32_size());
  ASSERT_EQ(15, message->payload().uint32(0));
  ASSERT_EQ(1, message->payload().boolean_size());
  ASSERT_TRUE(message->payload().boolean(0));
  ui_connector_.ClearMessages();

  // Query candidate list
  message = NewMessageForTest(
      MSG_QUERY_CANDIDATE_LIST, proto::Message::NEED_REPLY,
      ui_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ui_connector_, message));

  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_QUERY_CANDIDATE_LIST,
      builtin_consumers_[MSG_QUERY_CANDIDATE_LIST], ui_id_, icid_,
      proto::Message::IS_REPLY, true));
  ASSERT_EQ(1, message->payload().candidate_list().id());
  ASSERT_EQ(5, message->payload().candidate_list().candidate_size());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().owner());
  ASSERT_EQ(12, message->payload().candidate_list().candidate(2).
            sub_candidates().id());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().candidate(2).
            sub_candidates().owner());
  ASSERT_EQ(15, message->payload().candidate_list().candidate(2).
            sub_candidates().candidate(0).sub_candidates().id());
  ASSERT_EQ(ime_id_, message->payload().candidate_list().candidate(2).
            sub_candidates().candidate(0).sub_candidates().owner());
  ASSERT_TRUE(message->payload().candidate_list().has_selected_candidate());
  ASSERT_EQ(2, message->payload().candidate_list().selected_candidate());
  ASSERT_TRUE(message->payload().candidate_list().has_visible());
  ASSERT_TRUE(message->payload().candidate_list().visible());
  ASSERT_TRUE(message->payload().candidate_list().candidate(2).
              sub_candidates().has_selected_candidate());
  ASSERT_EQ(0, message->payload().candidate_list().candidate(2).
            sub_candidates().selected_candidate());
  ASSERT_FALSE(message->payload().candidate_list().candidate(2).
              sub_candidates().has_visible());
  ASSERT_TRUE(message->payload().candidate_list().candidate(2).
              sub_candidates().candidate(0).sub_candidates().has_visible());
  ASSERT_TRUE(message->payload().candidate_list().candidate(2).
              sub_candidates().candidate(0).sub_candidates().visible());
  ui_connector_.ClearMessages();

  // Delete candidate list
  message = NewMessageForTest(
      MSG_SET_CANDIDATE_LIST, proto::Message::NO_REPLY,
      ime_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));

  // The candidate ui should receive this message, but the application
  // shouldn't.
  ASSERT_EQ(0U, app_connector_.messages_.size());
  ASSERT_EQ(1U, ui_connector_.messages_.size());
  message = ui_connector_.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CANDIDATE_LIST_CHANGED,
      builtin_consumers_[MSG_SET_CANDIDATE_LIST], ui_id_, icid_,
      proto::Message::NO_REPLY, false));
  ui_connector_.ClearMessages();

  // Delete candidate list again, nothing should happen.
  message = NewMessageForTest(
      MSG_SET_CANDIDATE_LIST, proto::Message::NO_REPLY,
      ime_id_, kComponentDefault, icid_);
  ASSERT_TRUE(hub_->Dispatch(&ime_connector_, message));

  ASSERT_EQ(0U, app_connector_.messages_.size());
  ASSERT_EQ(0U, ui_connector_.messages_.size());
}

}  // namespace
