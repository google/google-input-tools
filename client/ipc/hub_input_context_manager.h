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

#ifndef GOOPY_IPC_HUB_INPUT_CONTEXT_MANAGER_H_
#define GOOPY_IPC_HUB_INPUT_CONTEXT_MANAGER_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"

namespace ipc {
namespace hub {

class HubImpl;
class Component;

// A built-in component for handling input context related messages.
class HubInputContextManager : public Hub::Connector {
 public:
  explicit HubInputContextManager(HubImpl* hub);
  virtual ~HubInputContextManager();

  // Implementation of Hub::Connector interface
  virtual bool Send(proto::Message* message) OVERRIDE;

 private:
  bool OnMsgCreateInputContext(Component* source, proto::Message* message);
  bool OnMsgDeleteInputContext(Component* source, proto::Message* message);
  bool OnMsgAttachToInputContext(Component* source, proto::Message* message);
  bool OnMsgDetachFromInputContext(Component* source, proto::Message* message);

  bool OnMsgQueryInputContext(Component* source, proto::Message* message);
  bool OnMsgFocusInputContext(Component* source, proto::Message* message);
  bool OnMsgBlurInputContext(Component* source, proto::Message* message);

  bool OnMsgActivateComponent(Component* source, proto::Message* message);
  bool OnMsgAssignActiveConsumer(Component* source, proto::Message* message);
  bool OnMsgResignActiveConsumer(Component* source, proto::Message* message);
  bool OnMsgQueryActiveConsumer(Component* source, proto::Message* message);
  bool OnMsgRequestConsumer(Component* source, proto::Message* message);

  bool OnMsgUpdateInputCaret(Component* source, proto::Message* message);
  bool OnMsgQueryInputCaret(Component* source, proto::Message* message);

  // The Component object representing the input context manager itself.
  Component* self_;

  // Weak pointer to our owner.
  HubImpl* hub_;

  DISALLOW_COPY_AND_ASSIGN(HubInputContextManager);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_INPUT_CONTEXT_MANAGER_H_
