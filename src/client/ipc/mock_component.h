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

#ifndef GOOPY_IPC_MOCK_COMPONENT_H_
#define GOOPY_IPC_MOCK_COMPONENT_H_
#pragma once

#include <queue>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "ipc/component_base.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

// A mock Component implementation for testing purpose.
class MockComponent : public ComponentBase {
 public:
  explicit MockComponent(const std::string& string_id);
  virtual ~MockComponent();

  // Overridden from Component:
  // Derived class should override GetInfo() to add more information to |info|.
  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(proto::Message* message) OVERRIDE;

  // Overridden from ComponentBase:
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;

  // Adds an outgoing message which will be sent out by the component when
  // either Handle(), OnRegistered() or OnDeregistered() gets called.
  // If the message requires reply message, then the component will wait for
  // the reply for |timeout| milliseconds. |expected_result| is the expected
  // result of ComponentHost's Send() or SendWithReply() or
  // SendWithReplyNonRecursive method if |non_recurisve| is true.
  void AddOutgoingMessage(proto::Message* message,
                          bool expected_result,
                          uint32 timeout);
  void AddOutgoingMessageWithMode(proto::Message* message,
                                  bool non_recursive,
                                  bool expected_result,
                                  uint32 timeout);

  // Waits for an incoming message sent to the component. Returns true if an
  // incoming message is received within |timeout| milliseconds.
  bool WaitIncomingMessage(int timeout);

  // Returns the next incoming message.
  proto::Message* PopIncomingMessage();

  const std::string& string_id() const { return string_id_; }

  base::PlatformThreadId thread_id() const { return thread_id_; }

  int handle_count() const { return handle_count_; }

 private:
  struct OutgoingMessage {
    proto::Message* message;
    bool non_recursive;
    bool expected_result;
    uint32 timeout;
  };

  // Processes one outgoing message.
  void ProcessOutgoingMessagesUnlocked();

  std::string string_id_;

  // A queue to store outgoing messages.
  std::queue<OutgoingMessage> outgoing_;

  // A queue to store incoming messages received by Handle() method.
  std::queue<proto::Message*> incoming_;

  base::Lock lock_;

  base::WaitableEvent incoming_event_;

  base::PlatformThreadId thread_id_;

  // Count of recursive calls to Handle() method.
  int handle_count_;

  DISALLOW_COPY_AND_ASSIGN(MockComponent);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MOCK_COMPONENT_H_
