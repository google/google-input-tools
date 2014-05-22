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

#include "ipc/settings_client.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"
#include "ipc/mock_component.h"
#include "ipc/mock_component_host.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace {

using namespace ipc;

static bool ProtoMessageEqual(const ::google::protobuf::MessageLite& a,
                              const ::google::protobuf::MessageLite& b) {
  return a.SerializePartialAsString() == b.SerializePartialAsString();
}

// Messages an input method can produce.
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
};

// Messages an input method can consume.
const uint32 kIMEConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
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
};

// A mock input method component with a SettingsClient sub component.
class IMEComponent : public MockComponent,
                     public SettingsClient::Delegate {
 public:
  explicit IMEComponent() : MockComponent("IME1") {
    settings_client_ = new SettingsClient(this, this);
  }

  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE {
    SetupComponentInfo("", "Mock Input Method", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       info);
    GetSubComponentsInfo(info);
    MockComponent::GetInfo(info);
  }

  // Overridden from Component.
  virtual void Handle(proto::Message* message) OVERRIDE {
    if (!HandleMessageBySubComponents(message))
      delete message;
  }

  SettingsClient* GetSettingsClient() const {
    return settings_client_;
  }

  // Overridden from SettingsClient::Delegate.
  virtual void OnValueChanged(const std::string& key,
                              const proto::VariableArray& array) OVERRIDE {
    DCHECK(!key.empty());
    changed_key_ = key;
  }

  std::string changed_key() {
    return changed_key_;
  }

 private:
  std::string changed_key_;
  SettingsClient* settings_client_;
  DISALLOW_COPY_AND_ASSIGN(IMEComponent);
};

class SettingsClientTest : public ::testing::Test {
 protected:
  SettingsClientTest() {
    settings_client_ = ime_.GetSettingsClient();
    keys_[0] = "key1";
    keys_[1] = "key2";
    keys_[2] = "key3";
    keys_[3] = "key4";
    values_[0].set_type(proto::Variable::STRING);
    values_[0].set_string(keys_[0]);
    values_[1].set_type(proto::Variable::BOOLEAN);
    values_[1].set_boolean(true);
    values_[2].set_type(proto::Variable::NONE);
    values_[3].set_type(proto::Variable::INTEGER);
    values_[3].set_integer(1);
  }

  virtual ~SettingsClientTest() {}

  // Sets up an ipc communication environment. An IME mock component accesses
  // the settings through the ipc layer.
  virtual void SetUp() {
    host_.AddComponent(&ime_);
  }

  virtual void TearDown() {
    host_.RemoveComponent(&ime_);
  }

  proto::Message* NewMessage(uint32 type, bool need_reply) {
    return ipc::NewMessage(type,
                           ime_.id(),
                           kComponentDefault,
                           kInputContextNone,
                           false);
  }

  proto::Message* NewReplyMessage(uint32 type) {
    proto::Message* message = NewMessage(type, false);
    message->set_reply_mode(proto::Message::IS_REPLY);
    return message;
  }

  proto::Message* NewErrorReplyMessage(uint32 type) {
    proto::Message* message = NewReplyMessage(type);
    message->mutable_payload()->mutable_error()->set_code(
        proto::Error::INVALID_PAYLOAD);
    message->mutable_payload()->mutable_error()->set_message("INVALID_PAYLOAD");
    return message;
  }

  void CheckMessage(const scoped_ptr<proto::Message>& mptr, uint32 type,
                    proto::Message::ReplyMode reply_mode) {
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(type, mptr->type());
    ASSERT_EQ(reply_mode, mptr->reply_mode());
    ASSERT_EQ(ime_.id(), mptr->source());
    ASSERT_EQ(kComponentDefault, mptr->target());
    ASSERT_EQ(kInputContextNone, mptr->icid());
  }

  SettingsClient* settings_client_;
  MockComponentHost host_;
  IMEComponent ime_;
  std::string keys_[4];
  proto::Variable values_[4];
};

