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
#include "components/common/focus_input_context_manager_sub_component.h"
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "ipc/component_base.h"
#include "ipc/constants.h"
#include "ipc/direct_message_channel.h"
#include "ipc/hub_host.h"
#include "ipc/message_types.h"
#include "ipc/multi_component_host.h"
#include "ipc/protos/ipc.pb.h"
#include <gtest/gtest.h>
namespace {

// Synchronuous user defined messages.
const uint32 MSG_USER_FOCUS_INPUT_CONTEXT = ipc::MSG_USER_DEFINED_START + 1;
const uint32 MSG_USER_DEFOCUS_INPUT_CONTEXT = ipc::MSG_USER_DEFINED_START + 2;

const uint32 kProduceMessages[] = {
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  MSG_USER_FOCUS_INPUT_CONTEXT,
  MSG_USER_DEFOCUS_INPUT_CONTEXT,
};

const uint32 kConsumeMessages[] = {
  MSG_USER_FOCUS_INPUT_CONTEXT,
  MSG_USER_DEFOCUS_INPUT_CONTEXT,
};

const uint32 kTestWaitTimeout = 5000;  // In milli-seconds.

const char kTestComponentName[] = "test_component";

using ime_goopy::components::FocusInputContextManagerSubComponent;
using ipc::HubHost;
using ipc::MultiComponentHost;
using ipc::DirectMessageChannel;

class TestComponent : public ipc::ComponentBase {
 public:
  TestComponent() : ipc::ComponentBase(), icid_(0) {
    focus_sub_component_ =
        new FocusInputContextManagerSubComponent(this);
  }

  virtual ~TestComponent() {
  }

  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE {
    info->set_string_id(kTestComponentName);
    for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
      info->add_produce_message(kProduceMessages[i]);

    for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
      info->add_consume_message(kConsumeMessages[i]);
    GetSubComponentsInfo(info);
  }

  virtual void Handle(ipc::proto::Message* message) OVERRIDE {
    if (HandleMessageBySubComponents(message))
      return;
    switch (message->type()) {
      case MSG_USER_FOCUS_INPUT_CONTEXT:
        EXPECT_EQ(icid_, focus_sub_component_->GetFocusIcid());
        break;
      case MSG_USER_DEFOCUS_INPUT_CONTEXT:
        EXPECT_EQ(0, focus_sub_component_->GetFocusIcid());
        break;
      case ipc::MSG_INPUT_CONTEXT_GOT_FOCUS:
      case ipc::MSG_INPUT_CONTEXT_LOST_FOCUS:
        break;
      default:
        ReplyError(message, ipc::proto::Error::INVALID_MESSAGE,
                   "unknown type");
        return;
    }
    ReplyTrue(message);
  }

  virtual void OnRegistered() OVERRIDE {
    scoped_ptr<ipc::proto::Message> mptr(NewMessage(
        ipc::MSG_CREATE_INPUT_CONTEXT,
        ipc::kInputContextNone,
        true));

    ipc::proto::Message* reply = NULL;
    if (!SendWithReply(mptr.release(), -1, &reply)) {
      DLOG(ERROR) << "SendWithReply error: MSG_CREATE_INPUT_CONTEXT";
      return;
    }

    DCHECK(reply);
    icid_ = reply->icid();
    delete reply;

    mptr.reset(NewMessage(
        ipc::MSG_FOCUS_INPUT_CONTEXT,
        icid_,
        false));
    if (!Send(mptr.release(), NULL)) {
      DLOG(ERROR) << "Send error type = MSG_FOCUS_INPUT_CONTEXT";
      return;
    }

    mptr.reset(NewMessage(
        MSG_USER_FOCUS_INPUT_CONTEXT,
        icid_,
        true));
    if (!SendWithReply(mptr.release(), -1, &reply)) {
      DLOG(ERROR) << "SendWithReply error: MSG_USER_FOCUS_INPUT_CONTEXT";
      return;
    }
    delete reply;

    // Defocus input context.
    mptr.reset(NewMessage(
        ipc::MSG_BLUR_INPUT_CONTEXT,
        icid_,
        false));
    if (!Send(mptr.release(), NULL)) {
      DLOG(ERROR) << "Send error type = MSG_FOCUS_INPUT_CONTEXT";
      return;
    }


    mptr.reset(NewMessage(
        MSG_USER_DEFOCUS_INPUT_CONTEXT,
        icid_,
        true));
    if (!SendWithReply(mptr.release(), -1, &reply)) {
      DLOG(ERROR) << "SendWithReply error: MSG_USER_DEFOCUS_INPUT_CONTEXT";
      return;
    }
    delete reply;
  }

  virtual void OnDeregistered() OVERRIDE {
    icid_ = 0;
  }

 private:
  uint32 icid_;
  FocusInputContextManagerSubComponent* focus_sub_component_;
  DISALLOW_COPY_AND_ASSIGN(TestComponent);
};

TEST(FocusInputContextManagerSubComponentTest, BasicTest) {
  scoped_ptr<HubHost> hub(new HubHost());
  hub->Run();

  scoped_ptr<MultiComponentHost> host(new MultiComponentHost(true));
  scoped_ptr<DirectMessageChannel> channel(
      new ipc::DirectMessageChannel(hub.get()));
  host->SetMessageChannel(channel.get());

  scoped_ptr<TestComponent> component(new TestComponent());
  host->AddComponent(component.get());

  int timeout = kTestWaitTimeout;
  ASSERT_TRUE(host->WaitForComponents(&timeout));

  component->RemoveFromHost();
  host.reset();
  hub.reset();
}

}  // namespace
