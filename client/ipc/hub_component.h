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

#ifndef GOOPY_IPC_HUB_COMPONENT_H_
#define GOOPY_IPC_HUB_COMPONENT_H_
#pragma once

#include <map>
#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

class HotkeyList;

// A class to hold all information of a component in the Hub.
// Following information are managed by this class:
// 1. An ipc::proto::ComponentInfo object provided by the component upon
//    registration.
// 2. Pointer to the Connector object owning this component
// 3. ipc::proto::HotkeyList objects registered by the component
//    Each component can register multiple HotkeyList objects with different
//    ids.
class Component {
 public:
  typedef Hub::Connector Connector;

  // Creates a Component object with a specified component id. |connector| is
  // the one owning the component. Content of |info| will be copied into the new
  // object except the id.
  Component(uint32 id,
            Connector* connector,
            const proto::ComponentInfo& info);
  ~Component();

  const proto::ComponentInfo& info() const { return info_; }

  Connector* connector() const { return connector_; }

  uint32 id() const { return info_.id(); }

  const std::string& string_id() const { return info_.string_id(); }

  // Adds a HotkeyList object.
  // Existing HotkeyList object with the same id will be replaced.
  void AddHotkeyList(const proto::HotkeyList& hotkey_list);

  void RemoveHotkeyList(uint32 id);

  const HotkeyList* GetHotkeyList(uint32 id) const;

  // Checks if the component may produce a specific message type.
  bool MayProduce(uint32 message_type) const {
    return produce_set_.count(message_type) > 0;
  }

  // Checks if the component can consume a specific message type.
  bool CanConsume(uint32 message_type) const {
    return consume_set_.count(message_type) > 0;
  }

  const std::set<uint32>& attached_input_contexts() const {
    return attached_input_contexts_;
  }

  std::set<uint32>& attached_input_contexts() {
    return attached_input_contexts_;
  }

  // Checks if this component matches the given information template.
  bool MatchInfoTemplate(const proto::ComponentInfo& query) const;

 private:
  proto::ComponentInfo info_;

  // Owner of the component. It's a weak reference.
  Connector* connector_;

  typedef std::map<uint32, HotkeyList*> HotkeyListMap;
  HotkeyListMap hotkey_list_map_;

  typedef std::set<uint32> MessageTypeSet;
  MessageTypeSet produce_set_;
  MessageTypeSet consume_set_;

  std::set<uint32> attached_input_contexts_;

  DISALLOW_COPY_AND_ASSIGN(Component);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_COMPONENT_H_
