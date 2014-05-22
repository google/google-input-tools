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
#ifndef GOOPY_COMPONENTS_COMMON_FOCUS_INPUT_CONTEXT_MANAGER_SUB_COMPONENT_H_
#define GOOPY_COMPONENTS_COMMON_FOCUS_INPUT_CONTEXT_MANAGER_SUB_COMPONENT_H_
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/sub_component_base.h"
namespace ipc {
class ComponentBase;

namespace proto {
class ComponentInfo;
class Message;
}  // namespace proto

}  // namespace ipc

namespace ime_goopy {
namespace components {

// This class is a sub component that keeps track of focus input context.
// it will process the MSG_INPUT_CONTEXT_GOT_FOCUS and
// MSG_INPUT_CONTEXT_LOST_FOCUS messages, but will not return true to stop other
// sub component or parent component to handle them.
class FocusInputContextManagerSubComponent : public ipc::SubComponentBase {
 public:
  explicit FocusInputContextManagerSubComponent(ipc::ComponentBase* owner);

  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual bool Handle(ipc::proto::Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;

  uint32 GetFocusIcid() const;

 private:
  friend class FocusInputContextManagerSubComponentTest;
  uint32 focus_icid_;
  DISALLOW_COPY_AND_ASSIGN(FocusInputContextManagerSubComponent);
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_COMMON_FOCUS_INPUT_CONTEXT_MANAGER_SUB_COMPONENT_H_
