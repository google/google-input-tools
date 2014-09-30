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

#include "ipc/testing.h"

namespace {

#define DECLARE_IPC_MSG(id, type) ipc::MSG_##type,
const uint32 kPredefinedMessageTypes[] = {
#include "ipc/message_types_decl.h"
};
#undef DECLARE_IPC_MSG

TEST(MessageTypesTest, Basic) {
  EXPECT_EQ(0, ipc::MSG_INVALID);
}

TEST(MessageTypesTest, Name) {
  EXPECT_STREQ("INVALID", ipc::GetMessageName(ipc::MSG_INVALID));
  EXPECT_STREQ("CREATE_INPUT_CONTEXT",
               ipc::GetMessageName(ipc::MSG_CREATE_INPUT_CONTEXT));
  EXPECT_STREQ("SYSTEM_RESERVED",
               ipc::GetMessageName(ipc::MSG_SYSTEM_RESERVED_START));
  EXPECT_STREQ("SYSTEM_RESERVED",
               ipc::GetMessageName(ipc::MSG_SYSTEM_RESERVED_START + 1));
  EXPECT_STREQ("SYSTEM_RESERVED",
               ipc::GetMessageName(ipc::MSG_SYSTEM_RESERVED_END));
  EXPECT_STREQ("USER_DEFINED",
               ipc::GetMessageName(ipc::MSG_USER_DEFINED_START));
  EXPECT_STREQ("USER_DEFINED",
               ipc::GetMessageName(ipc::MSG_USER_DEFINED_START + 1));
  EXPECT_STREQ("UNDEFINED",
               ipc::GetMessageName(ipc::MSG_SYSTEM_RESERVED_START - 1));
}

TEST(MessageTypesTest, Sorted) {
  for (size_t i = 1; i < arraysize(kPredefinedMessageTypes); ++i)
    EXPECT_LT(kPredefinedMessageTypes[i - 1], kPredefinedMessageTypes[i]);
}

}  // namespace
