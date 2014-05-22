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

#include "ipc/hub_host.h"

#include "base/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "build/build_config.h"
#include "ipc/message_types.h"
#include "ipc/testing.h"

namespace {

using ipc::Hub;
using ipc::proto::Message;
using ipc::proto::ComponentInfo;
using ipc::HubHost;

class MockConnector : public Hub::Connector {
 public:
  MockConnector()
    : message_received_(false),
      message_received_event_(false, false) {}

  virtual bool Send(Message* message) {
    DCHECK(message);
    scoped_ptr<Message> mptr(message);

    if (message->type() == ipc::MSG_REGISTER_COMPONENT
        && message->reply_mode() == Message::IS_REPLY) {
      DCHECK_EQ(Message::IS_REPLY, message->reply_mode());
      message_received_event_.Signal();
    }
    return true;
  }

  bool Wait() { return message_received_event_.Wait(); }

 private:
  bool message_received_;
  base::WaitableEvent message_received_event_;
};

TEST(HubHostTest, DispatchTest) {
  scoped_ptr<HubHost> hub_host(new HubHost());
  for (int i = 0; i < 2; ++i) {
    hub_host->Run();
    MockConnector* mock_connector =
        new MockConnector();
    hub_host->Attach(mock_connector);

    scoped_ptr<Message> mptr(new Message());
    mptr->set_type(ipc::MSG_REGISTER_COMPONENT);
    mptr->set_reply_mode(Message::NEED_REPLY);
    ComponentInfo* info =
        mptr->mutable_payload()->add_component_info();
    info->set_string_id("test_string_id");
    info->set_name("test_component");

    EXPECT_TRUE(hub_host->Dispatch(mock_connector, mptr.release()));
    EXPECT_TRUE(mock_connector->Wait());

    hub_host->Detach(mock_connector);
    hub_host->Quit();
  }
}

}  // namespace
