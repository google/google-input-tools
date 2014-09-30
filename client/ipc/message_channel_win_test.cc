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

#include <time.h>
#include <windows.h>
#include <string>

#include "base/scoped_ptr.h"
#include "base/time.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/hub_host.h"
#include "ipc/message_channel_client_win.h"
#include "ipc/message_channel_server_win.h"
#include "ipc/message_channel_win.h"
#include "ipc/message_channel_win_consts.h"
#include "ipc/multi_component_host.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace {

using ipc::MessageChannel;
using ipc::MessageChannelWin;

const wchar_t kTestPipeName[] =
    L"\\\\.\\pipe\\com_google_ime_goopy_test_pipe";
const uint32 kPipeTimeout = 1000;
const uint32 kMaxMessageSentNum = 1000;
const wchar_t kTestIPCSharedMemoryName[] = L"Local\\IPCTestSharedMemory";
const wchar_t kTestIPCServerName[] = L"ipc_test_server";

// ChannelDelegate will be notified by calling |OnChannelClosed|
// when the channel is closed.
class ChannelDelegate : public ipc::MessageChannelWin::Delegate {
 public:
  ChannelDelegate() : channel_closed_(false, false) {}

  virtual ~ChannelDelegate() {}

  virtual void OnChannelClosed(MessageChannelWin* channel) OVERRIDE {
    channel_closed_.Signal();
  }

  // Return true if OnChannelClosed is called.
  bool Wait() { return channel_closed_.Wait(); }

 private:
  base::WaitableEvent channel_closed_;
  DISALLOW_COPY_AND_ASSIGN(ChannelDelegate);
};

// ChannelListener receive/send messages for a MessageChannelWin instance.
class ChannelListener : public ipc::MessageChannel::Listener {
 public:
  ChannelListener()
      : channel_closed_(false, false),
        channel_connected_(false, false),
        all_msgs_received_(false, false),
        num_sent_msgs_(0),
        num_received_msgs_(0) {
    srand(time(NULL));
  }

  virtual ~ChannelListener() {
    for (std::vector<ipc::proto::Message*>::iterator i =
         received_messages_.begin();
         i != received_messages_.end(); ++i) {
      delete *i;
    }
  }

  bool Send(MessageChannel* channel, ipc::proto::Message* message) {
    num_sent_msgs_++;
    return channel->Send(message);
  }

  // Overriden from |ipc::MessageChannel::Listener|:
  virtual void OnMessageReceived(MessageChannel* channel,
                                 ipc::proto::Message* message) OVERRIDE {
    scoped_ptr<ipc::proto::Message> mptr(message);
    ++num_received_msgs_;
    if (num_received_msgs_ == kMaxMessageSentNum)
      all_msgs_received_.Signal();
    else if (num_received_msgs_ > kMaxMessageSentNum)
      return;

    EXPECT_LT(0, mptr->payload().uint32_size());
    EXPECT_EQ(num_received_msgs_ - 1, mptr->payload().uint32(0));

    // Randomly send 1 ~ 3 messages.
    int num_msgs_to_sent = rand() % 3 + 1;
    const int end =
        std::min(num_msgs_to_sent + num_sent_msgs_, kMaxMessageSentNum);
    for (int i = num_sent_msgs_; i < end; ++i) {
      ipc::proto::Message* msg = new ipc::proto::Message();
      msg->set_type(0);
      msg->mutable_payload()->add_uint32(i);
      if (!Send(channel, msg))
        break;
    }
  }

  virtual void OnMessageChannelConnected(MessageChannel* channel) OVERRIDE {
    channel_connected_.Signal();
  }

  virtual void OnMessageChannelClosed(MessageChannel* channel) {
    channel_closed_.Signal();
  }

  bool WaitConnected() {
    return channel_connected_.Wait();
  }

  bool WaitAllReceived() {
    return all_msgs_received_.Wait();
  }

  bool WaitClosed() {
    return channel_closed_.Wait();
  }

  int GetSentMessageNum() {
    return num_sent_msgs_;
  }

  int GetReceivedMessageNum() {
    return num_received_msgs_;
  }

 private:
  std::vector<ipc::proto::Message*> received_messages_;
  base::WaitableEvent channel_closed_;
  base::WaitableEvent channel_connected_;
  base::WaitableEvent all_msgs_received_;
  uint32 num_received_msgs_;
  uint32 num_sent_msgs_;
  DISALLOW_COPY_AND_ASSIGN(ChannelListener);
};

