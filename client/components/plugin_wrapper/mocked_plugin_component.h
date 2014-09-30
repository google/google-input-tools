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

#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_MOCKED_PLUGIN_COMPONENT_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_MOCKED_PLUGIN_COMPONENT_H_

#include "ipc/component_base.h"
#include "ipc/message_types.h"

namespace ime_goopy {
namespace components {

// A mock component for testing plugin wrapper.
class MockedPluginComponent : public ipc::ComponentBase {
 public:
  explicit MockedPluginComponent(const std::string& id);
  virtual ~MockedPluginComponent();
  // As the component will be encapsulated in a plug, we can not touch it
  // directly in the test so the component can only communicate with the test
  // program by sending and receiving messages. So we define a set of internal
  // messages to test the plug wrapper.
  enum Internal_Messages {
    // Test sending and receiving messages.
    MSG_TEST_MESSAGE = ipc::MSG_SYSTEM_RESERVED_START,
    // A message to let MockedPluginComponent send a message.
    MSG_REQUEST_SEND,
    // A message to let MockedPluginComponent send a message with reply.
    MSG_REQUEST_SEND_WITH_REPLY,
    // A message to let MockedPluginComponent pause message handling.
    MSG_REQUEST_PAUSE_MESSAGE_HANDLING,
    // A message to let MockedPluginComponent resume message handling.
    MSG_REQUEST_RESUME_MESSAGE_HANDLING,
    // A message sent by MockedPluginComponent when received
    // MSG_REQUEST_SEND_WITH_REPLY or MSG_REQUEST_SEND_WITH_REPLY_BLOCKED.
    MSG_TEST_SEND_WITH_REPLY
  };
  // Overridden from Component:
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

 private:
  bool SendMessageWithReply(bool blocked);

  std::string id_;
  DISALLOW_COPY_AND_ASSIGN(MockedPluginComponent);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_MOCKED_PLUGIN_COMPONENT_H_
