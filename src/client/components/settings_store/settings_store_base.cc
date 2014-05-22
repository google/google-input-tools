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

#include "components/settings_store/settings_store_base.h"

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"

namespace {

// Messages can be produced by the settings store component.
static const uint32 kProduceMessages[] = {
  ipc::MSG_SETTINGS_CHANGED,
};

// Messages can be consumed by the settings store component.
static const uint32 kConsumeMessages[] = {
  ipc::MSG_COMPONENT_DELETED,
  ipc::MSG_SETTINGS_SET_VALUES,
  ipc::MSG_SETTINGS_GET_VALUES,
  ipc::MSG_SETTINGS_SET_ARRAY_VALUE,
  ipc::MSG_SETTINGS_GET_ARRAY_VALUE,
  ipc::MSG_SETTINGS_ADD_CHANGE_OBSERVER,
  ipc::MSG_SETTINGS_REMOVE_CHANGE_OBSERVER,
};

// A unique string id to identify the settings store component.
static const char kStringId[] = "com.google.ime.goopy.settings-store";

// A human readable name of the settings store component.
static const char kName[] = "Settings Store";

// Normalizes any invalid key chars to '_'.
char NormalizeChar(char c) {
  return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '/') ? c : '_';
}

// Checks if a key has a trailiing '*' char.
bool HasTrailingWildcard(const std::string& key) {
  return *key.rbegin() == '*';
}

// Normalizes a key by replacing all invalid chars with '_'. If
// |allow_trailing_wildcard is true then a trailing '*' char will not be
// normalized.
std::string NormalizeKey(const std::string& key, bool allow_trailing_wildcard) {
  std::string result;
  if (key.empty())
    return result;

  size_t len = key.size();
  result.resize(len);
  if (allow_trailing_wildcard && HasTrailingWildcard(key))
    result[--len] = '*';

  std::transform(key.begin(), key.begin() + len, result.begin(), NormalizeChar);
  return result;
}

// A functor for comparing two strings with specified length.
class CompStringInLength {
 public:
  explicit CompStringInLength(size_t length)
      : length_(length) {
  }
  bool operator() (const std::string& a, const std::string& b) {
    return a.compare(0, std::min(length_, a.size()), b,
                     0, std::min(length_, b.size())) < 0;
  }
 private:
  size_t length_;
};

}  // namespace