// Create a pair of pipes connected to each other.
void CreatePipePair(HANDLE* server_pipe, HANDLE* client_pipe) {
  // Create server pipe.
  *server_pipe = CreateNamedPipe(
      kTestPipeName,
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
      PIPE_UNLIMITED_INSTANCES,
      ipc::MessageChannel::kMaximumMessageSize,
      ipc::MessageChannel::kMaximumMessageSize,
      kPipeTimeout,
      NULL);
  EXPECT_NE(INVALID_HANDLE_VALUE, *server_pipe);

  // Create client pipe.
  *client_pipe = CreateFile(
      kTestPipeName,
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_OVERLAPPED,
      NULL);
  EXPECT_NE(INVALID_HANDLE_VALUE, *client_pipe);

  // Connect client pipe.
  OVERLAPPED connected_overlapped;
  HANDLE connected_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  EXPECT_TRUE(NULL != connected_event);

  memset(&connected_overlapped, 0, sizeof(connected_overlapped));
  connected_overlapped.hEvent = connected_event;
  BOOL ret = ConnectNamedPipe(*server_pipe, &connected_overlapped);
  EXPECT_EQ(FALSE, ret);
  EXPECT_EQ(ERROR_PIPE_CONNECTED, ::GetLastError());
  ::CloseHandle(connected_event);
}

// Write data of buffer to a pipe synchronously.
void WriteToPipe(HANDLE pipe, const std::string& buffer) {
  OVERLAPPED write_overlapped;
  memset(&write_overlapped, 0, sizeof(OVERLAPPED));

  HANDLE write_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  EXPECT_TRUE(write_event != NULL);

  write_overlapped.hEvent = write_event;

  EXPECT_NE(0, ::WriteFile(pipe,
                           buffer.data(),
                           buffer.size(),
                           NULL,
                           &write_overlapped));
  EXPECT_EQ(WAIT_OBJECT_0, ::WaitForSingleObject(write_event, INFINITE));
  ::CloseHandle(write_event);
}

class MockHub : public ipc::Hub {
 public:
  MockHub();
  virtual ~MockHub();

  virtual void Attach(ipc::Hub::Connector* connector) OVERRIDE;
  virtual void Detach(ipc::Hub::Connector* connector) OVERRIDE;
  virtual bool Dispatch(
      ipc::Hub::Connector* connector, ipc::proto::Message* message) OVERRIDE;

  bool WaitChannelAttached();
  bool WaitMessageReceived();
  bool IsAttached();

 private:
  // Auto reset, signaled when |Dispatch| is called.
  base::WaitableEvent message_received_;
  // Manually set, signaled when |Attach| is called, reseted if |Detach| is
  // called.
  base::WaitableEvent channel_attached_;

  // Used to send a message to controller when the first message is received.
  bool first_message_;

  // Simulate real hub, which will detach the connector when deconstructed.
  ipc::Hub::Connector* connector_;

  DISALLOW_COPY_AND_ASSIGN(MockHub);
};


class MockChannelUser : public ipc::MessageChannel::Listener {
 public:
  MockChannelUser();
  virtual ~MockChannelUser();

  virtual void OnMessageReceived(ipc::MessageChannel* channel,
                                 ipc::proto::Message* message) OVERRIDE;
  virtual void OnMessageChannelConnected(ipc::MessageChannel* channel) OVERRIDE;
  virtual void OnMessageChannelClosed(ipc::MessageChannel* channel) OVERRIDE;
  virtual void OnAttachedToMessageChannel(
      ipc::MessageChannel* channel) OVERRIDE;
  virtual void OnDetachedFromMessageChannel(
      ipc::MessageChannel* channel) OVERRIDE;

  bool WaitChannelAttached();
  bool TimedWaitChannelAttached(const base::TimeDelta& time_to_wait);
  bool IsChannelConnected();
  bool WaitMessageReceived();
  bool IsAttached();

 private:
  // Manual reset, signaled when |OnAttachedToMessageChannel| is called.
  // reset when |OnDetachedFromMessageChannel| is called.
  base::WaitableEvent attached_event_;

  // Manual reset, signaled when |OnMessageChannelConnected| is called.
  // reset when |OnMessageChannelClosed| is called.
  base::WaitableEvent connected_event_;

  // Auto reset, signaled when |OnMessageReceived| is called.
  base::WaitableEvent received_event_;

  // Cached channel, verify messages are received from the same channel
  // connected.
  ipc::MessageChannel* channel_;

