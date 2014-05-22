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

#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_DAILY_PINGER_COMPONENT_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_DAILY_PINGER_COMPONENT_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
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

class DailyPingerComponent : public ipc::SubComponentBase {
 public:
  explicit DailyPingerComponent(ipc::ComponentBase* owner);
  virtual ~DailyPingerComponent();

  // Overridden from ipc::SubComponent:
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;
  virtual bool Handle(ipc::proto::Message* message) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DailyPingerComponent);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_DAILY_PINGER_COMPONENT_H_
