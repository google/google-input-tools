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

#include "ipc/multi_component_host.h"

#include <queue>
#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"
#include "ipc/mock_component.h"
#include "ipc/mock_message_channel.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace {

using namespace ipc;

const uint32 kTimeout = 10000;
const uint32 kSmallTimeout = 100;
enum CustumMessagesForWaitTest {
  MSG_TEST = MSG_USER_DEFINED_START,
};

class MockComponentForWait1 : public MockComponent {
 public:
  explicit MockComponentForWait1(const char* id)
    : registered_(false),
      MockComponent(id) {
  }

  virtual ~MockComponentForWait1() {}

  virtual void OnRegistered() OVERRIDE {
#if defined(OS_WIN)
    Sleep(kSmallTimeout);
#else
    usleep(kSmallTimeout * 1000);
#endif
    registered_ = true;
  }

  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE {
    MockComponent::GetInfo(info);
    info->add_produce_message(MSG_TEST);
  }

  bool IsRegistered() {
    return registered_;
  }

 private:
  bool registered_;
  DISALLOW_COPY_AND_ASSIGN(MockComponentForWait1);
};

// Base class holding all common test code.
class MultiComponentHostTestBase : public ::testing::Test {
 protected:
  enum {
    TEST_MSG_TYPE1 = MSG_USER_DEFINED_START,
    TEST_MSG_TYPE2,
    TEST_MSG_TYPE3,
    TEST_MSG_TYPE4,
    TEST_MSG_TYPE5,
  };
  virtual bool AddComponent(MockComponent* component) = 0;
  virtual bool RemoveComponent(MockComponent* component) = 0;

  void HandleMsgRegisterComponent(MockMessageChannel* channel,
                                  const std::string& string_id,
                                  uint32 id) {
    // Wait for MSG_REGISTER_COMPONENT
    scoped_ptr<proto::Message> mptr(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_REGISTER_COMPONENT, mptr->type());
    ASSERT_EQ(1, mptr->payload().component_info_size());
    EXPECT_EQ(kComponentDefault, mptr->payload().component_info(0).id());
    EXPECT_EQ(string_id, mptr->payload().component_info(0).string_id());

    // Reply MSG_REGISTER_COMPONENT
    ConvertToReplyMessage(mptr.get());
    mptr->mutable_payload()->mutable_component_info(0)->set_id(id);
    channel->PostMessageToListener(mptr.release());
  }

