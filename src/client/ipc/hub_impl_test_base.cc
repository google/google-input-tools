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

#include "ipc/hub_impl_test_base.h"

#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/hub_impl.h"
#include "ipc/message_types.h"
#include "ipc/mock_connector.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/test_util.h"

namespace ipc {
namespace hub {

namespace {

// Messages which should have built-in consumers.
const uint32 kMessagesWithBuiltInConsumers[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_QUERY_COMPONENT,

  MSG_CREATE_INPUT_CONTEXT,
  MSG_DELETE_INPUT_CONTEXT,
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACH_FROM_INPUT_CONTEXT,

  MSG_QUERY_INPUT_CONTEXT,
  MSG_FOCUS_INPUT_CONTEXT,
  MSG_BLUR_INPUT_CONTEXT,

  MSG_ACTIVATE_COMPONENT,
  MSG_ASSIGN_ACTIVE_CONSUMER,
  MSG_RESIGN_ACTIVE_CONSUMER,
  MSG_QUERY_ACTIVE_CONSUMER,
  MSG_REQUEST_CONSUMER,

  MSG_UPDATE_INPUT_CARET,
  MSG_QUERY_INPUT_CARET,

  MSG_SEND_KEY_EVENT,

  MSG_ADD_HOTKEY_LIST,
  MSG_REMOVE_HOTKEY_LIST,
  MSG_CHECK_HOTKEY_CONFLICT,
  MSG_ACTIVATE_HOTKEY_LIST,
  MSG_DEACTIVATE_HOTKEY_LIST,
  MSG_QUERY_ACTIVE_HOTKEY_LIST,

  MSG_SET_COMMAND_LIST,
  MSG_UPDATE_COMMANDS,
  MSG_QUERY_COMMAND_LIST,

  MSG_SET_COMPOSITION,
  MSG_QUERY_COMPOSITION,

  MSG_SET_CANDIDATE_LIST,
  MSG_SET_SELECTED_CANDIDATE,
  MSG_SET_CANDIDATE_LIST_VISIBILITY,
  MSG_QUERY_CANDIDATE_LIST,

  MSG_SWITCH_TO_INPUT_METHOD,
};

}  // anonymous namespace

HubImplTestBase::HubImplTestBase()
    : hub_(new HubImpl()) {
}

HubImplTestBase::~HubImplTestBase() {
  delete hub_;
}

void HubImplTestBase::SetUp() {
  int size = arraysize(kMessagesWithBuiltInConsumers);
  scoped_array<uint32> consumers(new uint32[size]);
  for (int i = 0; i < size; ++i)
    consumers[i] = kComponentBroadcast;
  ASSERT_NO_FATAL_FAILURE(QueryActiveConsumers(
      kInputContextNone, kMessagesWithBuiltInConsumers,
      consumers.get(), size));
  for (int i = 0; i < size; ++i) {
    ASSERT_NE(kComponentBroadcast, consumers[i])
        << "For Message:" << kMessagesWithBuiltInConsumers[i];
    builtin_consumers_[kMessagesWithBuiltInConsumers[i]] = consumers[i];
  }
}

void HubImplTestBase::TearDown() {
}

bool HubImplTestBase::IsConnectorAttached(Hub::Connector* connector) const {
  return hub_->IsConnectorAttached(connector);
}

Component* HubImplTestBase::GetComponent(uint32 id) const {
  return hub_->GetComponent(id);
}

InputContext* HubImplTestBase::GetInputContext(uint32 icid) const {
  return hub_->GetInputContext(icid);
}

uint32 HubImplTestBase::GetFocusedInputContextID() const {
  return hub_->focused_input_context_;
}

void HubImplTestBase::VerifyDefaultComponent() const {
  proto::ComponentInfo info;
  info.set_id(kComponentDefault);
  SetupComponentInfo(kHubStringID, kHubName, "",
                     kHubProduceMessages, kHubProduceMessagesSize,
                     kHubConsumeMessages, kHubConsumeMessagesSize,
                     &info);
  VerifyComponent(info);
}

void HubImplTestBase::VerifyDefaultInputContext() const {
  InputContext* ic = GetInputContext(kInputContextNone);
  ASSERT_TRUE(ic);
  ASSERT_EQ(hub_, ic->delegate());
  ASSERT_EQ(kInputContextNone, ic->id());

  Component* component = GetComponent(kComponentDefault);
  ASSERT_TRUE(component);

  ASSERT_EQ(ic->owner(), component);
  ASSERT_EQ(InputContext::ACTIVE_STICKY,
            ic->GetComponentAttachState(component));
}

void HubImplTestBase::VerifyComponent(const proto::ComponentInfo& info) const {
  uint32 id = info.id();

  Component* component = GetComponent(id);
  ASSERT_TRUE(component);

  ASSERT_EQ(info.id(), component->id());
  ASSERT_STREQ(info.string_id().c_str(),
               component->info().string_id().c_str());
  ASSERT_STREQ(info.name().c_str(), component->info().name().c_str());
  ASSERT_STREQ(info.description().c_str(),
               component->info().description().c_str());

  for (int i = 0; i < info.produce_message_size(); ++i)
    ASSERT_TRUE(component->MayProduce(info.produce_message(i)));

  for (int i = 0; i < info.consume_message_size(); ++i)
    ASSERT_TRUE(component->CanConsume(info.consume_message(i)));
}

void HubImplTestBase::CreateInputContext(MockConnector* connector,
                                         uint32 app_id,
                                         uint32* icid) {
  Component* app = GetComponent(app_id);
  ASSERT_TRUE(app);

  proto::Message* message = NewMessageForTest(
      MSG_CREATE_INPUT_CONTEXT, proto::Message::NEED_REPLY,
      app_id, kComponentDefault, kInputContextNone);

  const uint32 serial = message->serial();

  connector->ClearMessages();
  ASSERT_TRUE(hub_->Dispatch(connector, message));

  // Check reply message and retrieve the new icid.
  // If the application consumes MSG_COMPONENT_ACTIVATED, then we will get
  // this extra message before getting the reply message of
  // MSG_CREATE_INPUT_CONTEXT.
  if (app->CanConsume(MSG_COMPONENT_ACTIVATED)) {
    ASSERT_EQ(2U, connector->messages_.size());
    message = connector->messages_[0];
    *icid = message->icid();
    ASSERT_NO_FATAL_FAILURE(CheckMessage(
        message, MSG_COMPONENT_ACTIVATED, kComponentDefault, app_id,
        *icid, proto::Message::NO_REPLY, true));

    // Check activated message types.
    ASSERT_TRUE(message->has_payload());
    const proto::MessagePayload& payload = message->payload();
    const int size = payload.uint32_size();
    for (int i = 0; i < size; ++i)
      ASSERT_TRUE(app->CanConsume(payload.uint32(i)));

    message = connector->messages_[1];

    // These two messages should have the same icid.
    ASSERT_EQ(*icid, message->icid());
  } else {
    ASSERT_EQ(1U, connector->messages_.size());
    message = connector->messages_[0];
    *icid = message->icid();
  }

  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_CREATE_INPUT_CONTEXT,
      builtin_consumers_[MSG_CREATE_INPUT_CONTEXT], app_id,
      *icid, proto::Message::IS_REPLY, true));

