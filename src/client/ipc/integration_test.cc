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

#include <queue>

#include "base/basictypes.h"
#include "base/logging.h"
#include "ipc/constants.h"
#include "ipc/direct_message_channel.h"
#include "ipc/hub_host.h"
#include "ipc/message_types.h"
#include "ipc/mock_component.h"
#include "ipc/multi_component_host.h"
#include "ipc/simple_message_queue.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace {

using namespace ipc;

const uint32 kTimeout = 10000;

// Messages an application can produce
const uint32 kAppProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_CREATE_INPUT_CONTEXT,
  MSG_DELETE_INPUT_CONTEXT,
  MSG_FOCUS_INPUT_CONTEXT,
  MSG_BLUR_INPUT_CONTEXT,
  MSG_ASSIGN_ACTIVE_CONSUMER,
  MSG_RESIGN_ACTIVE_CONSUMER,
  MSG_REQUEST_CONSUMER,
  MSG_PROCESS_KEY_EVENT,
  MSG_CANCEL_COMPOSITION,
  MSG_COMPLETE_COMPOSITION,
  MSG_UPDATE_INPUT_CARET,
  MSG_SHOW_COMPOSITION_UI,
  MSG_HIDE_COMPOSITION_UI,
  MSG_SHOW_CANDIDATE_LIST_UI,
  MSG_HIDE_CANDIDATE_LIST_UI,
};

// Messages an application can consume
const uint32 kAppConsumeMessages[] = {
  MSG_COMPOSITION_CHANGED,
  MSG_INSERT_TEXT,
  MSG_GET_DOCUMENT_INFO,
  MSG_GET_DOCUMENT_CONTENT_IN_RANGE,
};

// Messages an input method can produce
const uint32 kIMEProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_SET_COMMAND_LIST,
  MSG_UPDATE_COMMANDS,
  MSG_ADD_HOTKEY_LIST,
  MSG_REMOVE_HOTKEY_LIST,
  MSG_CHECK_HOTKEY_CONFLICT,
  MSG_ACTIVATE_HOTKEY_LIST,
  MSG_DEACTIVATE_HOTKEY_LIST,
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_QUERY_INPUT_CONTEXT,
  MSG_REQUEST_CONSUMER,
  MSG_SET_COMPOSITION,
  MSG_INSERT_TEXT,
  MSG_SET_CANDIDATE_LIST,
};

// Messages an input method can consume
const uint32 kIMEConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_PROCESS_KEY_EVENT,
  MSG_CANCEL_COMPOSITION,
  MSG_COMPLETE_COMPOSITION,
  MSG_CANDIDATE_LIST_SHOWN,
  MSG_CANDIDATE_LIST_HIDDEN,
  MSG_CANDIDATE_LIST_PAGE_DOWN,
  MSG_CANDIDATE_LIST_PAGE_UP,
  MSG_CANDIDATE_LIST_SCROLL_TO,
  MSG_CANDIDATE_LIST_PAGE_RESIZE,
  MSG_SELECT_CANDIDATE,
  MSG_UPDATE_INPUT_CARET,
};

// Messages a candidate window can produce
const uint32 kUIProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
  MSG_DEREGISTER_COMPONENT,
  MSG_REQUEST_CONSUMER,
  MSG_CANDIDATE_LIST_SHOWN,
  MSG_CANDIDATE_LIST_HIDDEN,
  MSG_CANDIDATE_LIST_PAGE_DOWN,
  MSG_CANDIDATE_LIST_PAGE_UP,
  MSG_CANDIDATE_LIST_SCROLL_TO,
  MSG_CANDIDATE_LIST_PAGE_RESIZE,
  MSG_SELECT_CANDIDATE,
};

// Messages a candidate window can consume
const uint32 kUIConsumeMessages[] = {
  MSG_ATTACH_TO_INPUT_CONTEXT,
  MSG_DETACHED_FROM_INPUT_CONTEXT,
  MSG_INPUT_CONTEXT_GOT_FOCUS,
  MSG_INPUT_CONTEXT_LOST_FOCUS,
  MSG_COMPOSITION_CHANGED,
  MSG_CANDIDATE_LIST_CHANGED,
  MSG_SELECTED_CANDIDATE_CHANGED,
  MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
  MSG_UPDATE_INPUT_CARET,
  MSG_SHOW_COMPOSITION_UI,
  MSG_HIDE_COMPOSITION_UI,
  MSG_SHOW_CANDIDATE_LIST_UI,
  MSG_HIDE_CANDIDATE_LIST_UI,
};