namespace ime_goopy {
namespace components {

////////////////////////////////////////////////////////////////////////////////
// SettingsStoreBase::ObserverMap implementation.
////////////////////////////////////////////////////////////////////////////////

SettingsStoreBase::ObserverMap::ObserverMap()
    : min_prefix_length_(std::string::npos) {
}

SettingsStoreBase::ObserverMap::~ObserverMap() {
}

void SettingsStoreBase::ObserverMap::Add(const std::string& key,
                                           uint32 observer) {
  DCHECK(!key.empty());
  const std::string normalized_key = NormalizeKey(key, true);
  observers_[normalized_key].insert(observer);

  // A single '*' will be treated specially.
  if (HasTrailingWildcard(normalized_key) && normalized_key.size() > 1)
    AddPrefix(normalized_key);
}

void SettingsStoreBase::ObserverMap::Remove(const std::string& key,
                                              uint32 observer) {
  DCHECK(!key.empty());
  const std::string normalized_key = NormalizeKey(key, true);

  KeyObserversMap::iterator i = observers_.find(normalized_key);
  if (i == observers_.end())
    return;

  i->second.erase(observer);
  if (i->second.empty()) {
    observers_.erase(i);
    if (HasTrailingWildcard(normalized_key) && normalized_key.size() > 1)
      RemovePrefix(normalized_key);
  }
}

void SettingsStoreBase::ObserverMap::RemoveObserver(uint32 observer) {
  KeyVector unused_keys;
  for (KeyObserversMap::iterator i = observers_.begin();
       i != observers_.end(); ++i) {
    i->second.erase(observer);
    if (i->second.empty())
      unused_keys.push_back(i->first);
  }

  for (KeyVector::iterator i = unused_keys.begin();
       i != unused_keys.end(); ++i) {
    observers_.erase(*i);
    if (HasTrailingWildcard(*i) && i->size() > 1)
      RemovePrefix(*i);
  }
}

void SettingsStoreBase::ObserverMap::Match(const std::string& key,
                                             uint32 ignore,
                                             std::vector<uint32>* observers) {
  DCHECK(!key.empty());
  DCHECK(!HasTrailingWildcard(key));

  std::set<uint32> matched;

  // Matches the exact key first.
  MatchExact(key, &matched);

  // A single wildcard character can match any key.
  MatchExact("*", &matched);

  // Find out all prefixes matching the key.
  std::pair<KeyVector::iterator, KeyVector::iterator> range =
      std::make_pair(prefixes_.begin(), prefixes_.end());
  for (size_t len = min_prefix_length_ - 1; len <= key.size();) {
    range = std::equal_range(
        range.first, range.second, key, CompStringInLength(len));
    if (range.first == range.second)
      break;

    if (range.first->size() == len + 1) {
      DCHECK_EQ(range.first->compare(0, len, key, 0, len), 0);
      MatchExact(*range.first, &matched);
      ++range.first;
      if (range.first == range.second)
        break;
    }
    len = GetMinKeyLength(range.first, range.second) - 1;
  }

  matched.erase(ignore);
  observers->insert(observers->end(), matched.begin(), matched.end());
}

void SettingsStoreBase::ObserverMap::MatchExact(
    const std::string& key,
    std::set<uint32>* observers) {
  KeyObserversMap::const_iterator i = observers_.find(key);
  if (i != observers_.end())
    observers->insert(i->second.begin(), i->second.end());
}

void SettingsStoreBase::ObserverMap::AddPrefix(const std::string& key) {
  KeyVector::iterator i =
      std::lower_bound(prefixes_.begin(), prefixes_.end(), key);
  if (i == prefixes_.end() || *i != key) {
    // We do not strip trailing '*' when adding a prefix into |prefixes_|.
    prefixes_.insert(i, key);
    min_prefix_length_ = std::min(min_prefix_length_, key.size());
    DCHECK_GT(min_prefix_length_, 1);
  }
}

void SettingsStoreBase::ObserverMap::RemovePrefix(const std::string& key) {
  KeyVector::iterator i =
      std::lower_bound(prefixes_.begin(), prefixes_.end(), key);
  if (i != prefixes_.end() && *i == key) {
    prefixes_.erase(i);
    if (key.size() == min_prefix_length_)
      min_prefix_length_ = GetMinKeyLength(prefixes_.begin(), prefixes_.end());
    DCHECK_GT(min_prefix_length_, 1);
  }
}

// static
size_t SettingsStoreBase::ObserverMap::GetMinKeyLength(
    KeyVector::const_iterator begin,
    KeyVector::const_iterator end) {
  if (begin == end)
    return std::string::npos;

  size_t length = begin->size();
  for (KeyVector::const_iterator i = begin + 1; i != end; ++i)
    length = std::min(length, i->size());

  return length;
}

////////////////////////////////////////////////////////////////////////////////
// SettingsStoreBase implementation.
////////////////////////////////////////////////////////////////////////////////

SettingsStoreBase::SettingsStoreBase() {
}

SettingsStoreBase::~SettingsStoreBase() {
}

void SettingsStoreBase::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(kStringId);
  info->set_name(kName);

  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);
  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

void SettingsStoreBase::Handle(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  switch (mptr->type()) {
    case ipc::MSG_COMPONENT_DELETED:
      OnMsgComponentDeleted(mptr.release());
      return;
    case ipc::MSG_SETTINGS_SET_VALUES:
      OnMsgSettingsSetValues(mptr.release());
      return;
    case ipc::MSG_SETTINGS_GET_VALUES:
      OnMsgSettingsGetValues(mptr.release());
      return;
    case ipc::MSG_SETTINGS_SET_ARRAY_VALUE:
      OnMsgSettingsSetArrayValue(mptr.release());
      return;
    case ipc::MSG_SETTINGS_GET_ARRAY_VALUE:
      OnMsgSettingsGetArrayValue(mptr.release());
      return;
    case ipc::MSG_SETTINGS_ADD_CHANGE_OBSERVER:
      OnMsgSettingsAddChangeObserver(mptr.release());
      return;
    case ipc::MSG_SETTINGS_REMOVE_CHANGE_OBSERVER:
      OnMsgSettingsRemoveChangeObserver(mptr.release());
      return;
    default:
      break;
  }
  DLOG(ERROR) << "Unexpected message: " << ipc::GetMessageName(mptr->type());
  ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE, NULL);
}

