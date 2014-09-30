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

#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_COMPONENT_HOST_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_COMPONENT_HOST_H_

#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "components/plugin_wrapper/callbacks.h"
#include "ipc/component_host.h"

namespace ipc {
class Component;
}  // namespace ipc

namespace ime_goopy {
namespace components {
// A plugin component adapter class that acts as a host for actual plugin
// components, and it is an adapter between actual plugin component and the
// PluginComponentStub. An PluginComponentAdaptor objects hosts one and only
// actual plugin components, and the hosted component can not be changed once
// added.
class PluginComponentAdaptor : public ipc::ComponentHost {
 public:
  PluginComponentAdaptor(const ComponentCallbacks& callbacks,
                         ipc::Component* component);
  virtual ~PluginComponentAdaptor();
  // Overridden from ComponentHost.
  // AddComponent and RemoveComponent cannot be called.
  virtual bool AddComponent(ipc::Component* component) OVERRIDE;
  virtual bool RemoveComponent(ipc::Component* component) OVERRIDE;
  virtual bool Send(ipc::Component* component,
                    ipc::proto::Message* message,
                    uint32* serial) OVERRIDE;
  virtual bool SendWithReply(ipc::Component* component,
                             ipc::proto::Message* message,
                             int timeout,
                             ipc::proto::Message** reply) OVERRIDE;
  virtual void PauseMessageHandling(ipc::Component* component) OVERRIDE;
  virtual void ResumeMessageHandling(ipc::Component* component) OVERRIDE;
  // Gets the ComponentInfo object of the component added to the
  // PluginComponentHost, and serialize it into |buffer|. This function will
  // create a byte buffer with length |size|.
  void GetComponentInfo(char** buffer, int* size);
  // Lets the PluginComponentHost handle a message. The message is serialized
  // into buffer with length size.
  void HandleMessage(const char* buffer, int size);
  // Called when the component is registered to the hub.
  void Registered(int id);
  // Called when the component is deregistered from the hub.
  void Deregistered();

 private:
  scoped_ptr<ipc::Component> component_;
  ComponentCallbacks callbacks_;
  bool has_component_;
  bool registered_;
  DISALLOW_COPY_AND_ASSIGN(PluginComponentAdaptor);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_COMPONENT_HOST_H_
