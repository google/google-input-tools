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

#include "components/win_frontend/application_ui_component.h"
#include "components/win_frontend/ipc_singleton.h"
#include "base/at_exit.h"

namespace {

class SimpleAtExitManager : public base::AtExitManager {
 public:
  SimpleAtExitManager() : AtExitManager(true) {
  }
};

SimpleAtExitManager at_exit_manager;

}  // namespace

namespace ime_goopy {
namespace components {

IPCEnvironment* IPCEnvironment::GetInstance() {
  return Singleton<IPCEnvironment>::get();
}

void IPCEnvironment::DeleteInstance() {
  at_exit_manager.ProcessCallbacksNow();
}

IPCEnvironment::IPCEnvironment() {
  app_host_.reset(new ipc::MultiComponentHost(false));
  channel_client_.reset(new ipc::MessageChannelClientWin(app_host_.get()));
  channel_client_->Start();
}

IPCEnvironment::~IPCEnvironment() {
  channel_client_->Stop();
  app_host_.reset();
  channel_client_.reset();
}

}  // namespace components
}  // namespace ime_goopy
