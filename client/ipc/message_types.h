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

#ifndef GOOPY_IPC_MESSAGE_TYPES_H_
#define GOOPY_IPC_MESSAGE_TYPES_H_
#pragma once

#include "base/basictypes.h"

namespace ipc {

#undef DECLARE_IPC_MSG
#define DECLARE_IPC_MSG(id, type)  MSG_##type = id,

enum MessageType {
#include "ipc/message_types_decl.h"
};

#undef DECLARE_IPC_MSG

// Returns the string name of a specified message type. It's mainly for
// debugging purpose.
const char* GetMessageName(uint32 type);

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_TYPES_H_