  // Tests adding and removing components.
  void TestAddRemove() {
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));
    scoped_ptr<MockComponent> comp2(new MockComponent("comp2"));
    scoped_ptr<MockComponent> comp_dup(new MockComponent("comp1"));

    // Add and remove components without message channel.
    ASSERT_TRUE(AddComponent(comp1.get()));
    ASSERT_NE(base::kInvalidThreadId, comp1->thread_id());
    ASSERT_NE(base::PlatformThread::CurrentId(), comp1->thread_id());

    // Test Component::DidAddToHost().
    ASSERT_EQ(host_.get(), comp1->host());

    EXPECT_FALSE(AddComponent(comp1.get()));
    EXPECT_TRUE(AddComponent(comp2.get()));
    EXPECT_FALSE(AddComponent(comp_dup.get()));

    EXPECT_TRUE(RemoveComponent(comp1.get()));

    // Test Component::DidRemoveFromHost().
    EXPECT_EQ(NULL, comp1->host());

    EXPECT_FALSE(RemoveComponent(comp1.get()));
    EXPECT_TRUE(RemoveComponent(comp2.get()));
    EXPECT_FALSE(RemoveComponent(comp_dup.get()));

    EXPECT_TRUE(AddComponent(comp_dup.get()));
    EXPECT_TRUE(RemoveComponent(comp_dup.get()));

    // Add and remove components with a message channel.
    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());

    // Add one component.
    ASSERT_TRUE(AddComponent(comp1.get()));

    // Handle MSG_REGISTER_COMPONENT
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    // comp1 should have been registered successfully.
    EXPECT_EQ(1, comp1->id());

    // Add another component.
    ASSERT_TRUE(AddComponent(comp2.get()));

    // Handle MSG_REGISTER_COMPONENT
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp2", 2));

    // Wait until |comp2->OnRegistered()| gets called.
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp2->PopIncomingMessage());

    // comp1 should have been registered successfully.
    EXPECT_EQ(2, comp2->id());

    // Components should be deregistered when the message channel is broken.
    channel->SetConnected(false);

    // Wait until |comp1->OnDeregistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    EXPECT_EQ(kComponentDefault, comp1->id());

    // Wait until |comp2->OnDeregistered()| gets called.
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp2->PopIncomingMessage());

    EXPECT_EQ(kComponentDefault, comp2->id());

    // Components should be registered automatically when the message channel
    // gets connected again.
    channel->SetConnected(true);

    // Wait for MSG_REGISTER_COMPONENT messages
    std::vector<proto::Message*> messages;
    for (int i = 0; i < 2; ++i) {
      scoped_ptr<proto::Message> mptr(channel->WaitMessage(kTimeout));
      ASSERT_TRUE(mptr.get());
      EXPECT_EQ(MSG_REGISTER_COMPONENT, mptr->type());
      ASSERT_EQ(1, mptr->payload().component_info_size());
      EXPECT_EQ(kComponentDefault, mptr->payload().component_info(0).id());
      ConvertToReplyMessage(mptr.get());

      if (mptr->payload().component_info(0).string_id() == "comp1") {
        mptr->mutable_payload()->mutable_component_info(0)->set_id(1);
      } else if (mptr->payload().component_info(0).string_id() == "comp2") {
        mptr->mutable_payload()->mutable_component_info(0)->set_id(2);
      } else {
        ASSERT_TRUE(false) << "Unexpected MSG_REGISTER_COMPONENT message:"
                           << mptr->payload().component_info(0).string_id();
      }
      messages.push_back(mptr.release());
    }

    // Removing a component before receiving the reply of its
    // MSG_REGISTER_COMPONENT message should cause a MSG_DEREGISTER_COMPONENT
    // message.
    EXPECT_TRUE(RemoveComponent(comp2.get()));

    // Send reply messages back.
    for (int i = 0; i < 2; ++i)
      channel->PostMessageToListener(messages[i]);

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    // comp1 should have been registered successfully.
    EXPECT_EQ(1, comp1->id());

    // Wait for MSG_DEREGISTER_COMPONENT for comp2.
    scoped_ptr<proto::Message> mptr(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_DEREGISTER_COMPONENT, mptr->type());
    ASSERT_EQ(1, mptr->payload().uint32_size());
    EXPECT_EQ(2, mptr->payload().uint32(0));

    // comp2 should never be registered.
    EXPECT_EQ(0, comp2->id());

    // Remove a component with a connected message channel.
    EXPECT_TRUE(RemoveComponent(comp1.get()));

    // Wait until |comp1->OnDeregistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    EXPECT_EQ(kComponentDefault, comp1->id());

    // Wait for MSG_DEREGISTER_COMPONENT for comp1.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_DEREGISTER_COMPONENT, mptr->type());
    ASSERT_EQ(1, mptr->payload().uint32_size());
    EXPECT_EQ(1, mptr->payload().uint32(0));
  }

  // Tests switching message channel.
  void TestSwitchMessageChannel() {
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));

    // Add a component before setting up message channel.
    ASSERT_TRUE(AddComponent(comp1.get()));

    // Check if the component can send message inside OnRegistered() method.
    comp1->AddOutgoingMessage(new proto::Message(), true, 0);

    // Setup message channel.
    scoped_ptr<MockMessageChannel> channel1(new MockMessageChannel());
    ASSERT_TRUE(channel1->Init());
    channel1->SetConnected(true);
    host_->SetMessageChannel(channel1.get());

    // Handle MSG_REGISTER_COMPONENT
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel1.get(), "comp1", 1));

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    // comp1 should have been registered successfully.
    EXPECT_EQ(1, comp1->id());

    // Wait for the message sending from comp1's OnRegistered() method.
    scoped_ptr<proto::Message> mptr(channel1->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(1, mptr->source());

    // Make sure the component cannot send message inside OnDeregistered()
    // method.
    comp1->AddOutgoingMessage(new proto::Message(), false, 0);

    // Switch to another message channel.
    scoped_ptr<MockMessageChannel> channel2(new MockMessageChannel());
    ASSERT_TRUE(channel2->Init());
    channel2->SetConnected(true);
    host_->SetMessageChannel(channel2.get());

    // comp1 should be deregistered first and registered again through the new
    // message channel.
    // Wait until |comp1->OnDeregistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(kComponentDefault, comp1->id());

    // Wait for MSG_REGISTER_COMPONENT on the new channel.
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel2.get(), "comp1", 1));

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    // comp1 should have been registered successfully.
    EXPECT_EQ(1, comp1->id());

    EXPECT_TRUE(RemoveComponent(comp1.get()));
  }

  // Tests message dispatching among multiple components.
  void TestMessageDispatch() {
    // Setup message channel.
    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());


    // Add some components.
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));
    ASSERT_TRUE(AddComponent(comp1.get()));
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(1, comp1->id());

    scoped_ptr<MockComponent> comp2(new MockComponent("comp2"));
    ASSERT_TRUE(AddComponent(comp2.get()));
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp2", 2));
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp2->PopIncomingMessage());
    EXPECT_EQ(2, comp2->id());

    // Send a message to comp1.
    scoped_ptr<proto::Message> mptr(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(123);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(1, mptr->target());
    ASSERT_EQ(123, mptr->serial());

    // Send a message to comp2.
    mptr.reset(new proto::Message());
    mptr->set_target(2);
    mptr->set_serial(456);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    mptr.reset(comp2->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(2, mptr->target());
    ASSERT_EQ(456, mptr->serial());

    // Test sending a message from a component.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START);
    comp1->AddOutgoingMessage(mptr.release(), true, 0);

    // Send a message to comp1, then it should send back the outgoing message.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());

    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(1, mptr->source());
    ASSERT_EQ(MSG_USER_DEFINED_START, mptr->type());

    EXPECT_TRUE(RemoveComponent(comp1.get()));
    EXPECT_TRUE(RemoveComponent(comp2.get()));
  }

  // Tests MultiComponentHost::SendWithReply() method.
  void TestSendWithReply() {
    // Setup message channel.
    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());

    // Add a component.
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));
    ASSERT_TRUE(AddComponent(comp1.get()));
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(1, comp1->id());

    scoped_ptr<proto::Message> mptr;

    // Let comp1 do 4 recursive calls to SendWithReply() method.
    for (int i = 0; i < 4; ++i) {
      mptr.reset(new proto::Message());
      mptr->set_type(MSG_USER_DEFINED_START + i);
      mptr->set_target(1);
      mptr->set_reply_mode(proto::Message::NEED_REPLY);

      // We'll only reply level 2 and 3 and break level 0 and 1 below.
      comp1->AddOutgoingMessage(mptr.release(), i >= 2, kTimeout);
    }

    std::vector<uint32> reply_serials;
    // Send 4 messages to trigger 4 recursive SendWithReply() calls.
    for (int i = 0; i < 4; ++i) {
      mptr.reset(new proto::Message());
      mptr->set_type(MSG_PROCESS_KEY_EVENT);
      mptr->set_target(1);
      mptr->set_serial(i);
      mptr->set_reply_mode(proto::Message::NEED_REPLY);
      channel->PostMessageToListener(mptr.release());
      ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
      mptr.reset(comp1->PopIncomingMessage());
      ASSERT_TRUE(mptr.get());

      // Wait for the outgoing message from comp1.
      mptr.reset(channel->WaitMessage(kTimeout));
      ASSERT_TRUE(mptr.get());
      ASSERT_EQ(MSG_USER_DEFINED_START + i, mptr->type());
      ASSERT_EQ(proto::Message::NEED_REPLY, mptr->reply_mode());
      ASSERT_EQ(1, mptr->source());
      reply_serials.push_back(mptr->serial());

      // Check recursive level.
      ASSERT_EQ(i + 1, comp1->handle_count());
    }

    // Send back reply message of level 2 first, it should be delayed until we
    // send back reply message of level 3.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START + 2);
    mptr->set_target(1);
    mptr->set_serial(reply_serials[2]);
    mptr->set_reply_mode(proto::Message::IS_REPLY);
    channel->PostMessageToListener(mptr.release());

    // The component should still be able to handle any unrelated messages in
    // middle.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(123);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(123, mptr->serial());

    // The 4 recursived SendWithReply() calls should still be pending.
    ASSERT_EQ(4, comp1->handle_count());

    // A duplicated reply message of level 2 should be dispatched through
    // Component::Handle() method as a normal message.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START + 2);
    mptr->set_target(1);
    mptr->set_serial(reply_serials[2]);
    mptr->set_reply_mode(proto::Message::IS_REPLY);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_USER_DEFINED_START + 2, mptr->type());
    ASSERT_EQ(reply_serials[2], mptr->serial());

    // Send back reply message of level 3, it should unblock both level 2 and 3,
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START + 3);
    mptr->set_target(1);
    mptr->set_serial(reply_serials[3]);
    mptr->set_reply_mode(proto::Message::IS_REPLY);
    channel->PostMessageToListener(mptr.release());

    // Wait for reply message of level 2 and 3.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_PROCESS_KEY_EVENT, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_EQ(3, mptr->serial());

    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_PROCESS_KEY_EVENT, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_EQ(2, mptr->serial());

    // Wait for reply message of level 2 and 3 received by |comp1|.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_USER_DEFINED_START + 3, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_EQ(reply_serials[3], mptr->serial());

    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_USER_DEFINED_START + 2, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_EQ(reply_serials[2], mptr->serial());

    // level 0 and 1 of SendWithReply() calls should still be pending.
    ASSERT_EQ(2, comp1->handle_count());

    // All pending SendWithReply() calls should be unblocked when the
    // message channel is broken.
    channel->SetConnected(false);
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(kComponentDefault, comp1->id());

    channel->SetConnected(true);
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(1, comp1->id());

    // All recursive SendWithReply() calls should have been unblocked.
    ASSERT_EQ(0, comp1->handle_count());

    // Checks if SendWithReply() works with timeout == 0, which should return
    // false directly if the message needs reply.
    // Setup one level recursive SendWithReply() call again.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START);
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    comp1->AddOutgoingMessage(mptr.release(), false, 0);

    // Trigger the SendWithReply() call.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_PROCESS_KEY_EVENT);
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());

    // Wait for the outgoing message from comp1.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_USER_DEFINED_START, mptr->type());
    ASSERT_EQ(proto::Message::NEED_REPLY, mptr->reply_mode());
    ASSERT_EQ(1, mptr->source());

    // Wait for the reply message of the trigger message.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_PROCESS_KEY_EVENT, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_EQ(1, mptr->source());

    ASSERT_EQ(0, comp1->handle_count());

    if (!host_->create_thread()) {
      EXPECT_TRUE(RemoveComponent(comp1.get()));
      return;
    }

    // Following test only works if the component is running in its dedicated
    // thread.

    // Setup one level recursive SendWithReply() call again.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START);
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    comp1->AddOutgoingMessage(mptr.release(), false, kTimeout);

    // Trigger the SendWithReply() call.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_PROCESS_KEY_EVENT);
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());

    // Wait for the outgoing message from comp1.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_USER_DEFINED_START, mptr->type());
    ASSERT_EQ(proto::Message::NEED_REPLY, mptr->reply_mode());
    ASSERT_EQ(1, mptr->source());

    ASSERT_EQ(1, comp1->handle_count());

    // The blocked SendWithReply() call should be unblocked when removing the
    // component.
    EXPECT_TRUE(RemoveComponent(comp1.get()));

    // Wait until |comp1->OnDeregistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(kComponentDefault, comp1->id());

    ASSERT_EQ(0, comp1->handle_count());

    // The component should not send out any message after being removed, except
    // a MSG_DEREGISTER_COMPONENT message sent by the host.
    mptr.reset(channel->WaitMessage(0));
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_DEREGISTER_COMPONENT, mptr->type());
    ASSERT_EQ(1, mptr->payload().uint32_size());
    EXPECT_EQ(1, mptr->payload().uint32(0));

    ASSERT_FALSE(channel->WaitMessage(0));
  }

  // Tests destroying the |host_| with some components in it.
  void TestDestroyHostWithComponents() {
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));
    scoped_ptr<MockComponent> comp2(new MockComponent("comp2"));

    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());

    ASSERT_TRUE(AddComponent(comp1.get()));

    // Handle MSG_REGISTER_COMPONENT
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    ASSERT_TRUE(AddComponent(comp2.get()));

    // Handle MSG_REGISTER_COMPONENT
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp2", 2));

    // Wait until |comp2->OnRegistered()| gets called.
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp2->PopIncomingMessage());

    // Test Component::DidAddToHost().
    ASSERT_EQ(host_.get(), comp1->host());
    ASSERT_EQ(host_.get(), comp2->host());

    host_.reset();

    // Wait until |comp1->OnDeregistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    // Wait until |comp2->OnDeregistered()| gets called.
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp2->PopIncomingMessage());

    // Test Component::DidRemoveToHost().
    EXPECT_EQ(NULL, comp1->host());
    EXPECT_EQ(NULL, comp2->host());

    // Wait for MSG_DEREGISTER_COMPONENT for comp1 and comp2.
    scoped_ptr<proto::Message> mptr(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_DEREGISTER_COMPONENT, mptr->type());
    ASSERT_EQ(1, mptr->payload().uint32_size());

    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_DEREGISTER_COMPONENT, mptr->type());
    ASSERT_EQ(1, mptr->payload().uint32_size());
  }

  // Tests MultiComponentHost::{Pause|Resume}MessageHandling() methods.
  void TestPauseResumeMessageHandling() {
    // Setup message channel.
    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());

    // Add a component.
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));
    ASSERT_TRUE(AddComponent(comp1.get()));

    // {Pause|Resume}MessageHandling() do not affect MSG_REGISTER_COMPONENT.
    comp1->PauseMessageHandling();
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    EXPECT_EQ(1, comp1->id());
    comp1->ResumeMessageHandling();

    scoped_ptr<proto::Message> mptr;

    // Message handling is not paused.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(1);
    channel->PostMessageToListener(mptr.release());
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(1, mptr->serial());

    // Pause message handling.
    comp1->PauseMessageHandling();
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(2);
    channel->PostMessageToListener(mptr.release());
    channel->WaitForPostingMessagesToListener();
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // Call PauseMessageHandling() again should still keep it paused.
    comp1->PauseMessageHandling();
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(3);
    channel->PostMessageToListener(mptr.release());
    channel->WaitForPostingMessagesToListener();
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // Call ResumeMessageHandling() once shouldn't resume it.
    comp1->ResumeMessageHandling();
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // Actually resume message handing.
    comp1->ResumeMessageHandling();
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(2, mptr->serial());

    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(3, mptr->serial());

    // {Pause|Resume}MessageHandling() should work with SendWithReply().
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START);
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    comp1->AddOutgoingMessage(mptr.release(), true, kTimeout);

    // Trigger SendWithReply().
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    mptr->set_serial(5);
    channel->PostMessageToListener(mptr.release());

    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(5, mptr->serial());

    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_USER_DEFINED_START, mptr->type());
    ASSERT_EQ(proto::Message::NEED_REPLY, mptr->reply_mode());
    const int reply_serial = mptr->serial();

    comp1->PauseMessageHandling();

    // All other messages sent in the middle should be delayed.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(6);
    channel->PostMessageToListener(mptr.release());
    channel->WaitForPostingMessagesToListener();
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // Send back reply message to let SendWithReply() return.
    mptr.reset(new proto::Message());
    mptr->set_type(MSG_USER_DEFINED_START);
    mptr->set_target(1);
    mptr->set_reply_mode(proto::Message::IS_REPLY);
    mptr->set_serial(reply_serial);
    channel->PostMessageToListener(mptr.release());

    // Wait for reply message of message 5.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_EQ(5, mptr->serial());

    // Wait for reply message received by |comp1|.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(MSG_USER_DEFINED_START, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());

    // Message 6 should still be pending.
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // Resume message handling.
    comp1->ResumeMessageHandling();
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(6, mptr->serial());

    // All pending messages should be discarded if the channel is broken while
    // message handling is paused.
    comp1->PauseMessageHandling();
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(7);
    channel->PostMessageToListener(mptr.release());
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(8);
    channel->PostMessageToListener(mptr.release());
    channel->WaitForPostingMessagesToListener();

    channel->SetConnected(false);

    // Wait until |comp1->OnDeregistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());
    comp1->ResumeMessageHandling();

    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    EXPECT_TRUE(RemoveComponent(comp1.get()));
  }

  scoped_ptr<MultiComponentHost> host_;
};

