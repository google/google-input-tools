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

#include "imm/context.h"
#include "imm/context_manager.h"
#include "imm/immdev.h"
#include "imm/mock_objects.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
typedef ContextT<MockImmLockPolicy, MockMessageQueue> TestContext;
typedef ContextManagerT<TestContext> TestContextManager;

TEST(ContextManager, Access) {
  MockMessageQueue* message_queue = new MockMessageQueue(0);
  TestContextManager& manager = TestContextManager::Instance();
  TestContext* context = new TestContext(0, message_queue);
  EXPECT_TRUE(manager.Add(0, context));
  EXPECT_EQ(context, manager.Get(0));

  HIMC himc = reinterpret_cast<HIMC>(1);
  EXPECT_FALSE(manager.Destroy(himc));
  EXPECT_TRUE(manager.Destroy(0));
  EXPECT_TRUE(NULL == manager.Get(0));

  MockMessageQueue* message_queue1 = new MockMessageQueue(0);
  TestContext* context1 = new TestContext(0, message_queue1);
  EXPECT_TRUE(manager.Add(0, context1));
  MockMessageQueue* message_queue2 = new MockMessageQueue(himc);
  TestContext* context2 = new TestContext(0, message_queue2);
  EXPECT_TRUE(manager.Add(himc, context2));
  EXPECT_EQ(context1, manager.Get(0));
  EXPECT_EQ(context2, manager.Get(himc));
  manager.DestroyAll();
  EXPECT_TRUE(NULL == manager.Get(0));
  EXPECT_TRUE(NULL == manager.Get(himc));
}
}  // namespace imm
}  // namespace ime_goopy
