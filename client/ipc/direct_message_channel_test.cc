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

#include "ipc/direct_message_channel.h"

#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace {
using namespace ipc;

class DirectMessageChannelTest : public ::testing::Test,
                                 public Hub,
                                 public MessageChannel::Listener {
 protected:
  DirectMessageChannelTest()
      : connector_(NULL),
        dispatched_(false),
        received_(false),
        connected_(false),
        closed_(false),
        attached_(false),
        detached_(false) {
  }

  virtual ~DirectMessageChannelTest() {
  }

  virtual void SetUp() {
    channel_.reset(new DirectMessageChannel(this));
  }

  virtual void TearDown() {
  }

  // Overridden from Hub:
  virtual void Attach(Connector* connector) OVERRIDE {
    ASSERT_TRUE(connector);
    ASSERT_FALSE(connector_);
    connector_ = connector;
    connector->Attached();
  }

  virtual void Detach(Connector* connector) OVERRIDE {
    ASSERT_TRUE(connector);
    ASSERT_EQ(connector_, connector);
    connector_ = NULL;
    connector->Detached();
  }

  virtual bool Dispatch(Connector* connector,
                        proto::Message* message) OVERRIDE {
    delete message;
    EXPECT_EQ(connector_, connector);
    dispatched_ = true;
    return true;
  }

  // Overridden from MessageChannel::Listener:
  virtual void OnMessageReceived(MessageChannel* channel,
                                 proto::Message* message) OVERRIDE {
    delete message;
    ASSERT_EQ(channel_.get(), channel);
    received_ = true;
  }

  virtual void OnMessageChannelConnected(MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(channel_.get(), channel);
    ASSERT_TRUE(channel->IsConnected());
    connected_ = true;
  }

  virtual void OnMessageChannelClosed(MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(channel_.get(), channel);
    ASSERT_FALSE(channel->IsConnected());
    closed_ = true;
  }

  virtual void OnAttachedToMessageChannel(MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(channel_.get(), channel);
    ASSERT_FALSE(channel->IsConnected());
    attached_ = true;
  }

  virtual void OnDetachedFromMessageChannel(MessageChannel* channel) OVERRIDE {
    ASSERT_EQ(channel_.get(), channel);
    ASSERT_FALSE(channel->IsConnected());
    detached_ = true;
  }

  void Clear() {
    dispatched_ = false;
    received_ = false;
    connected_ = false;
    closed_ = false;
    attached_ = false;
    detached_ = false;
  }

  scoped_ptr<DirectMessageChannel> channel_;
  Connector* connector_;
  bool dispatched_;
  bool received_;
  bool connected_;
  bool closed_;
  bool attached_;
  bool detached_;
};

TEST_F(DirectMessageChannelTest, SetListener) {
  ASSERT_FALSE(channel_->IsConnected());
  channel_->SetListener(this);
  ASSERT_TRUE(channel_->IsConnected());
  ASSERT_TRUE(connected_);
  ASSERT_FALSE(closed_);
  ASSERT_TRUE(attached_);
  ASSERT_FALSE(detached_);

  Clear();
  channel_->SetListener(this);
  ASSERT_TRUE(channel_->IsConnected());
  ASSERT_FALSE(connected_);
  ASSERT_FALSE(closed_);
  ASSERT_FALSE(attached_);
  ASSERT_FALSE(detached_);

  Clear();
  channel_->SetListener(NULL);
  ASSERT_FALSE(channel_->IsConnected());
  ASSERT_FALSE(connected_);
  ASSERT_TRUE(closed_);
  ASSERT_FALSE(attached_);
  ASSERT_TRUE(detached_);

  channel_->SetListener(this);

  Clear();
  channel_.reset();
  ASSERT_FALSE(connected_);
  ASSERT_TRUE(closed_);
  ASSERT_FALSE(attached_);
  ASSERT_TRUE(detached_);
}

TEST_F(DirectMessageChannelTest, SendDispatch) {
  ASSERT_FALSE(channel_->Send(new proto::Message()));
  ASSERT_FALSE(dispatched_);
  channel_->SetListener(this);
  ASSERT_TRUE(channel_->Send(new proto::Message()));
  ASSERT_TRUE(dispatched_);

  Clear();
  ASSERT_TRUE(connector_->Send(new proto::Message()));
  ASSERT_TRUE(received_);
}

}  // namespace
