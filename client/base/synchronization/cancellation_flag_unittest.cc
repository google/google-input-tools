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

// Tests of CancellationFlag class.

#include "base/synchronization/cancellation_flag.h"

#include "base/logging.h"
//#include "base/message_loop.h"
//#include "base/spin_wait.h"
//#include "base/time.h"
//#include "base/threading/thread.h"
#include "testing/base/gunit.h"
//#include "testing/platform_test.h"

namespace base {

namespace {

//------------------------------------------------------------------------------
// Define our test class.
//------------------------------------------------------------------------------

 /*
class CancelTask : public Task {
 public:
  explicit CancelTask(CancellationFlag* flag) : flag_(flag) {}
  virtual void Run() {
    ASSERT_DEBUG_DEATH(flag_->Set(), "");
  }
 private:
  CancellationFlag* flag_;
};
*/

TEST(CancellationFlagTest, SimpleSingleThreadedTest) {
  CancellationFlag flag;
  ASSERT_FALSE(flag.IsSet());
  flag.Set();
  ASSERT_TRUE(flag.IsSet());
}

TEST(CancellationFlagTest, DoubleSetTest) {
  CancellationFlag flag;
  ASSERT_FALSE(flag.IsSet());
  flag.Set();
  ASSERT_TRUE(flag.IsSet());
  flag.Set();
  ASSERT_TRUE(flag.IsSet());
}
/*
TEST(CancellationFlagTest, SetOnDifferentThreadDeathTest) {
  // Checks that Set() can't be called from any other thread.
  // CancellationFlag should die on a DCHECK if Set() is called from
  // other thread.
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  Thread t("CancellationFlagTest.SetOnDifferentThreadDeathTest");
  ASSERT_TRUE(t.Start());
  ASSERT_TRUE(t.message_loop());
  ASSERT_TRUE(t.IsRunning());

  CancellationFlag flag;
  t.message_loop()->PostTask(FROM_HERE, new CancelTask(&flag));
}
*/

}  // namespace

}  // namespace base
