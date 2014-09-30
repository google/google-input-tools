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
#include "ipc/component_base.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"

namespace {

// Messages can be consumed by this settings_client sub_component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_SETTINGS_CHANGED,
};
const size_t kConsumeMessageSize = arraysize(kConsumeMessages);

const uint32 kProduceMessages[] = {
  ipc::MSG_SETTINGS_SET_VALUES,
  ipc::MSG_SETTINGS_GET_VALUES,
  ipc::MSG_SETTINGS_SET_ARRAY_VALUE,
  ipc::MSG_SETTINGS_GET_ARRAY_VALUE,
  ipc::MSG_SETTINGS_ADD_CHANGE_OBSERVER,
  ipc::MSG_SETTINGS_REMOVE_CHANGE_OBSERVER,
};
const size_t kProduceMessageSize = arraysize(kProduceMessages);

}  // namespace

namespace ipc {

SettingsClient::SettingsClient(ComponentBase* owner, Delegate* delegate)
    : SubComponentBase(owner),
      delegate_(delegate) {
}

SettingsClient::~SettingsClient() {
}

void SettingsClient::GetInfo(proto::ComponentInfo* info) {
  for (size_t i = 0; i < kProduceMessageSize; ++i)
    info->add_produce_message(kProduceMessages[i]);
  for (size_t i = 0; i < kConsumeMessageSize; ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

bool SettingsClient::Handle(proto::Message* message) {
  if (!delegate_)
    return false;
  if (message->type() == MSG_SETTINGS_CHANGED) {
    scoped_ptr<proto::Message> received_message(message);
    DCHECK_EQ(received_message->payload().string_size(), 1);
    const std::string& key = received_message->payload().string(0);
    proto::VariableArray array;
    array.mutable_variable()->Swap(
        received_message->mutable_payload()->mutable_variable());
    delegate_->OnValueChanged(key, array);
    if (MessageNeedReply(received_message.get())) {
      ConvertToBooleanReplyMessage(received_message.get(), true);
      owner_->Send(received_message.release(), NULL);
    }
    return true;
  }
  return false;
}

void SettingsClient::OnRegistered() {
}

void SettingsClient::OnDeregistered() {
}

bool SettingsClient::SetValues(const KeyList& keys,
                               const proto::VariableArray& values,
                               ResultList* results) {
  size_t key_size = keys.size();
  size_t value_size = values.variable_size();
  if (!key_size || !value_size || key_size != value_size) {
    DLOG(WARNING) << "empty key value list or key vale list size not match, "
                  << "source: " << owner_->id();
    return false;
  }
  scoped_ptr<proto::Message> mptr(NewMessage(MSG_SETTINGS_SET_VALUES));
  for (size_t i = 0; i < key_size; ++i)
    mptr->mutable_payload()->add_string(keys[i]);
  mptr->mutable_payload()->mutable_variable()->CopyFrom(values.variable());
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  if (!reply.get())
    return false;
  DCHECK_EQ(key_size, reply->payload().boolean_size());
  if (results) {
    results->clear();
    for (size_t i = 0; i < reply->payload().boolean_size(); ++i)
      results->push_back(reply->payload().boolean(i));
  }
  return true;
}

bool SettingsClient::GetValues(const KeyList& keys,
                               proto::VariableArray* values) {
  DCHECK(values);
  size_t key_size = keys.size();
  if (!key_size) {
    DLOG(WARNING) << "empty keyvalue list, source: " << owner_->id();
    return false;
  }
  scoped_ptr<proto::Message> mptr(NewMessage(MSG_SETTINGS_GET_VALUES));
  for (size_t i = 0; i < key_size; ++i)
    mptr->mutable_payload()->add_string(keys[i]);
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  if (!reply.get())
    return false;
  DCHECK_EQ(key_size, reply->payload().string_size());
  DCHECK_EQ(key_size, reply->payload().variable_size());
#ifdef DEBUG
  for (size_t i = 0; i < key_size; ++i)
    DCHECK_EQ(reply->payload().string(i), keys[i]);
#endif
  values->mutable_variable()->Swap(
      reply->mutable_payload()->mutable_variable());
  return true;
}

bool SettingsClient::SetValue(const std::string& key,
                              const proto::Variable& value) {
  scoped_ptr<proto::Message> mptr(NewMessage(MSG_SETTINGS_SET_VALUES));
  mptr->mutable_payload()->add_string(key);
  mptr->mutable_payload()->add_variable()->CopyFrom(value);
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  if (!reply.get())
    return false;
  DCHECK_EQ(reply->payload().boolean_size(), 1);
  return reply->payload().boolean(0);
}

bool SettingsClient::GetValue(const std::string& key, proto::Variable* value) {
  DCHECK(value);
  KeyList keys;
  proto::VariableArray values;
  keys.push_back(key);
  if (!GetValues(keys, &values))
    return false;
  DCHECK_EQ(values.variable_size(), 1);
  value->Swap(values.mutable_variable(0));
  return true;
}

bool SettingsClient::SetArrayValue(const std::string& key,
                                   const proto::VariableArray& array) {
  scoped_ptr<proto::Message> mptr(NewMessage(MSG_SETTINGS_SET_ARRAY_VALUE));
  mptr->mutable_payload()->add_string(key);
  mptr->mutable_payload()->mutable_variable()->CopyFrom(array.variable());
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  if (!reply.get())
    return false;
  DCHECK_EQ(reply->payload().boolean_size(), 1);
  return reply->payload().boolean(0);
}

bool SettingsClient::GetArrayValue(const std::string& key,
                                   proto::VariableArray* array) {
  DCHECK(array);
  scoped_ptr<proto::Message> mptr(NewMessage(MSG_SETTINGS_GET_ARRAY_VALUE));
  mptr->mutable_payload()->add_string(key);
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  if (!reply.get())
    return false;
  DCHECK_EQ(reply->payload().string_size(), 1);
  DCHECK_EQ(reply->payload().string(0), key);
  array->clear_variable();
  array->mutable_variable()->Swap(
      reply->mutable_payload()->mutable_variable());
  return true;
}

bool SettingsClient::AddChangeObserverForKeys(const KeyList& key_list) {
  if (!key_list.size()) {
    DLOG(WARNING) << "empty key list, source: " << owner_->id();
    return false;
  }
  scoped_ptr<proto::Message> mptr(NewMessage(MSG_SETTINGS_ADD_CHANGE_OBSERVER));
  KeyList::const_iterator iter;
  for (iter = key_list.begin(); iter != key_list.end(); ++iter)
    mptr->mutable_payload()->add_string(*iter);
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  return reply.get() != NULL;
}

bool SettingsClient::AddChangeObserver(const std::string& key) {
  KeyList key_list;
  key_list.push_back(key);
  return AddChangeObserverForKeys(key_list);
}

bool SettingsClient::RemoveChangeObserverForKeys(const KeyList& key_list) {
  if (!key_list.size()) {
    DLOG(WARNING) << "empty key list, source: " << owner_->id();
    return false;
  }
  scoped_ptr<proto::Message> mptr(NewMessage(
      MSG_SETTINGS_REMOVE_CHANGE_OBSERVER));
  KeyList::const_iterator iter;
  for (iter = key_list.begin(); iter != key_list.end(); ++iter)
    mptr->mutable_payload()->add_string(*iter);
  scoped_ptr<proto::Message> reply(SendWithReply(mptr.release()));
  return reply.get() != NULL;
}

bool SettingsClient::RemoveChangeObserver(const std::string& key) {
  KeyList key_list;
  key_list.push_back(key);
  return RemoveChangeObserverForKeys(key_list);
}

bool SettingsClient::SetIntegerValue(const std::string& key, int64 value) {
  proto::Variable variable;
  variable.set_type(proto::Variable::INTEGER);
  variable.set_integer(value);
  return SetValue(key, variable);
}

bool SettingsClient::GetIntegerValue(const std::string& key, int64* value) {
  DCHECK(value);
  proto::Variable variable;
  if (!GetValue(key, &variable))
    return false;
  if (variable.type() != proto::Variable::INTEGER)
    return false;
  *value = variable.integer();
  return true;
}

bool SettingsClient::SetStringValue(const std::string& key,
                                    const std::string& value) {
  proto::Variable variable;
  variable.set_type(proto::Variable::STRING);
  variable.set_string(value);
  return SetValue(key, variable);
}

bool SettingsClient::GetStringValue(const std::string& key,
                                    std::string* value) {
  DCHECK(value);
  proto::Variable variable;
  if (!GetValue(key, &variable))
    return false;
  if (variable.type() != proto::Variable::STRING)
    return false;
  *value = variable.string();
  return true;
}

bool SettingsClient::SetBooleanValue(const std::string& key, bool value) {
  proto::Variable variable;
  variable.set_type(proto::Variable::BOOLEAN);
  variable.set_boolean(value);
  return SetValue(key, variable);
}

bool SettingsClient::GetBooleanValue(const std::string&key, bool* value) {
  DCHECK(value);
  proto::Variable variable;
  if (!GetValue(key, &variable))
    return false;
  if (variable.type() != proto::Variable::BOOLEAN)
    return false;
  *value = variable.boolean();
  return true;
}

proto::Message* SettingsClient::SendWithReply(proto::Message* send_message) {
  proto::Message* reply = NULL;
  if (!owner_->SendWithReplyNonRecursive(send_message, -1, &reply)) {
    DLOG(ERROR) << "SendWithReply failed with type = " << send_message->type()
                << ", source: " << owner_->id();
    return NULL;
  }
  DCHECK(reply);
  // Hub will return error message if external settings store does not exist.
  if (MessageIsErrorReply(reply)) {
    DLOG(ERROR) << "Received error reply: "
                << GetMessageErrorInfo(reply, NULL).c_str()
                << ", source: " << owner_->id();
    delete reply;
    reply = NULL;
  }
  return reply;
}

proto::Message* SettingsClient::NewMessage(uint32 type) {
  return ipc::NewMessage(type, owner_->id(), kComponentDefault,
                         kInputContextNone, true);
}

}  // namespace ipc
