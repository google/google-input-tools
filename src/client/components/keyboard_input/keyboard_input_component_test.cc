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

#include "components/keyboard_input/keyboard_input_component.h"

#include "base/stringprintf.h"
#include "base/basictypes.h"
#include "base/resource_bundle.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/time.h"
#include "components/common/mock_app_component.h"
#include "ipc/direct_message_channel.h"
#include "ipc/hub_host.h"
#include "ipc/multi_component_host.h"
#include <gtest/gunit.h>

#if defined(OS_WIN)
#define UNIT_TEST
#include "base/at_exit.h"

base::ShadowingAtExitManager at_exit_manager;
#endif

namespace {

typedef ime_goopy::components::MockAppComponent::Listener MockAppListener;
using ime_goopy::components::MockAppComponent;
using ime_goopy::components::Typist;
using ime_goopy::components::KeyboardInputComponent;
using ipc::proto::Message;

void WaitCheck(base::WaitableEvent* event) {
    ASSERT_TRUE(event->TimedWait(
        base::TimeDelta::FromMilliseconds(5000)));
}

const uint32 kMonitorProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
};

const uint32 kMonitorConsumeMessages[] = {
  ipc::MSG_COMPONENT_CREATED,
};

const char* monitor_string_id = "monitor";
static base::WaitableEvent components_ready_event(false, false);
static base::WaitableEvent monitor_ready_event(false, false);

// TODO(linxinfen): Removes this monitor after the monitor mechanism is
// implemented in MultiComponentHost.
class MonitorComponent : public ipc::ComponentBase {
 public:
  explicit MonitorComponent(uint32 component_count)
      : ready_component_count_(0),
        component_count_(component_count) {
    DCHECK_GE(component_count_, 0);
  }
  virtual ~MonitorComponent() {}

  // Overridden interface ipc::Component.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(Message* message) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;

 private:
  uint32 ready_component_count_;
  uint32 component_count_;

  DISALLOW_COPY_AND_ASSIGN(MonitorComponent);
};

void MonitorComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(monitor_string_id);

  for (size_t i = 0; i < arraysize(kMonitorProduceMessages); ++i)
    info->add_produce_message(kMonitorProduceMessages[i]);

  for (size_t i = 0; i < arraysize(kMonitorConsumeMessages); ++i)
    info->add_consume_message(kMonitorConsumeMessages[i]);
}

void MonitorComponent::Handle(Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  switch (mptr->type()) {
    case ipc::MSG_COMPONENT_CREATED:
      ready_component_count_++;
      if (ready_component_count_ == component_count_)
        components_ready_event.Signal();
      break;
    default:
      DLOG(ERROR) << "Can't handle message type: " << mptr->type();
      ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE, NULL);
      break;
  }

  if (mptr.get())
    ReplyTrue(mptr.release());
}

void MonitorComponent::OnRegistered() {
  monitor_ready_event.Signal();
}

class KeyboardInputComponentTest : public ::testing::Test,
                                   public MockAppListener {
 protected:
  KeyboardInputComponentTest() : input_context_event_(false, false),
                                 app_component_ready_event_(false, false) {
  }

  virtual void SetUp() {
    if (ime_goopy::ResourceBundle::HasSharedInstance())
      ime_goopy::ResourceBundle::CleanupSharedInstance();
    ime_goopy::ResourceBundle::InitSharedInstanceForTest();

    hub_.reset(new ipc::HubHost());
    hub_->Run();

    app_host_.reset(new ipc::MultiComponentHost(true));
    app_host_channel_.reset(new ipc::DirectMessageChannel(hub_.get()));
    app_host_->SetMessageChannel(app_host_channel_.get());

    mock_app_comp1_.reset(new MockAppComponent("mock_app1"));
    mock_app_comp1_->set_listener(this);

    component_host_.reset(new ipc::MultiComponentHost(true));
    component_host_channel_.reset(new ipc::DirectMessageChannel(hub_.get()));
    component_host_->SetMessageChannel(component_host_channel_.get());

    keyboard_input_.reset(new KeyboardInputComponent());
    monitor_.reset(new MonitorComponent(1));

    component_host_->AddComponent(monitor_.get());
    component_host_->AddComponent(keyboard_input_.get());
    app_host_->AddComponent(mock_app_comp1_.get());
    ASSERT_NO_FATAL_FAILURE(WaitCheck(&monitor_ready_event));
    ASSERT_NO_FATAL_FAILURE(WaitCheck(&components_ready_event));
    ASSERT_NO_FATAL_FAILURE(WaitCheck(&input_context_event_));
  }

  virtual void TearDown() {
    monitor_->RemoveFromHost();
    keyboard_input_->RemoveFromHost();
    mock_app_comp1_->RemoveFromHost();
    component_host_.reset();
    app_host_.reset();
    hub_.reset();
  }

  // Overriden from MockAppListener.
  virtual void OnRegistered() {
    input_context_event_.Signal();
  }

  virtual void OnAppComponentReady() OVERRIDE {
    app_component_ready_event_.Signal();
  }

  virtual void OnAppComponentStopped() OVERRIDE {
    input_context_event_.Signal();
  }

  scoped_ptr<ipc::HubHost> hub_;
  scoped_ptr<ipc::MultiComponentHost> app_host_;
  scoped_ptr<ipc::DirectMessageChannel> app_host_channel_;

  scoped_ptr<ipc::MultiComponentHost> component_host_;
  scoped_ptr<ipc::DirectMessageChannel> component_host_channel_;

  scoped_ptr<MonitorComponent> monitor_;
  scoped_ptr<KeyboardInputComponent> keyboard_input_;
  scoped_ptr<MockAppComponent> mock_app_comp1_;

  base::WaitableEvent input_context_event_;
  base::WaitableEvent app_component_ready_event_;
};

