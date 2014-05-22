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

#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_IPC_UI_MANAGER_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_IPC_UI_MANAGER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "common/framework_interface.h"

namespace ime_goopy {
namespace components {

// IPCUIManager is light weight implementation of UIManagerInterface.
// All actions from ui window are redirected to frontend component to handle.
// TODO(haicsun): further refactor this class if it's possible to remove ui
// manager interface.
class IPCUIManager : public ime_goopy::UIManagerInterface {
 public:
  IPCUIManager();
  virtual ~IPCUIManager();

  // Overidden from |UIManagerInterface|:
  virtual void SetContext(ContextInterface* context) OVERRIDE;
  virtual void SetToolbarStatus(bool is_open) OVERRIDE;
  virtual void Update(uint32 component) OVERRIDE;
  virtual void LayoutChanged() OVERRIDE;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_COPY_AND_ASSIGN(IPCUIManager);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_IPC_UI_MANAGER_H_
