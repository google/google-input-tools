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

// This file defines an ime component class which just returns what the user
// inputs from the keyboard.

#ifndef GOOPY_COMPONENTS_KEYBOARD_INPUT_KEYBOARD_INPUT_COMPONENT_H_
#define GOOPY_COMPONENTS_KEYBOARD_INPUT_KEYBOARD_INPUT_COMPONENT_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/component_base.h"

namespace ipc {
namespace proto {
class ComponentInfo;
class Message;
}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace components {

class KeyboardInputComponent : public ipc::ComponentBase {
 public:
  KeyboardInputComponent();
  virtual ~KeyboardInputComponent();

  // Overridden interface ipc::Component.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

 private:
  // Handles input context related messages.
  void OnMsgAttachToInputContext(ipc::proto::Message* message);
  void OnMsgProcessKey(ipc::proto::Message* message);

  DISALLOW_COPY_AND_ASSIGN(KeyboardInputComponent);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_KEYBOARD_INPUT_KEYBOARD_INPUT_COMPONENT_H_