// Class for testing the case that |create_thread| == true, i.e. each component
// has its own runner thread.
class MultiComponentHostTestCreateThread : public MultiComponentHostTestBase {
 protected:
  virtual void SetUp() {
    // Create runner thread for each component.
    host_.reset(new MultiComponentHost(true));
  }

  virtual void TearDown() {
    host_.reset();
  }

  virtual bool AddComponent(MockComponent* component) {
    // Each component has its own runner thread, so we can just add the
    // component on main thread.
    return host_->AddComponent(component);
  }

  virtual bool RemoveComponent(MockComponent* component) {
    // Each component has its own runner thread, so we can just remove the
    // component on main thread.
    return host_->RemoveComponent(component);
  }

  // Only Test WaitForComponents in CreateThread because it will block the main
  // thread.
  void TestWait() {
    // Setup message channel.
    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());

    scoped_ptr<MockComponentForWait1> component1(
        new MockComponentForWait1("comp1"));
    host_->AddComponent(component1.get());
    HandleMsgRegisterComponent(channel.get(), "comp1", 1);
    scoped_ptr<MockComponentForWait1> component2(
        new MockComponentForWait1("comp2"));
    host_->AddComponent(component2.get());
    HandleMsgRegisterComponent(channel.get(), "comp2", 2);
    int timeout = kTimeout;
    host_->WaitForComponents(&timeout);
    EXPECT_TRUE(component1->IsRegistered());
    EXPECT_LE(timeout + kSmallTimeout, kTimeout);
    host_->RemoveComponent(component1.get());
    host_->RemoveComponent(component2.get());
  }

  // Test order if pending messages exists before reply of SendWithReply
  // returns.
  // The message sequence is as followings:
  //
  // Component A    -    Component B
  //
  // M1(need_reply)                     -->
  // M2(no_reply)                       <--
  // M3(need reply, non_recursive)      -->
  // M4(no_reply)                       <--
  // R1                                 <--
  // M5(no_reply)                       <--
  // R3                                 <--
  //
  // The expected sequence of message handling of A is:
  // 1.Message 2
  // 2.Reply of Message 3
  // 3.Message 4
  // 4.Reply of Message 1
  // 5.Message 5
  //
  void TestMessageDispatchingOrderWithPausing() {
    // Setup message channel.
    scoped_ptr<MockMessageChannel> channel(new MockMessageChannel());
    ASSERT_TRUE(channel->Init());
    channel->SetConnected(true);
    host_->SetMessageChannel(channel.get());

    // Add component A.
    scoped_ptr<MockComponent> comp1(new MockComponent("comp1"));
    ASSERT_TRUE(AddComponent(comp1.get()));

    // Handle MSG_REGISTER_COMPONENT.
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp1", 1));

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp1->PopIncomingMessage());

    // Add component B.
    scoped_ptr<MockComponent> comp2(new MockComponent("comp2"));
    ASSERT_TRUE(AddComponent(comp2.get()));

    // Handle MSG_REGISTER_COMPONENT.
    ASSERT_NO_FATAL_FAILURE(
        HandleMsgRegisterComponent(channel.get(), "comp2", 2));

    // Wait until |comp1->OnRegistered()| gets called.
    ASSERT_TRUE(comp2->WaitIncomingMessage(kTimeout));
    EXPECT_EQ(NULL, comp2->PopIncomingMessage());

    // Test Component::DidAddToHost().
    ASSERT_EQ(host_.get(), comp1->host());
    ASSERT_EQ(host_.get(), comp2->host());

    scoped_ptr<proto::Message> mptr;

    // A should call SendWithReply(Message1) when trigger message is received.
    mptr.reset(new proto::Message());
    mptr->set_target(2);
    mptr->set_type(TEST_MSG_TYPE1);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    comp1->AddOutgoingMessage(mptr.release(), true, kTimeout);

    // Trigger SendWithReply(Message1) : A --> B
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(1);
    channel->PostMessageToListener(mptr.release());

    // Wait until A receives trigger message.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(1, mptr->serial());

    // Wait until channel receives Message 1.
    proto::Message* msg1 = channel->WaitMessage(kTimeout);
    ASSERT_TRUE(msg1);
    ASSERT_EQ(TEST_MSG_TYPE1, msg1->type());
    uint32 msg1_serial = msg1->serial();

    // A should calls SendWithReplyNonRecursive(Message 3) when Message 2 is
    // being handled.
    mptr.reset(new proto::Message());
    mptr->set_target(2);
    mptr->set_type(TEST_MSG_TYPE3);
    mptr->set_reply_mode(proto::Message::NEED_REPLY);
    comp1->AddOutgoingMessageWithMode(mptr.release(), true, true, kTimeout);

    // B posts Message 2 to A.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_type(TEST_MSG_TYPE2);
    mptr->set_serial(2);
    channel->PostMessageToListener(mptr.release());

    // Wait until A receives Message 2.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(2, mptr->serial());

    // Wait until channel receives Message 3 from A.
    mptr.reset(channel->WaitMessage(kTimeout));
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(TEST_MSG_TYPE3, mptr->type());
    uint32 msg3_serial = mptr->serial();

    // B sends Message 4 to A.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_type(TEST_MSG_TYPE4);
    channel->PostMessageToListener(mptr.release());

    // Wait until message channel finishes to post the message.
    channel->WaitForPostingMessagesToListener();

    // A should not receive any messages because message handling is paused.
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // B sends reply of Message 1 to A.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_serial(msg1_serial);
    mptr->set_type(TEST_MSG_TYPE1);
    mptr->set_reply_mode(proto::Message::IS_REPLY);
    channel->PostMessageToListener(mptr.release());

    // Wait until message channel finishes to post the message.
    channel->WaitForPostingMessagesToListener();

    // A should not receive any messages because message handling is paused.
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // B sends Message 5 to A.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_type(TEST_MSG_TYPE5);
    mptr->set_reply_mode(proto::Message::NO_REPLY);
    channel->PostMessageToListener(mptr.release());

    // Wait until message channel finishes to post the message.
    channel->WaitForPostingMessagesToListener();

    // A should not receive any messages because message handling is paused.
    ASSERT_FALSE(comp1->WaitIncomingMessage(kSmallTimeout));

    // B sends reply of Message 3 to A.
    mptr.reset(new proto::Message());
    mptr->set_target(1);
    mptr->set_type(TEST_MSG_TYPE3);
    mptr->set_serial(msg3_serial);
    mptr->set_reply_mode(proto::Message::IS_REPLY);
    channel->PostMessageToListener(mptr.release());

    // Wait until message channel finishes to post the message.
    channel->WaitForPostingMessagesToListener();

    // A should receive messages in following order:
    // reply of message 3 --> reply of message 4 --> reply of message 1
    // --> message 5.
    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));

    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(TEST_MSG_TYPE3, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());

    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(TEST_MSG_TYPE4, mptr->type());
    ASSERT_EQ(proto::Message::NO_REPLY, mptr->reply_mode());

    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(TEST_MSG_TYPE1, mptr->type());
    ASSERT_EQ(msg1_serial, mptr->serial());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());

    ASSERT_TRUE(comp1->WaitIncomingMessage(kTimeout));
    mptr.reset(comp1->PopIncomingMessage());
    ASSERT_EQ(TEST_MSG_TYPE5, mptr->type());
    ASSERT_EQ(proto::Message::NO_REPLY, mptr->reply_mode());
    EXPECT_TRUE(RemoveComponent(comp1.get()));
    EXPECT_TRUE(RemoveComponent(comp2.get()));
  }

};

