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

#include "base/time.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "testing/base/public/gunit.h"

namespace base {

TEST(WaitableEventTest, ManualBasics) {
  WaitableEvent event(true, false);

  EXPECT_FALSE(event.IsSignaled());

  event.Signal();
  EXPECT_TRUE(event.IsSignaled());
  EXPECT_TRUE(event.IsSignaled());

  event.Reset();
  EXPECT_FALSE(event.IsSignaled());
  EXPECT_FALSE(event.TimedWait(TimeDelta::FromMilliseconds(10)));

  event.Signal();
  EXPECT_TRUE(event.Wait());
  EXPECT_TRUE(event.TimedWait(TimeDelta::FromMilliseconds(10)));
}

TEST(WaitableEventTest, AutoBasics) {
  WaitableEvent event(false, false);

  EXPECT_FALSE(event.IsSignaled());

  event.Signal();
  EXPECT_TRUE(event.IsSignaled());
  EXPECT_FALSE(event.IsSignaled());

  event.Reset();
  EXPECT_FALSE(event.IsSignaled());
  EXPECT_FALSE(event.TimedWait(TimeDelta::FromMilliseconds(10)));

  event.Signal();
  EXPECT_TRUE(event.Wait());
  EXPECT_FALSE(event.TimedWait(TimeDelta::FromMilliseconds(10)));

  event.Signal();
  EXPECT_TRUE(event.TimedWait(TimeDelta::FromMilliseconds(10)));
}

TEST(WaitableEventTest, WaitManyShortcut) {
  WaitableEvent* ev[5];
  for (unsigned i = 0; i < 5; ++i)
    ev[i] = new WaitableEvent(false, false);

  ev[3]->Signal();
  EXPECT_EQ(WaitableEvent::WaitMany(ev, 5), 3u);

  ev[3]->Signal();
  EXPECT_EQ(WaitableEvent::WaitMany(ev, 5), 3u);

  ev[4]->Signal();
  EXPECT_EQ(WaitableEvent::WaitMany(ev, 5), 4u);

  ev[0]->Signal();
  EXPECT_EQ(WaitableEvent::WaitMany(ev, 5), 0u);

  for (unsigned i = 0; i < 5; ++i)
    delete ev[i];
}

class WaitableEventSignaler : public PlatformThread::Delegate {
 public:
  WaitableEventSignaler(double seconds, WaitableEvent* ev)
      : seconds_(seconds),
        ev_(ev) {
  }

  void ThreadMain() {
    PlatformThread::Sleep(static_cast<int>(seconds_ * 1000));
    ev_->Signal();
  }

 private:
  const double seconds_;
  WaitableEvent *const ev_;
};

TEST(WaitableEventTest, WaitMany) {
  WaitableEvent* ev[5];
  for (unsigned i = 0; i < 5; ++i)
    ev[i] = new WaitableEvent(false, false);

  WaitableEventSignaler signaler(0.1, ev[2]);
  PlatformThreadHandle thread;
  PlatformThread::Create(0, &signaler, &thread);

  EXPECT_EQ(WaitableEvent::WaitMany(ev, 5), 2u);

  PlatformThread::Join(thread);

  for (unsigned i = 0; i < 5; ++i)
    delete ev[i];
}

}  // namespace base
