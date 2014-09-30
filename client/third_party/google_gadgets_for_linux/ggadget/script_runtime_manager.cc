/*
  Copyright 2008 Google Inc.

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

#include <vector>
#include <string>
#include <utility>
#include "logger.h"
#include "script_runtime_manager.h"
#include "small_object.h"

namespace ggadget {

class ScriptRuntimeManager::Impl : public SmallObject<> {
 public:
  bool RegisterScriptRuntime(const char *tag_name,
                             ScriptRuntimeInterface *runtime) {
    ASSERT(tag_name && *tag_name && runtime);
    if (GetScriptRuntime(tag_name) != NULL)
      return false;
    runtimes_.push_back(std::make_pair(std::string(tag_name), runtime));
    return true;
  }

  ScriptContextInterface *CreateScriptContext(const char *tag_name) {
    ScriptRuntimeInterface *runtime = GetScriptRuntime(tag_name);
    return runtime ? runtime->CreateContext() : NULL;
  }

  ScriptRuntimeInterface *GetScriptRuntime(const char *tag_name) {
    ASSERT(tag_name && *tag_name);
    std::string name(tag_name);
    for (size_t i = 0; i < runtimes_.size(); ++i) {
      if (runtimes_[i].first == name)
        return runtimes_[i].second;
    }
    return NULL;
  }

  std::vector<std::pair<std::string, ScriptRuntimeInterface *> > runtimes_;
  static ScriptRuntimeManager *manager_;
};

ScriptRuntimeManager *ScriptRuntimeManager::Impl::manager_ = NULL;

ScriptRuntimeManager::ScriptRuntimeManager()
  : impl_(new Impl()) {
}

ScriptRuntimeManager::~ScriptRuntimeManager() {
  DLOG("ScriptRuntimeManager singleton is destroyed, but it shouldn't.");
  ASSERT(Impl::manager_ == this);
  Impl::manager_ = NULL;
  delete impl_;
  impl_ = NULL;
}

bool ScriptRuntimeManager::RegisterScriptRuntime(
    const char *tag_name, ScriptRuntimeInterface *runtime) {
  return impl_->RegisterScriptRuntime(tag_name, runtime);
}

ScriptContextInterface *
ScriptRuntimeManager::CreateScriptContext(const char *tag_name) {
  return impl_->CreateScriptContext(tag_name);
}

ScriptRuntimeInterface *
ScriptRuntimeManager::GetScriptRuntime(const char *tag_name) {
  return impl_->GetScriptRuntime(tag_name);
}

ScriptRuntimeManager *ScriptRuntimeManager::get() {
  if (!Impl::manager_)
    Impl::manager_ = new ScriptRuntimeManager();

  return Impl::manager_;
}

} // namespace ggadget