  ASSERT_EQ(serial, message->serial());
  ASSERT_EQ(1U, message->payload().boolean_size());
  ASSERT_TRUE(message->payload().boolean(0));
  connector->ClearMessages();

  // Check the new input context
  ASSERT_NE(kInputContextNone, *icid);

  InputContext* ic = GetInputContext(*icid);
  ASSERT_TRUE(ic);
  ASSERT_EQ(app, ic->owner());
  ASSERT_TRUE(app->attached_input_contexts().count(*icid));
}

void HubImplTestBase::QueryActiveConsumers(uint32 icid,
                                           const uint32* messages,
                                           uint32* consumers,
                                           int size) {
  proto::ComponentInfo tester;
  SetupComponentInfo("com.google.HubImplTestBase.tester", "Tester", "",
                     NULL, 0, NULL, 0, &tester);
  tester.add_produce_message(MSG_QUERY_ACTIVE_CONSUMER);

  MockConnector tester_connector;
  tester_connector.AddComponent(tester);
  ASSERT_NO_FATAL_FAILURE(tester_connector.Attach(hub_));
  uint32 tester_id = tester_connector.components_[0].id();

  proto::Message* message = NewMessageForTest(
      MSG_QUERY_ACTIVE_CONSUMER, proto::Message::NEED_REPLY,
      tester_id, kComponentDefault, icid);

  const uint32 serial = message->serial();

  proto::MessagePayload* payload = message->mutable_payload();
  for (int i = 0; i < size; ++i)
    payload->add_uint32(messages[i]);

  ASSERT_TRUE(hub_->Dispatch(&tester_connector, message));
  ASSERT_EQ(1U, tester_connector.messages_.size());

  message = tester_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_QUERY_ACTIVE_CONSUMER, kComponentBroadcast, tester_id,
      icid, proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  ASSERT_TRUE(message->has_payload());
  payload = message->mutable_payload();
  ASSERT_EQ(size, payload->uint32_size());
  for (int i = 0; i < size; ++i)
    consumers[i] = payload->uint32(i);
}

