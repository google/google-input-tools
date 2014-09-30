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

#include "components/plugin_wrapper/plugin_instance.h"

#include <windows.h>
#include "base/logging.h"
#include "base/string_utils_win.h"
#include "components/plugin_wrapper/exports.h"

namespace ime_goopy {
namespace components {
namespace {
HMODULE GetHMODULE(void* handle) {
  return reinterpret_cast<HMODULE>(handle);
}
}  // namespace

PluginInstance::PluginInstance(const std::string& path)
    : handle_(reinterpret_cast<void*>(
          ::LoadLibrary(Utf8ToWide(path.c_str()).c_str()))),
      list_components_(NULL),
      create_instance_(NULL),
      destroy_instance_(NULL),
      get_component_info_(NULL),
      handle_message_(NULL),
      free_buffer_(NULL),
      registered_(NULL),
      deregistered_(NULL),
      initialized_(false) {
  if (handle_) {
    list_components_ =
        GetProcAddress<ListComponentsProc>(ListComponentsProcName);
    DCHECK(list_components_);
    create_instance_= GetProcAddress<CreateInstanceProc>(
        kCreateInstanceProcName);
    DCHECK(create_instance_);
    destroy_instance_= GetProcAddress<DestroyInstanceProc>(
        kDestroyInstanceProcName);
    DCHECK(destroy_instance_);
    get_component_info_=
        GetProcAddress<GetInfoProc>(kGetInfoProcName);
    DCHECK(get_component_info_);
    handle_message_= GetProcAddress<HandleMessageProc>(kHandleMessageProcName);
    DCHECK(handle_message_);
    free_buffer_ = GetProcAddress<FreeBufferProc>(kFreeBufferProcName);
    DCHECK(free_buffer_);
    registered_ = GetProcAddress<RegisteredProc>(kRegisteredProcName);
    DCHECK(registered_);
    deregistered_ = GetProcAddress<DeregisteredProc>(kDeregisteredProcName);
    DCHECK(deregistered_);
    initialized_ = list_components_ &&
                   create_instance_ &&
                   destroy_instance_ &&
                   get_component_info_ &&
                   handle_message_ &&
                   free_buffer_ &&
                   registered_ &&
                   deregistered_;
  }
}

PluginInstance::~PluginInstance() {
  if (handle_)
    ::FreeLibrary(GetHMODULE(handle_));
}

void* PluginInstance::GetProcAddress(const char* proc_name) {
  if (handle_)
     return ::GetProcAddress(GetHMODULE(handle_), proc_name);
  return NULL;
}

bool PluginInstance::IsInitialized() {
  return initialized_;
}

int PluginInstance::ListComponents(ipc::proto::MessagePayload* payload) {
  DCHECK(IsInitialized());
  DCHECK(payload);
  if (!IsInitialized() || !payload)
    return 0;
  char* buf = NULL;
  int size = 0;
  int count = (*list_components_)(&buf, &size);
  if (!buf || !size || !count) {
    (*free_buffer_)(buf);  // Freeing NULL will be acceptable.
    return 0;
  }
  bool success = payload->ParseFromArray(buf, size);
  (*free_buffer_)(buf);
  DCHECK(success);
  DCHECK_EQ(payload->component_info_size(), count);
  if (!success || payload->component_info_size() != count) {
    payload->Clear();
    return 0;
  }
  return count;
}

ComponentInstance PluginInstance::CreateInstance(ComponentCallbacks callbacks,
                                                 const char* id) {
  DCHECK(initialized_);
  if (initialized_)
    return (*create_instance_)(callbacks, id);
  return NULL;
}

void PluginInstance::DestroyInstance(ComponentInstance instance) {
  DCHECK(initialized_);
  if (initialized_)
    (*destroy_instance_)(instance);
}

void PluginInstance::GetInfo(ComponentInstance instance,
                             char** buffer,
                             int* buffer_length) {
  DCHECK(initialized_);
  if (initialized_)
    (*get_component_info_)(instance, buffer, buffer_length);
}

void PluginInstance::Registered(ComponentInstance instance, int id) {
  DCHECK(initialized_);
  if (initialized_)
    (*registered_)(instance, id);
}

void PluginInstance::Deregistered(ComponentInstance instance) {
  DCHECK(initialized_);
  if (initialized_)
    (*deregistered_)(instance);
}

void PluginInstance::HandleMessage(ComponentInstance instance,
                                   const char* message_buffer,
                                   int buffer_length) {
  DCHECK(initialized_);
  if (initialized_)
    (*handle_message_)(instance, message_buffer, buffer_length);
}

void PluginInstance::FreeBuffer(char* buffer) {
  DCHECK(initialized_);
  if (initialized_)
    (*free_buffer_)(buffer);
}

}  // namespace components
}  // namespace ime_goopy
