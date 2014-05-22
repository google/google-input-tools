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

#ifndef GOOPY_IPC_COMPONENT_HOST_H_
#define GOOPY_IPC_COMPONENT_HOST_H_
#pragma once

#include "base/basictypes.h"

namespace ipc {

namespace proto {
class Message;
}

class Component;

// An interface for implementing classes to host components. It supports:
// 1. Component registration/deregistration
// 2. Asynchronous/synchronous message sending
class ComponentHost {
 public:
  virtual ~ComponentHost() {}

  // Adds a component to the host. The component will be registered to Hub
  // automatically. It will fail if the component is already added or the host
  // is not able to host more component.
  // Returns true if success.
  virtual bool AddComponent(Component* component) = 0;

  // Removes a component from the host. The component will be deregistered from
  // Hub automatically.
  // Returns true if success.
  virtual bool RemoveComponent(Component* component) = 0;

  // Sends a message without waiting for reply. The |message| will be deleted
  // automatically. If the message is not a reply message, then a unique serial
  // number will be allocated to the message and stored in |*serial|.
  // Returns true if success.
  virtual bool Send(Component* component,
                    proto::Message* message,
                    uint32* serial) = 0;

  // Sends a message and waits for reply. The |message| will be deleted
  // automatically. Reply message will be stored in |*reply|.
  // If no message is received within |timeout| milliseconds, NULL will be
  // stored in |*reply| and false will be returned. |timeout| < 0 means
  // unlimited timeout.
  virtual bool SendWithReply(Component* component,
                             proto::Message* message,
                             int timeout,
                             proto::Message** reply) = 0;

  // Asks the component host to stop dispatching incoming messages to the
  // specified component.
  // All incoming messages will be cached inside the component host and will be
  // dispatched to the component later when message dispatching is resumed by
  // calling ResumeMessageHandling().
  //
  // This method is useful if a component does not want other messages to be
  // handled recursively during a SendWithReply() call. In such case, the
  // component can just call PauseMessageHandling() and ResumeMessageHandling()
  // before and after calling SendWithReply().
  //
  // This method can be called for many times, but in order to resume message
  // handing, ResumeMessageHandling() must be called for exact same times.
  // SendWithReply() is not affected, which will still get the reply message
  // it's waiting.
  virtual void PauseMessageHandling(Component* component) = 0;

  // Asks the component host to resume dispatching incoming messages to the
  // specified component. This method must be called for exact same times as
  // PauseMessageHandling() to actually resume message disaptching.
  virtual void ResumeMessageHandling(Component* component) = 0;

  // Waits for the components added to the host to be registered.
  // This function will block the calling thread until all components added are
  // all registered to the hub, or the timeout expires.
  // |*timeout| is number of milliseconds to wait before return, if all
  // components are registered in time, then remained time will be stored in
  // |*timeout| and true will be returned. Otherwise |*timeout| will be set to
  // zero and false will be returned.
  virtual bool WaitForComponents(int* timeout) {
    return true;
  }
  // If multi_component host is waiting for components added to the host to be
  // registered by calling "WaitForComponents", this method is called to quit
  // the waiting and let "WaitForComponents" returns.
  // If not, calling this method do nothing.
  // It's called when the process is quiting.
  // Use this method with caustion because |WaitForComponents| will return true
  // on receiving this quit signal with the components may not ready yet.
  virtual void QuitWaitingComponents() {
  }
};

}  // namespace ipc

#endif  // GOOPY_IPC_COMPONENT_HOST_H_
