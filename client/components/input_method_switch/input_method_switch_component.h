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

#ifndef GOOPY_COMPONENTS_INPUT_METHOD_SWITCH_INPUT_METHOD_SWITCH_COMPONENT_H_
#define GOOPY_COMPONENTS_INPUT_METHOD_SWITCH_INPUT_METHOD_SWITCH_COMPONENT_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "ipc/component_base.h"

namespace ipc {
namespace proto {
class ComponentInfo;
class Message;
}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace components {

// This class is responsible for registering menu command and hotkeys of input
// method switch.
class InputMethodSwitchComponent : public ipc::ComponentBase {
 public:
  typedef google::protobuf::RepeatedPtrField<ipc::proto::ComponentInfo>
      ComponentInfos;

  InputMethodSwitchComponent();
  virtual ~InputMethodSwitchComponent();

  // Overridden interface ipc::Component.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;

 protected:
  virtual void OnMsgAttachToInputContext(ipc::proto::Message* message);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_COPY_AND_ASSIGN(InputMethodSwitchComponent);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_INPUT_METHOD_SWITCH_INPUT_METHOD_SWITCH_COMPONENT_H_
