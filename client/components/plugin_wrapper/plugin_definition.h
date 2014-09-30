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

// A set of functions that defines a plugin package. These functions should be
// implemented by the plugin developer.
#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_DEFINITION_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_DEFINITION_H_

#include "ipc/component.h"
#include "ipc/protos/ipc.pb.h"

// Gets the ComponentInfo objects of each available component in the plugin and
// stores them in |payload|. This function will be called by ListComponents.
// Returns the count of the available components.
int GetAvailableComponentInfos(ipc::proto::MessagePayload* payload);

// Creates an instance of a component identified by the null-terminated |id|.
// The caller will own the returned instance. These function will be called by
// CreateInstance.
// The component created by this function will be owned by the
// PluginComponentAdaptor.
ipc::Component* CreateComponent(const char* id);

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_DEFINITION_H_