typedef SettingsClient::KeyList KeyList;
typedef SettingsClient::ResultList ResultList;

TEST_F(SettingsClientTest, HandleTest) {
  scoped_ptr<proto::Message> mptr;
  std::string key = "key";
  proto::Variable value;
  value.set_type(proto::Variable::STRING);
  value.set_string(key);
  mptr.reset(ipc::NewMessage(MSG_SETTINGS_CHANGED,
                             kComponentDefault,
                             ime_.id(),
                             kInputContextNone,
                             true));
  mptr->mutable_payload()->add_string(key);
  mptr->mutable_payload()->add_variable()->CopyFrom(value);
  host_.HandleMessage(mptr.release());
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_CHANGED,
                                       proto::Message::IS_REPLY));
  EXPECT_EQ(mptr->payload().boolean_size(), 1);
  EXPECT_EQ(mptr->payload().boolean(0), true);
  // Checks if OnValueChanged was called.
  EXPECT_EQ(ime_.changed_key(), key);
}

TEST_F(SettingsClientTest, SetValuesTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  KeyList key_list;
  proto::VariableArray array;
  ResultList results;

  // SetValues with successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  for (int i = 0; i < 3; ++i) {
    key_list.push_back(keys_[i]);
    array.add_variable()->CopyFrom(values_[i]);
    reply_mptr->mutable_payload()->add_boolean(true);
  }
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->SetValues(key_list, array, &results));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_SET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 3);
  EXPECT_EQ(mptr->payload().variable_size(), 3);
  EXPECT_EQ(results.size(), 3);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(keys_[i], mptr->payload().string(i));
    EXPECT_TRUE(ProtoMessageEqual(values_[i], mptr->payload().variable(i)));
    EXPECT_TRUE(results[i]);
  }
  // SetValues with error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_SET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetValues(key_list, array, &results));
  host_.PopOutgoingMessage();

  // SetValues with key_list.size() != array.size(), no message sent.
  key_list.push_back(keys_[3]);
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_SET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetValues(key_list, array, &results));

  // SetValues with empty key list, return false without reply.
  key_list.clear();
  ASSERT_FALSE(settings_client_->SetValues(key_list, array, &results));
}

TEST_F(SettingsClientTest, GetValuesTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  KeyList key_list;
  proto::VariableArray array;

  // GetValues with successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_GET_VALUES));
  for (int i = 0; i < 3; ++i) {
    key_list.push_back(keys_[i]);
    reply_mptr->mutable_payload()->add_string(keys_[i]);
    reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[i]);
  }
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->GetValues(key_list, &array));
  EXPECT_EQ(3, array.variable_size());
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_GET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 3);
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(keys_[i], mptr->payload().string(i));
    EXPECT_TRUE(ProtoMessageEqual(values_[i], array.variable(i)));
  }

  // GetValues with error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_GET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->GetValues(key_list, &array));
  host_.PopOutgoingMessage();

  // GetValues with empty key list. False is returned without message sent.
  key_list.clear();
  ASSERT_FALSE(settings_client_->GetValues(key_list, &array));
}

TEST_F(SettingsClientTest, SetValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;

  // SetValue with successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->SetValue(keys_[0], values_[0]));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_SET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().variable_size(), 1);
  EXPECT_EQ(keys_[0], mptr->payload().string(0));
  EXPECT_TRUE(ProtoMessageEqual(values_[0], mptr->payload().variable(0)));

  // SetValue with empty key. False is returned.
  std::string empty_key;
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(false);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetValue(empty_key, values_[0]));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, GetValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  proto::Variable reply_value;

  // GetValue with successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_GET_VALUES));
  reply_mptr->mutable_payload()->add_string(keys_[0]);
  reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[0]);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->GetValue(keys_[0], &reply_value));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_GET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(keys_[0], mptr->payload().string(0));
  EXPECT_TRUE(ProtoMessageEqual(values_[0], reply_value));
}