// Messages a monitor component can produce
const uint32 kMonitorProduceMessages[] = {
  MSG_REGISTER_COMPONENT,
};

// Messages a monitor component can consume
const uint32 kMonitorConsumeMessages[] = {
  MSG_COMPONENT_CREATED,
  MSG_COMPONENT_DELETED,
  MSG_INPUT_CONTEXT_CREATED,
  MSG_INPUT_CONTEXT_DELETED,
};


// A mock application component
class AppComponent : public MockComponent {
 public:
  explicit AppComponent(const std::string& string_id)
      : MockComponent(string_id) {
  }

  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE {
    SetupComponentInfo("", "Mock Application", "",
                       kAppProduceMessages, arraysize(kAppProduceMessages),
                       kAppConsumeMessages, arraysize(kAppConsumeMessages),
                       info);
    MockComponent::GetInfo(info);
  }
};

// A mock input method component
class IMEComponent : public MockComponent {
 public:
  explicit IMEComponent(const std::string& string_id)
      : MockComponent(string_id) {
  }

  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE {
    SetupComponentInfo("", "Mock Input Method", "",
                       kIMEProduceMessages, arraysize(kIMEProduceMessages),
                       kIMEConsumeMessages, arraysize(kIMEConsumeMessages),
                       info);
    MockComponent::GetInfo(info);
  }
};

// A mock UI component
class UIComponent : public MockComponent {
 public:
  explicit UIComponent(const std::string& string_id)
      : MockComponent(string_id) {
  }

  virtual void GetInfo(proto::ComponentInfo* info) OVERRIDE {
    SetupComponentInfo("", "Mock UI", "",
                       kUIProduceMessages, arraysize(kUIProduceMessages),
                       kUIConsumeMessages, arraysize(kUIConsumeMessages),
                       info);
    MockComponent::GetInfo(info);
  }
};

