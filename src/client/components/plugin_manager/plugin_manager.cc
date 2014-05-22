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

#include "components/plugin_manager/plugin_manager.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "components/common/file_utils.h"
#include "components/plugin_manager/plugin_manager_utils.h"
#include "components/plugin_wrapper/plugin_component_stub.h"
#include "components/plugin_wrapper/plugin_instance.h"
#include "ipc/component_host.h"

namespace ime_goopy {
namespace components {

PluginManager::PluginManager(const std::string& path,
                             ipc::ComponentHost* host,
                             PluginManager::Delegate* delegate)
  : path_(path),
    host_(host),
    delegate_(delegate) {
  DCHECK(!path_.empty());
  DCHECK(host);
  DCHECK(delegate_);
}

PluginManager::~PluginManager() {
  for (int i = 0; i < monitors_.size(); ++i)
    monitors_[i]->Stop();
  STLDeleteElements(&monitors_);
  // StopAndClearAllPlugins must be called after stopping all monitors, in case
  // that a monitor will trigger PluginChanged and start a component.
  StopAndClearAllPlugins();
}

bool PluginManager::Init() {
  base::AutoLock auto_lock(lock_);
  if (!ScanAllPluginFilesUnlocked())
    return false;
  AutoStartComponentsUnlocked();
  return true;
}

int PluginManager::GetComponents(ComponentInfos* info) {
  base::AutoLock auto_lock(lock_);
  for (StringIDToInfoMap::const_iterator it = string_id_to_info_map_.begin();
       it != string_id_to_info_map_.end();
       ++it) {
    info->Add()->CopyFrom(
        it->second.first->component_infos(it->second.second));
  }
  return string_id_to_info_map_.size();
}

bool PluginManager::StartComponent(const std::string& id) {
  base::AutoLock auto_lock(lock_);
  StringIDToInfoMap::const_iterator it = string_id_to_info_map_.find(id);
  if (it == string_id_to_info_map_.end())
    return false;
  if (started_components_map_.find(id) == started_components_map_.end()) {
    const std::string& path = it->second.first->path();
    return StartComponentUnlocked(path, id);
  }
  return true;
}

bool PluginManager::StopComponent(const std::string& id) {
  base::AutoLock auto_lock(lock_);
  StopComponentUnlocked(id);
  return true;
}

bool PluginManager::UnloadPlugin(const std::string& path) {
  {
    base::AutoLock auto_lock(lock_);
    PluginInfoMap::iterator it = file_to_info_map_.find(path);
    if (it == file_to_info_map_.end())
      return false;
    for (int i = 0; i < it->second->component_infos_size(); ++i) {
      const std::string& id = it->second->component_infos(i).string_id();
      StringIDToInfoMap::const_iterator component =
          string_id_to_info_map_.find(id);
      DCHECK(component != string_id_to_info_map_.end());
      DCHECK_EQ(component->second.first->path(), path);
      if (component != string_id_to_info_map_.end() &&
          component->second.first->path() == path) {
        StopComponentUnlocked(id);
        string_id_to_info_map_.erase(id);
      }
    }
    file_to_info_map_.erase(it);
  }
  delegate_->PluginComponentChanged();
  // TODO(synch): we need to remember the unloaded plugin when we are able to
  // find which plugin is changed to prevent the change of other component from
  // load the unloaded component unintentionally.
  return true;
}

void PluginManager::AddMonitor(PluginMonitorInterface* monitor) {
  if (monitor->Start()) {
    monitors_.push_back(monitor);
  } else {
    DLOG(ERROR) << "Error starting monitor";
    delete monitor;
  }
}

void PluginManager::PluginChanged() {
  {
    base::AutoLock auto_lock(lock_);
    ScanAllPluginFilesUnlocked();
    AutoStartComponentsUnlocked();
  }
  delegate_->PluginComponentChanged();
}

bool PluginManager::ScanAllPluginFilesUnlocked() {
  std::vector<std::string> plugin_files;
  if (!PluginManagerUtils::ListPluginFile(path_, &plugin_files)) {
    DLOG(ERROR) << "Error listing plugin files in:" << path_;
    return false;
  }
  for (size_t i = 0; i < plugin_files.size(); ++i) {
    if (file_to_info_map_.find(plugin_files[i]) != file_to_info_map_.end()) {
      // If the plugin file is already in the map, then the plugin is not
      // updated because it is locked.
      continue;
    }
    PluginInstance instance(plugin_files[i]);
    if (!instance.IsInitialized())
      continue;
    ipc::proto::MessagePayload payload;
    instance.ListComponents(&payload);
    if (payload.component_info_size()) {
      ipc::proto::PluginInfo* info = new ipc::proto::PluginInfo;
      info->set_path(plugin_files[i]);
      info->mutable_component_infos()->Swap(payload.mutable_component_info());
      file_to_info_map_[plugin_files[i]] = info;
      for (int j = 0; j < info->component_infos_size(); ++j) {
        const std::string& id = info->component_infos(j).string_id();
        if (string_id_to_info_map_.find(id) != string_id_to_info_map_.end()) {
          NOTREACHED() << "Duplicated component string id:" << id
                       << " in file:" << plugin_files[i]
                       << " and:" << string_id_to_info_map_[id].first->path();
        } else {
          string_id_to_info_map_[id].first = info;
          string_id_to_info_map_[id].second = j;
        }
      }
    }
  }
  return true;
}

void PluginManager::AutoStartComponentsUnlocked() {
  // TODO(synch): Now we don't have settings UI, so we start all available
  // components. Later we need to load autostart components from settingstore.
  for (StringIDToInfoMap::const_iterator it = string_id_to_info_map_.begin();
       it != string_id_to_info_map_.end();
       ++it) {
    const std::string& id = it->first;
    if (started_components_map_.find(id) == started_components_map_.end()) {
      const std::string& path = it->second.first->path();
      if (!StartComponentUnlocked(path, id))
        DLOG(ERROR) << "Error starting component:" << id;
    }
  }
}

bool PluginManager::StartComponentUnlocked(const std::string& path,
                                           const std::string& id) {
  scoped_ptr<PluginComponentStub> component(
      new PluginComponentStub(path, id.c_str()));
  if (!component->IsInitialized())
    return false;
  DCHECK(started_components_map_.find(id) == started_components_map_.end());
  host_->AddComponent(component.get());
  started_components_map_[id] = component.release();
  return true;
}

void PluginManager::StopComponentUnlocked(const std::string& id) {
  StartedComponentsMap::iterator it = started_components_map_.find(id);
  if (it != started_components_map_.end()) {
    PluginComponentStub* component = it->second;
    started_components_map_.erase(it);
    host_->RemoveComponent(component);
    delete component;
  }
}

void PluginManager::StopAndClearAllPlugins() {
  for (StartedComponentsMap::const_iterator i = started_components_map_.begin();
       i != started_components_map_.end();
       ++i) {
    i->second->RemoveFromHost();
  }
  STLDeleteContainerPairSecondPointers(file_to_info_map_.begin(),
                                       file_to_info_map_.end());
  STLDeleteContainerPairSecondPointers(started_components_map_.begin(),
                                       started_components_map_.end());
  file_to_info_map_.clear();
  string_id_to_info_map_.clear();
}
}  // namespace components
}  // namespace ime_goopy