  DISALLOW_COPY_AND_ASSIGN(MockChannelUser);
};

MockHub::MockHub()
    : message_received_(false, false),
      channel_attached_(true, false),
      first_message_(true),
      connector_(NULL) {
}

MockHub::~MockHub() {
  if (connector_)
    Detach(connector_);
}

void MockHub::Attach(ipc::Hub::Connector* connector) {
  EXPECT_FALSE(channel_attached_.IsSignaled());
  ASSERT_FALSE(message_received_.IsSignaled());
  EXPECT_EQ(NULL, connector_);

  channel_attached_.Signal();
  first_message_ = true;
  EXPECT_EQ(NULL, connector_);
  connector_ = connector;
  connector_->Attached();
}

void MockHub::Detach(ipc::Hub::Connector* connector) {
  EXPECT_TRUE(channel_attached_.IsSignaled());
  EXPECT_EQ(connector, connector_);

  channel_attached_.Reset();
  message_received_.Reset();
  connector_->Detached();
  connector_ = NULL;
}

bool MockHub::Dispatch(ipc::Hub::Connector* connector,
                       ipc::proto::Message* message) {
  EXPECT_TRUE(channel_attached_.IsSignaled());
  EXPECT_EQ(connector_, connector);

  scoped_ptr<ipc::proto::Message> mptr(message);
  if (first_message_) {
    EXPECT_TRUE(connector->Send(mptr.release()));
    first_message_ = false;
  }
  message_received_.Signal();
  return true;
}

bool MockHub::WaitChannelAttached() {
  return channel_attached_.Wait();
}

bool MockHub::WaitMessageReceived() {
  return message_received_.Wait();
}

bool MockHub::IsAttached() {
  return channel_attached_.IsSignaled();
}


// Implementation of MockChannelUser.
MockChannelUser::MockChannelUser()
    : attached_event_(true, false),
      connected_event_(true, false),
      received_event_(false, false),
      channel_(NULL) {
}

MockChannelUser::~MockChannelUser() {
}

void MockChannelUser::OnMessageReceived(ipc::MessageChannel* channel,
                                        ipc::proto::Message* message) {
  delete message;
  received_event_.Signal();
}

void MockChannelUser::OnMessageChannelConnected(ipc::MessageChannel* channel) {
  ipc::proto::Message* message = new ipc::proto::Message();
  message->set_type(0);
  EXPECT_TRUE(channel->Send(message));
  attached_event_.Signal();
}

void MockChannelUser::OnMessageChannelClosed(ipc::MessageChannel* channel) {
  received_event_.Reset();
}

void MockChannelUser::OnAttachedToMessageChannel(ipc::MessageChannel* channel) {
  attached_event_.Signal();
  channel_ = channel;
  EXPECT_FALSE(received_event_.IsSignaled());
  EXPECT_FALSE(connected_event_.IsSignaled());
}

void MockChannelUser::OnDetachedFromMessageChannel(
    ipc::MessageChannel* channel) {
  EXPECT_TRUE(attached_event_.IsSignaled());

  attached_event_.Reset();
  connected_event_.Reset();
  received_event_.Reset();
  channel_ = NULL;
}

bool MockChannelUser::WaitChannelAttached() {
  return attached_event_.Wait();
}

bool MockChannelUser::TimedWaitChannelAttached(
    const base::TimeDelta& time_to_wait) {
  return attached_event_.TimedWait(time_to_wait);
}

bool MockChannelUser::IsChannelConnected() {
  return channel_ != NULL && channel_->IsConnected();
}

bool MockChannelUser::WaitMessageReceived() {
  return received_event_.Wait();
}

bool MockChannelUser::IsAttached() {
  return attached_event_.IsSignaled();
}