// A test class to emulate a real application environment. A new thread is
// created to mock an application, where an application component is created and
// added to a MultiComponentHost object with create_thread == false.
// The main thread acts as a controller to control the behavior of the
// application thread by sending control messages.
class IntegrationTest : public ::testing::Test,
                        public base::PlatformThread::Delegate,
                        public MessageQueue::Handler,
                        public Hub::Connector {
 protected:
  enum {
    MSG_TEST_APP_QUIT = MSG_SYSTEM_RESERVED_START,
    MSG_TEST_APP_ADD_COMPONENT,
    MSG_TEST_APP_REMOVE_COMPONENT,
    MSG_TEST_APP_CREATE_IC,
    MSG_TEST_APP_DELETE_IC,
    MSG_TEST_APP_REQUEST_CONSUMER,
    MSG_TEST_APP_KEY_DOWN,
  };

  IntegrationTest()
      : thread_handle_(base::kNullThreadHandle),
        control_event_(false, false),
        control_result_(false),
        monitor_event_(false, false),
        icid_(kInputContextNone) {
  }

  virtual ~IntegrationTest() {
  }

  virtual void SetUp() {
    hub_.reset(new HubHost());
    hub_->Run();

    hub_->Attach(this);
    ASSERT_TRUE(monitor_event_.Wait());
    scoped_ptr<proto::Message> mptr(PopMonitorMessage());
    ASSERT_TRUE(mptr.get());
    EXPECT_EQ(MSG_REGISTER_COMPONENT, mptr->type());
    EXPECT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());

    app_host_.reset(new MultiComponentHost(false));
    app_host_channel_.reset(new DirectMessageChannel(hub_.get()));
    app_host_->SetMessageChannel(app_host_channel_.get());

    comp_host_.reset(new MultiComponentHost(true));
    comp_host_channel_.reset(new DirectMessageChannel(hub_.get()));
    comp_host_->SetMessageChannel(comp_host_channel_.get());

    ime_.reset(new IMEComponent("ime1"));
    ui_.reset(new UIComponent("ui1"));

#if defined(OS_WIN)
    ASSERT_TRUE(base::PlatformThread::Create(0, this, &thread_handle_));
    control_event_.Wait();
    ASSERT_TRUE(control_queue_.get());
    ASSERT_TRUE(app_.get());
#endif
  }

  virtual void TearDown() {
#if defined(OS_WIN)
    proto::Message* msg = new proto::Message();
    msg->set_type(MSG_TEST_APP_QUIT);
    control_queue_->Post(msg, NULL);
    base::PlatformThread::Join(thread_handle_);
    ASSERT_FALSE(control_queue_.get());
    ASSERT_FALSE(app_.get());
#endif

    ui_->RemoveFromHost();
    ui_.reset();
    ime_->RemoveFromHost();
    ime_.reset();
    comp_host_.reset();
    app_host_.reset();
    hub_.reset();
  }

  // Overridden from base::PlatformThread::Delegate:
  virtual void ThreadMain() OVERRIDE {
#if defined(OS_WIN)
    app_.reset(new AppComponent("app1"));
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
    app_->RemoveFromHost();
    app_.reset();
#endif
  }

  // Overridden from MessageQueue::Handler:
  virtual void HandleMessage(proto::Message* message,
                             void* user_data) OVERRIDE {
#if defined(OS_WIN)
    scoped_ptr<proto::Message> mptr(message);
    switch (mptr->type()) {
      case MSG_TEST_APP_QUIT:
        ::PostQuitMessage(0);
        break;
      case MSG_TEST_APP_ADD_COMPONENT:
        control_result_ = app_host_->AddComponent(app_.get());
        ASSERT_EQ(base::PlatformThread::CurrentId(), app_->thread_id());
        break;
      case MSG_TEST_APP_REMOVE_COMPONENT:
        control_result_ = app_host_->RemoveComponent(app_.get());
        break;
      case MSG_TEST_APP_CREATE_IC: {
        proto::Message* msg = new proto::Message();
        msg->set_type(MSG_CREATE_INPUT_CONTEXT);
        msg->set_reply_mode(proto::Message::NEED_REPLY);
        ASSERT_TRUE(app_host_->SendWithReply(app_.get(), msg, kTimeout, &msg));
        ASSERT_TRUE(msg);
        icid_ = msg->icid();
        EXPECT_NE(kInputContextNone, icid_);
        delete msg;
        break;
      }
      case MSG_TEST_APP_DELETE_IC: {
        proto::Message* msg = new proto::Message();
        msg->set_type(MSG_DELETE_INPUT_CONTEXT);
        msg->set_reply_mode(proto::Message::NO_REPLY);
        msg->set_icid(icid_);
        ASSERT_TRUE(app_host_->Send(app_.get(), msg, NULL));
        break;
      }
      case MSG_TEST_APP_REQUEST_CONSUMER: {
        proto::Message* msg = new proto::Message();
        msg->set_type(MSG_REQUEST_CONSUMER);
        msg->set_reply_mode(proto::Message::NEED_REPLY);
        msg->set_icid(icid_);
        for (size_t i = 0; i < arraysize(kAppProduceMessages); ++i)
          msg->mutable_payload()->add_uint32(kAppProduceMessages[i]);
        ASSERT_TRUE(app_host_->Send(app_.get(), msg, NULL));
        break;
      }
      case MSG_TEST_APP_KEY_DOWN: {
        proto::Message* msg = new proto::Message();
        msg->set_type(MSG_PROCESS_KEY_EVENT);
        msg->set_reply_mode(proto::Message::NEED_REPLY);
        msg->set_icid(icid_);
        ASSERT_TRUE(app_host_->SendWithReply(app_.get(), msg, kTimeout, &msg));
        break;
      }
      default:
        FAIL();
        break;
    }
    control_event_.Signal();
#endif
  }

  // Overridden from Hub::Connector:
  virtual bool Send(proto::Message* message) OVERRIDE {
    monitor_queue_.push(message);
    monitor_event_.Signal();
    return true;
  }

  virtual void Attached() OVERRIDE {
    proto::Message* msg = new proto::Message();
    msg->set_type(MSG_REGISTER_COMPONENT);
    msg->set_reply_mode(proto::Message::NEED_REPLY);

    SetupComponentInfo("monitor", "Monitor", "",
                       kMonitorProduceMessages,
                       arraysize(kMonitorProduceMessages),
                       kMonitorConsumeMessages,
                       arraysize(kMonitorConsumeMessages),
                       msg->mutable_payload()->add_component_info());

    hub_->Dispatch(this, msg);
  }

  proto::Message* PopMonitorMessage() {
    if (monitor_queue_.empty())
      return NULL;
    proto::Message* msg = monitor_queue_.front();
    monitor_queue_.pop();
    return msg;
  }

  // Hub instance.
  scoped_ptr<HubHost> hub_;

  // Message channel to |app_host_|.
  scoped_ptr<DirectMessageChannel> app_host_channel_;

  // Message channel to |comp_host_|.
  scoped_ptr<DirectMessageChannel> comp_host_channel_;

  // Host for application components.
  scoped_ptr<MultiComponentHost> app_host_;

  // Host for other components.
  scoped_ptr<MultiComponentHost> comp_host_;

  // A mock application component.
  scoped_ptr<AppComponent> app_;

  // A mock input method component.
  scoped_ptr<IMEComponent> ime_;

  // A mock UI component.
  scoped_ptr<UIComponent> ui_;

  // Handle of the app thread.
  base::PlatformThreadHandle thread_handle_;

  // Event for keeping synchronous between main and app thread.
  base::WaitableEvent control_event_;

  // Message queue for sending control messages from main thread to app thread.
  scoped_ptr<MessageQueue> control_queue_;

  // Stores the result of control message.
  bool control_result_;

  // A queue for storing monitor messages.
  std::queue<proto::Message*> monitor_queue_;

  // Event fired when receiving a monitor message.
  base::WaitableEvent monitor_event_;

  uint32 icid_;
};

