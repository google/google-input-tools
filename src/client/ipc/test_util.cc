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

#include "ipc/test_util.h"

#include "base/logging.h"
#include "base/time.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/constants.h"
#include "ipc/hub_component.h"
#include "ipc/testing.h"

namespace ipc {

void SetupComponentInfo(const char* string_id,
                        const char* name,
                        const char* description,
                        const uint32* produce_message,
                        size_t produce_message_size,
                        const uint32* consume_message,
                        size_t consume_message_size,
                        proto::ComponentInfo* info) {
  info->set_string_id(string_id);
  info->set_name(name);
  info->set_description(description);

  for (size_t i = 0; i < produce_message_size; ++i)
    info->add_produce_message(produce_message[i]);
  for (size_t i = 0; i < consume_message_size; ++i)
    info->add_consume_message(consume_message[i]);
}

hub::Component* CreateTestComponent(const uint32 id,
                                    Hub::Connector* connector,
                                    const char* string_id,
                                    const char* name,
                                    const char* description,
                                    const uint32* produce_message,
                                    size_t produce_message_size,
                                    const uint32* consume_message,
                                    size_t consume_message_size) {
  proto::ComponentInfo info;
  info.set_id(id);

  SetupComponentInfo(string_id, name, description,
                     produce_message, produce_message_size,
                     consume_message, consume_message_size,
                     &info);

  return new hub::Component(id, connector, info);
}

bool WaitOnMessageQueue(int timeout,
                        std::queue<proto::Message*>* queue,
                        base::WaitableEvent* event,
                        base::Lock* lock) {
  DCHECK(queue);
  DCHECK(event);
  DCHECK(lock);

  base::AutoLock auto_lock(*lock);
  if (!timeout)
    return !queue->empty();

  const base::TimeTicks start_time = base::TimeTicks::Now();
  int64 remained_timeout = timeout;

  while (queue->empty()) {
    base::AutoUnlock auto_unlock(*lock);
    if (timeout > 0) {
      event->TimedWait(base::TimeDelta::FromMilliseconds(remained_timeout));
      remained_timeout = timeout -
          (base::TimeTicks::Now() - start_time).InMilliseconds();
      if (remained_timeout <= 0)
        break;
    } else {
      event->Wait();
    }
  }

  return !queue->empty();
}

proto::Message* NewMessageForTest(uint32 type,
                                  proto::Message::ReplyMode reply_mode,
                                  uint32 source,
                                  uint32 target,
                                  uint32 icid) {
  static uint32 g_message_serial = 0;
  proto::Message* message = new proto::Message();
  message->set_type(type);
  message->set_reply_mode(reply_mode);
  message->set_source(source);
  message->set_target(target);
  message->set_icid(icid);
  message->set_serial(++g_message_serial);
  return message;
}

void CheckMessage(const proto::Message* message,
                  uint32 type,
                  uint32 source,
                  uint32 target,
                  uint32 icid,
                  proto::Message::ReplyMode reply_mode,
                  bool has_payload) {
  ASSERT_EQ(type, message->type());
  if (source != kComponentBroadcast)
    ASSERT_EQ(source, message->source());
  if (target != kComponentBroadcast)
    ASSERT_EQ(target, message->target());
  ASSERT_EQ(icid, message->icid());
  ASSERT_EQ(reply_mode, message->reply_mode());
  ASSERT_EQ(has_payload, message->has_payload());
}

void CheckUnorderedUint32Payload(const proto::Message* message,
                                 const uint32* expected_values,
                                 size_t expected_size,
                                 bool exact) {
  std::set<uint32> values;
  ASSERT_TRUE(message->has_payload());
  const proto::MessagePayload& payload = message->payload();
  int size = payload.uint32_size();
  for (int i = 0; i < size; ++i)
    values.insert(payload.uint32(i));

  if (exact)
    ASSERT_EQ(expected_size, size);
  else
    ASSERT_LE(expected_size, size);

  for (size_t i = 0; i < expected_size; ++i) {
    ASSERT_TRUE(values.count(expected_values[i]))
        << "Missing:" << expected_values[i];
  }
}

}  // namespace ipc