// Test message channel basic function.
TEST(MessageChannelWinTest, BaseTest) {
  HANDLE server_pipe, client_pipe;
  // Create pipes.
  CreatePipePair(&server_pipe, &client_pipe);

  // Create message channel for server & client.
  scoped_ptr<ipc::MessageChannelWin> server_channel(
      new ipc::MessageChannelWin(NULL));
  scoped_ptr<ipc::MessageChannelWin> client_channel(
      new ipc::MessageChannelWin(NULL));

  scoped_ptr<ChannelListener> server_listener(
      new ChannelListener);
  scoped_ptr<ChannelListener> client_listener(
      new ChannelListener);

  // Create listeners.
  server_channel->SetListener(server_listener.get());
  client_channel->SetListener(client_listener.get());

  // Enpower both channels with pipe handles.
  EXPECT_TRUE(server_channel->SetHandle(server_pipe));
  EXPECT_TRUE(client_channel->SetHandle(client_pipe));

  EXPECT_TRUE(server_channel->IsConnected());
  EXPECT_TRUE(client_channel->IsConnected());

  EXPECT_TRUE(server_listener->WaitConnected());
  EXPECT_TRUE(client_listener->WaitConnected());

#if !defined(DEBUG)
  // Channel should return false if already has a valid pipe.
  HANDLE fake_handle = NULL;
  EXPECT_FALSE(server_channel->SetHandle(fake_handle));
  EXPECT_FALSE(client_channel->SetHandle(fake_handle));
#endif

  // Send & receive message test.
  ipc::proto::Message* msg = new ipc::proto::Message();
  msg->set_type(0);
  msg->mutable_payload()->add_uint32(0);
  EXPECT_TRUE(server_listener->Send(server_channel.get(), msg));

  // Wait until all messges are received.
  EXPECT_TRUE(server_listener->WaitAllReceived());
  EXPECT_TRUE(client_listener->WaitAllReceived());

  // Verify numbers of received/sent messages for both pipes.
  EXPECT_EQ(kMaxMessageSentNum, server_listener->GetSentMessageNum());
  EXPECT_EQ(kMaxMessageSentNum, client_listener->GetSentMessageNum());
  EXPECT_EQ(kMaxMessageSentNum, server_listener->GetReceivedMessageNum());
  EXPECT_EQ(kMaxMessageSentNum, client_listener->GetReceivedMessageNum());

  // Verify if large message(>16M) is rejected by channel.
  ipc::proto::Message* large_msg = new ipc::proto::Message();
  large_msg->set_type(0);
  large_msg->mutable_payload()->add_string(
      std::string(ipc::MessageChannel::kMaximumMessageSize, 0));
  EXPECT_FALSE(server_channel->Send(large_msg));

  // Verify if malicious message will stop the channel from going on.
  std::string buffer = "try to overflow the message channel!";
  WriteToPipe(client_pipe, buffer);
  EXPECT_TRUE(server_listener->WaitClosed());
  EXPECT_TRUE(client_listener->WaitClosed());

  EXPECT_FALSE(server_channel->IsConnected());
  EXPECT_FALSE(client_channel->IsConnected());

  // Check if channel works fine after restart.
  CreatePipePair(&server_pipe, &client_pipe);
  EXPECT_TRUE(server_channel->SetHandle(server_pipe));
  EXPECT_TRUE(client_channel->SetHandle(client_pipe));

  EXPECT_TRUE(server_channel->IsConnected());
  EXPECT_TRUE(client_channel->IsConnected());

  EXPECT_TRUE(server_listener->WaitConnected());
  EXPECT_TRUE(client_listener->WaitConnected());

  msg = new ipc::proto::Message();
  msg->set_type(0);
  msg->mutable_payload()->add_uint32(0);
  EXPECT_TRUE(server_listener->Send(server_channel.get(), msg));

  // Close one pipe to stop both.
  ::CloseHandle(server_pipe);
  EXPECT_TRUE(server_listener->WaitClosed());
  EXPECT_TRUE(client_listener->WaitClosed());

  // Provent that listeners desconstructs before channel.
  server_channel->SetListener(NULL);
  client_channel->SetListener(NULL);
}

// Test message channel server and client.
// Client should auto connect server.
TEST(MessageChannelWinTest, AutoRestartConnectingTest) {
  scoped_ptr<MockChannelUser> mock_channel_user(new MockChannelUser);

  scoped_ptr<MockHub> mock_hub(new MockHub);

  scoped_ptr<ipc::MessageChannelServerWin> server(
      new ipc::MessageChannelServerWin(
          mock_hub.get(),
          kTestIPCSharedMemoryName,
          kTestIPCServerName));

  scoped_ptr<ipc::MessageChannelClientWin> client(
      new ipc::MessageChannelClientWin(mock_channel_user.get(),
                                       kTestIPCSharedMemoryName,
                                       kTestIPCServerName));

  // Test client start.
  EXPECT_TRUE(client->Start());
  EXPECT_FALSE(mock_channel_user->IsAttached());

  // Test client restart.
  client->Stop();
  EXPECT_TRUE(client->Start());

  // Test server start.
  EXPECT_FALSE(mock_hub->IsAttached());
  EXPECT_TRUE(server->Initialize());

  // Test channel works.
  EXPECT_TRUE(mock_channel_user->WaitChannelAttached());
  EXPECT_TRUE(mock_hub->WaitChannelAttached());
  EXPECT_TRUE(mock_channel_user->WaitMessageReceived());
  EXPECT_TRUE(mock_channel_user->IsChannelConnected());
  EXPECT_TRUE(mock_hub->WaitMessageReceived());

  client.reset(NULL);
  server.reset(NULL);
}