// Test adding/removing normal components to/from a MultiComponentHost object
// with create_thread == true.
TEST_F(IntegrationTest, AddRemoveNormalComponent) {
  // Add an IME component.
  ASSERT_TRUE(comp_host_->AddComponent(ime_.get()));

  // Wait until |ime_->OnRegistered()| gets called.
  ASSERT_TRUE(ime_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, ime_->PopIncomingMessage());

  uint32 ime_id = ime_->id();
  EXPECT_NE(kComponentDefault, ime_id);

  // check MSG_COMPONENT_CREATED broadcast message.
  ASSERT_TRUE(monitor_event_.Wait());
  scoped_ptr<proto::Message> mptr(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_CREATED, mptr->type());
  EXPECT_EQ(1, mptr->payload().component_info_size());
  EXPECT_EQ(ime_id, mptr->payload().component_info(0).id());

  // Add a UI component.
  ASSERT_TRUE(comp_host_->AddComponent(ui_.get()));

  // Wait until |ui_->OnRegistered()| gets called.
  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, ui_->PopIncomingMessage());

  uint32 ui_id = ui_->id();
  EXPECT_NE(kComponentDefault, ui_id);

  // |ime_| and |ui_| should have different id.
  EXPECT_NE(ime_id, ui_id);

  // check MSG_COMPONENT_CREATED broadcast message.
  ASSERT_TRUE(monitor_event_.Wait());
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_CREATED, mptr->type());
  EXPECT_EQ(1, mptr->payload().component_info_size());
  EXPECT_EQ(ui_id, mptr->payload().component_info(0).id());

  // Remove |ime_| from Hub.
  ASSERT_TRUE(ime_->RemoveFromHost());
  // Wait until |ime_->OnDeregistered()| gets called.
  ASSERT_TRUE(ime_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, ime_->PopIncomingMessage());
  EXPECT_EQ(kComponentDefault, ime_->id());

  // check MSG_COMPONENT_DELETED broadcast message.
  ASSERT_TRUE(monitor_event_.Wait());
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_DELETED, mptr->type());
  EXPECT_EQ(1, mptr->payload().uint32_size());
  EXPECT_EQ(ime_id, mptr->payload().uint32(0));

  // Remove |ui_| from Hub.
  ASSERT_TRUE(ui_->RemoveFromHost());
  // Wait until |ime_->OnDeregistered()| gets called.
  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, ui_->PopIncomingMessage());
  EXPECT_EQ(kComponentDefault, ui_->id());

  // check MSG_COMPONENT_DELETED broadcast message.
  ASSERT_TRUE(monitor_event_.Wait());
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_DELETED, mptr->type());
  EXPECT_EQ(1, mptr->payload().uint32_size());
  EXPECT_EQ(ui_id, mptr->payload().uint32(0));
}

