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

#ifndef GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_MEMORY_H_
#define GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_MEMORY_H_

#include <map>
#include <string>

#include "components/settings_store/settings_store_base.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
namespace components {

// An in memory SettingsStore implementation.
class SettingsStoreMemory : public SettingsStoreBase {
 public:
  // An interface class used for enumerating all stored settings.
  class Enumerator {
   public:
    // Enumerates a value. Returns false to stop the enumeration process.
    virtual bool EnumerateValue(const std::string& key,
                                const ipc::proto::Variable& value) = 0;

    // Enumerates an array value. Returns false to stop the enumeration process.
    virtual bool EnumerateArrayValue(
        const std::string& key,
        const ipc::proto::VariableArray& array) = 0;

   protected:
    virtual ~Enumerator() {}
  };

  SettingsStoreMemory();
  virtual ~SettingsStoreMemory();

  // Enumerates all stored settings. It'll enumerate all single value settings
  // first and then all array value settings.
  // Returns true if all settings have been enumerated.
  bool Enumerate(Enumerator* enumerator);

  // TODO(suzhe): Implement methods for bulk loading/saving settings.

 protected:
  FRIEND_TEST(SettingsStoreMemoryTest, Value);
  FRIEND_TEST(SettingsStoreMemoryTest, ArrayValue);
  FRIEND_TEST(SettingsStoreMemoryTest, Enumerator);

  bool IsValueAvailable(const std::string& key);
  bool IsArrayValueAvailable(const std::string& key);

  // Overridden from SettingsStoreBase:
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
  typedef std::map<std::string, ipc::proto::Variable> ValueMap;
  typedef std::map<std::string, ipc::proto::VariableArray> ArrayValueMap;

  ValueMap values_;
  ArrayValueMap array_values_;

  DISALLOW_COPY_AND_ASSIGN(SettingsStoreMemory);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_MEMORY_H_