// Test server should not fail to start if same name is used by another pipe
// server with different mode in which case client should fail to connect to
// pipe created with different parameters.
TEST(MessageChannelWinTest, PipeNameOccupiedTest) {
  scoped_ptr<MockChannelUser> mock_channel_user(new MockChannelUser);

  scoped_ptr<MockHub> mock_hub(new MockHub);

  scoped_ptr<ipc::MessageChannelServerWin> server(
      new ipc::MessageChannelServerWin(
          mock_hub.get(),
          kTestIPCSharedMemoryName,
          kTestIPCServerName));

  scoped_ptr<ipc::MessageChannelClientWin> client(
      new ipc::MessageChannelClientWin(mock_channel_user.get(),
                                       kTestIPCSharedMemoryName,
                                       kTestIPCServerName));

  DWORD session_id;
  ::ProcessIdToSessionId(GetCurrentProcessId(), &session_id);
  if (!session_id) {
    // If the process doesn't have enough privilege to run, just escape this
    // test.
    return;
  }

  wchar_t pipe_name[MAX_PATH] = {0};
  ::_snwprintf_s(pipe_name,
                 MAX_PATH,
                 L"%s%d%s",
                 ipc::kWinIPCPipeNamePrefix,
                 session_id,
                 ipc::kWinIPCServerName);

  HANDLE name_holder_pipe = ::CreateNamedPipe(
      pipe_name,
      // Assign different access mode.
      // OVERLAPPED vs NON-OVERLAPPED.
      PIPE_ACCESS_DUPLEX,
      // Assign different pipe type and behavior.
      // PIPE_TYPE_BYTE vs PIPE_TYPE_MESSAGE.
      // PIPE_WAIT vs PIPE_NOWAIT.
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT,
      // Allow only one instance.
      1,
      1024,
      1024,
      10,
      NULL);
  EXPECT_NE(INVALID_HANDLE_VALUE, name_holder_pipe);

  // Start client.
  EXPECT_TRUE(client->Start());

  // client should fail to connect to a server pipe created with different
  // parameter.
  EXPECT_FALSE(mock_channel_user->TimedWaitChannelAttached(
      base::TimeDelta::FromMilliseconds(200)));

  MockHub* new_hub = new MockHub();
  server.reset(new ipc::MessageChannelServerWin(
      new_hub,
      kTestIPCSharedMemoryName,
      kTestIPCServerName));
  mock_hub.reset(new_hub);
  EXPECT_TRUE(server->Initialize());

  EXPECT_TRUE(mock_channel_user->WaitChannelAttached());

  ::CloseHandle(name_holder_pipe);

  // Reorder creation of pipe with wrong parameter and the one with right
  // parameter and try again, client should connect success.
  client->Stop();

  // Start right parameter pipe first.
  new_hub = new MockHub();
  server.reset(new ipc::MessageChannelServerWin(
      new_hub,
      kTestIPCSharedMemoryName,
      kTestIPCServerName));
  mock_hub.reset(new_hub);
  EXPECT_TRUE(server->Initialize());

  // Start wrong parameter pipe.
  name_holder_pipe = ::CreateNamedPipe(
      pipe_name,
      // Assign different access mode.
      // OVERLAPPED vs NON-OVERLAPPED.
      PIPE_ACCESS_DUPLEX,
      // Assign different pipe type and behavior.
      // PIPE_TYPE_BYTE vs PIPE_TYPE_MESSAGE.
      // PIPE_WAIT vs PIPE_NOWAIT.
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT,
      // Allow only one instance.
      1,
      1024,
      1024,
      10,
      NULL);
  EXPECT_NE(INVALID_HANDLE_VALUE, name_holder_pipe);
  EXPECT_TRUE(client->Start());

  // channel user should connect.
  EXPECT_TRUE(mock_channel_user->WaitChannelAttached());
  ::CloseHandle(name_holder_pipe);
  client.reset(NULL);
  server.reset(NULL);
}

}  // namespace
