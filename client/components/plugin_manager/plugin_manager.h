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

#ifndef GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_H_
#define GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "components/plugin_manager/plugin_monitor_interface.h"
#include "ipc/protos/ipc.pb.h"

namespace google {
namespace protobuf {
template <class T> class RepeatedPtrField;
}  // namespace protobuf
}  // namespace google

namespace ipc {
class ComponentHost;
}  // namespace ipc

namespace ime_goopy {
namespace components {

class PluginComponentStub;

typedef google::protobuf::RepeatedPtrField<ipc::proto::ComponentInfo>
        ComponentInfos;

// A PluginManager that manages all the plugin components.
// It is responsible for managing the information of all plugin components and
// starting/stopping plugin components.
class PluginManager : public PluginMonitorInterface::Delegate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() { }
    // Notifies the delegate that some plugin components have changed.
    // The delegate can call GetComponents to get all the components.
    // This method should be re-entrantable.
    virtual void PluginComponentChanged() = 0;
  };
  // Constructs a plugin manger.
  // |path| is the root path of plugin files.
  // |host| is the component host object that all plugin component will be added
  //     to. It is preferred that |host| is a MultiComponentHost with
  //     |create_thread_| = true.
  // |delegate| is a Delegate object to which the PluginComponentChanged
  //     notification will be sent.
  PluginManager(const std::string& path,
                ipc::ComponentHost* host,
                PluginManager::Delegate* delegate);
  virtual ~PluginManager();
  // Initialize the PluginManager. Returns false if initialization failed.
  bool Init();
  // Gets the ComponentInfo objects of all the components in all plugins.
  // Returns the count of the components.
  int GetComponents(ComponentInfos* info);
  // Starts a component by its id.
  // Returns false if this no component with the specific string id.
  bool StartComponent(const std::string& id);
  // Stops a component by its id.
  bool StopComponent(const std::string& id);
  // Unloads a plugin file. It will stop all the components within the plugin.
  bool UnloadPlugin(const std::string& path);
  // Add a monitor to the plugins. The monitor should notify the manager if
  // any of the plugins are changed.
  // The manager will own the |monitor| object.
  void AddMonitor(PluginMonitorInterface* monitor);
  // Overriden from PluginMonitorInterface::Delegate.
  virtual void PluginChanged() OVERRIDE;

 private:
  bool ScanAllPluginFilesUnlocked();
  void AutoStartComponentsUnlocked();
  bool StartComponentUnlocked(const std::string& path, const std::string& id);
  void StopComponentUnlocked(const std::string& id);
  void StopAndClearAllPlugins();

  typedef std::map<std::string /*path*/, ipc::proto::PluginInfo* /*info*/>
          PluginInfoMap;
  typedef std::map<std::string /*id*/,
                   std::pair<ipc::proto::PluginInfo*, int> /*info*/>
          StringIDToInfoMap;
  typedef std::map<std::string /*id*/, PluginComponentStub* /*component*/>
          StartedComponentsMap;
  PluginInfoMap file_to_info_map_;
  StringIDToInfoMap string_id_to_info_map_;
  StartedComponentsMap started_components_map_;
  std::vector<PluginMonitorInterface*> monitors_;
  std::string path_;
  ipc::ComponentHost* host_;
  base::Lock lock_;
  PluginManager::Delegate* delegate_;
  DISALLOW_COPY_AND_ASSIGN(PluginManager);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_H_
