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

#include <algorithm>
#include <set>
#include <vector>
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_handle.h"
#include "base/scoped_ptr.h"
#include "common/string_utils.h"
#include "ipc/component.h"
#include "ipc/component_host.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace components {

static const wchar_t kPluginPath1[] = PLUGIN_PATH1;
static const wchar_t kPluginPath2[] = PLUGIN_PATH2;
static const wchar_t kSubPath[] = L"test";

class MockedMultiComponentHost : public ipc::ComponentHost {
 public:
  MockedMultiComponentHost() { }
  virtual ~MockedMultiComponentHost() { }
  int ComponentCount() {
    return components_.size();
  }
  int GetComponents(std::set<std::string>* component_ids) {
    component_ids->clear();
    for (std::set<ipc::Component*>::iterator it = components_.begin();
         it != components_.end();
         ++it) {
      ipc::proto::ComponentInfo info;
      (*it)->GetInfo(&info);
      component_ids->insert(info.string_id());
    }
    return component_ids->size();
  }
  virtual bool AddComponent(ipc::Component* component) OVERRIDE {
    EXPECT_EQ(components_.end(), components_.find(component));
    components_.insert(component);
    return true;
  }
  virtual bool RemoveComponent(ipc::Component* component) OVERRIDE {
    EXPECT_NE(components_.end(), components_.find(component));
    components_.erase(component);
    return true;
  }
  virtual bool Send(ipc::Component* component,
                    ipc::proto::Message* message,
                    uint32* serial) OVERRIDE {
    return true;
  }
  virtual bool SendWithReply(ipc::Component* component,
                             ipc::proto::Message* message,
                             int timeout,
                             ipc::proto::Message** reply) OVERRIDE {
    return true;
  }
  virtual void PauseMessageHandling(ipc::Component* component) OVERRIDE { }
  virtual void ResumeMessageHandling(ipc::Component* component) OVERRIDE { }

 private:
  std::set<ipc::Component*> components_;
};

class PluginManagerTest
  : public PluginManager::Delegate,
    public ::testing::Test {
 public:
  void PluginComponentChanged() OVERRIDE {
    changed_ = true;
  }

 protected:
  void SetUp() {
    changed_ = false;
    wchar_t path[MAX_PATH] = {0};
    wchar_t subpath[MAX_PATH] = {0};
    ::GetModuleFileName(NULL, path, MAX_PATH);
    ::PathRemoveFileSpec(path);
    ::PathCombine(subpath, path, kSubPath);
    ASSERT_TRUE(::CreateDirectory(subpath, NULL));
    path_ = subpath;
    manager_.reset(new PluginManager(WideToUtf8(path_), &host_, this));
  }

  void TearDown() {
    manager_.reset(NULL);
    // Use EXPECT_TRUE rather than ASSERT_TRUE, because we want to recover from
    // broken environment as much as possible. For example, the top-level
    // directory is created but plugin directory is not created yet.
    EXPECT_TRUE(::DeleteFile(plugin_path1_.c_str()));
    EXPECT_TRUE(::DeleteFile(plugin_path2_.c_str()));
    EXPECT_TRUE(::RemoveDirectory(sub_path_.c_str()));
    EXPECT_TRUE(::RemoveDirectory(path_.c_str()));
  }

  void CopyFirstPlugin() {
    plugin_path1_ = path_ + L"\\plugin.dll";
    ASSERT_TRUE(::CopyFile(kPluginPath1, plugin_path1_.c_str(), FALSE));
    manager_->PluginChanged();
  }

  void CopySecondPlugin() {
    sub_path_ = path_ + L"\\sub";
    ASSERT_TRUE(::CreateDirectory(sub_path_.c_str(), NULL));
    plugin_path2_ = sub_path_ + L"\\plugin.dll";
    ASSERT_TRUE(::CopyFile(kPluginPath2, plugin_path2_.c_str(), FALSE));
    manager_->PluginChanged();
  }

  int GetComponents(std::set<std::string>* component_ids) {
    component_ids->clear();
    ipc::proto::MessagePayload payload;
    manager_->GetComponents(payload.mutable_component_info());
    for (int i = 0; i < payload.component_info_size(); ++i)
      component_ids->insert(payload.component_info(i).string_id());
    return component_ids->size();
  }

  int GetStartedComponents(std::set<std::string>* component_ids) {
    return host_.GetComponents(component_ids);
  }

  void StartComponent(const std::string& id) {
    manager_->StartComponent(id);
  }

  void StopComponent(const std::string& id) {
    manager_->StopComponent(id);
  }

  void UnloadFirstPlugin() {
    manager_->UnloadPlugin(WideToUtf8(plugin_path1_));
  }

  void UnloadSecondPlugin() {
    manager_->UnloadPlugin(WideToUtf8(plugin_path2_));
  }

  void PluginChanged() {
    manager_->PluginChanged();
  }

  bool Changed() {
    bool ret = changed_;
    changed_ = false;
    return ret;
  }

 private:
  scoped_ptr<PluginManager> manager_;
  // The root path of plugin manager.
  std::wstring path_;
  std::wstring plugin_path1_;
  std::wstring plugin_path2_;
  std::wstring sub_path_;
  bool changed_;
  MockedMultiComponentHost host_;
};

