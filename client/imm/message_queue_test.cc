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

#include "base/scoped_ptr.h"
#include "imm/message_queue.h"
#include "imm/immdev.h"
#include "imm/mock_objects.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
class MockImmMessagePolicy {
 public:
  static BOOL ImmGenerateMessage(HIMC himc) {
    generate_message_called_ = true;
    return TRUE;
  }

  static void Reset() {
    generate_message_called_ = false;
  }

  static bool generate_message_called_;
};
bool MockImmMessagePolicy::generate_message_called_ = false;

typedef MessageQueueT<MockImmLockPolicy, MockImmMessagePolicy> TestMessageQueue;

static const HIMC kDummyHIMC = reinterpret_cast<HIMC>(1);
TEST(MessageQueue, TransMsgList) {
  TestMessageQueue message_queue(kDummyHIMC);

  TRANSMSGLIST list;
  ZeroMemory(&list, sizeof(list));
  list.uMsgCount = 1;

  message_queue.Attach(&list);
  message_queue.AddMessage(WM_CHAR, L'A', 0);

  MockImmMessagePolicy::Reset();
  EXPECT_EQ(1, message_queue.Detach());
  EXPECT_FALSE(MockImmMessagePolicy::generate_message_called_);
  EXPECT_EQ(WM_CHAR, list.TransMsg[0].message);
  EXPECT_EQ(L'A', list.TransMsg[0].wParam);
  EXPECT_EQ(0, list.TransMsg[0].lParam);
}

TEST(MessageQueue, TransMsgListExtraMessage) {
  TestMessageQueue message_queue(kDummyHIMC);

  TRANSMSGLIST list;
  ZeroMemory(&list, sizeof(list));
  list.uMsgCount = 1;

  MockImmLockPolicy::input_context_.hMsgBuf = NULL;

  message_queue.Attach(&list);
  message_queue.AddMessage(WM_CHAR, L'A', 0);
  message_queue.AddMessage(WM_CHAR, L'B', 0);

  MockImmMessagePolicy::Reset();
  MockImmLockPolicy::input_context_.hMsgBuf = NULL;
  EXPECT_EQ(2, message_queue.Detach());
  EXPECT_FALSE(MockImmMessagePolicy::generate_message_called_);

  HIMCC himcc = MockImmLockPolicy::input_context_.hMsgBuf;
  MockImmLockPolicy::Component* component =
    reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  EXPECT_EQ(0, MockImmLockPolicy::input_context_.dwNumMsgBuf);
  EXPECT_EQ(sizeof(TRANSMSG) * 2, component->size);
  TRANSMSG* messages = reinterpret_cast<TRANSMSG*>(component->buffer);
  EXPECT_EQ(WM_CHAR, messages[0].message);
  EXPECT_EQ(L'A', messages[0].wParam);
  EXPECT_EQ(0, messages[0].lParam);
  EXPECT_EQ(WM_CHAR, messages[1].message);
  EXPECT_EQ(L'B', messages[1].wParam);
  EXPECT_EQ(0, messages[1].lParam);
  MockImmLockPolicy::DestroyComponent(
    MockImmLockPolicy::input_context_.hMsgBuf);
}

TEST(MessageQueue, Send) {
  TestMessageQueue message_queue(kDummyHIMC);
  message_queue.AddMessage(WM_CHAR, L'A', 0);

  MockImmMessagePolicy::Reset();
  MockImmLockPolicy::input_context_.hMsgBuf = NULL;
  EXPECT_TRUE(message_queue.Send());
  EXPECT_TRUE(MockImmMessagePolicy::generate_message_called_);

  HIMCC himcc = MockImmLockPolicy::input_context_.hMsgBuf;
  MockImmLockPolicy::Component* component =
    reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  EXPECT_EQ(1, MockImmLockPolicy::input_context_.dwNumMsgBuf);
  EXPECT_EQ(sizeof(TRANSMSG), component->size);
  TRANSMSG* messages = reinterpret_cast<TRANSMSG*>(component->buffer);
  EXPECT_EQ(WM_CHAR, messages[0].message);
  EXPECT_EQ(L'A', messages[0].wParam);
  EXPECT_EQ(0, messages[0].lParam);
  MockImmLockPolicy::DestroyComponent(
    MockImmLockPolicy::input_context_.hMsgBuf);
}
}  // namespace imm
}  // namespace ime_goopy
