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

#include "components/settings_store/settings_store_memory.h"

#include "base/logging.h"

namespace {

bool CompareProto(const google::protobuf::MessageLite& a,
                  const google::protobuf::MessageLite& b) {
  return a.SerializeAsString() == b.SerializeAsString();
}

}  // anonymous namespace

namespace ime_goopy {
namespace components {

SettingsStoreMemory::SettingsStoreMemory() {
}

SettingsStoreMemory::~SettingsStoreMemory() {
}

bool SettingsStoreMemory::Enumerate(Enumerator* enumerator) {
  DCHECK(enumerator);

  bool result = true;
  for (ValueMap::iterator i = values_.begin();
       result && i != values_.end(); ++i) {
    result = enumerator->EnumerateValue(i->first, i->second);
  }
  for (ArrayValueMap::iterator i = array_values_.begin();
       result && i != array_values_.end(); ++i) {
    result = enumerator->EnumerateArrayValue(i->first, i->second);
  }
  return result;
}

bool SettingsStoreMemory::IsValueAvailable(const std::string& key) {
  return !key.empty() && values_.count(key) != 0;
}

bool SettingsStoreMemory::IsArrayValueAvailable(const std::string& key) {
  return !key.empty() && array_values_.count(key) != 0;
}

bool SettingsStoreMemory::StoreValue(const std::string& key,
                                     const ipc::proto::Variable& value,
                                     bool* changed) {
  if (key.empty())
    return false;

  // We do not allow duplicate keys, so if the |key| is associated to a value
  // then it will never be associated to an array value.
  array_values_.erase(key);

  // Erases the old value associated to the key.
  if (value.type() == ipc::proto::Variable::NONE) {
    const size_t result = values_.erase(key);
    if (changed)
      *changed = (result != 0);
    return true;
  }

  ValueMap::iterator i = values_.find(key);
  if (changed)
    *changed = (i == values_.end() || !CompareProto(i->second, value));

  if (i != values_.end())
    i->second.CopyFrom(value);
  else
    values_[key].CopyFrom(value);

  return true;
}

bool SettingsStoreMemory::LoadValue(const std::string& key,
                                    ipc::proto::Variable* value) {
  DCHECK(value);

  ValueMap::iterator i = values_.find(key);
  if (i == values_.end())
    return false;

  value->CopyFrom(i->second);
  return true;
}

bool SettingsStoreMemory::StoreArrayValue(
    const std::string& key,
    const ipc::proto::VariableArray& array,
    bool* changed) {
  if (key.empty())
    return false;

  // We do not allow duplicate keys, so if the |key| is associated to an array
  // value then it will never be associated to a value.
  values_.erase(key);

  // Erases the old value associated to the key.
  if (!array.variable_size()) {
    const size_t result = array_values_.erase(key);
    if (changed)
      *changed = (result != 0);
    return true;
  }

  ArrayValueMap::iterator i = array_values_.find(key);
  if (changed)
    *changed = (i == array_values_.end() || !CompareProto(i->second, array));

  if (i != array_values_.end())
    i->second.CopyFrom(array);
  else
    array_values_[key].CopyFrom(array);

  return true;
}

bool SettingsStoreMemory::LoadArrayValue(const std::string& key,
                                         ipc::proto::VariableArray* array) {
  DCHECK(array);

  ArrayValueMap::iterator i = array_values_.find(key);
  if (i == array_values_.end())
    return false;

  array->CopyFrom(i->second);
  return true;
}

}  // namespace components
}  // namespace ime_goopy