bool SetEqual(const std::set<std::string>& left,
              const std::set<std::string>& right) {
  if (left.size() != right.size())
    return false;
  for (std::set<std::string>::const_iterator it = left.begin();
       it != left.end();
       ++it) {
    if (!right.count(*it))
      return false;
  }
  return true;
}

TEST_F(PluginManagerTest, PluginManagerTest) {
  std::set<std::string> component_ids;
  std::set<std::string> started_component_ids;
  EXPECT_EQ(0, GetComponents(&component_ids));
  EXPECT_EQ(0, GetStartedComponents(&started_component_ids));
  CopyFirstPlugin();
  EXPECT_TRUE(Changed());
  EXPECT_EQ(2, GetComponents(&component_ids));
  EXPECT_EQ(2, GetStartedComponents(&started_component_ids));
  EXPECT_TRUE(SetEqual(component_ids, started_component_ids));
  CopySecondPlugin();
  EXPECT_TRUE(Changed());
  EXPECT_EQ(4, GetComponents(&component_ids));
  EXPECT_EQ(4, GetStartedComponents(&started_component_ids));
  EXPECT_TRUE(SetEqual(component_ids, started_component_ids));
  std::set<std::string>::const_iterator it = component_ids.begin();
  std::string id = *it;
  StopComponent(id);
  EXPECT_FALSE(Changed());
  EXPECT_EQ(4, GetComponents(&component_ids));
  EXPECT_EQ(3, GetStartedComponents(&started_component_ids));
  EXPECT_EQ(0, started_component_ids.count(id));
  EXPECT_EQ(1, component_ids.count(id));
  StartComponent(id);
  EXPECT_FALSE(Changed());
  EXPECT_EQ(4, GetComponents(&component_ids));
  EXPECT_EQ(4, GetStartedComponents(&started_component_ids));
  EXPECT_EQ(1, started_component_ids.count(id));
  EXPECT_TRUE(SetEqual(component_ids, started_component_ids));
  UnloadFirstPlugin();
  EXPECT_TRUE(Changed());
  EXPECT_EQ(2, GetComponents(&component_ids));
  EXPECT_EQ(2, GetStartedComponents(&started_component_ids));
  EXPECT_TRUE(SetEqual(component_ids, started_component_ids));
  // PluginChanged will make the manager rescan all the components.
  PluginChanged();
  EXPECT_TRUE(Changed());
  EXPECT_EQ(4, GetComponents(&component_ids));
  EXPECT_EQ(4, GetStartedComponents(&started_component_ids));
  EXPECT_TRUE(SetEqual(component_ids, started_component_ids));
  UnloadSecondPlugin();
  EXPECT_TRUE(Changed());
  EXPECT_EQ(2, GetComponents(&component_ids));
  EXPECT_EQ(2, GetStartedComponents(&started_component_ids));
  EXPECT_TRUE(SetEqual(component_ids, started_component_ids));
  // Left 2 component in the host to check if them are unloaded correctly when
  // the manager exits.
}

}  // namespace components
}  // namespace ime_goopy

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
