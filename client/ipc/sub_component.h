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

#ifndef GOOPY_IPC_SUB_COMPONENT_H_
#define GOOPY_IPC_SUB_COMPONENT_H_

namespace ipc {

namespace proto {
class ComponentInfo;
class Message;
}

// An interface class for implementing a sub component. A sub component is not a
// component but could be a part of a component which is designed to help the
// owner component to handle one group of messages.
class SubComponent {
 public:
  virtual ~SubComponent() {}

  // Stores the information of this sub component to |*info|.
  // The information includes messages may produced and comsumed by this sub
  // component.
  // The ownership of |info| will stay with the caller.
  virtual void GetInfo(proto::ComponentInfo* info) = 0;

  // Handles an incoming message. Ownership of this message will be
  // transfered to the sub component only if the message was consumed by
  // current sub component.
  // Returns true if the message is consumed by the sub component.
  virtual bool Handle(proto::Message* message) = 0;

  // Called when the owner component has been registered to Hub successfully or
  // failed to register.
  virtual void OnRegistered() = 0;

  // Called when the comonent has been deregistered from Hub successfully.
  // This method might be called from a different thread than the one running
  // the sub component.
  virtual void OnDeregistered() = 0;
};

}  // namespace ipc

#endif  // GOOPY_IPC_SUB_COMPONENT_H_