TEST_F(SettingsClientTest, SetArrayValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  proto::VariableArray array;

  // SetArrayValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_ARRAY_VALUE));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  array.clear_variable();
  array.add_variable()->CopyFrom(values_[0]);
  array.add_variable()->CopyFrom(values_[2]);
  ASSERT_TRUE(settings_client_->SetArrayValue(keys_[0], array));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_SET_ARRAY_VALUE,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().variable_size(), 2);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_TRUE(ProtoMessageEqual(mptr->payload().variable(0), values_[0]));
  EXPECT_TRUE(ProtoMessageEqual(mptr->payload().variable(1), values_[2]));

  // SetArrayValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_SET_ARRAY_VALUE));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetArrayValue(keys_[0], array));
  host_.PopOutgoingMessage();

  // SetArrayValue with an empty key. False is replyed.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(false);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetArrayValue(keys_[0], array));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, GetArrayValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  proto::VariableArray array;

  // GetArrayValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_GET_ARRAY_VALUE));
  reply_mptr->mutable_payload()->add_string(keys_[0]);
  reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[0]);
  reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[1]);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->GetArrayValue(keys_[0], &array));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_GET_ARRAY_VALUE,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(array.variable_size(), 2);
  EXPECT_TRUE(ProtoMessageEqual(array.variable(0), values_[0]));
  EXPECT_TRUE(ProtoMessageEqual(array.variable(1), values_[1]));

  // GetArrayValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_GET_ARRAY_VALUE));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->GetArrayValue(keys_[0], &array));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, AddChangeObserverTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  KeyList key_list;

  // AddChangeObserverForKeys with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_ADD_CHANGE_OBSERVER));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  key_list.clear();
  key_list.push_back(keys_[0]);
  key_list.push_back(keys_[1]);
  ASSERT_TRUE(settings_client_->AddChangeObserverForKeys(key_list));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_ADD_CHANGE_OBSERVER,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 2);
  EXPECT_EQ(keys_[0], mptr->payload().string(0));
  EXPECT_EQ(keys_[1], mptr->payload().string(1));

  // AddChangeObserverForKeys with an empty key_list. False is replyed without
  // message sent.
  key_list.clear();
  ASSERT_FALSE(settings_client_->AddChangeObserverForKeys(key_list));

  // AddChangeObserver with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_ADD_CHANGE_OBSERVER));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->AddChangeObserver(keys_[0]));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_ADD_CHANGE_OBSERVER,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(keys_[0], mptr->payload().string(0));
}

