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

#include "components/plugin_wrapper/mocked_plugin_component.h"

#include "ime/shared/base/logging.h"
#include "base/scoped_ptr.h"
#include "components/plugin_wrapper/plugin_definition.h"
#include "ipc/component_host.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"

namespace {
#ifdef COMPONENT_ID_PREFIX
static const char kComponentIDPrefix[] = COMPONENT_ID_PREFIX;
#else
static const char kComponentIDPrefix[] = "";
#endif
}  // namespace

int GetAvailableComponentInfos(ipc::proto::MessagePayload* payload) {
  std::string prefix = kComponentIDPrefix;
  scoped_ptr<ime_goopy::components::MockedPluginComponent> component(
      new ime_goopy::components::MockedPluginComponent(prefix + "component1"));
  payload->add_component_info();
  component->GetInfo(payload->mutable_component_info(0));
  component.reset(
      new ime_goopy::components::MockedPluginComponent(prefix + "component2"));
  payload->add_component_info();
  component->GetInfo(payload->mutable_component_info(1));
  return 2;  // Two components in the plugin;
}

ipc::Component* CreateComponent(const char* id) {
  return new ime_goopy::components::MockedPluginComponent(id);
}

bool IsAvailable(const char* id) {
  return true;
}

namespace ime_goopy {
namespace components {

const int kProduceMessages[] = {
  MockedPluginComponent::MSG_TEST_MESSAGE,
  MockedPluginComponent::MSG_TEST_SEND_WITH_REPLY
};

const int kConsumeMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  MockedPluginComponent::MSG_REQUEST_SEND,
  MockedPluginComponent::MSG_REQUEST_SEND_WITH_REPLY,
  MockedPluginComponent::MSG_REQUEST_PAUSE_MESSAGE_HANDLING,
  MockedPluginComponent::MSG_REQUEST_RESUME_MESSAGE_HANDLING,
};

MockedPluginComponent::MockedPluginComponent(const std::string& id) : id_(id) {
}

MockedPluginComponent::~MockedPluginComponent() {
}

void MockedPluginComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(id_);
  for (size_t i = 0; i < arraysize(kProduceMessages) ; ++i)
    info->add_produce_message(kProduceMessages[i]);
  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

void MockedPluginComponent::Handle(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr;
  bool success = true;
  switch (message->type()) {
    case MSG_TEST_MESSAGE:
      break;
    case MSG_REQUEST_SEND:
      mptr.reset(NewMessage(
          MSG_TEST_MESSAGE,
          ipc::kInputContextNone,
          false));
      Send(mptr.release(), NULL);
      break;
    case MSG_REQUEST_SEND_WITH_REPLY:
      SendMessageWithReply(false);
      break;
    case MSG_REQUEST_PAUSE_MESSAGE_HANDLING:
      PauseMessageHandling();
      break;
    case MSG_REQUEST_RESUME_MESSAGE_HANDLING:
      ResumeMessageHandling();
      break;
    default:
      DLOG(ERROR) << "Invalid Message type = " << message->type();
  }
  ReplyBoolean(message, success);
}

bool MockedPluginComponent::SendMessageWithReply(bool blocked) {
  scoped_ptr<ipc::proto::Message> mptr;
  mptr.reset(NewMessage(
      MSG_TEST_SEND_WITH_REPLY,
      ipc::kInputContextNone,
      true));
  ipc::proto::Message* reply = NULL;
  bool success = false;
  if (blocked)
    success = SendWithReplyNonRecursive(mptr.release(), -1, &reply);
  else
    success = SendWithReply(mptr.release(), -1, &reply);
  DCHECK(success);
  DCHECK(reply);
  DCHECK_EQ(MSG_TEST_SEND_WITH_REPLY, reply->type());
  DCHECK_EQ(ipc::proto::Message::IS_REPLY, reply->reply_mode());
  success = success && reply && reply->type() == MSG_TEST_SEND_WITH_REPLY &&
            ipc::proto::Message::IS_REPLY == reply->reply_mode();
  delete reply;
  return success;
}

}  // namespace components
}  // namespace ime_goopy
