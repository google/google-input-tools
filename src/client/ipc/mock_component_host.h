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

#ifndef GOOPY_IPC_MOCK_COMPONENT_HOST_H_
#define GOOPY_IPC_MOCK_COMPONENT_HOST_H_

#include <queue>
#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/component_host.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

// A mock ComponentHost implementation for unit tests. This class can only
// contain one component.
class MockComponentHost : public ComponentHost {
 public:
  // An arbitrary id for the hosted component.
  static const uint32 kMockComponentId = 1234;

  MockComponentHost();
  virtual ~MockComponentHost();

  // Overridden from ComponentHost:
  virtual bool AddComponent(Component* component) OVERRIDE;
  virtual bool RemoveComponent(Component* component) OVERRIDE;
  virtual bool Send(Component* component,
                    proto::Message* message,
                    uint32* serial) OVERRIDE;
  virtual bool SendWithReply(Component* component,
                             proto::Message* message,
                             int timeout,
                             proto::Message** reply) OVERRIDE;
  virtual void PauseMessageHandling(Component* component) OVERRIDE;
  virtual void ResumeMessageHandling(Component* component) OVERRIDE;

  Component* component() const {
    return component_;
  }

  uint32 id() const {
    return info_.id();
  }

  const std::string& string_id() const {
    return info_.string_id();
  }

  const proto::ComponentInfo& info() const {
    return info_;
  }

  // Waits for an outgoing message sent by the component. Returns true if an
  // outgoing message is available within |timeout| milliseconds.
  // This method should only be used if the component may send messages from a
  // different thread than the one running the component.
  bool WaitOutgoingMessage(int timeout);

  // Returns the first message in the outgoing message queue, or NULL if the
  // queue is empty.
  proto::Message* PopOutgoingMessage();

  // Sets the reply message that will be returned by the next call to
  // SendWithReply() method.
  void SetNextReplyMessage(proto::Message* message);

  // Calls |component_->Handle()| to handle a message.
  // Returns false if the |component_| cannot handle the message.
  bool HandleMessage(proto::Message* message);

  // Checks if |component_| may produce the given message type.
  bool MayProduce(uint32 message_type);

  // Checks if |component_| can consume the given message type.
  bool CanConsume(uint32 message_type);

  // Checks if message handling is paused.
  bool IsMessageHandlingPaused();

 private:
  // Initializes |info_| by calling component_->GetInfo(). Returns true if the
  // content of |info_| is valid.
  bool InitInfo(Component* component);

  // Puts |message| into |outgoing_messages_| and sets |*serial| when necessary.
  // This method should be called when |lock_| is already locked.
  void SendUnlocked(proto::Message* message, uint32* serial);

  Component* component_;

  // A queue for storing outgoing messages sent by Send() and SendWithReply()
  // method.
  std::queue<proto::Message*> outgoing_messages_;
  base::WaitableEvent outgoing_event_;

  scoped_ptr<proto::Message> next_reply_message_;
  proto::ComponentInfo info_;
  std::set<uint32> produce_messages_;
  std::set<uint32> consume_messages_;

  // Counter for generating message serial number.
  uint32 serial_count_;

  int pause_count_;

  base::Lock message_lock_;
  base::Lock component_lock_;

  DISALLOW_COPY_AND_ASSIGN(MockComponentHost);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MOCK_COMPONENT_HOST_H_
