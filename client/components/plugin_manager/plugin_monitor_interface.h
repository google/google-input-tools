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

#ifndef GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MONITOR_INTERFACE_H_
#define GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MONITOR_INTERFACE_H_

namespace ime_goopy {
namespace components {

class PluginMonitorInterface {
 public:
  virtual ~PluginMonitorInterface() { }
  class Delegate {
   public:
    virtual ~Delegate() { }
    // Notifies that plugin files have been change.
    virtual void PluginChanged() = 0;
  };
  virtual bool Start() = 0;
  virtual void Stop() = 0;
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MONITOR_INTERFACE_H_
