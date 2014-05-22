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

#ifndef GOOPY_COMPONENTS_PLUGIN_MANAGER_REGISTRY_MONITOR_WRAPPER_H_
#define GOOPY_COMPONENTS_PLUGIN_MANAGER_REGISTRY_MONITOR_WRAPPER_H_

#include <atlbase.h>
#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_handle.h"
#include "base/threading/platform_thread.h"
#include "common/registry_monitor.h"
#include "components/plugin_manager/plugin_monitor_interface.h"

namespace ime_goopy {
namespace components {

// An monitor class that monitors a specific registry key.
class RegistryMonitorWrapper
  : public PluginMonitorInterface,
    public RegistryMonitor::Delegate {
 public:
  RegistryMonitorWrapper(HKEY hkey,
                         const wchar_t* sub_key,
                         PluginMonitorInterface::Delegate* delegate)
      : ALLOW_THIS_IN_INITIALIZER_LIST(moniter_(hkey, sub_key, this)),
        delegate_(delegate) {
    DCHECK(delegate);
  }
  // Overridden from PluginMoniterInterface.
  virtual ~RegistryMonitorWrapper() {}
  // Starts monitoring the registry.
  virtual bool Start() OVERRIDE {
    return moniter_.Start();
  }
  // Stops monitoring the registry.
  virtual void Stop() OVERRIDE {
    return moniter_.Stop();
  }
  // Overridden from RegistryMonitor::Delegate.
  virtual void KeyChanged() {
    delegate_->PluginChanged();
  }

 private:
  RegistryMonitor moniter_;
  PluginMonitorInterface::Delegate* delegate_;
  DISALLOW_COPY_AND_ASSIGN(RegistryMonitorWrapper);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_MANAGER_REGISTRY_MONITOR_WRAPPER_H_
