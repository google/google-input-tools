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

#ifndef GOOPY_IPC_COMPONENT_BASE_H_
#define GOOPY_IPC_COMPONENT_BASE_H_
#pragma once

#include <vector>

#include "base/compiler_specific.h"
#include "base/basictypes.h"
#include "ipc/component.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

class ComponentHost;
class SubComponent;

// A base class for implementing a component. It provides some utility methods.
class ComponentBase : public Component {
 public:
  ComponentBase();
  virtual ~ComponentBase();

  // Overridden from Component:
  virtual void Registered(uint32 component_id) OVERRIDE;
  virtual void Deregistered() OVERRIDE;
  virtual void DidAddToHost(ComponentHost* host) OVERRIDE;
  virtual void DidRemoveFromHost() OVERRIDE;

  ComponentHost* host() const { return host_; }
  uint32 id() const { return id_; }

  // Convenience method to call host_->RemoveComponent(this); Returns false if
  // |host_| is NULL or host_->RemoveComponent(this) returns false.
  bool RemoveFromHost();

  // Calls |host_->Send()|.
  bool Send(proto::Message* message, uint32* serial);

  // Calls |host_->SendWithReply()|.
  bool SendWithReply(proto::Message* message,
                     int timeout,
                     proto::Message** reply);

  // Calls |host_->PauseMessageHandling()|.
  void PauseMessageHandling();

  // Calls |host_->ResumeMessageHandling()|.
  void ResumeMessageHandling();

  // Calls:
  // host_->PauseMessageHandling();
  // host_->SendWithReply();
  // host_->ResumeMessageHandling();
  //
  // So that no other messages will be dispatched to the component recursively
  // before host_->SendWithReply() returns.
  bool SendWithReplyNonRecursive(proto::Message* message,
                                 int timeout,
                                 proto::Message** reply);

  // Adds sub component. This method should be called before the component
  // being added to a component host. The ownership of |sub_component| will be
  // transfered to ComponentBase after this function is called successfully.
  void AddSubComponent(SubComponent* sub_component);

 protected:
  // Called by Registered(). Derived classes may override this method to do
  // further initialization. If it's failed to register the component to Hub,
  // then this method will be called with id() == kComponentDefault;
  virtual void OnRegistered() {}

  // Called by Deregistered(). Derived classes my override this method  to do
  // further finalization.
  virtual void OnDeregistered() {}

  // Gets info of all sub components. Any inheritated class wants to use
  // sub components to handle one defined group of messages, should call
  // the GetSubComponentInfo in its own OVERRIDE function
  // GetInfo. Otherwise, the added sub component will not work.
  void GetSubComponentsInfo(proto::ComponentInfo* info);

  // Forwards the message to all sub components for handling. If |message| is
  // consumed by one sub component, the ownership of the message will be
  // transfered to this sub component. Otherwise, the caller is responsible
  // for deleting the |message|.
  // Returns true if the message is consumed by any sub component.
  bool HandleMessageBySubComponents(proto::Message* message);

  // If the given message needs reply, then turns it into a boolean reply
  // message and sends it out. The message will be deleted automatically.
  void ReplyBoolean(proto::Message* message, bool value);

  // Convenience method to reply a true.
  void ReplyTrue(proto::Message* message) {
    ReplyBoolean(message, true);
  }

  // Convenience method to reply a false.
  void ReplyFalse(proto::Message* message) {
    ReplyBoolean(message, false);
  }

  // If the given message needs reply, then turns it into an error reply
  // message and sends it out. The message will be deleted automatically.
  void ReplyError(proto::Message* message,
                  proto::Error::Code error_code,
                  const char* error_message);

  // Creates a new message.
  proto::Message* NewMessage(uint32 type, uint32 icid, bool need_reply);

 private:
  ComponentHost* host_;
  uint32 id_;
  std::vector<SubComponent*> subcomponent_list_;

  DISALLOW_COPY_AND_ASSIGN(ComponentBase);
};

}  // namespace ipc

#endif  // GOOPY_IPC_COMPONENT_BASE_H_
