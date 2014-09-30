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

#ifndef GOOPY_IPC_SETTINGS_CLIENT_H_
#define GOOPY_IPC_SETTINGS_CLIENT_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/sub_component_base.h"

namespace ipc {

class ComponentBase;

// An helper class for accessing settings through ipc layer. This class
// encapsulates the interfaces of remote settings store component. Any component
// can add this sub_component as a part of own and use it to access settings
// with the local interfaces without considerring any ipc layer details.
class SettingsClient : public SubComponentBase {
 public:
  typedef std::vector<std::string> KeyList;
  typedef std::vector<bool> ResultList;

  // An interface that should be implemented by the class who wants to monitor
  // value changes.
  class Delegate {
   public:
    // Called when value of an observed key changes.
    virtual void OnValueChanged(const std::string& key,
                                const proto::VariableArray& array) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // |delegate| can be NULL if any component only wants to use this sub
  // component to access external settings stores without caring about the value
  // changes.
  SettingsClient(ComponentBase* owner, Delegate* delegate);
  virtual ~SettingsClient();

  // Overridden from SubComponent
  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE;
  virtual bool Handle(proto::Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;

  // Encapsulates remote settings store interfaces.

  // Stores a list of key-value pairs to external settings store.
  // Returns true if success. Results indicating if values were set successfully
  // or not will be returned in |results|.
  bool SetValues(const KeyList& keys,
                 const proto::VariableArray& values,
                 ResultList* results);

  // Gets a list of key-value pairs from the external settings store.
  // Returns true if success.
  bool GetValues(const KeyList& keys, proto::VariableArray* values);

  // Stores a key-value pair to external settings store.
  // Returns true if success.
  bool SetValue(const std::string& key, const proto::Variable& value);

  // Get a value from the external settings store.
  // Returns true if success.
  bool GetValue(const std::string& key, proto::Variable* value);

  // Stores an array value to the external settings store. If values is empty
  // then the old values associated to the given key should be deleted.
  // Returns true if success.
  bool SetArrayValue(const std::string& key, const proto::VariableArray& array);

  // Gets an array value from the external settings store.
  // Returns true if success. |*array| should remain unchanged if failed.
  bool GetArrayValue(const std::string& key, proto::VariableArray* array);

  // Adds owner component as an observer of specific keys of the external
  // settings store. If any key changes, the source will receive notification
  // message.
  // Returns true if success.
  bool AddChangeObserverForKeys(const KeyList& key_list);

  // An wrapper function to add owner component as an observer of one key.
  bool AddChangeObserver(const std::string& key);

  // Remove the source as an observer of specific keys of the external settings
  // store.
  // Returns true if success.
  bool RemoveChangeObserverForKeys(const KeyList& key_list);

  // An wrapper function to remove owner component as an observer of one key.
  bool RemoveChangeObserver(const std::string& key);

  // Stores an integer value to external settings store.
  bool SetIntegerValue(const std::string& key, int64 value);

  // Gets an integer value from external settings store.
  bool GetIntegerValue(const std::string& key, int64* value);

  // Stores a string value to external settings store.
  bool SetStringValue(const std::string& key, const std::string& value);

  // Gets a string value from external settings store.
  bool GetStringValue(const std::string& key, std::string* value);

  // Stores a boolean value to external settings store.
  bool SetBooleanValue(const std::string& key, bool value);

  // Gets a boolean value from external settings store.
  bool GetBooleanValue(const std::string& key, bool* value);

 private:
  Delegate* delegate_;

  // Sends the |send_message| with SendWithReply and parse the reply message.
  // Returns NULL if SendWithReply fails or hub returns error reply. The caller
  // should delete the replied message.
  proto::Message* SendWithReply(proto::Message* send_message);
  proto::Message* NewMessage(uint32 type);

  DISALLOW_COPY_AND_ASSIGN(SettingsClient);
};

}  // namespace ipc

#endif  // GOOPY_IPC_SETTINGS_CLIENT_H_
