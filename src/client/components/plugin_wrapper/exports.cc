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

#include "components/plugin_wrapper/exports.h"

#ifdef OS_WINDOWS
#include <winnt.h>
#else
#include "common/app_utils_posix.h"
#endif
#include "components/plugin_wrapper/plugin_component_host.h"
#include "components/plugin_wrapper/plugin_definition.h"
#include "ipc/component.h"
#include "ipc/protos/ipc.pb.h"

// Dll interfaces.
extern "C" {
int API_CALL ListComponents(char** buffer, int* size) {
  ipc::proto::MessagePayload payload;
  int component_count = GetAvailableComponentInfos(&payload);
  std::string buffer_str;
  payload.SerializeToString(&buffer_str);
  *size = buffer_str.size();
  *buffer = new char[*size];
  memcpy(*buffer, buffer_str.c_str(), *size);
  return component_count;
}

ComponentInstance API_CALL CreateInstance(ComponentCallbacks callbacks,
                                          const char* id) {
  ipc::Component* component = CreateComponent(id);
  ime_goopy::components::PluginComponentAdaptor* host =
      new ime_goopy::components::PluginComponentAdaptor(callbacks, component);
  return reinterpret_cast<ComponentInstance>(host);
}

void API_CALL DestroyInstance(ComponentInstance instance) {
  DCHECK(instance);
  delete reinterpret_cast<ime_goopy::components::PluginComponentAdaptor*>(
      instance);
}

void API_CALL GetInfo(ComponentInstance instance,
                      char** buffer,
                      int* buffer_length) {
  DCHECK(instance);
  reinterpret_cast<ime_goopy::components::PluginComponentAdaptor*>(
      instance)->GetComponentInfo(buffer, buffer_length);
}

void API_CALL Registered(ComponentInstance instance, int id) {
  DCHECK(instance);
  reinterpret_cast<ime_goopy::components::PluginComponentAdaptor*>(
      instance)->Registered(id);
}

void API_CALL Deregistered(ComponentInstance instance) {
  DCHECK(instance);
  reinterpret_cast<ime_goopy::components::PluginComponentAdaptor*>(
      instance)->Deregistered();
}

void API_CALL HandleMessage(ComponentInstance instance,
                            const char* message_buffer,
                            int buffer_length) {
  DCHECK(instance);
  reinterpret_cast<ime_goopy::components::PluginComponentAdaptor*>(
      instance)->HandleMessage(message_buffer, buffer_length);
}

void API_CALL FreeBuffer(char* buffer) {
  delete[] buffer;
}
};
