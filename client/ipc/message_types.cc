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

#include "ipc/message_types.h"

#include "base/basictypes.h"
#include <algorithm>

namespace {

struct MessageInfo {
  uint32 id;
  const char* name;
};

#define DECLARE_IPC_MSG(id, type)  { id, #type },
static const MessageInfo kMessageNames[] = {
#include "ipc/message_types_decl.h"
};
#undef DECLARE_IPC_MSG

bool CompareMessageInfo(const MessageInfo& a, const MessageInfo& b) {
  return a.id < b.id;
}

}  // namespace

namespace ipc {

const char* GetMessageName(uint32 type) {
  if (type >= MSG_SYSTEM_RESERVED_START && type <= MSG_SYSTEM_RESERVED_END)
    return "SYSTEM_RESERVED";
  if (type >= MSG_USER_DEFINED_START)
    return "USER_DEFINED";

  const MessageInfo info = { type, NULL };
  const MessageInfo* end = kMessageNames + arraysize(kMessageNames);
  const MessageInfo* iter =
      std::lower_bound(kMessageNames, end, info, CompareMessageInfo);

  return (iter < end && iter->id == type) ? iter->name : "UNDEFINED";
}

}  // namespace ipc