// Test MSG_PROCESS_KEY_EVENT
// Input:
// E N 'space'
class MockTypist1 : public Typist {
 public:
  explicit MockTypist1(Typist::Delegate* delegate);
  virtual ~MockTypist1();

  // Overriden from Typist
  virtual void StartComposite() OVERRIDE;
  virtual void Composite() OVERRIDE;
  virtual void WaitComplete() OVERRIDE;
  virtual void OnMessageReceived(Message* msg) OVERRIDE {
    delete msg;
  }
  virtual void OnMessageReplyReceived(Message* msg) OVERRIDE;
  virtual void CheckResult() OVERRIDE {}
 private:
  Typist::Delegate* delegate_;
  base::WaitableEvent reply_received_event_;
  int reply_received_count_;
  DISALLOW_COPY_AND_ASSIGN(MockTypist1);
};

MockTypist1::MockTypist1(Typist::Delegate* delegate)
    : delegate_(delegate),
      reply_received_event_(true, false),
      reply_received_count_(0) {
  delegate_->set_typist(this);
}

MockTypist1::~MockTypist1() {
}

void MockTypist1::StartComposite() {
  delegate_->UserComposite();
}

void MockTypist1::Composite() {
  // Input 'E'.
  delegate_->HandleKey(this, 'E' - 'A' + 0x41);
  // Input 'N'.
  delegate_->HandleKey(this, 'N' - 'A' + 0x41);
  // Input '0'.
  delegate_->HandleKey(this, 0x30);
  // Input 'space'.
  delegate_->HandleKey(this, 0x20);
}

void MockTypist1::OnMessageReplyReceived(Message* msg) {
  scoped_ptr<Message> mptr(msg);

  switch (mptr->type()) {
    case ipc::MSG_SEND_KEY_EVENT: {
      SCOPED_TRACE(StringPrintf("reply received: %d\n", reply_received_count_));
      ASSERT_EQ(Message::IS_REPLY, mptr->reply_mode());
      ASSERT_EQ(1, mptr->payload().boolean_size());
      reply_received_count_++;
      ASSERT_FALSE(mptr->payload().boolean(0));
      if (reply_received_count_ == 4)
        reply_received_event_.Signal();
      break;
    }
    default:
      NOTREACHED() << "Unexpected message type";
      break;
  }
}

void MockTypist1::WaitComplete() {
  ASSERT_NO_FATAL_FAILURE(WaitCheck(&reply_received_event_));
}

TEST_F(KeyboardInputComponentTest, ProcessKeyTest) {
  mock_app_comp1_->Start();
  ASSERT_NO_FATAL_FAILURE(WaitCheck(&app_component_ready_event_));
  scoped_ptr<Typist> typist1(new MockTypist1(mock_app_comp1_.get()));
  typist1->StartComposite();
  typist1->WaitComplete();
  typist1->CheckResult();

  mock_app_comp1_->Stop();
  ASSERT_NO_FATAL_FAILURE(WaitCheck(&input_context_event_));
}
}  // namespace
