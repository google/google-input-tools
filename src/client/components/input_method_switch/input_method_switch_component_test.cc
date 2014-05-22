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

#include "components/input_method_switch/input_method_switch_component.h"

#include "i18n/input/engine/lib/public/proto_utils.h"
#include "base/basictypes.h"
#include "base/file/file_path.h"
#include "base/file/file_util.h"
#include "base/resource_bundle.h"
#include "base/scoped_ptr.h"
#include "base/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/time.h"
#include "common/string_utils.h"
#include "components/common/file_utils.h"
#include "components/common/mock_app_component.h"
#include "components/common/mock_decoder_factory.h"
#include "components/keyboard_input/keyboard_input_component.h"
#include "components/t13n_ime/t13n_ime_component.h"
#include "components/t13n_ime/proto/language_pack.pb.h"
#include "components/t13n_ime/t13n_config.h"
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
using ime_goopy::components::MockDecoderFactory;
using ime_goopy::components::InputMethodSwitchComponent;
using ime_goopy::components::KeyboardInputComponent;
using ime_goopy::components::MockAppComponent;
using ime_goopy::components::Typist;
using ime_goopy::components::T13nImeComponent;
using ipc::proto::Message;

const char kHindiPackageConfigFile[] =
    "/googleclient/components/t13n_ime/test_data/hindi.configure.txt";
void WaitCheck(base::WaitableEvent* event) {
    ASSERT_TRUE(event->TimedWait(
        base::TimeDelta::FromMilliseconds(5000)));
}

static base::WaitableEvent ime_switch_finished_event(false, false);
static base::WaitableEvent ime_switch_component_attached_event(false, false);
class InputMethodSwitchComponentForTest : public InputMethodSwitchComponent {
 public:
  InputMethodSwitchComponentForTest() {
  }

  virtual ~InputMethodSwitchComponentForTest() {
  }

  virtual void OnMsgAttachToInputContext(ipc::proto::Message* message) {
    uint32 icid = message->icid();
    // When OnMsgAttachToInputContext method returns, the message
    // MSG_ACTIVATE_HOTKEY_LIST has already been sent. So we can ensure that
    // when the message MSG_SEND_KEY_EVENT for hotkey is handled by
    // hub_hotkey_manager, MSG_ACTIVATE_HOTKEY_LIST has already been processed.
    InputMethodSwitchComponent::OnMsgAttachToInputContext(message);
    // Skips kInputContextNone
    if (icid != ipc::kInputContextNone)
        ime_switch_component_attached_event.Signal();
  }
};

class MultiComponentHostForTest : public ipc::MultiComponentHost {
 public:
  explicit MultiComponentHostForTest(bool create_thread)
      : MultiComponentHost(create_thread), expected_outgoing_message_count_(0) {
  }
  virtual ~MultiComponentHostForTest() {
  }

  // Overridden from MultiComponentHost
  virtual bool Send(ipc::Component* component, ipc::proto::Message* message,
                    uint32* serial) OVERRIDE {
    outgoing_messages_.push_back(*message);
    DLOG(INFO) << "sent message:" << message->type();
    bool return_code = MultiComponentHost::Send(component, message, serial);
    if (expected_outgoing_message_count_ != 0) {
      if (outgoing_messages_.size() == expected_outgoing_message_count_)
        ime_switch_finished_event.Signal();
    }
    return return_code;
  }

  std::vector<Message> outgoing_messages_;
  int expected_outgoing_message_count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultiComponentHostForTest);
};

