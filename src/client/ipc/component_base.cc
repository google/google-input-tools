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

#include "ipc/component_base.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/component_host.h"
#include "ipc/constants.h"
#include "ipc/message_util.h"
#include "ipc/sub_component.h"

namespace ipc {

ComponentBase::ComponentBase()
    : host_(NULL),
      id_(kComponentDefault) {
}

ComponentBase::~ComponentBase() {
  // The component must be removed from the host first before being destroyed.
  DCHECK(!host_);
  std::vector<SubComponent*>::iterator iter = subcomponent_list_.begin();
  for (; iter != subcomponent_list_.end(); ++iter)
    delete (*iter);
}

void ComponentBase::Registered(uint32 component_id) {
  DLOG(WARNING) << "Component registered:" << component_id;
  DCHECK(host_);
  DCHECK(id_ == kComponentDefault);
  DCHECK(component_id != kComponentDefault);
  id_ = component_id;

  std::vector<SubComponent*>::iterator iter = subcomponent_list_.begin();
  for (; iter != subcomponent_list_.end(); ++iter)
    (*iter)->OnRegistered();
  OnRegistered();
}

void ComponentBase::Deregistered() {
  DLOG(WARNING) << "Component deregistered:" << id_;
  DCHECK(host_);
  id_ = kComponentDefault;

  std::vector<SubComponent*>::iterator iter = subcomponent_list_.begin();
  for (; iter != subcomponent_list_.end(); ++iter)
    (*iter)->OnDeregistered();
  OnDeregistered();
}

void ComponentBase::DidAddToHost(ComponentHost* host) {
  DCHECK(!host_);
  DCHECK(host);
  DCHECK(id_ == kComponentDefault);
  host_ = host;
}

void ComponentBase::DidRemoveFromHost() {
  DCHECK(host_);
  DCHECK(id_ == kComponentDefault);
  host_ = NULL;
}

bool ComponentBase::RemoveFromHost() {
  return host_ && host_->RemoveComponent(this);
}

bool ComponentBase::Send(proto::Message* message, uint32* serial) {
  DCHECK(host_);
  return host_->Send(this, message, serial);
}

bool ComponentBase::SendWithReply(proto::Message* message,
                                  int timeout,
                                  proto::Message** reply) {
  DCHECK(host_);
  return host_->SendWithReply(this, message, timeout, reply);
}

void ComponentBase::PauseMessageHandling() {
  DCHECK(host_);
  host_->PauseMessageHandling(this);
}

void ComponentBase::ResumeMessageHandling() {
  DCHECK(host_);
  host_->ResumeMessageHandling(this);
}

bool ComponentBase::SendWithReplyNonRecursive(proto::Message* message,
                                              int timeout,
                                              proto::Message** reply) {
  DCHECK(host_);
  host_->PauseMessageHandling(this);
  const bool result = host_->SendWithReply(this, message, timeout, reply);
  host_->ResumeMessageHandling(this);
  return result;
}

void ComponentBase::ReplyBoolean(proto::Message* message, bool value) {
  DCHECK(host_);
  if (message->reply_mode() != proto::Message::NEED_REPLY) {
    delete message;
    return;
  }
  ConvertToBooleanReplyMessage(message, value);
  host_->Send(this, message, NULL);
}

void ComponentBase::ReplyError(proto::Message* message,
                               proto::Error::Code error_code,
                               const char* error_message) {
  DCHECK(host_);
  if (message->reply_mode() != proto::Message::NEED_REPLY) {
    delete message;
    return;
  }
  ConvertToErrorReplyMessage(message, error_code, error_message);
  host_->Send(this, message, NULL);
}

proto::Message* ComponentBase::NewMessage(uint32 type,
                                          uint32 icid,
                                          bool need_reply) {
  return ipc::NewMessage(type, id_, kComponentDefault, icid, need_reply);
}

void ComponentBase::AddSubComponent(SubComponent* sub_component) {
  DCHECK(!host_);
  DCHECK(sub_component);
  subcomponent_list_.push_back(sub_component);
}

void ComponentBase::GetSubComponentsInfo(proto::ComponentInfo* info) {
  std::vector<SubComponent*>::iterator iter = subcomponent_list_.begin();
  for (; iter != subcomponent_list_.end(); ++iter)
    (*iter)->GetInfo(info);
}

bool ComponentBase::HandleMessageBySubComponents(proto::Message* message) {
  std::vector<SubComponent*>::iterator iter = subcomponent_list_.begin();
  for (; iter != subcomponent_list_.end(); ++iter) {
    if ((*iter)->Handle(message))
      return true;
  }
  return false;
}

}  // namespace ipc