void HubImplTestBase::CheckActiveConsumers(uint32 icid,
                                           const uint32* messages,
                                           int messages_size,
                                           const uint32* consumers,
                                           int consumers_size) {
  ASSERT_EQ(messages_size, consumers_size);
  scoped_array<uint32> result(new uint32[consumers_size]);
  ASSERT_NO_FATAL_FAILURE(
      QueryActiveConsumers(icid, messages, result.get(), messages_size));
  for (int i = 0; i < messages_size; ++i)
    ASSERT_EQ(consumers[i], result[i]);
}

void HubImplTestBase::ActivateComponent(uint32 icid, uint32 component_id) {
  proto::ComponentInfo tester;
  SetupComponentInfo("com.google.HubImplTestBase.tester", "Tester", "",
                     NULL, 0, NULL, 0, &tester);
  tester.add_produce_message(MSG_ACTIVATE_COMPONENT);

  MockConnector tester_connector;
  tester_connector.AddComponent(tester);
  ASSERT_NO_FATAL_FAILURE(tester_connector.Attach(hub_));
  uint32 tester_id = tester_connector.components_[0].id();

  proto::Message* message = NewMessageForTest(
      MSG_ACTIVATE_COMPONENT, proto::Message::NEED_REPLY,
      tester_id, kComponentDefault, icid);
  message->mutable_payload()->add_uint32(component_id);

  const uint32 serial = message->serial();

  ASSERT_TRUE(hub_->Dispatch(&tester_connector, message));

  ASSERT_EQ(1, tester_connector.messages_.size());
  message = tester_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_ACTIVATE_COMPONENT,
      builtin_consumers_[MSG_ACTIVATE_COMPONENT], tester_id,
      icid, proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  ASSERT_EQ(1, message->payload().boolean_size());
  ASSERT_TRUE(message->payload().boolean(0));
}

void HubImplTestBase::CheckAndReplyMsgAttachToInputContext(
    MockConnector* connector,
    uint32 component_id,
    uint32 icid,
    bool active) {
  InputContext* ic = GetInputContext(icid);
  ASSERT_GE(1U, connector->messages_.size());
  proto::Message* message = connector->messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_ATTACH_TO_INPUT_CONTEXT, kComponentDefault, component_id,
      icid, proto::Message::NEED_REPLY, true));
  InputContext::AttachState state =
      active ? InputContext::PENDING_ACTIVE : InputContext::PENDING_PASSIVE;
  ASSERT_EQ(state, ic->GetComponentAttachState(GetComponent(component_id)));
  connector->ClearMessages();

  // Actually attach the component.
  message = NewMessageForTest(
      MSG_ATTACH_TO_INPUT_CONTEXT, proto::Message::IS_REPLY,
      component_id, kComponentDefault, icid);
  message->mutable_payload()->add_boolean(true);
  ASSERT_TRUE(hub_->Dispatch(connector, message));
}

void HubImplTestBase::CheckMsgComponentActivated(
    MockConnector* connector,
    uint32 component_id,
    uint32 icid,
    const uint32* activated_messages,
    size_t activated_messages_size) {
  // The component should receive MSG_COMPONENT_ACTIVATED message.
  ASSERT_EQ(1U, connector->messages_.size());
  proto::Message* message = connector->messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_COMPONENT_ACTIVATED, kComponentDefault, component_id,
      icid, proto::Message::NO_REPLY, true));

  ASSERT_NO_FATAL_FAILURE(CheckUnorderedUint32Payload(
      message, activated_messages, activated_messages_size, false));
  connector->ClearMessages();
}

