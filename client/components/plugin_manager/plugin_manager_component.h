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

#ifndef GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_COMPONENT_H_
#define GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_COMPONENT_H_

#include "base/scoped_ptr.h"
#include "components/plugin_manager/plugin_manager.h"
#include "ipc/component_base.h"

namespace ime_goopy {
namespace components {

class PluginManagerComponent
  : public ipc::ComponentBase,
    public PluginManager::Delegate {
 public:
  PluginManagerComponent();
  virtual ~PluginManagerComponent();
  // Overridden from ComponentBase.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;
  // Overridden from PluginManager::Delegate.
  virtual void PluginComponentChanged();

 private:
  void OnMsgPluginQueryComponents(ipc::proto::Message* message);
  void OnMsgPluginStartComponents(ipc::proto::Message* message);
  void OnMsgPluginStopComponents(ipc::proto::Message* message);
  void OnMsgPluginUnload(ipc::proto::Message* message);
  void OnMsgPluginInstalled(ipc::proto::Message* message);
  scoped_ptr<PluginManager> manager_;
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_COMPONENT_H_