#if defined(OS_WIN)
// Class for testing the case that |create_thread| == false, i.e. each component
// just runs on the thread where it gets added.
// This class creates a worker thread for adding/removing/running components, so
// that the main thread can be used for testing code.
class MultiComponentHostTestNoThread : public MultiComponentHostTestBase,
                                       public base::PlatformThread::Delegate,
                                       public MessageQueue::Handler {
 protected:
  MultiComponentHostTestNoThread()
      : thread_handle_(base::kNullThreadHandle),
        control_event_(false, false),
        control_result_(false) {
  }

  virtual void SetUp() {
    // Do not create runner thread for each component.
    host_.reset(new MultiComponentHost(false));

    ASSERT_TRUE(base::PlatformThread::Create(0, this, &thread_handle_));
    control_event_.Wait();
    ASSERT_TRUE(control_queue_.get());
  }

  virtual void TearDown() {
    proto::Message* msg = new proto::Message();
    msg->set_type(MSG_QUIT_WORKER_THREAD);
    control_queue_->Post(msg, NULL);
    base::PlatformThread::Join(thread_handle_);
    ASSERT_FALSE(control_queue_.get());
    host_.reset();
  }

  virtual bool AddComponent(MockComponent* component) {
    proto::Message* msg = new proto::Message();
    msg->set_type(MSG_ADD_COMPONENT);
    control_queue_->Post(msg, component);
    control_event_.Wait();
    return control_result_;
  }

  virtual bool RemoveComponent(MockComponent* component) {
    proto::Message* msg = new proto::Message();
    msg->set_type(MSG_REMOVE_COMPONENT);
    control_queue_->Post(msg, component);
    control_event_.Wait();
    return control_result_;
  }

 private:
  enum {
    MSG_QUIT_WORKER_THREAD = MSG_SYSTEM_RESERVED_START,
    MSG_ADD_COMPONENT,
    MSG_REMOVE_COMPONENT,
  };

  // Overridden from base::PlatformThread::Delegate:
  virtual void ThreadMain() OVERRIDE {
    control_queue_.reset(MessageQueue::Create(this));
    control_event_.Signal();

    BOOL ret;
    MSG msg;
    while ((ret = ::GetMessage(&msg, NULL, 0, 0)) != 0) {
      if (ret == -1)
        break;
      else
        ::DispatchMessage(&msg);
    }
    control_queue_.reset();
  }

  // Overridden from MessageQueue::Handler:
  virtual void HandleMessage(proto::Message* message,
                             void* user_data) OVERRIDE {
    scoped_ptr<proto::Message> mptr(message);
    MockComponent* comp = reinterpret_cast<MockComponent*>(user_data);
    switch (mptr->type()) {
      case MSG_QUIT_WORKER_THREAD:
        ::PostQuitMessage(0);
        break;
      case MSG_ADD_COMPONENT:
        control_result_ = host_->AddComponent(comp);
        ASSERT_EQ(base::PlatformThread::CurrentId(), comp->thread_id());
        break;
      case MSG_REMOVE_COMPONENT:
        control_result_ = host_->RemoveComponent(comp);
        break;
      default:
        FAIL();
        break;
    }
    control_event_.Signal();
  }

  // Handle of the worker thread.
  base::PlatformThreadHandle thread_handle_;

  // Event for keeping synchronous between main and worker thread.
  base::WaitableEvent control_event_;

  // Message queue for sending control messages from main thread to worker
  // thread.
  scoped_ptr<MessageQueue> control_queue_;

  // Stores the result of control message.
  bool control_result_;
};
#endif