void HubImplTestBase::FocusOrBlurInputContext(MockConnector* connector,
                                              uint32 component_id,
                                              uint32 icid,
                                              bool focus) {
  uint32 old_focused_icid = GetFocusedInputContextID();

  proto::Message* message = NewMessageForTest(
      focus ? MSG_FOCUS_INPUT_CONTEXT : MSG_BLUR_INPUT_CONTEXT,
      proto::Message::NO_REPLY, component_id, kComponentDefault, icid);
  ASSERT_TRUE(hub_->Dispatch(connector, message));

  if (icid == kInputContextFocused)
    icid = old_focused_icid;

  if (focus) {
    ASSERT_EQ(icid, GetFocusedInputContextID());
    ASSERT_NO_FATAL_FAILURE(CheckInputContext(
        kInputContextFocused, icid, component_id));
  } else {
    ASSERT_EQ(kInputContextNone, GetFocusedInputContextID());
    ASSERT_NO_FATAL_FAILURE(CheckInputContext(
        kInputContextFocused, kInputContextNone, kComponentDefault));
  }
}

void HubImplTestBase::CheckFocusChangeMessages(MockConnector* connector,
                                               uint32 component_id,
                                               uint32 icid_lost_focus,
                                               uint32 icid_got_focus) {
  size_t size = (icid_lost_focus != kInputContextNone &&
                 icid_got_focus != kInputContextNone) ? 2 : 1;
  ASSERT_EQ(size, connector->messages_.size());

  size_t index = 0;
  proto::Message* message;
  if (icid_lost_focus != kInputContextNone) {
    message = connector->messages_[index];
    ASSERT_NO_FATAL_FAILURE(CheckMessage(
        message, MSG_INPUT_CONTEXT_LOST_FOCUS, kComponentDefault,
        component_id, icid_lost_focus, proto::Message::NO_REPLY, false));
    ++index;
  }
  if (icid_got_focus != kInputContextNone) {
    message = connector->messages_[index];
    ASSERT_NO_FATAL_FAILURE(CheckMessage(
        message, MSG_INPUT_CONTEXT_GOT_FOCUS, kComponentDefault,
        component_id, icid_got_focus, proto::Message::NO_REPLY, false));
  }
  connector->ClearMessages();
}

void HubImplTestBase::RequestConsumers(MockConnector* connector,
                                       uint32 component_id,
                                       uint32 icid,
                                       const uint32* messages,
                                       size_t size) {
  proto::Message* message = NewMessageForTest(
      MSG_REQUEST_CONSUMER, proto::Message::NO_REPLY, component_id,
      kComponentDefault, icid);
  for (size_t i = 0; i < size; ++i)
    message->mutable_payload()->add_uint32(messages[i]);

  connector->ClearMessages();
  ASSERT_TRUE(hub_->Dispatch(connector, message));
  ASSERT_EQ(0, connector->messages_.size());
}

void HubImplTestBase::CheckInputContext(
    uint32 query_icid, uint32 icid, uint32 owner) {
  proto::ComponentInfo tester;
  SetupComponentInfo("com.google.HubImplTestBase.tester", "Tester", "",
                     NULL, 0, NULL, 0, &tester);
  tester.add_produce_message(MSG_QUERY_INPUT_CONTEXT);

  MockConnector tester_connector;
  tester_connector.AddComponent(tester);
  ASSERT_NO_FATAL_FAILURE(tester_connector.Attach(hub_));
  uint32 tester_id = tester_connector.components_[0].id();

  proto::Message* message = NewMessageForTest(
      MSG_QUERY_INPUT_CONTEXT, proto::Message::NEED_REPLY,
      tester_id, kComponentDefault, query_icid);

  const uint32 serial = message->serial();

  ASSERT_TRUE(hub_->Dispatch(&tester_connector, message));

  ASSERT_EQ(1U, tester_connector.messages_.size());
  message = tester_connector.messages_[0];
  ASSERT_NO_FATAL_FAILURE(CheckMessage(
      message, MSG_QUERY_INPUT_CONTEXT,
      builtin_consumers_[MSG_QUERY_INPUT_CONTEXT], tester_id,
      icid, proto::Message::IS_REPLY, true));
  ASSERT_EQ(serial, message->serial());
  ASSERT_TRUE(message->payload().has_input_context_info());
  ASSERT_EQ(icid, message->payload().input_context_info().id());
  ASSERT_EQ(owner, message->payload().input_context_info().owner());
}

}  // namespace hub
}  // namespace ipc
