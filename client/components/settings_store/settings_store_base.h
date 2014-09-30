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

#ifndef GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_BASE_H_
#define GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_BASE_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "ipc/component_base.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing_prod.h"

namespace ime_goopy {
namespace components {

// Base class of platform dependent settings store implementations.
class SettingsStoreBase : public ipc::ComponentBase {
 public:
  // This base class will not add itself to the |host| automatically. It should
  // be done somewhere else.
  SettingsStoreBase();
  virtual ~SettingsStoreBase();

  // Overridden from Component: derived classes should not override this two
  // message anymore.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

 protected:
  // Overridden from ComponentBase:
  virtual void OnDeregistered() OVERRIDE;

  // Virtual methods need to be implemented by derived classes.

  // Stores a value to the external settings store. If value.type() == NONE
  // then the old value associated to the given key should be deleted.
  // Returns true if success. |*changed| should be set to true if the new value
  // was different than the old one.
  virtual bool StoreValue(const std::string& key,
                          const ipc::proto::Variable& value,
                          bool* changed) = 0;

  // Loads a value from the external settings store.
  // Returns true if success. |*value| should remain unchanged if failed.
  virtual bool LoadValue(const std::string& key,
                         ipc::proto::Variable* value) = 0;

  // Stores an array value to the external settings store. If values is empty
  // then the old values associated to the given key should be deleted.
  // Returns true if success. |*changed| should be set to true if the new value
  // was different than the old one.
  virtual bool StoreArrayValue(const std::string& key,
                               const ipc::proto::VariableArray& array,
                               bool* changed) = 0;

  // Loads an array value from the external settings store.
  // Returns true if success. |*array| should remain unchanged if failed.
  virtual bool LoadArrayValue(const std::string& key,
                              ipc::proto::VariableArray* array) = 0;

 private:
  FRIEND_TEST(SettingsStoreBaseTest, ObserverMapTest);
  friend class SettingsStoreTestCommon;

  // A class for mapping from keys to ids of observer components.
  // Declaring this class hear instead of the .cc file to help implement unit
  // tests.
  class ObserverMap {
   public:
    ObserverMap();
    ~ObserverMap();

    // Adds a key-observer pair to the map.
    void Add(const std::string& key, uint32 observer);

    // Removes a key-observer pair from the map.
    void Remove(const std::string& key, uint32 observer);

    // Removes a given observer from the map completely.
    void RemoveObserver(uint32 observer);

    // Matches a given key and stores all matched observers into |*observers|,
    // except the |ignore|. The |key| must already be normalized and should not
    // contain trailing '*' char.
    void Match(const std::string& key,
               uint32 ignore,
               std::vector<uint32>* observers);

    bool empty() const {
      return observers_.empty();
    }

   private:
    FRIEND_TEST(SettingsStoreBaseTest, ObserverMapTest);
    typedef std::vector<std::string> KeyVector;
    typedef std::map<std::string, std::set<uint32> > KeyObserversMap;

    // Matches a given key in |observers_|.
    void MatchExact(const std::string& key, std::set<uint32>* observers);

    // Adds a key with trailing '*' into |prefixes_| keeping the order.
    void AddPrefix(const std::string& key);

    // Removes a key with trailing '*' from |prefixes_|.
    void RemovePrefix(const std::string& key);

    // Gets the minimum key length of given keys.
    static size_t GetMinKeyLength(KeyVector::const_iterator begin,
                                  KeyVector::const_iterator end);

    // An ordered list of all keys with trailing '*'.
    KeyVector prefixes_;

    // A key to observers map, including all keys with trailing '*'.
    KeyObserversMap observers_;

    size_t min_prefix_length_;

    DISALLOW_COPY_AND_ASSIGN(ObserverMap);
  };

  // Message handlers.
  void OnMsgComponentDeleted(ipc::proto::Message* message);
  void OnMsgSettingsSetValues(ipc::proto::Message* message);
  void OnMsgSettingsGetValues(ipc::proto::Message* message);
  void OnMsgSettingsSetArrayValue(ipc::proto::Message* message);
  void OnMsgSettingsGetArrayValue(ipc::proto::Message* message);
  void OnMsgSettingsAddChangeObserver(ipc::proto::Message* message);
  void OnMsgSettingsRemoveChangeObserver(ipc::proto::Message* message);

  // Sends notification message about the change of a value to all matched
  // observers, except |ignore|. |*value| will be cleared (moved into the
  // notification message) to reduce memory copy.
  void NotifyValueChange(const std::string& key,
                         const std::string& normalized_key,
                         uint32 ignore,
                         ipc::proto::Variable* value);

  // Sends notification message about the change of an array value to all
  // matched observers, except |ignore|. |*value| will be cleared (moved into
  // the notification message) to reduce memory copy.
  void NotifyArrayValueChange(const std::string& key,
                              const std::string& normalized_key,
                              uint32 ignore,
                              ipc::proto::VariableArray* array);

  // Sends the given notify message to specified observers. The |message| will
  // be deleted when finish.
  void SendNotifyMessage(const std::vector<uint32>& observers,
                         ipc::proto::Message* message);

  ObserverMap observers_;

  DISALLOW_COPY_AND_ASSIGN(SettingsStoreBase);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_BASE_H_