void SettingsStoreBase::OnDeregistered() {
#ifndef NDEBUG
  if (!observers_.empty()) {
    DLOG(ERROR) << "Settings store component should be deregistered "
                   "after all other components.";
  }
#endif
}

void SettingsStoreBase::OnMsgComponentDeleted(ipc::proto::Message* message) {
  const int size = message->payload().uint32_size();
  for (int i = 0; i < size; ++i)
    observers_.RemoveObserver(message->payload().uint32(i));
  ReplyTrue(message);
}

void SettingsStoreBase::OnMsgSettingsSetValues(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  const int size = mptr->payload().string_size();
  const int variable_size = mptr->payload().variable_size();
  if (!size) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD, NULL);
    return;
  }

  const uint32 source = mptr->source();
  mptr->mutable_payload()->clear_boolean();
  ipc::proto::Variable empty_value;
  for (int i = 0; i < size; ++i) {
    const std::string& key = mptr->payload().string(i);
    if (key.empty()) {
      mptr->mutable_payload()->add_boolean(false);
      continue;
    }

    const std::string normalized_key = NormalizeKey(key, false);
    ipc::proto::Variable* value = (i < variable_size) ?
        mptr->mutable_payload()->mutable_variable(i) : &empty_value;

    bool changed = false;
    const bool result = StoreValue(normalized_key, *value, &changed);
    mptr->mutable_payload()->add_boolean(result);

    // Sends change notification.
    if (result && changed)
      NotifyValueChange(key, normalized_key, source, value);
  }

  mptr->mutable_payload()->clear_variable();

  // Sends back a reply message with boolean results indicating if the values
  // were set correctly.
  if (ipc::ConvertToReplyMessage(mptr.get()))
    Send(mptr.release(), NULL);
}

void SettingsStoreBase::OnMsgSettingsGetValues(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);

  // Just ignore this message if the sender does not expect a reply message.
  if (!ipc::MessageNeedReply(mptr.get()))
    return;

  const int size = mptr->payload().string_size();
  if (!size) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD, NULL);
    return;
  }

  mptr->mutable_payload()->clear_variable();
  for (int i = 0; i < size; ++i) {
    ipc::proto::Variable* value = mptr->mutable_payload()->add_variable();
    value->set_type(ipc::proto::Variable::NONE);

    const std::string& key = mptr->payload().string(i);
    if (key.empty())
      continue;

    LoadValue(NormalizeKey(key, false), value);
  }

  ipc::ConvertToReplyMessage(mptr.get());
  Send(mptr.release(), NULL);
}

void SettingsStoreBase::OnMsgSettingsSetArrayValue(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  if (mptr->payload().string_size() != 1 || mptr->payload().string(0).empty()) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD, NULL);
    return;
  }

  const std::string& key = mptr->payload().string(0);
  const std::string normalized_key = NormalizeKey(key, false);
  ipc::proto::VariableArray array;
  array.mutable_variable()->Swap(mptr->mutable_payload()->mutable_variable());
  mptr->mutable_payload()->clear_boolean();

  bool changed = false;
  const bool result = StoreArrayValue(normalized_key, array, &changed);
  mptr->mutable_payload()->add_boolean(result);

  if (result && changed)
    NotifyArrayValueChange(key, normalized_key, mptr->source(), &array);

  if (ipc::ConvertToReplyMessage(mptr.get()))
    Send(mptr.release(), NULL);
}

