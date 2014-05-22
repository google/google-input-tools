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

#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_IPC_SINGLETON_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_IPC_SINGLETON_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/singleton.h"
#include "ipc/multi_component_host.h"
#include "ipc/message_channel_client_win.h"

namespace ime_goopy {
namespace components {

class ApplicationUIComponent;

// Provide singleton access for multiple ipc skeleton objects.
class IPCEnvironment {
 public:
  static IPCEnvironment* GetInstance();
  static void DeleteInstance();

  ~IPCEnvironment();

  ipc::ComponentHost* app_host() { return app_host_.get(); }

 private:
  friend struct DefaultSingletonTraits<IPCEnvironment>;

  scoped_ptr<ipc::MultiComponentHost> app_host_;
  scoped_ptr<ipc::MessageChannelClientWin> channel_client_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(IPCEnvironment);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_IPC_SINGLETON_H_
