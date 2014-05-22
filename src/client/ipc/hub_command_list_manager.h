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

#ifndef GOOPY_IPC_HUB_COMMAND_LIST_MANAGER_H_
#define GOOPY_IPC_HUB_COMMAND_LIST_MANAGER_H_

#include <map>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

class HubImpl;
class Component;
class InputContext;

// A built-in component for managing CommandList objects of all components.
class HubCommandListManager : public Hub::Connector {
 public:
  explicit HubCommandListManager(HubImpl* hub);
  virtual ~HubCommandListManager();

  // Implementation of Hub::Connector interface
  virtual bool Send(proto::Message* message) OVERRIDE;

 private:
  // Key: component id
  typedef std::map<uint32, proto::CommandList> ComponentCommandListMap;

  // Key: input context id
  typedef std::map<uint32, ComponentCommandListMap> CommandListMap;

  // Handlers for broadcast messages that we can consume.
  bool OnMsgInputContextCreated(proto::Message* message);
  bool OnMsgInputContextDeleted(proto::Message* message);
  bool OnMsgComponentDetached(proto::Message* message);

  // Handlers for other messages that we can consume.
  bool OnMsgSetCommandList(Component* source, proto::Message* message);
  bool OnMsgUpdateCommands(Component* source, proto::Message* message);
  bool OnMsgQueryCommandList(Component* source, proto::Message* message);

  void DeleteCommandList(uint32 icid, uint32 component);

  void BroadcastCommandListChanged(
      uint32 icid,
      uint32 changed_component,
      const ComponentCommandListMap& command_lists);

  // Sets the owner of a CommandList object recursively.
  static void SetCommandListOwner(uint32 owner, proto::CommandList* commands);

  // Updates an existing command in a CommandList object.
  // Returns true if a command with the same id is found and updated, and the
  // content of |*new_command| will be invalidated.
  static bool UpdateCommand(proto::Command* new_command,
                            proto::CommandList* commands);

  // The Component object representing the command list manager itself.
  Component* self_;

  // Weak pointer to our owner.
  HubImpl* hub_;

  CommandListMap command_lists_;

  DISALLOW_COPY_AND_ASSIGN(HubCommandListManager);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_COMMAND_LIST_MANAGER_H_
