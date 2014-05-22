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

#ifndef GOOPY_IPC_TEST_UTIL_H_
#define GOOPY_IPC_TEST_UTIL_H_

#include <queue>

#include "base/basictypes.h"
#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace base {
class Lock;
class WaitableEvent;
}  // namespace base

namespace ipc {

namespace hub {
class Component;
}  // namespace hub

// Initializes the content of the |info| object by given data.
void SetupComponentInfo(const char* string_id,
                        const char* name,
                        const char* description,
                        const uint32* produce_message,
                        size_t produce_message_size,
                        const uint32* consume_message,
                        size_t consume_message_size,
                        proto::ComponentInfo* info);

// Creates a hub::Component object with given information. It's only for testing
// hub::HubImpl related code.
hub::Component* CreateTestComponent(const uint32 id,
                                    Hub::Connector* connector,
                                    const char* string_id,
                                    const char* name,
                                    const char* description,
                                    const uint32* produce_message,
                                    size_t produce_message_size,
                                    const uint32* consume_message,
                                    size_t consume_message_size);

// Waits on the |queue| for |timeout| milliseconds until the |queue| is not
// empty. The |event| is used for wait and the |lock| will be locked when
// accessing the |queue|. |timeout| == 0 means no wait, |timeout| < 0 means
// wait forever until the |queue| is not empty. Returns true if the |queue|
// becomes not empty within the |timeout|.
bool WaitOnMessageQueue(int timeout,
                        std::queue<proto::Message*>* queue,
                        base::WaitableEvent* event,
                        base::Lock* lock);

// Creates a new message object with given information and a unique serial
// number.
proto::Message* NewMessageForTest(uint32 type,
                                  proto::Message::ReplyMode reply_mode,
                                  uint32 source,
                                  uint32 target,
                                  uint32 icid);

// Checks some information of a given message object. This function calls
// ASSERT_XXX to do the check, so ASSERT_NO_FATAL_FAILURE(CheckMessage(...))
// should be used to capture check failures.
void CheckMessage(const proto::Message* message,
                  uint32 type,
                  uint32 source,
                  uint32 target,
                  uint32 icid,
                  proto::Message::ReplyMode reply_mode,
                  bool has_payload);

// Checks unordered values in the uint32 payload of a message object. If |exact|
// is true, then the uint32 payload should have the exact same number of values
// specified by |expected_size|, otherwise the payload could have more values.
// This function calls ASSERT_XXX to do the check, so
// ASSERT_NO_FATAL_FAILURE(CheckUnorderedUint32Payload(...)) should be used to
// capture check failures.
void CheckUnorderedUint32Payload(const proto::Message* message,
                                 const uint32* expected_values,
                                 size_t expected_size,
                                 bool exact);
}  // namespace ipc

#endif  // GOOPY_IPC_TEST_UTIL_H_
