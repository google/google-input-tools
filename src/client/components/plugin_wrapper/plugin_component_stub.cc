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

#include "components/plugin_wrapper/plugin_component_stub.h"

#include <string>

#include "base/logging.h"
#include "components/plugin_wrapper/callbacks.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"

namespace ime_goopy {
namespace components {
namespace {
extern "C" {
bool API_CALL SendProcedure(
    ComponentOwner owner,
    const char* message_buf, int length,
    uint32* serial) {
  scoped_ptr<ipc::proto::Message> mptr(new ipc::proto::Message);
  if (!mptr->ParseFromArray(message_buf, length))
    return false;
  return reinterpret_cast<ime_goopy::components::PluginComponentStub*>(
      owner)->Send(mptr.release(), serial);
}

bool API_CALL SendWithReplyProcedure(
    ComponentOwner owner,
    const char* message_buf, int length,
    int time_out,
    char** reply_buf, int* reply_length) {
  scoped_ptr<ipc::proto::Message> mptr(new ipc::proto::Message);
  if (!mptr->ParseFromArray(message_buf, length))
    return false;
  ipc::proto::Message* reply = NULL;
  ime_goopy::components::PluginComponentStub* component =
      reinterpret_cast<ime_goopy::components::PluginComponentStub*>(owner);
  if (!component->SendWithReply(mptr.release(), time_out, &reply))
    return false;
  std::string reply_str;
  if (!reply->SerializeToString(&reply_str)) {
    delete reply;
    return false;
  }
  *reply_buf = new char[reply_str.size()];
  *reply_length = reply_str.size();
  memcpy(*reply_buf, reply_str.c_str(), reply_str.size());
  return true;
}

void API_CALL PauseMessageHandlingProcedure(ComponentOwner owner) {
  reinterpret_cast<ime_goopy::components::PluginComponentStub*>(
      owner)->PauseMessageHandling();
}

void API_CALL ResumeMessageHandlingProcedure(ComponentOwner owner) {
  return reinterpret_cast<ime_goopy::components::PluginComponentStub*>(
      owner)->ResumeMessageHandling();
}

bool API_CALL RemoveComponentProcedure(ComponentOwner owner,
                                       ComponentInstance instance) {
  return reinterpret_cast<ime_goopy::components::PluginComponentStub*>(
      owner)->RemoveFromHost();
}

void API_CALL FreeBufferProcedure(char* buffer) {
  delete[] buffer;
}
}
}  // namespace

PluginComponentStub::PluginComponentStub(const std::string& dll_path,
                                         const char* id)
    : component_(NULL),
      plugin_instance_(dll_path.c_str()),
      initialized_(false) {
  if (!plugin_instance_.IsInitialized())
    return;
  ComponentCallbacks callbacks = {
    this,
    SendProcedure,
    SendWithReplyProcedure,
    PauseMessageHandlingProcedure,
    ResumeMessageHandlingProcedure,
    RemoveComponentProcedure,
    FreeBufferProcedure,
  };
  component_ = plugin_instance_.CreateInstance(callbacks, id);
  DCHECK(component_);
  initialized_ = component_ &&
                 plugin_instance_.IsInitialized();
}

PluginComponentStub::~PluginComponentStub() {
  if (component_)
    plugin_instance_.DestroyInstance(component_);
}

void PluginComponentStub::GetInfo(ipc::proto::ComponentInfo* info) {
  if (!initialized_) return;
  char* buffer = NULL;
  int size = 0;
  plugin_instance_.GetInfo(component_, &buffer, &size);
  info->ParseFromArray(buffer, size);
  plugin_instance_.FreeBuffer(buffer);
}

void PluginComponentStub::Handle(ipc::proto::Message* message) {
  if (!initialized_) {
    delete message;
    return;
  }
  std::string str;
  message->SerializeToString(&str);
  delete message;
  plugin_instance_.HandleMessage(component_, str.c_str(), str.size());
}

void PluginComponentStub::OnRegistered() {
  if (!initialized_) return;
  plugin_instance_.Registered(component_, id());
}

void PluginComponentStub::OnDeregistered() {
  if (!initialized_) return;
  plugin_instance_.Deregistered(component_);
}

bool PluginComponentStub::IsInitialized() {
  return initialized_;
}

}  // namespace ime_goopy
}  // namespace components
