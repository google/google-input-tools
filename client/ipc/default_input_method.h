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

#ifndef GOOPY_IPC_DEFAULT_INPUT_METHOD_H_
#define GOOPY_IPC_DEFAULT_INPUT_METHOD_H_
#pragma once

#include "base/basictypes.h"
#include "ipc/hub.h"

namespace ipc {

class Hub;

extern const char kDefaultInputMethodStringID[];

// A default input method that can be attached to Hub directly. It simply
// returns all key events unhandled.
class DefaultInputMethod : public Hub::Connector {
 public:
  explicit DefaultInputMethod(Hub* hub);
  virtual ~DefaultInputMethod();

  // Implementation of Hub::Connector interface
  virtual bool Send(proto::Message* message);

 private:
  void OnMsgProcessKeyEvent(proto::Message* message);

  // Weak pointer to our owner.
  Hub* hub_;

  uint32 id_;

  DISALLOW_COPY_AND_ASSIGN(DefaultInputMethod);
};

}  // namespace ipc

#endif  // GOOPY_IPC_DEFAULT_INPUT_METHOD_H_