#if defined(OS_WIN)
// Test adding/removing application components to/from a MultiComponentHost
// object with create_thread == true.
TEST_F(IntegrationTest, AddRemoveAppComponent) {
  // Add |app_| to Hub.
  scoped_ptr<proto::Message> mptr(new proto::Message());
  mptr->set_type(MSG_TEST_APP_ADD_COMPONENT);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();
  ASSERT_TRUE(control_result_);

  // Wait until |app_->OnRegistered()| gets called.
  ASSERT_TRUE(app_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, app_->PopIncomingMessage());

  uint32 app_id = app_->id();
  EXPECT_NE(kComponentDefault, app_id);

  // check MSG_COMPONENT_CREATED broadcast message.
  ASSERT_TRUE(monitor_event_.Wait());
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_CREATED, mptr->type());
  EXPECT_EQ(1, mptr->payload().component_info_size());
  EXPECT_EQ(app_id, mptr->payload().component_info(0).id());

  // Remove |app_| from Hub.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_TEST_APP_REMOVE_COMPONENT);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();
  ASSERT_TRUE(control_result_);

  // Wait until |app_->OnDeregistered()| gets called.
  ASSERT_TRUE(app_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, app_->PopIncomingMessage());
  EXPECT_EQ(kComponentDefault, app_->id());

  // check MSG_COMPONENT_DELETED broadcast message.
  ASSERT_TRUE(monitor_event_.Wait());
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_DELETED, mptr->type());
  EXPECT_EQ(1, mptr->payload().uint32_size());
  EXPECT_EQ(app_id, mptr->payload().uint32(0));
}

