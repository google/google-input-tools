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

#include "components/settings_store/settings_store_win.h"

#include <windows.h>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_utils_win.h"

namespace ime_goopy {
namespace components {

SettingsStoreWin::SettingsStoreWin(CRegKey* registry)
    : registry_(registry) {
}

SettingsStoreWin::~SettingsStoreWin() {
}

bool SettingsStoreWin::StoreValue(const std::string& key,
                                  const ipc::proto::Variable& value,
                                  bool* changed) {
  if (key.empty())
    return false;
  bool value_changed = false;

  std::string value_str;

  ipc::proto::Variable old_value;
  if (!SettingsStoreMemory::IsValueAvailable(key))
    LoadValueFromRegistry(key, &old_value);

  if (!SettingsStoreMemory::StoreValue(key, value, &value_changed))
    return false;

  if (changed)
    *changed = value_changed;

  return !value_changed || StoreValueToRegistry(key, value);
}

bool SettingsStoreWin::LoadValue(const std::string& key,
                                 ipc::proto::Variable* value) {
  DCHECK(value);
  if (key.empty())
    return false;

  return SettingsStoreMemory::LoadValue(key, value) ||
      LoadValueFromRegistry(key, value);
}

bool SettingsStoreWin::StoreArrayValue(
    const std::string& key,
    const ipc::proto::VariableArray& array,
    bool* changed) {
  bool array_changed = false;

  if (key.empty())
    return false;

  ipc::proto::VariableArray old_array;
  if (!SettingsStoreMemory::IsArrayValueAvailable(key))
    LoadArrayValueFromRegistry(key, &old_array);

  if (!SettingsStoreMemory::StoreArrayValue(key, array, &array_changed))
    return false;

  if (changed)
    *changed = array_changed;
  return !array_changed || StoreArrayValueToRegistry(key, array);
}

bool SettingsStoreWin::LoadArrayValue(
    const std::string& key,
    ipc::proto::VariableArray* array) {
  DCHECK(array);
  if (key.empty())
    return false;

  return SettingsStoreMemory::LoadArrayValue(key, array) ||
      LoadArrayValueFromRegistry(key, array);
}

bool SettingsStoreWin::StoreValueToRegistry(
    const std::string& key,
    const ipc::proto::Variable& value) {
  std::wstring wkey = Utf8ToWide(key);
  registry_->DeleteValue(wkey.c_str());

  if (!value.IsInitialized() || value.type() == ipc::proto::Variable::NONE)
    return true;

  bool success = false;
  LONG ret = -1;
  switch (value.type()) {
    case ipc::proto::Variable::INTEGER:
      ret = registry_->SetQWORDValue(wkey.c_str(), value.integer());
      success = (ret == ERROR_SUCCESS);
      break;
    case ipc::proto::Variable::STRING:
      ret = registry_->SetStringValue(
          wkey.c_str(), Utf8ToWide(value.string()).c_str());
      success = (ret == ERROR_SUCCESS);
      break;
    default: {
      std::string output_buffer;
      value.AppendToString(&output_buffer);
      ret = registry_->SetBinaryValue(wkey.c_str(),
                                      output_buffer.c_str(),
                                      output_buffer.size());
      success = (ret == ERROR_SUCCESS);
      break;
    }
  }

  return success;
}

bool SettingsStoreWin::LoadValueFromRegistry(const std::string& key,
                                             ipc::proto::Variable* value) {
  bool changed = false;
  bool success = false;

  ipc::proto::Variable_Type type = ipc::proto::Variable::NONE;
  std::wstring wkey = Utf8ToWide(key);

  DWORD value_type = 0;
  DWORD value_length = 0;
  if (registry_->QueryValue(wkey.c_str(), &value_type, NULL, &value_length) !=
      ERROR_SUCCESS || !value_length) {
    return false;
  }

  switch (value_type) {
    case REG_QWORD: {
      DWORD64 integer_value = 0;
      if (registry_->QueryQWORDValue(
          wkey.c_str(), integer_value) == ERROR_SUCCESS) {
        value->set_type(ipc::proto::Variable::INTEGER);
        value->set_integer(integer_value);
        success = SettingsStoreMemory::StoreValue(key, *value, &changed);
        DCHECK(success);
      }
      break;
    }
    case REG_DWORD: {
      DWORD integer_value = 0;
      if (registry_->QueryDWORDValue(
          wkey.c_str(), integer_value) == ERROR_SUCCESS) {
        value->set_type(ipc::proto::Variable::INTEGER);
        value->set_integer(static_cast<DWORD64>(integer_value));
        success = SettingsStoreMemory::StoreValue(key, *value, &changed);
        DCHECK(success);
      }
      break;
    }
    case REG_SZ: {
      value_length /= sizeof(wchar_t);
      scoped_array<wchar_t> str_buffer(
          new wchar_t[value_length + 1]);
      if (registry_->QueryStringValue(
          wkey.c_str(), str_buffer.get(), &value_length) == ERROR_SUCCESS) {
        value->set_type(ipc::proto::Variable::STRING);
        value->set_string(WideToUtf8(str_buffer.get()));
        success = SettingsStoreMemory::StoreValue(key, *value, &changed);
        DCHECK(success);
      }
      break;
    }
    case REG_BINARY: {
      scoped_array<uint8> buffer(new uint8[value_length]);
      if (registry_->QueryBinaryValue(
          wkey.c_str(), buffer.get(), &value_length) == ERROR_SUCCESS) {
        if (value->ParseFromArray(buffer.get(), value_length)) {
          success = SettingsStoreMemory::StoreValue(key, *value, &changed);
          DCHECK(success);
        }
      }
      break;
    }
  }
  return success;
}

bool SettingsStoreWin::StoreArrayValueToRegistry(
    const std::string& key,
    const ipc::proto::VariableArray& array) {
  std::wstring wkey = Utf8ToWide(key);
  // Delete old value if any.
  registry_->DeleteValue(wkey.c_str());

  std::string string_value;
  if (!array.variable_size() ||
      !array.IsInitialized() ||
      !array.AppendToString(&string_value)) {
    return true;
  }

  return registry_->SetBinaryValue(wkey.c_str(),
                                   string_value.c_str(),
                                   string_value.size()) == ERROR_SUCCESS;
}

bool SettingsStoreWin::LoadArrayValueFromRegistry(
    const std::string& key,
    ipc::proto::VariableArray* array) {
  bool changed = false;

  ULONG binary_size = 0;
  if (registry_->QueryBinaryValue(
      Utf8ToWide(key).c_str(), NULL, &binary_size) != ERROR_SUCCESS ||
      binary_size == 0) {
    return false;
  }

  scoped_array<uint8> buffer(new uint8[binary_size]);
  if (registry_->QueryBinaryValue(
      Utf8ToWide(key).c_str(), buffer.get(), &binary_size) != ERROR_SUCCESS) {
    return false;
  }

  ipc::proto::VariableArray values;
  if (values.ParseFromArray(buffer.get(), binary_size)) {
    array->CopyFrom(values);
    bool success = SettingsStoreMemory::StoreArrayValue(key, *array, &changed);
    DCHECK(success);
  }

  return true;
}

}  // namespace components
}  // namespace ime_goopy