class InputMethodSwitchComponentTest : public ::testing::Test,
                                       public MockAppListener {
 protected:
  InputMethodSwitchComponentTest() : input_context_event_(false, false),
                                     app_component_ready_event_(false, false) {
  }

  virtual void SetUp() {
    hub_.reset(new ipc::HubHost());
    hub_->Run();

    app_host_.reset(new ipc::MultiComponentHost(true));
    app_host_channel_.reset(new ipc::DirectMessageChannel(hub_.get()));
    app_host_->SetMessageChannel(app_host_channel_.get());

    ime_host_.reset(new ipc::MultiComponentHost(true));
    ime_host_channel_.reset(new ipc::DirectMessageChannel(hub_.get()));
    ime_host_->SetMessageChannel(ime_host_channel_.get());

    ime_switch_host_.reset(new MultiComponentHostForTest(true));
    ime_switch_host_channel_.reset(new ipc::DirectMessageChannel(hub_.get()));
    ime_switch_host_->SetMessageChannel(ime_switch_host_channel_.get());

    if (ime_goopy::ResourceBundle::HasSharedInstance())
      ime_goopy::ResourceBundle::CleanupSharedInstance();
    ime_goopy::ResourceBundle::InitSharedInstanceForTest();

    ime_goopy::components::proto::LanguagePackDescription package_info;
    std::string configure_file_path = FLAGS_test_srcdir +
                                      kHindiPackageConfigFile;
    ASSERT_TRUE(i18n_input::engine::ParseTextFormatProtoFromFile(
        configure_file_path, &package_info));
    t13n_ime_.reset(new T13nImeComponent(package_info, "",
                                         new MockDecoderFactory));
    ASSERT_TRUE(t13n_ime_->Init());
    keyboard_input_.reset(new KeyboardInputComponent());
    ime_switch_component_.reset(new InputMethodSwitchComponentForTest);
    mock_app_comp_.reset(new MockAppComponent("mock_app"));
    mock_app_comp_->set_listener(this);

    ime_host_->AddComponent(keyboard_input_.get());
    ime_host_->AddComponent(t13n_ime_.get());
    ime_switch_host_->AddComponent(ime_switch_component_.get());
    app_host_->AddComponent(mock_app_comp_.get());

    int timeout = 2000;
    ASSERT_NO_FATAL_FAILURE(ime_host_->WaitForComponents(&timeout));
    ASSERT_NO_FATAL_FAILURE(ime_switch_host_->WaitForComponents(&timeout));
    ASSERT_NO_FATAL_FAILURE(WaitCheck(&input_context_event_));
  }

  virtual void TearDown() {
    t13n_ime_->RemoveFromHost();
    keyboard_input_->RemoveFromHost();
    ime_switch_component_->RemoveFromHost();
    mock_app_comp_->RemoveFromHost();
    ime_host_.reset();
    app_host_.reset();
    ime_switch_host_.reset();
    hub_.reset();
    t13n_ime_.reset();
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

  void CheckOutgoingMessages() {
    int set_command_list_count = 0;
    for (int i = 0; i < ime_switch_host_->expected_outgoing_message_count_;
        ++i) {
      SCOPED_TRACE(StringPrintf("outgoing message index:%d\n", i));
      if (ime_switch_host_->outgoing_messages_[i].type() ==
          ipc::MSG_SET_COMMAND_LIST) {
        ++set_command_list_count;
      }
    }
    ASSERT_EQ(5, set_command_list_count);
  }

  scoped_ptr<ipc::HubHost> hub_;
  scoped_ptr<ipc::MultiComponentHost> app_host_;
  scoped_ptr<ipc::DirectMessageChannel> app_host_channel_;

  scoped_ptr<ipc::MultiComponentHost> ime_host_;
  scoped_ptr<ipc::DirectMessageChannel> ime_host_channel_;

  scoped_ptr<MultiComponentHostForTest> ime_switch_host_;
  scoped_ptr<ipc::DirectMessageChannel> ime_switch_host_channel_;

  scoped_ptr<T13nImeComponent> t13n_ime_;
  scoped_ptr<KeyboardInputComponent> keyboard_input_;
  scoped_ptr<InputMethodSwitchComponent> ime_switch_component_;
  scoped_ptr<MockAppComponent> mock_app_comp_;

  base::WaitableEvent input_context_event_;
  base::WaitableEvent app_component_ready_event_;
};

// Tests hotkey.
// Ctrl+G for Toggling direct input mode.
// F12 works the same with Ctrl+G.
// Ctrl+J for switching to previous ime.
class MockTypist1 : public Typist {
 public:
  explicit MockTypist1(Typist::Delegate* delegate);
  virtual ~MockTypist1();

  // Overriden from Typist:
  virtual void StartComposite() OVERRIDE;
  virtual void Composite() OVERRIDE;
  virtual void WaitComplete() OVERRIDE;
  virtual void OnMessageReceived(Message* msg) OVERRIDE;
  virtual void CheckResult() OVERRIDE;

 private:
  Typist::Delegate* delegate_;
  DISALLOW_COPY_AND_ASSIGN(MockTypist1);
};

MockTypist1::MockTypist1(Typist::Delegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);
  delegate_->set_typist(this);
}

MockTypist1::~MockTypist1() {
}

void MockTypist1::StartComposite() {
  delegate_->FocusInputContext();
  delegate_->SwitchToKeyboardInput();
  delegate_->UserComposite();
}

void MockTypist1::Composite() {
  // Input Ctrl + 'G'
  ipc::proto::KeyEvent key_event;
  key_event.set_keycode(ipc::VKEY_G);
  key_event.set_type(ipc::proto::KeyEvent::DOWN);
  key_event.set_modifiers(ipc::kControlKeyMask);
  delegate_->HandleKeyEvent(this, key_event);

  // Input Ctrl + 'G' again.
  delegate_->HandleKeyEvent(this, key_event);

  // Input F12.
  delegate_->HandleKey(this, ipc::VKEY_F12);

  // Input F12 again.
  delegate_->HandleKey(this, ipc::VKEY_F12);

  // Input Ctrl + 'J'
  key_event.set_keycode(ipc::VKEY_J);
  delegate_->HandleKeyEvent(this, key_event);
}

void MockTypist1::WaitComplete() {
}

void MockTypist1::OnMessageReceived(Message* msg) {
  DLOG(INFO) << "message received " << msg->type();
  delete msg;
}

void MockTypist1::CheckResult() {
}

static uint32 kExpectedOutgoingMessages[] = {
  ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,  // Ctrl+G
  ipc::MSG_SWITCH_TO_INPUT_METHOD,  // second Ctrl+G
  ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,  // F12
  ipc::MSG_SWITCH_TO_INPUT_METHOD,  // second F12
  ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,  // Ctrl+J
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_SET_COMMAND_LIST,
};

TEST_F(InputMethodSwitchComponentTest, HotkeyTest) {
  mock_app_comp_->Start();
  ASSERT_NO_FATAL_FAILURE(WaitCheck(&app_component_ready_event_));
  ASSERT_NO_FATAL_FAILURE(WaitCheck(&ime_switch_component_attached_event));

  ime_switch_host_->expected_outgoing_message_count_ =
      arraysize(kExpectedOutgoingMessages);
  // Clears messages already sent before typing.
  ime_switch_host_->outgoing_messages_.clear();

  DLOG(INFO) << "start composition";
  scoped_ptr<Typist> typist1(new MockTypist1(mock_app_comp_.get()));
  typist1->StartComposite();
  typist1->WaitComplete();
  typist1->CheckResult();

  ASSERT_NO_FATAL_FAILURE(WaitCheck(&ime_switch_finished_event));
  ASSERT_NO_FATAL_FAILURE(CheckOutgoingMessages());

  mock_app_comp_->Stop();
  ASSERT_NO_FATAL_FAILURE(WaitCheck(&input_context_event_));
}

}  // namespace