void SettingsStoreBase::OnMsgSettingsGetArrayValue(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);

  // Just ignore this message if the sender does not expect a reply message.
  if (!ipc::MessageNeedReply(mptr.get()))
    return;

  if (mptr->payload().string_size() != 1 || mptr->payload().string(0).empty()) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD, NULL);
    return;
  }

  mptr->mutable_payload()->clear_variable();

  ipc::proto::VariableArray array;
  if (LoadArrayValue(NormalizeKey(mptr->payload().string(0), false), &array)) {
    mptr->mutable_payload()->mutable_variable()->Swap(array.mutable_variable());
  } else {
    mptr->mutable_payload()->add_variable()->set_type(
        ipc::proto::Variable::NONE);
  }

  ipc::ConvertToReplyMessage(mptr.get());
  Send(mptr.release(), NULL);
}

void SettingsStoreBase::OnMsgSettingsAddChangeObserver(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  const int size = mptr->payload().string_size();
  if (!size) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD, NULL);
    return;
  }

  const uint32 observer = mptr->source();
  for (int i = 0; i < size; ++i) {
    const std::string& key = mptr->payload().string(i);
    if (!key.empty())
      observers_.Add(key, observer);
  }

  ReplyTrue(mptr.release());
}

void SettingsStoreBase::OnMsgSettingsRemoveChangeObserver(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  const int size = mptr->payload().string_size();
  if (!size) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD, NULL);
    return;
  }

  const uint32 observer = mptr->source();
  for (int i = 0; i < size; ++i) {
    const std::string& key = mptr->payload().string(i);
    if (!key.empty())
      observers_.Remove(key, observer);
  }

  ReplyTrue(mptr.release());
}

void SettingsStoreBase::NotifyValueChange(const std::string& key,
                                            const std::string& normalized_key,
                                            uint32 ignore,
                                            ipc::proto::Variable* value) {
  std::vector<uint32> matched_observers;
  observers_.Match(normalized_key, ignore, &matched_observers);
  if (!matched_observers.size())
    return;

  ipc::proto::Message* msg =
      NewMessage(ipc::MSG_SETTINGS_CHANGED, ipc::kInputContextNone, false);

  msg->mutable_payload()->add_string(key);
  msg->mutable_payload()->add_variable()->Swap(value);
  SendNotifyMessage(matched_observers, msg);
}

void SettingsStoreBase::NotifyArrayValueChange(
    const std::string& key,
    const std::string& normalized_key,
    uint32 ignore,
    ipc::proto::VariableArray* array) {
  std::vector<uint32> matched_observers;
  observers_.Match(normalized_key, ignore, &matched_observers);
  if (!matched_observers.size())
    return;

  ipc::proto::Message* msg =
      NewMessage(ipc::MSG_SETTINGS_CHANGED, ipc::kInputContextNone, false);

  msg->mutable_payload()->add_string(key);
  msg->mutable_payload()->mutable_variable()->Swap(array->mutable_variable());
  SendNotifyMessage(matched_observers, msg);
}

void SettingsStoreBase::SendNotifyMessage(
    const std::vector<uint32>& observers,
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);

  // Sends notification messages to observers, except the last one. We will
  // send the original |message| to the last one to avoid one copy.
  const size_t last = observers.size() - 1;
  for (size_t i = 0; i < last; ++i) {
    // We need to copy the message for each observer.
    ipc::proto::Message* copy = new ipc::proto::Message(*mptr);
    copy->set_target(observers[i]);
    Send(copy, NULL);
  }

  mptr->set_target(observers[last]);
  Send(mptr.release(), NULL);
}

}  // namespace components
}  // namespace ime_goopy
