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

#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_INSTANCE_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_INSTANCE_H_

#include <string>
#include "components/plugin_wrapper/exports.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
namespace components {

// An platform independent definition of plugin instance, which on windows
// should be a dll instance.
class PluginInstance {
 public:
  // Initialize the instance for the plugin file in |path|.
  explicit PluginInstance(const std::string& path);
  ~PluginInstance();
  // Wrapper of functions in exports.h
  // Lists the information of the components in the plugin.
  // Returns the count of the components in the plugin.
  int ListComponents(ipc::proto::MessagePayload* payload);
  ComponentInstance CreateInstance(ComponentCallbacks callbacks,
                                   const char* id);
  void DestroyInstance(ComponentInstance instance);
  void GetInfo(ComponentInstance instance,
               char** buffer,
               int* buffer_length);
  void Registered(ComponentInstance instance, int id);
  void Deregistered(ComponentInstance instance);
  void HandleMessage(ComponentInstance instance,
                     const char* message_buffer,
                     int buffer_length);
  void FreeBuffer(char* buffer);

  bool IsInitialized();

 private:
  void* GetProcAddress(const char* proc_name);
  template<typename ProcType>
  ProcType GetProcAddress(const char* proc_name) {
    return reinterpret_cast<ProcType>(this->GetProcAddress(proc_name));
  }
  void* handle_;
  ListComponentsProc list_components_;
  CreateInstanceProc create_instance_;
  DestroyInstanceProc destroy_instance_;
  GetInfoProc get_component_info_;
  HandleMessageProc handle_message_;
  FreeBufferProc free_buffer_;
  RegisteredProc registered_;
  DeregisteredProc deregistered_;
  bool initialized_;
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_INSTANCE_H_