// Test cases for |create_thread| == true:
TEST_F(MultiComponentHostTestCreateThread, AddRemove) {
  TestAddRemove();
}

TEST_F(MultiComponentHostTestCreateThread, SwitchMessageChannel) {
  TestSwitchMessageChannel();
}

TEST_F(MultiComponentHostTestCreateThread, MessageDispatch) {
  TestMessageDispatch();
}

TEST_F(MultiComponentHostTestCreateThread, SendWithReply) {
  TestSendWithReply();
}

TEST_F(MultiComponentHostTestCreateThread, DestroyHostWithComponents) {
  TestDestroyHostWithComponents();
}

TEST_F(MultiComponentHostTestCreateThread, PauseResumeMessageHandling) {
  TestPauseResumeMessageHandling();
}

TEST_F(MultiComponentHostTestCreateThread, MessageDispatchingOrderWithPausing) {
  TestMessageDispatchingOrderWithPausing();
}

TEST_F(MultiComponentHostTestCreateThread, WaitForRegister) {
  TestWait();
}

#if defined(OS_WIN)

// Test cases for |create_thread| == false:
TEST_F(MultiComponentHostTestNoThread, AddRemove) {
  TestAddRemove();
}

TEST_F(MultiComponentHostTestNoThread, SwitchMessageChannel) {
  TestSwitchMessageChannel();
}

TEST_F(MultiComponentHostTestNoThread, MessageDispatch) {
  TestMessageDispatch();
}

TEST_F(MultiComponentHostTestNoThread, SendWithReply) {
  TestSendWithReply();
}

TEST_F(MultiComponentHostTestNoThread, DestroyHostWithComponents) {
  TestDestroyHostWithComponents();
}

TEST_F(MultiComponentHostTestNoThread, PauseResumeMessageHandling) {
  TestPauseResumeMessageHandling();
}
#endif

}  // namespace
