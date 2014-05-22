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

#ifndef GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_WIN_H_
#define GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_WIN_H_

#include <atlbase.h>
#include <set>
#include <string>

#include "base/scoped_ptr.h"
#include "components/settings_store/settings_store_memory.h"

namespace ime_goopy {
namespace components {

class SettingsStoreWin : public SettingsStoreMemory {
 public:
  // |registry| is owned by the SettingsStoreWin instance.
  explicit SettingsStoreWin(CRegKey* registry);
  virtual ~SettingsStoreWin();

  // Overridden from SettingsStoreMemory:
  // If serialized value is larger than 64K bytes, The call will fail because
  // of Windows system's limitation.
  virtual bool StoreValue(const std::string& key,
                          const ipc::proto::Variable& value,
                          bool* changed) OVERRIDE;
  virtual bool LoadValue(const std::string& key,
                         ipc::proto::Variable* value) OVERRIDE;
  virtual bool StoreArrayValue(const std::string& key,
                               const ipc::proto::VariableArray& array,
                               bool* changed) OVERRIDE;
  virtual bool LoadArrayValue(const std::string& key,
                              ipc::proto::VariableArray* array) OVERRIDE;

 private:
  // Store value to registry, which must already be in memory store.
  bool StoreValueToRegistry(const std::string& key,
                            const ipc::proto::Variable& value);
  // Load value from registry and store it into memory store.
  bool LoadValueFromRegistry(const std::string& key,
                             ipc::proto::Variable* value);
  // Store array value to registry, which must already be in memory store.
  bool StoreArrayValueToRegistry(const std::string& key,
                                 const ipc::proto::VariableArray& array);
  // Load array value from registry and store it into memory store.
  bool LoadArrayValueFromRegistry(const std::string& key,
                                  ipc::proto::VariableArray* array);

  scoped_ptr<CRegKey> registry_;
  DISALLOW_COPY_AND_ASSIGN(SettingsStoreWin);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_WIN_H_
