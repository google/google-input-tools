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

#include "components/win_frontend/frontend_factory.h"
#include <algorithm>
#include "appsensorapi/appsensor_helper.h"
#include "appsensorapi/common.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "components/win_frontend/application_ui_component.h"
#include "components/win_frontend/ipc_singleton.h"
#include "components/win_frontend/frontend_component.h"

namespace ime_goopy {

using components::ApplicationUIComponent;
using components::FrontendComponent;
using ggadget::win32::ThreadLocalSingletonHolder;

FrontendFactory::FrontendFactory() {
  components::IPCEnvironment::GetInstance()->app_host()->AddComponent(
      ApplicationUIComponent::GetThreadLocalInstance());
}

FrontendFactory::~FrontendFactory() {
  STLDeleteElements(&frontends_);
  components::IPCEnvironment::GetInstance()->app_host()->RemoveComponent(
      ApplicationUIComponent::GetThreadLocalInstance());
  ApplicationUIComponent::ClearThreadLocalInstance();
}

EngineInterface* FrontendFactory::CreateFrontend() {
  FrontendFactory* thread_local_factory = GetThreadLocalInstance();
  std::vector<EngineInterface*>* frontends =
      &(thread_local_factory->frontends_);
  FrontendComponent* new_frontend = new FrontendComponent(
      ApplicationUIComponent::GetThreadLocalInstance());

  components::IPCEnvironment::GetInstance()->app_host()
      ->AddComponent(new_frontend);

  frontends->push_back(new_frontend);
  return new_frontend;
}

void FrontendFactory::DestroyFrontend(EngineInterface* frontend) {
  DCHECK(frontend);
  FrontendFactory* thread_local_factory = GetThreadLocalInstance();
  std::vector<EngineInterface*>* frontends =
      &(thread_local_factory->frontends_);

  std::vector<EngineInterface*>::iterator iter = std::find(
      frontends->begin(), frontends->end(), frontend);
  if (iter == frontends->end()) {
    DLOG(ERROR) << "DestroyFrontend: can't find frontend";
    return;
  } else {
    frontends->erase(iter);
    delete frontend;
  }
  ShelvedFrontends* shelved_frontends =
      &(thread_local_factory->shelved_frontends_);
  for (ShelvedFrontends::iterator iter = shelved_frontends->begin();
       iter != shelved_frontends->end(); ++iter) {
    if (iter->second == frontend) {
      shelved_frontends->erase(iter);
      break;
    }
  }
  if (frontends->empty()) {
    // Deletes factory itself.
    delete thread_local_factory;
    ThreadLocalSingletonHolder<FrontendFactory>::SetValue(NULL);
  }
}

bool FrontendFactory::ShelveFrontend(
    ContextInterface::ContextId id, EngineInterface* frontend) {
  DCHECK(frontend);
  bool should_destroy = false;
  AppSensorHelper::Instance()->HandleCommand(CMD_SHOULD_DESTROY_FRONTEND,
                                             &should_destroy);
  if (!id || should_destroy) {
    DestroyFrontend(frontend);
    return false;
  }
  FrontendFactory* thread_local_factory = GetThreadLocalInstance();
  thread_local_factory->PurgeShelvedFrontends();
  {
    const std::vector<EngineInterface*>& frontends =
        thread_local_factory->frontends_;
    if (std::find(frontends.begin(), frontends.end(), frontend) ==
        frontends.end()) {
      // Unknown frontend.
      DLOG(ERROR) << "ShelveFrontend: unknown frontend";
      return false;
    }
  }

  ShelvedFrontends* shelved_frontends =
      &(thread_local_factory->shelved_frontends_);
  ShelvedFrontends::iterator iter = shelved_frontends->find(id);
  if (iter != shelved_frontends->end()) {
    DLOG(ERROR) << "ShelveFrontend: Given id has a frontend shelved";
    return false;
  }
  (*shelved_frontends)[id] = frontend;
  return true;
}

EngineInterface* FrontendFactory::UnshelveFrontend(
    ContextInterface::ContextId id) {
  DCHECK(id);
  FrontendFactory* thread_local_factory = GetThreadLocalInstance();

  ShelvedFrontends* shelved_frontends =
      &(thread_local_factory->shelved_frontends_);
  ShelvedFrontends::iterator iter = shelved_frontends->find(id);
  if (iter == shelved_frontends->end()) {
    DLOG(ERROR) << "UnshelveFrontend: Unknown id";
    return NULL;
  }
  EngineInterface* old_frontend = iter->second;
  {
    // For Debug check only.
    const std::vector<EngineInterface*>& frontends =
        thread_local_factory->frontends_;
    DCHECK(std::find(frontends.begin(), frontends.end(), old_frontend) !=
           frontends.end());
  }
  shelved_frontends->erase(iter);
  return old_frontend;
}

FrontendFactory* FrontendFactory::GetThreadLocalInstance() {
  FrontendFactory* thread_local_factory =
      ThreadLocalSingletonHolder<FrontendFactory>::GetValue();
  if (!thread_local_factory) {
    thread_local_factory = new FrontendFactory();
    bool result =
        ThreadLocalSingletonHolder<FrontendFactory>::SetValue(
            thread_local_factory);
    DCHECK(result);
  }
  return thread_local_factory;
}

EngineInterface* FrontendFactory::UnshelveOrCreateFrontend(
    ContextInterface::ContextId id) {
  if (!id)
    return CreateFrontend();
  EngineInterface* frontend = UnshelveFrontend(id);
  if (!frontend)
    frontend = CreateFrontend();
  return frontend;
}

void FrontendFactory::PurgeShelvedFrontends() {
  std::vector<ContextInterface::ContextId> should_remove;
  for (ShelvedFrontends::iterator it = shelved_frontends_.begin();
       it != shelved_frontends_.end();
       ++it) {
    if (!it->first || !ContextInterface::IsValidContextID(it->first)) {
      should_remove.push_back(it->first);
      delete it->second;
      it->second = NULL;
    }
  }
  for (size_t i = 0; i < should_remove.size(); ++i)
    shelved_frontends_.erase(should_remove[i]);
}

}  // namespace ime_goopy