TEST_F(SettingsClientTest, RemoveChangeObserverTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  KeyList key_list;

  // RemoveChangeObserverForKeys with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_REMOVE_CHANGE_OBSERVER));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  key_list.clear();
  key_list.push_back(keys_[0]);
  key_list.push_back(keys_[1]);
  ASSERT_TRUE(settings_client_->RemoveChangeObserverForKeys(key_list));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(
      CheckMessage(mptr, MSG_SETTINGS_REMOVE_CHANGE_OBSERVER,
                   proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 2);
  EXPECT_EQ(keys_[0], mptr->payload().string(0));
  EXPECT_EQ(keys_[1], mptr->payload().string(1));

  // RemoveChangeObserverForKeys with an empty key_list. False is returned
  // without message sent.
  key_list.clear();
  ASSERT_FALSE(settings_client_->RemoveChangeObserverForKeys(key_list));

  // RemoveChangeObserver with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_REMOVE_CHANGE_OBSERVER));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->RemoveChangeObserver(keys_[0]));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(
      CheckMessage(mptr, MSG_SETTINGS_REMOVE_CHANGE_OBSERVER,
                   proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(keys_[0], mptr->payload().string(0));
}

TEST_F(SettingsClientTest, SetIntegerValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;

  // SetIntegerValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->SetIntegerValue(keys_[0],
                                                values_[3].integer()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_SET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().variable_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(mptr->payload().variable(0).type(), proto::Variable::INTEGER);
  EXPECT_EQ(mptr->payload().variable(0).integer(), values_[3].integer());

  // SetIntegerValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_SET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetIntegerValue(keys_[0],
                                                 values_[3].integer()));
  host_.PopOutgoingMessage();

  // SetIntegerValue with a false reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(false);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetIntegerValue(keys_[0],
                                                 values_[3].integer()));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, GetIntegerValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  int64 int_value;

  // GetIntegerValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_GET_VALUES));
  reply_mptr->mutable_payload()->add_string(keys_[0]);
  reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[3]);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->GetIntegerValue(keys_[0], &int_value));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_GET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(int_value, values_[3].integer());

  // GetIntegerValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_GET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->GetIntegerValue(keys_[0], &int_value));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, SetStringValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;

  // SetStringValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->SetStringValue(keys_[0], values_[0].string()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_SET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().variable_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(mptr->payload().variable(0).type(), proto::Variable::STRING);
  EXPECT_EQ(mptr->payload().variable(0).string(), values_[0].string());

  // SetStringValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_SET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetStringValue(keys_[0], values_[0].string()));
  host_.PopOutgoingMessage();

  // SetStringValue with a false reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(false);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetStringValue(keys_[0], values_[0].string()));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, GetStringValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  string string_value;

  // GetStringValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_GET_VALUES));
  reply_mptr->mutable_payload()->add_string(keys_[0]);
  reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[0]);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->GetStringValue(keys_[0], &string_value));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_GET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(string_value, values_[0].string());

  // GetStringValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_GET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->GetStringValue(keys_[0], &string_value));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, SetBooleanValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  // SetBooleanValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(true);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->SetBooleanValue(keys_[0],
                                                values_[1].boolean()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_SET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().variable_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(mptr->payload().variable(0).type(), proto::Variable::BOOLEAN);
  EXPECT_EQ(mptr->payload().variable(0).boolean(), values_[1].boolean());

  // SetBooleanValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_SET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetBooleanValue(keys_[0],
                                                 values_[1].boolean()));
  host_.PopOutgoingMessage();

  // SetBooleanValue with a false reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_SET_VALUES));
  reply_mptr->mutable_payload()->add_boolean(false);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->SetBooleanValue(keys_[0],
                                                 values_[1].boolean()));
  host_.PopOutgoingMessage();
}

TEST_F(SettingsClientTest, GetBooleanValueTest) {
  scoped_ptr<proto::Message> mptr;
  scoped_ptr<proto::Message> reply_mptr;
  bool boolean_value;

  // GetBooleanValue with a successful reply.
  reply_mptr.reset(NewReplyMessage(MSG_SETTINGS_GET_VALUES));
  reply_mptr->mutable_payload()->add_string(keys_[0]);
  reply_mptr->mutable_payload()->add_variable()->CopyFrom(values_[1]);
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_TRUE(settings_client_->GetBooleanValue(keys_[0], &boolean_value));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_NO_FATAL_FAILURE(CheckMessage(mptr, MSG_SETTINGS_GET_VALUES,
                                       proto::Message::NEED_REPLY));
  EXPECT_EQ(mptr->payload().string_size(), 1);
  EXPECT_EQ(mptr->payload().string(0), keys_[0]);
  EXPECT_EQ(boolean_value, values_[1].boolean());

  // GetIntegerValue with an error reply.
  reply_mptr.reset(NewErrorReplyMessage(MSG_SETTINGS_GET_VALUES));
  host_.SetNextReplyMessage(reply_mptr.release());
  ASSERT_FALSE(settings_client_->GetBooleanValue(keys_[0], &boolean_value));
  host_.PopOutgoingMessage();
}

}  // namespace
