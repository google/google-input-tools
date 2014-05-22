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

#include "ipc/mock_message_channel.h"

#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace {

class MockMessageChannelTest : public ::testing::Test,
                               public ipc::MessageChannel::Listener {
 public:
  MockMessageChannelTest()
      : received_(true, false),
        thread_id_(base::PlatformThread::CurrentId()),
        connected_called_(false),
        closed_called_(false),
        attached_called_(false),
        detached_called_(false) {
  }

 protected:
  // Overridden from ipc::MessageChannel::Listener:
  virtual void OnMessageReceived(ipc::MessageChannel* channel,
                                 ipc::proto::Message* message) OVERRIDE {
    ASSERT_EQ(&channel_, channel);
    ASSERT_NE(thread_id_, base::PlatformThread::CurrentId());
    delete message;
    received_.Signal();
  }

  virtual void OnMessageChannelConnected(
      ipc::MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(&channel_, channel);
    connected_called_ = true;
  }

  virtual void OnMessageChannelClosed(ipc::MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(&channel_, channel);
    closed_called_ = true;
  }

  virtual void OnAttachedToMessageChannel(
      ipc::MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(&channel_, channel);
    attached_called_ = true;
  }

  virtual void OnDetachedFromMessageChannel(
      ipc::MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(&channel_, channel);
    detached_called_ = true;
  }

  void Reset() {
    received_.Reset();
    connected_called_ = false;
    closed_called_ = false;
    attached_called_ = false;
    detached_called_ = false;
  }

  ipc::MockMessageChannel channel_;
  base::WaitableEvent received_;
  base::PlatformThreadId thread_id_;

  bool connected_called_;
  bool closed_called_;
  bool attached_called_;
  bool detached_called_;
};

TEST_F(MockMessageChannelTest, SetListener) {
  ASSERT_TRUE(channel_.Init());
  channel_.SetListener(this);
  EXPECT_TRUE(attached_called_);
  EXPECT_FALSE(detached_called_);

  Reset();
  channel_.SetListener(NULL);
  EXPECT_FALSE(attached_called_);
  EXPECT_TRUE(detached_called_);
}

TEST_F(MockMessageChannelTest, Connected) {
  ASSERT_TRUE(channel_.Init());
  channel_.SetListener(this);

  EXPECT_FALSE(channel_.IsConnected());

  channel_.SetConnected(true);
  EXPECT_TRUE(channel_.IsConnected());
  EXPECT_TRUE(connected_called_);
  EXPECT_FALSE(closed_called_);

  Reset();
  channel_.SetConnected(true);
  EXPECT_TRUE(channel_.IsConnected());
  EXPECT_FALSE(connected_called_);
  EXPECT_FALSE(closed_called_);

  channel_.SetConnected(false);
  EXPECT_FALSE(channel_.IsConnected());
  EXPECT_FALSE(connected_called_);
  EXPECT_TRUE(closed_called_);

  Reset();
  channel_.SetConnected(false);
  EXPECT_FALSE(channel_.IsConnected());
  EXPECT_FALSE(connected_called_);
  EXPECT_FALSE(closed_called_);
}

TEST_F(MockMessageChannelTest, Send) {
  ASSERT_TRUE(channel_.Init());
  channel_.SetListener(this);

  EXPECT_FALSE(channel_.Send(new ipc::proto::Message()));
  channel_.SetConnected(true);
  EXPECT_TRUE(channel_.Send(new ipc::proto::Message()));
  channel_.SetSendEnabled(false);
  EXPECT_FALSE(channel_.Send(new ipc::proto::Message()));
  scoped_ptr<ipc::proto::Message> msg(channel_.WaitMessage(0));
  EXPECT_TRUE(msg.get());
  msg.reset(channel_.WaitMessage(0));
  EXPECT_FALSE(msg.get());
}

TEST_F(MockMessageChannelTest, PostMessageToListener) {
  ASSERT_TRUE(channel_.Init());
  channel_.SetListener(this);

  channel_.PostMessageToListener(new ipc::proto::Message());
  received_.Wait();
  EXPECT_TRUE(received_.IsSignaled());
}

}  // namespace
