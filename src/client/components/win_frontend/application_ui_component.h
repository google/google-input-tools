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

#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_APPLICATION_UI_COMPONENT_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_APPLICATION_UI_COMPONENT_H_

#include <windows.h>

#include <set>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "components/win_frontend/frontend_component.h"
#include "ipc/component_base.h"

namespace ipc {
class ComponentHost;
class SettingsClient;
namespace proto {
class Message;
}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace components {

// A component to handle all UI notifications and interactions that should be in
// the application process, including context menu and modal message box.
// In our multi-process framework, UI and application should be in different
// process, but if we show context menus in the UI process, the menu will not
// disappear when the user click somewhere in the application, also clicking on
// the context menu will make the application lose focus. So we need to show the
// menu in the application's process.
// And if we want to show a message box and interrupt user's input, we must do
// it in the application's process.
class ApplicationUIComponent : public ipc::ComponentBase,
                               public FrontendComponent::Delegate {
 public:
  ApplicationUIComponent();
  virtual ~ApplicationUIComponent();

  // Overridden from Component: derived classes should not override this two
  // message anymore.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;

  // Overridden from FrontendComponent::Delegate.
  virtual void InputContextCreated(int icid) OVERRIDE;
  static ApplicationUIComponent* GetThreadLocalInstance();
  static void ClearThreadLocalInstance();

 private:
  void OnMsgInputContextDeleted(ipc::proto::Message* message);
  void OnMsgShowMenu(ipc::proto::Message* message);
  void OnMsgShowMessageBox(ipc::proto::Message* message);

  ipc::SettingsClient* settings_;

  // The process id of ipc console.
  DWORD console_pid_;

  std::set<int> attached_icids_;

  // A lock to protect attached_icids_ as InputContextCreated will be called
  // from different threads.
  base::Lock lock_;

  // An owner window of all pop menus.
  HWND menu_owner_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationUIComponent);
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_APPLICATION_UI_COMPONENT_H_