// Test creating an input context and handling a key event.
TEST_F(IntegrationTest, InputContext) {
  scoped_ptr<proto::Message> mptr;

  // Add an IME component.
  ASSERT_TRUE(comp_host_->AddComponent(ime_.get()));
  ASSERT_TRUE(ime_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, ime_->PopIncomingMessage());
  monitor_event_.Wait();
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_CREATED, mptr->type());

  // Add a UI component.
  ASSERT_TRUE(comp_host_->AddComponent(ui_.get()));
  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, ui_->PopIncomingMessage());
  monitor_event_.Wait();
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_CREATED, mptr->type());

  // Add |app_| to Hub.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_TEST_APP_ADD_COMPONENT);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();
  ASSERT_TRUE(control_result_);
  ASSERT_TRUE(app_->WaitIncomingMessage(kTimeout));
  EXPECT_EQ(NULL, app_->PopIncomingMessage());
  monitor_event_.Wait();
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_COMPONENT_CREATED, mptr->type());

  // Create an input context.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_TEST_APP_CREATE_IC);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();
  monitor_event_.Wait();
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_INPUT_CONTEXT_CREATED, mptr->type());
  EXPECT_TRUE(mptr->payload().has_input_context_info());
  EXPECT_EQ(icid_, mptr->payload().input_context_info().id());

  // App to request consumer, which will cause |ime_| to be attached.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_REQUEST_CONSUMER);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_icid(icid_);
  for (size_t i = 0; i < arraysize(kIMEProduceMessages); ++i)
    mptr->mutable_payload()->add_uint32(kIMEProduceMessages[i]);
  ime_->AddOutgoingMessage(mptr.release(), true, 0);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_REQUEST_CONSUMER);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_icid(icid_);
  for (size_t i = 0; i < arraysize(kUIProduceMessages); ++i)
    mptr->mutable_payload()->add_uint32(kUIProduceMessages[i]);
  ui_->AddOutgoingMessage(mptr.release(), true, 0);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_TEST_APP_REQUEST_CONSUMER);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();

  // Wait for reply message of MSG_REQUEST_CONSUMER.
  ASSERT_TRUE(app_->WaitIncomingMessage(kTimeout));
  mptr.reset(app_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_REQUEST_CONSUMER, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_EQ(icid_, mptr->icid());

  const uint32 kRequestConsumerReplyMessageTypes[] = {
    MSG_CREATE_INPUT_CONTEXT,
    MSG_DELETE_INPUT_CONTEXT,
    MSG_FOCUS_INPUT_CONTEXT,
    MSG_BLUR_INPUT_CONTEXT,
    MSG_ASSIGN_ACTIVE_CONSUMER,
    MSG_RESIGN_ACTIVE_CONSUMER,
    MSG_REQUEST_CONSUMER,
    MSG_UPDATE_INPUT_CARET,
  };

  ASSERT_EQ(arraysize(kRequestConsumerReplyMessageTypes),
            mptr->payload().uint32_size());
  for (size_t i = 0; i < arraysize(kRequestConsumerReplyMessageTypes); ++i) {
    EXPECT_EQ(1, std::count(mptr->payload().uint32().data(),
                            mptr->payload().uint32().data() +
                            mptr->payload().uint32_size(),
                            kRequestConsumerReplyMessageTypes[i]));
  }

  ASSERT_EQ(1, mptr->payload().boolean_size());
  EXPECT_TRUE(mptr->payload().boolean(0));


  // Wait for MSG_ATTACH_TO_INPUT_CONTEXT.
  ASSERT_TRUE(ime_->WaitIncomingMessage(kTimeout));
  mptr.reset(ime_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_ATTACH_TO_INPUT_CONTEXT, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());

  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  mptr.reset(ui_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_ATTACH_TO_INPUT_CONTEXT, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());

  // Test a key event.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_SET_CANDIDATE_LIST);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_icid(icid_);
  mptr->mutable_payload()->mutable_candidate_list()->set_id(1);
  ime_->AddOutgoingMessage(mptr.release(), true, 0);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_SET_COMPOSITION);
  mptr->set_reply_mode(proto::Message::NO_REPLY);
  mptr->set_icid(icid_);
  mptr->mutable_payload()->mutable_composition()->mutable_text()->set_text("C");
  ime_->AddOutgoingMessage(mptr.release(), true, 0);

  mptr.reset(new proto::Message());
  mptr->set_type(MSG_TEST_APP_KEY_DOWN);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();

  ASSERT_TRUE(ime_->WaitIncomingMessage(kTimeout));
  mptr.reset(ime_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_PROCESS_KEY_EVENT, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());

  // Only UI will get MSG_CANDIDATE_LIST_CHANGED message.
  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  mptr.reset(ui_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_CANDIDATE_LIST_CHANGED, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());
  ASSERT_EQ(1, mptr->payload().candidate_list().id());

  // Both UI and App will get MSG_COMPOSITION_CHANGED message.
  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  mptr.reset(ui_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_COMPOSITION_CHANGED, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());
  ASSERT_STREQ("C",mptr->payload().composition().text().text().c_str());

  ASSERT_TRUE(app_->WaitIncomingMessage(kTimeout));
  mptr.reset(app_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_COMPOSITION_CHANGED, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());
  ASSERT_STREQ("C",mptr->payload().composition().text().text().c_str());

  // Delete the input context.
  mptr.reset(new proto::Message());
  mptr->set_type(MSG_TEST_APP_DELETE_IC);
  control_queue_->Post(mptr.release(), NULL);
  control_event_.Wait();
  monitor_event_.Wait();
  mptr.reset(PopMonitorMessage());
  ASSERT_TRUE(mptr.get());
  EXPECT_EQ(MSG_INPUT_CONTEXT_DELETED, mptr->type());
  EXPECT_EQ(1, mptr->payload().uint32_size());
  EXPECT_EQ(icid_, mptr->payload().uint32(0));

  ASSERT_TRUE(ime_->WaitIncomingMessage(kTimeout));
  mptr.reset(ime_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_DETACHED_FROM_INPUT_CONTEXT, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());

  ASSERT_TRUE(ui_->WaitIncomingMessage(kTimeout));
  mptr.reset(ui_->PopIncomingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_DETACHED_FROM_INPUT_CONTEXT, mptr->type());
  ASSERT_EQ(icid_, mptr->icid());
}
#endif

}  // namespace
