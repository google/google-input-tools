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

#ifndef GOOPY_IPC_SUB_COMPONENT_BASE_H_
#define GOOPY_IPC_SUB_COMPONENT_BASE_H_

#include "base/basictypes.h"
#include "ipc/sub_component.h"

namespace ipc {

class ComponentBase;
class SubComponentBase : public SubComponent {
 public:
  // The sub component will be added into |owner| automatically, the ownership
  // will be transfered to |owner|.
  explicit SubComponentBase(ComponentBase* owner);

 protected:
  // Weak reference to owner.
  ComponentBase* owner_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SubComponentBase);
};

}  // namespace ipc

#endif  // GOOPY_IPC_SUB_COMPONENT_BASE_H_
