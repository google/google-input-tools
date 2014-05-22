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

#include "components/plugin_wrapper/plugin_component_host.h"

#include <string>

#include "google/protobuf/message.h"
#include "base/logging.h"
#include "common/debug.h"
#include "ipc/component.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
namespace components {

PluginComponentAdaptor::PluginComponentAdaptor(
    const ComponentCallbacks& callbacks,
    ipc::Component* component)
    : component_(component),
      callbacks_(callbacks),
      registered_(false) {
  DCHECK(component_.get());
  DCHECK(callbacks_.owner);
  DCHECK(callbacks_.send);
  DCHECK(callbacks_.send_with_reply);
  DCHECK(callbacks_.pause_message_handling);
  DCHECK(callbacks_.resume_message_handling);
  DCHECK(callbacks_.remove_component);
  DCHECK(callbacks_.free_buffer);
  component_->DidAddToHost(this);
}

PluginComponentAdaptor::~PluginComponentAdaptor() {
  if (registered_)
    Deregistered();
  component_->DidRemoveFromHost();
}

bool PluginComponentAdaptor::AddComponent(ipc::Component* component) {
  NOTREACHED() << "PluginComponentAdaptor::AddComponent cannot be called";
  return false;
}

bool PluginComponentAdaptor::RemoveComponent(ipc::Component* component) {
  NOTREACHED() << "PluginComponentAdaptor::RemoveComponent cannot be called";
  return false;
}

bool PluginComponentAdaptor::Send(ipc::Component* component,
                                  ipc::proto::Message* message,
                                  uint32* serial) {
  std::string message_str;
  message->SerializeToString(&message_str);
  delete message;
  return (*callbacks_.send)(callbacks_.owner,
                            message_str.c_str(),
                            message_str.size(),
                            serial);
}

bool PluginComponentAdaptor::SendWithReply(ipc::Component* component,
                                           ipc::proto::Message* message,
                                           int timeout,
                                           ipc::proto::Message** reply) {
  DCHECK(timeout);
  DCHECK(reply);
  std::string message_str;
  message->SerializeToString(&message_str);
  delete message;
  char* reply_buffer = NULL;
  int reply_size = 0;
  bool success = (*callbacks_.send_with_reply)(
      callbacks_.owner,
      message_str.c_str(), message_str.size(),
      timeout,
      &reply_buffer,
      &reply_size);
  if (!success || !reply_buffer || !reply_size) {
    (callbacks_.free_buffer)(reply_buffer);
    DLOG(ERROR) << __SHORT_FUNCTION__ << "SendWithReply failed";
    return false;
  }
  scoped_ptr<ipc::proto::Message> reply_message(new ipc::proto::Message);
  success = reply_message->ParseFromArray(reply_buffer, reply_size);
  (callbacks_.free_buffer)(reply_buffer);
  if (!success)
    reply_message.reset(NULL);
  *reply = reply_message.release();
  return success;
}

void PluginComponentAdaptor::PauseMessageHandling(ipc::Component* component) {
  (*callbacks_.pause_message_handling)(callbacks_.owner);
}

void PluginComponentAdaptor::ResumeMessageHandling(ipc::Component* component) {
  (*callbacks_.resume_message_handling)(callbacks_.owner);
}

void PluginComponentAdaptor::GetComponentInfo(char** buffer, int* size) {
  DCHECK(component_.get());
  DCHECK(buffer);
  DCHECK(!*buffer);
  DCHECK(size);
  ipc::proto::ComponentInfo info;
  component_->GetInfo(&info);
  std::string info_str;
  info.SerializeToString(&info_str);
  (*size) = info_str.size();
  (*buffer) = new char[*size + 1];
  memcpy(*buffer, info_str.c_str(), *size);
}

void PluginComponentAdaptor::HandleMessage(const char* buffer, int size) {
  scoped_ptr<ipc::proto::Message> mptr(new ipc::proto::Message());
  if (!mptr->ParseFromArray(buffer, size)) {
    DLOG(ERROR) << __SHORT_FUNCTION__ << L"Parsing message failed.";
    return;
  }
  DCHECK(component_.get() && registered_);
  component_->Handle(mptr.release());
}

void PluginComponentAdaptor::Registered(int id) {
  DCHECK_GT(id, ipc::kComponentDefault);
  DCHECK(component_.get());
  registered_ = true;
  component_->Registered(id);
}

void PluginComponentAdaptor::Deregistered() {
  DCHECK(component_.get());
  component_->Deregistered();
  registered_ = false;
}

}  // namespace components
}  // namespace ime_goopy
