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

#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_COMPONENT_STUB_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_COMPONENT_STUB_H_

#include "base/scoped_ptr.h"
#include "components/plugin_wrapper/plugin_instance.h"
#include "ipc/component_base.h"

namespace ime_goopy {
namespace components {

// A stub component that can loads an actual component from a given dll and
// deliver the messages between hub and the component.
class PluginComponentStub : public ipc::ComponentBase {
 public:
  PluginComponentStub(const std::string& dll_path,
                      const char* id);
  virtual ~PluginComponentStub();
  // Overridden from ComponentBase:
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;
  ComponentInstance component();
  bool IsInitialized();

 private:
  ComponentInstance component_;
  PluginInstance plugin_instance_;
  bool initialized_;
  DISALLOW_COPY_AND_ASSIGN(PluginComponentStub);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_COMPONENT_STUB_H_
