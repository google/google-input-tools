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

#ifndef GOOPY_IPC_COMPONENT_H_
#define GOOPY_IPC_COMPONENT_H_
#pragma once

#include "base/basictypes.h"

namespace ipc {

namespace proto {
class ComponentInfo;
class Message;
}

class ComponentHost;

// An interface class for implementing a component. All methods defined in this
// interface will be guaranteed to run synchronously.
class Component {
 public:
  virtual ~Component() {}

  // Stores the information of this component to |*info|. No ComponentHost
  // method should be called within this method, otherwise deadlock may happen.
  virtual void GetInfo(proto::ComponentInfo* info) = 0;

  // Handles an incoming message. Ownership of the message will be transfered to
  // the component.
  virtual void Handle(proto::Message* message) = 0;

  // Called when the component has been registered to Hub successfully or
  // failed to register.
  // When success, |component_id| is a unique id allocated by Hub, otherwise
  // it's kComponentDefault, indicating the component cannot be registered.
  virtual void Registered(uint32 component_id) = 0;

  // Called when the component has been deregistered from Hub successfully.
  // This method might be called from a different thread than the one running
  // the component.
  virtual void Deregistered() = 0;

  // Called when the component is added to a ComponentHost object. This method
  // should only be called by the |host| object when |host->AddComponent()|
  // method is called with the component. No method of |host| should be called
  // within this method, except for remembering the |host| pointer.
  // This method might be called from a different thread than the one running
  // the component.
  virtual void DidAddToHost(ComponentHost* host) = 0;

  // Called when the component is removed from the ComponentHost object
  // previously hosting this component. This method should only be called by the
  // ComponentHost object when its RemoveComponent() method is called with the
  // component, or when it's destroyed. No method of the remembered host object
  // should be called within this method.
  // This method might be called from a different thread than the one running
  // the component.
  virtual void DidRemoveFromHost() = 0;
};

}  // namespace ipc

#endif  // GOOPY_IPC_COMPONENT_H_
