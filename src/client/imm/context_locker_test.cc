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

#include "imm/context_locker.h"
#include "imm/immdev.h"
#include "imm/mock_objects.h"
#include "imm/test_main.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
typedef HIMCLockerT<INPUTCONTEXT, MockImmLockPolicy> TestHIMCLocker;

static const HIMC kDummyHIMC = reinterpret_cast<HIMC>(1);

TEST(HIMCLockerTest, LockCount) {
  MockImmLockPolicy::Reset();
  {
    TestHIMCLocker locker(kDummyHIMC);
    ASSERT_TRUE(locker.get());
    EXPECT_EQ(1, MockImmLockPolicy::input_context_ref_);
  }
  EXPECT_EQ(0, MockImmLockPolicy::input_context_ref_);
}

TEST(HIMCLockerTest, Access) {
  MockImmLockPolicy::Reset();
  TestHIMCLocker locker(kDummyHIMC);
  ASSERT_TRUE(locker.get());

  locker->fOpen = TRUE;
  EXPECT_TRUE(MockImmLockPolicy::input_context_.fOpen);
}

typedef HIMCCLockerT<uint8, MockImmLockPolicy> TestHIMCCLocker;

static const int kComponentSize = 32;

TEST(HIMCCLockerTest, Create) {
  HIMCC himcc = NULL;
  MockImmLockPolicy::Component* component = NULL;
  {
    TestHIMCCLocker locker(&himcc, kComponentSize);
    ASSERT_TRUE(himcc);
    component = reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
    EXPECT_EQ(kComponentSize, component->size);
    EXPECT_EQ(1, component->ref);
  }
  EXPECT_EQ(0, component->ref);
  MockImmLockPolicy::DestroyComponent(himcc);
}

TEST(HIMCCLockerTest, Resize) {
  HIMCC himcc = MockImmLockPolicy::CreateComponent(10);
  MockImmLockPolicy::Component* component =
      reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  EXPECT_EQ(10, component->size);
  {
    TestHIMCCLocker locker(&himcc, kComponentSize);
    ASSERT_TRUE(himcc);
    EXPECT_EQ(kComponentSize, component->size);
    EXPECT_EQ(1, component->ref);
  }
  EXPECT_EQ(0, component->ref);
  MockImmLockPolicy::DestroyComponent(himcc);
}

TEST(HIMCCLockerTest, Access) {
  HIMCC himcc = MockImmLockPolicy::CreateComponent(kComponentSize);
  MockImmLockPolicy::Component* component =
      reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  EXPECT_EQ(0, component->ref);
  {
    TestHIMCCLocker locker(himcc);
    // Test ! operator.
    ASSERT_FALSE(!locker);
    // Test != operator.
    ASSERT_FALSE(locker != NULL);
    // Test == operator.
    ASSERT_TRUE(locker == component->buffer);

    // Test [] operator.
    locker[0] = 0;
    locker[1] = 1;
    EXPECT_EQ(0, locker[0]);
    EXPECT_EQ(1, locker[1]);

    EXPECT_EQ(1, component->ref);
  }
  EXPECT_EQ(0, component->ref);
  MockImmLockPolicy::DestroyComponent(himcc);
}
}  // namespace imm
}  // namespace ime_goopy
