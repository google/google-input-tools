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

#include "components/plugin_manager/plugin_manager_component.h"

#include "base/stringprintf.h"
#include "common/app_const.h"
#include "components/common/file_utils.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"

#ifdef OS_WINDOWS
#include "common/app_utils.h"
#include "common/registry.h"
#include "components/plugin_manager/registry_monitor_wrapper.h"
#else
#include "common/windows_types.h"
#include "common/app_utils_posix.h"
#include "base/logging.h"
#endif

namespace ime_goopy {
namespace components {

const int kProduceMessages[] = {
  ipc::MSG_PLUGIN_CHANGED,
};

const int kConsumeMessages[] = {
  ipc::MSG_PLUGIN_QUERY_COMPONENTS,
  ipc::MSG_PLUGIN_START_COMPONENTS,
  ipc::MSG_PLUGIN_STOP_COMPONENTS,
  ipc::MSG_PLUGIN_UNLOAD,
  ipc::MSG_PLUGIN_INSTALLED,
};

static const char kStringID[] = "com.google.input_tools.plugin_manager";

PluginManagerComponent::PluginManagerComponent() {
}

PluginManagerComponent::~PluginManagerComponent() {
}

void PluginManagerComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(kStringID);
  for (size_t i = 0; i < arraysize(kProduceMessages) ; ++i)
    info->add_produce_message(kProduceMessages[i]);
  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

void PluginManagerComponent::OnRegistered() {
  manager_.reset(new PluginManager(
      FileUtils::GetSystemPluginPath(), host(), this));
#ifdef OS_WINDOWS
  scoped_ptr<RegistryKey> parent(AppUtils::OpenSystemRegistry(true));
  RegistryMonitorWrapper* monitor_ = new RegistryMonitorWrapper(
      parent->Detach(), kPluginRegistryKey, manager_.get());
  manager_->AddMonitor(monitor_);
#endif
  manager_->Init();
}

void PluginManagerComponent::OnDeregistered() {
  manager_.reset(NULL);
}

void PluginManagerComponent::Handle(ipc::proto::Message* message) {
  switch (message->type()) {
    case ::ipc::MSG_PLUGIN_QUERY_COMPONENTS:
      OnMsgPluginQueryComponents(message);
      break;
    case ::ipc::MSG_PLUGIN_START_COMPONENTS:
      OnMsgPluginStartComponents(message);
      break;
    case ::ipc::MSG_PLUGIN_STOP_COMPONENTS:
      OnMsgPluginStopComponents(message);
      break;
    case ::ipc::MSG_PLUGIN_UNLOAD:
      OnMsgPluginUnload(message);
      break;
    case ::ipc::MSG_PLUGIN_INSTALLED:
      OnMsgPluginInstalled(message);
      break;
    default:
      DLOG(ERROR) <<"Error message: " << ipc::GetMessageName(message->type())
                  <<" in PluginManagerComponent";
  }
}

void PluginManagerComponent::OnMsgPluginQueryComponents(
    ipc::proto::Message* message) {
  DCHECK(manager_.get());
  scoped_ptr<ipc::proto::Message> mptr(message);
  if (!ipc::MessageNeedReply(mptr.get()))
    return;
  ipc::ConvertToReplyMessage(mptr.get());
  mptr->mutable_payload()->Clear();
  manager_->GetComponents(mptr->mutable_payload()->mutable_component_info());
  Send(mptr.release(), NULL);
}

void PluginManagerComponent::OnMsgPluginStartComponents(
    ipc::proto::Message* message) {
  DCHECK(manager_.get());
  scoped_ptr<ipc::proto::Message> mptr(message);
  if (!ipc::MessageNeedReply(mptr.get()))
    return;
  ipc::ConvertToReplyMessage(mptr.get());
  if (mptr->has_payload()) {
    for (int i = 0; i < mptr->payload().string_size(); ++i) {
      mptr->mutable_payload()->add_boolean(
          manager_->StartComponent(mptr->payload().string(i)));
    }
  }
  mptr->mutable_payload()->clear_string();
  Send(mptr.release(), NULL);
}

void PluginManagerComponent::OnMsgPluginStopComponents(
    ipc::proto::Message* message) {
  DCHECK(manager_.get());
  scoped_ptr<ipc::proto::Message> mptr(message);
  if (!ipc::MessageNeedReply(mptr.get()))
    return;
  ipc::ConvertToReplyMessage(mptr.get());
  if (mptr->has_payload()) {
    for (int i = 0; i < mptr->payload().string_size(); ++i) {
      mptr->mutable_payload()->add_boolean(
          manager_->StopComponent(mptr->payload().string(i)));
    }
  }
  mptr->mutable_payload()->clear_string();
  Send(mptr.release(), NULL);
}

void PluginManagerComponent::OnMsgPluginUnload(
    ipc::proto::Message* message) {
  DCHECK(manager_.get());
  if (message->has_payload() && message->payload().string_size())
    manager_->UnloadPlugin(message->payload().string(0));
  ReplyTrue(message);
}

void PluginManagerComponent::OnMsgPluginInstalled(
    ipc::proto::Message* message) {
  DCHECK(manager_.get());
  manager_->PluginChanged();
  ReplyTrue(message);
}

void PluginManagerComponent::PluginComponentChanged() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_PLUGIN_CHANGED,
      ipc::kInputContextNone,
      FALSE));
  mptr->set_target(ipc::kComponentBroadcast);
  Send(mptr.release(), NULL);
}

}  // namespace components
}  // namespace ime_goopy
