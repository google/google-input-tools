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

#include "ipc/mock_connector.h"

#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace ipc {

MockConnector::MockConnector()
    : hub_(NULL),
      send_result_(true) {
}

MockConnector::~MockConnector() {
  // ClearMessages() is called in Detach().
  Detach();
}

bool MockConnector::Send(proto::Message* message) {
  messages_.push_back(message);
  return send_result_;
}

void MockConnector::AddComponent(const proto::ComponentInfo& info) {
  uint32 id = kComponentDefault;
  if (hub_) {
    // Register the component to the Hub.
    proto::Message* message = NewMessageForTest(
        MSG_REGISTER_COMPONENT, proto::Message::NEED_REPLY,
        kComponentDefault, kComponentDefault, kInputContextNone);
    const uint32 serial = message->serial();

    // Real code should construct the ComponentInfo object in-place.
    message->mutable_payload()->add_component_info()->CopyFrom(info);

    send_result_ = true;
    ClearMessages();
    ASSERT_TRUE(hub_->Dispatch(this, message));

    // We should receive a reply message with the allocated component id.
    // After that, we may receive a MSG_COMPONENT_ACTIVATED message which
    // is generated when the component got activated for the default input
    // context.
    ASSERT_GE(messages_.size(), 1U);

    message = messages_[0];
    ASSERT_EQ(info.string_id(),
              message->payload().component_info(0).string_id());
    id = message->payload().component_info(0).id();
    ASSERT_NO_FATAL_FAILURE(CheckMessage(
        message, MSG_REGISTER_COMPONENT, kComponentDefault,
        kComponentDefault, kInputContextNone, proto::Message::IS_REPLY,
        true));
    ASSERT_EQ(serial, message->serial());
    ASSERT_NE(kComponentDefault, id);

    components_index_[id] = components_.size();
    ClearMessages();
  }

  components_.push_back(info);
  components_[components_.size() - 1].set_id(id);
}

void MockConnector::RemoveComponentByIndex(size_t index) {
  ASSERT_LT(index, components_.size());

  uint32 id = components_[index].id();
  size_t last_index = components_.size() - 1;
  if (last_index > 0 && index != last_index) {
    uint32 last_id = components_[last_index].id();
    components_[index].Swap(&components_[last_index]);

    if (last_id != kComponentDefault)
      components_index_[last_id] = index;
  }
  components_.pop_back();

  if (id != kComponentDefault)
    components_index_.erase(id);

  if (hub_) {
    ASSERT_NE(kComponentDefault, id);

    // Deregister the component from the Hub.
    proto::Message* message = NewMessageForTest(
        MSG_DEREGISTER_COMPONENT, proto::Message::NEED_REPLY,
        kComponentDefault, kComponentDefault, kInputContextNone);
    const uint32 serial = message->serial();
    message->mutable_payload()->add_uint32(id);

    send_result_ = true;
    ClearMessages();
    ASSERT_TRUE(hub_->Dispatch(this, message));

    // Before the reply message, we may receive some
    // MSG_COMPONENT_DEACTIVATED messages which are generated when
    // components got deactivated from the default input context.
    ASSERT_GE(messages_.size(), 1U);

    message = messages_[messages_.size() - 1];
    ASSERT_NO_FATAL_FAILURE(CheckMessage(
        message, MSG_DEREGISTER_COMPONENT, kComponentDefault,
        kComponentDefault, kInputContextNone, proto::Message::IS_REPLY,
        true));
    ASSERT_EQ(serial, message->serial());
    ASSERT_TRUE(message->payload().boolean_size());
    ASSERT_TRUE(message->payload().boolean(0));

    ClearMessages();
  }
}

void MockConnector::RemoveComponent(uint32 id) {
  if (id != kComponentDefault)
    RemoveComponentByIndex(components_index_[id]);
}

void MockConnector::Attach(Hub* hub) {
  Detach();
  hub_ = hub;
  hub_->Attach(this);

  if (!components_.size())
    return;

  proto::Message* message = NewMessageForTest(
      MSG_REGISTER_COMPONENT, proto::Message::NEED_REPLY,
      kComponentDefault, kComponentDefault, kInputContextNone);
  const uint32 serial = message->serial();

  for (size_t i = 0; i < components_.size(); ++i) {
    proto::ComponentInfo* info =
        message->mutable_payload()->add_component_info();
    info->CopyFrom(components_[i]);
  }

  send_result_ = true;
  ClearMessages();
  ASSERT_TRUE(hub_->Dispatch(this, message));

  // We should receive a reply message with the allocated component id.
  // Before the reply message, we may receive some MSG_COMPONENT_ACTIVATED
  // messages which are generated when components got activated for the
  // default input context.
  ASSERT_GE(messages_.size(), 1U);

  message = NULL;
  for (size_t i = 0; i < messages_.size(); ++i) {
    if (messages_[i]->type() == MSG_REGISTER_COMPONENT) {
      message = messages_[i];
      break;
    }
  }
  ASSERT_TRUE(message);
  ASSERT_EQ(proto::Message::IS_REPLY, message->reply_mode());
  ASSERT_EQ(kComponentDefault, message->source());
  ASSERT_EQ(serial, message->serial());

  ASSERT_EQ(kComponentDefault, message->target());
  ASSERT_TRUE(message->has_payload());
  const proto::MessagePayload& payload = message->payload();
  ASSERT_EQ(components_.size(),
            static_cast<size_t>(payload.component_info_size()));

  for (int i = 0; i < payload.component_info_size(); ++i) {
    ASSERT_EQ(components_[i].string_id(),
              payload.component_info(i).string_id());
    uint32 id = payload.component_info(i).id();
    ASSERT_NE(kComponentDefault, id);
    components_[i].set_id(id);
    components_index_[id] = static_cast<size_t>(i);
  }
  ClearMessages();
}

void MockConnector::Detach() {
  ClearMessages();
  if (hub_) {
    hub_->Detach(this);
    hub_ = NULL;

    // We should not receive any message when detaching.
    ASSERT_EQ(0, messages_.size());
  }

  for (size_t i = 0; i < components_.size(); ++i)
    components_[i].set_id(kComponentDefault);

  components_index_.clear();
}

void MockConnector::ClearMessages() {
  for (size_t i = 0; i < messages_.size(); ++i)
    delete messages_[i];
  messages_.clear();
}

}  // namespace ipc
