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

#ifndef GOOPY_IPC_HUB_IMPL_H_
#define GOOPY_IPC_HUB_IMPL_H_
#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"

#include "ipc/constants.h"
#include "ipc/hub.h"
#include "ipc/hub_component.h"
#include "ipc/hub_input_context.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

extern const uint32 kHubProduceMessages[];
extern const size_t kHubProduceMessagesSize;
extern const uint32 kHubConsumeMessages[];
extern const size_t kHubConsumeMessagesSize;

extern const char kHubStringID[];
extern const char kHubName[];

class HubCommandListManager;
class HubCompositionManager;
class HubDefaultInputMethod;
class HubHotkeyManager;
class HubImplTestBase;
class HubInputContextManager;
class HubInputMethodManager;

// The real implementation of Hub class. This class provides following
// functionalities:
// 1. Manages components and input contexts.
// 2. Supports messages related to component management.
// 3. Provides utility functions for its built-in components.
// 4. Dispatches messages among components.
// This class is not thread safe. A wrapper providing necessary lock or
// multi-threading support should be used when using it in a multi-thread
// environment.
class HubImpl : public Hub,
                public Hub::Connector,
                public InputContext::Delegate {
 public:
  typedef Hub::Connector Connector;
  typedef InputContext::MessageTypeVector MessageTypeVector;

  HubImpl();
  virtual ~HubImpl();

  // Overridden from ipc::Hub interface
  virtual void Attach(Connector* connector) OVERRIDE;
  virtual void Detach(Connector* connector) OVERRIDE;
  virtual bool Dispatch(Connector* connector, proto::Message* message) OVERRIDE;

  // Gets a component object by id.
  Component* GetComponent(uint32 id) const {
    ComponentMap::const_iterator i = components_.find(id);
    return i != components_.end() ? i->second : NULL;
  }

  Component* GetComponentByStringID(const std::string& id) const {
    ComponentStringIDMap::const_iterator i = components_by_string_id_.find(id);
    return i != components_by_string_id_.end() ? i->second : NULL;
  }

  // Gets an input context object by id.
  InputContext* GetInputContext(uint32 id) const {
    if (id == kInputContextFocused)
      id = focused_input_context_;

    if (id == kInputContextNone)
      return hub_input_context_;

    InputContextMap::const_iterator iter = input_contexts_.find(id);
    return iter != input_contexts_.end() ? iter->second : NULL;
  }

  // Creates a component object for a specified |connector|. If |built_in| is
  // true, then it'll be attached to the default input context with
  // ACTIVE_STICKY mode directly.
  Component* CreateComponent(Connector* connector,
                             const proto::ComponentInfo& info,
                             bool built_in);

  // Deletes a component of a specified |connector| by id.
  bool DeleteComponent(Connector* connector, uint32 id);

  // Creates an input context object for a specified |owner| component.
  InputContext* CreateInputContext(Component* owner);

  // Deletes an input context owned by the |owner| component.
  bool DeleteInputContext(Component* owner, uint32 icid);

  // Converts a given |message| object into a reply message with an error
  // payload and sends it to the |connector| object directly.
  bool ReplyError(Connector* connector,
                  proto::Message* message,
                  proto::Error::Code error_code);

  // Converts a given |message| object into a reply message with a boolean value
  // as payload and sends it to the |connector| object directly.
  bool ReplyBoolean(Connector* connector, proto::Message* message, bool value);

  bool ReplyTrue(Connector* connector, proto::Message* message) {
    return ReplyBoolean(connector, message, true);
  }

  bool ReplyFalse(Connector* connector, proto::Message* message) {
    return ReplyBoolean(connector, message, false);
  }

  // Attaches the |component| object to the |input_context| object with
  // specified |state| and |persistent| mode.
  bool AttachToInputContext(Component* component,
                            InputContext* input_context,
                            InputContext::AttachState state,
                            bool persistent);

  // Tries to attach a component to an input context with a desired state.
  // If the component can consume MSG_ATTACH_TO_INPUT_CONTEXT message, then
  // it'll be attached with an appropriate pending state and a message will
  // be sent to it. The component will be actually attached when we receive
  // its reply message. Otherwise, the component will only be attached to the
  // input context if |allow_implicit_attach| is true.
  // The |state| can only be PASSIVE or ACTIVE.
  // The actual attach state will be returned. NOT_ATTACHED will be returned on
  // error.
  InputContext::AttachState RequestAttachToInputContext(
      Component* component,
      InputContext* input_context,
      InputContext::AttachState state,
      bool allow_implicit_attach);

  // Focuses a specified input context.
  bool FocusInputContext(uint32 icid);

  // Blur a specified input context. The input context must be the current
  // focused one.
  bool BlurInputContext(uint32 icid);

  // Checks if a component is valid or not.
  bool IsComponentValid(Component* component) const {
    return component && components_.count(component->id()) &&
        connectors_.count(component->connector());
  }

  // Checks if a component id is valid or not.
  bool IsComponentIDValid(uint32 id) const {
    return IsComponentValid(GetComponent(id));
  }

  // A utility method for built-in components to check if the given message
  // needs reply. If the check fails, then this method will delete the
  // message and return false. Returns true if the check passed.
  // The built-in component calling this method should always return true to
  // Hub indicating that it has handled the message, despite of this method's
  // return value.
  bool CheckMsgNeedReply(Component* source, proto::Message* message);

  // A utility method for built-in components to check if icid of the given
  // message is valid. If the check fails, then this method will reply an error
  // message to source if necessary and delete the message and return false.
  // Returns true if the check passed.
  // The built-in component calling this method should always return true to
  // Hub indicating that it has handled the message, despite of this method's
  // return value.
  bool CheckMsgInputContext(Component* source, proto::Message* message);

  // A utility method for built-in components to check if icid of the given
  // message is valid and if the source component is really attached to the IC.
  // If any of these two checks fails, then this method will reply an error
  // message to source if necessary and delete the message and return false.
  // Returns true if all checks passed.
  // The built-in component calling this method should always return true to
  // Hub indicating that it has handled the message, despite of this method's
  // return value.
  bool CheckMsgInputContextAndSourceAttached(Component* source,
                                             proto::Message* message);

 private:
  friend class HubImplTestBase;

  // Implementation of Hub::Connector interface for the special Hub component,
  // whose component id is kComponentDefault.
  virtual bool Send(proto::Message* message) OVERRIDE;

  // Implementation of InputContext::Delegate interface
  virtual void OnComponentActivated(InputContext* input_context,
                                    Component* component,
                                    const MessageTypeVector& messages) OVERRIDE;

  virtual void OnComponentDeactivated(
      InputContext* input_context,
      Component* component,
      const MessageTypeVector& messages) OVERRIDE;

  virtual void OnComponentDetached(
      InputContext* input_context,
      Component* component,
      InputContext::AttachState state) OVERRIDE;

  virtual void OnActiveConsumerChanged(
      InputContext* input_context,
      const MessageTypeVector& messages) OVERRIDE;

  virtual void MaybeDetachComponent(
      InputContext* input_context,
      Component* component) OVERRIDE;

  virtual void RequestConsumer(
      InputContext* input_context,
      const MessageTypeVector& messages,
      Component* exclude) OVERRIDE;

  // Allocates a unique component id.
  uint32 AllocateComponentID();

  // Allocates a unique input context id.
  uint32 AllocateInputContextID();

  // For testing purpose.
  bool IsConnectorAttached(Connector* connector) const {
    return connector && connectors_.count(connector);
  }

  bool RegisterComponents(Connector* connector, proto::Message* message);
  bool DeregisterComponents(Connector* connector, proto::Message* message);

  // Handles MSG_ATTACH_TO_INPUT_CONTEXT messages' reply messages.
  bool OnMsgAttachToInputContextReply(Component* source,
                                      proto::Message* message);

  bool OnMsgQueryComponent(Component* source, proto::Message* message);

  // TODO(suzhe):

  // MSG_SET_COMMAND_LIST,
  // MSG_CLEAR_COMMAND_LIST,
  // MSG_UPDATE_COMMANDS,
  // MSG_QUERY_COMMAND_LIST,

  // Dispatches a message to the active consumer component attached to the
  // target input context.
  bool DispatchToActiveConsumer(Component* source, proto::Message* message);

  // Broadcasts a specified message. The message will be deleted by this method.
  // Returns false if any error occurs. The message won't be sent to the
  // |exclude_component|.
  bool BroadcastMessage(proto::Message* message, uint32 exclude_component);

  bool BroadcastMessage(proto::Message* message) {
    return BroadcastMessage(message, kComponentDefault);
  }

  // Helper functions
  static proto::Message* NewMessage(uint32 type, uint32 target, uint32 icid);

  static bool CanComponentProduce(const Component* component,
                                  const proto::Message* message);

  static bool CanComponentConsume(const Component* component,
                                  const proto::Message* message);

  // Counter for generating unique component id.
  uint32 component_counter_;

  // Counter for generating unique input context id.
  uint32 input_context_counter_;

  // Id of current focused input context. kInputContextNone means no input
  // context is focused.
  uint32 focused_input_context_;

  typedef std::map<uint32, Component*> ComponentMap;

  // Component id -> Component object map.
  ComponentMap components_;

  typedef std::map<std::string, Component*> ComponentStringIDMap;

  // Component string id -> Component object map.
  ComponentStringIDMap components_by_string_id_;

  typedef std::map<uint32, InputContext*> InputContextMap;

  // Input context id -> InputContext object map.
  InputContextMap input_contexts_;

  typedef std::map<Connector*, std::set<uint32> > ConnectorMap;

  // Connector pointer -> component ids owned by it.
  ConnectorMap connectors_;

  // The special Component object representing the Hub itself.
  Component* hub_component_;

  // The special InputContext object represending kInputContextNone.
  InputContext* hub_input_context_;

  // A built-in component for handling input context related messages.
  scoped_ptr<HubInputContextManager> input_context_manager_;

  // A built-in component for managing input method components.
  scoped_ptr<HubInputMethodManager> input_method_manager_;

  // A built-in component for handling hotkey related messages.
  scoped_ptr<HubHotkeyManager> hotkey_manager_;

  // A built-in component for handling command list related messages.
  scoped_ptr<HubCommandListManager> command_list_manager_;

  // A built-in component for handling composition text and candidate list
  // related messages.
  scoped_ptr<HubCompositionManager> composition_manager_;

  DISALLOW_COPY_AND_ASSIGN(HubImpl);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_IMPL_H_
