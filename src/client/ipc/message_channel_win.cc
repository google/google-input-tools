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

#include "ipc/message_channel_win.h"

#include <windows.h>
#include <sddl.h>
#include <string>
#include <queue>

#include "base/atomic_ref_count.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

class MessageChannelWin::Impl : public base::PlatformThread::Delegate {
 public:
  Impl(MessageChannelWin::Delegate* delegate, MessageChannelWin* owner);

  void AddRef();

  void Release();

  // Overriden from |base::PlatformThread::Delegate|:
  virtual void ThreadMain() OVERRIDE;

  // Serialize/Deserialize between message and raw data.
  bool PackOutgoingMessage(const proto::Message& message, std::string* buffer);
  bool ParseIncomingMessage(
      const char* buffer, int32 size, proto::Message** message);

  // Asynchronously receive a message.
  void ReceiveInternal();

  // Called when received_event is signaled.
  void OnReceivedEvent();

  // Called when pending |WriteFile| operation has completed.
  void OnSentEvent();

  // Called to close the pipe and inform the listener.
  void CleanupPipe();

  // Called to send a message from |sending_list_|.
  void SendInternal();

  // Buffers for read/write operation.
  char incoming_buffer_[ipc::MessageChannel::kReadBufferSize];

  // Buffer for reading large or incompleted message
  std::string overflow_buffer_;

  // Windows asynchronous IO specified data.
  OVERLAPPED receive_overlapped_;
  OVERLAPPED sent_overlapped_;

  // Windows named pipe for delivering messages.
  volatile HANDLE pipe_;

  // Signaled when |Stop| is called.
  HANDLE quit_event_;

  // Signaled when |ReadFile| completes.
  HANDLE on_received_event_;

  // Signaled when pending |WriteFile| completes.
  HANDLE on_sent_event_;

  // Signaled when a message is ready to sent or last message is sent.
  HANDLE send_event_;

  std::queue<std::string> sending_list_;
  base::Lock sending_list_lock_;

  // The atomic variable(integer) is used as bool type , only 0 and 1 are
  // valid.
  base::AtomicRefCount is_running_;

  // Reference count for private resources of the pipe, at most two references
  // to the resources are allowed, one is for delegate, another is worker
  // thread itself, when all references are relesed, the resources will be
  // destroyed.
  base::AtomicRefCount refcount_;

  // Listener of this channel, will be set to NULL when the channel is destroyed
  // by delegate/worker thread.
  Listener* listener_;
  base::Lock listener_lock_;

  // Delegate is responsible for destroying the channel, if delegate_ is NULL,
  // any objects own a pointer to this channel could destroy it.
  MessageChannelWin::Delegate* delegate_;
  base::Lock delegate_lock_;

  // Signaled when thread started.
  base::WaitableEvent thread_event_;

  MessageChannelWin* owner_;

 private:
  virtual ~Impl();
};

MessageChannelWin::Impl::Impl(MessageChannelWin::Delegate* delegate,
                              MessageChannelWin* owner)
    : pipe_(INVALID_HANDLE_VALUE),
      quit_event_(::CreateEvent(NULL, FALSE, FALSE, NULL)),
      on_received_event_(::CreateEvent(NULL, TRUE, FALSE, NULL)),
      on_sent_event_(::CreateEvent(NULL, TRUE, FALSE, NULL)),
      send_event_(::CreateEvent(NULL, FALSE, FALSE, NULL)),
      is_running_(0),
      refcount_(1),
      listener_(NULL),
      delegate_(delegate),
      thread_event_(false, false),
      owner_(owner) {
  DCHECK(quit_event_ && on_received_event_ && on_sent_event_ && send_event_);
  memset(&receive_overlapped_, 0, sizeof(receive_overlapped_));
  receive_overlapped_.hEvent = on_received_event_;

  memset(&sent_overlapped_, 0, sizeof(sent_overlapped_));
  sent_overlapped_.hEvent = on_sent_event_;
}

MessageChannelWin::Impl::~Impl() {
  if (pipe_ != INVALID_HANDLE_VALUE)
    ::CloseHandle(pipe_);
  ::CloseHandle(quit_event_);
  ::CloseHandle(on_received_event_);
  ::CloseHandle(on_sent_event_);
  ::CloseHandle(send_event_);
}

void MessageChannelWin::Impl::AddRef() {
  base::AtomicRefCountInc(&refcount_);
}

void MessageChannelWin::Impl::Release() {
  if (!base::AtomicRefCountDec(&refcount_))
    delete this;
}

void MessageChannelWin::Impl::ThreadMain() {
  AddRef();

  // Notify the thread has been started.
  base::AtomicRefCountInc(&is_running_);

  thread_event_.Signal();

  {
    base::AutoLock lock(listener_lock_);
    if (listener_)
      listener_->OnMessageChannelConnected(owner_);
  }

  ReceiveInternal();

  // Parse all messages from incoming_buffer_ and deliver them to listener. Each
  // message in the buffer begins with a 32-bit integer length followed by the
  // message body.
  //
  // The loop will process all completed messages until it finish all messages
  // or meets a incompleted one. (msg_size is larger than the remaining data)
  // In such case, the rest of the data will be moved to the front and fetch
  // the next batch of data by calling ReceiveInternal().
  while (true) {
    HANDLE event_handles[4];
    event_handles[0] = quit_event_;
    event_handles[1] = on_received_event_;
    event_handles[2] = on_sent_event_;
    event_handles[3] = send_event_;

    DWORD ret = ::WaitForMultipleObjects(arraysize(event_handles),
                                         event_handles,
                                         FALSE,
                                         INFINITE);
    if (ret == WAIT_OBJECT_0) {
      break;
    } else if (ret == WAIT_OBJECT_0 + 1) {
      OnReceivedEvent();
    } else if (ret == WAIT_OBJECT_0 + 2) {
      OnSentEvent();
    } else if (ret == WAIT_OBJECT_0 + 3) {
      SendInternal();
    } else {
      DWORD error = GetLastError();
      NOTREACHED() << "WaitForMultipleObjects failed error = " << error;
    }
  }

  CleanupPipe();

  base::AtomicRefCountDec(&is_running_);

  {
    base::AutoLock lock(listener_lock_);
    if (listener_)
      listener_->OnMessageChannelClosed(owner_);
  }

  {
    base::AutoLock lock(delegate_lock_);
    if (delegate_)
      delegate_->OnChannelClosed(owner_);
  }

  Release();
}

void MessageChannelWin::Impl::OnReceivedEvent() {
  DWORD bytes_transferred;
  DWORD ret = ::GetOverlappedResult(pipe_,
                                    &receive_overlapped_,
                                    &bytes_transferred,
                                    TRUE);
  if (!ret) {
    DWORD error = GetLastError();
    DLOG(ERROR) << "receive message failed error = " << error;
    ::SetEvent(quit_event_);
    return;
  }
  ::ResetEvent(on_received_event_);
  int32 incoming_buffer_size = bytes_transferred;

  // The following code parse messages from incoming buffer together with
  // overflow buffer. overflow buffer contains incomplete messages.
  //
  // case 1: if overflow buffer is not empty, which means last received message
  // is incomplete, we need to append new incoming buffer to overflow buffer,
  // and if an complete message could be retrieved, call |ParseIncomingMessage|
  // to generate a ipc message and deliver it to listener, repeat parsing the
  // step until no complete message in overflow buffer.
  //
  // case 2: if overflow buffer is empty, which means new incoming buffer begin
  // from a start of ipc message, we will then try to find any complete messages
  // from incoming buffer, if found, call |ParseIncomingMessage| to generate a
  // ipc message and deliver it to listener, otherwise copy uncomplete message
  // buffer to overflow buffer.
  const char* buffer_to_parse = NULL;
  int32 buffer_to_parse_size = 0;
  if (overflow_buffer_.empty()) {
    buffer_to_parse = incoming_buffer_;
    buffer_to_parse_size = incoming_buffer_size;
  } else {
    overflow_buffer_.append(incoming_buffer_, incoming_buffer_size);
    buffer_to_parse = overflow_buffer_.data();
    buffer_to_parse_size = overflow_buffer_.size();
  }
  // Parse all messages in buffer.
  while (buffer_to_parse_size > 0) {
    int32 msg_size = 0;
    if (buffer_to_parse_size < sizeof(msg_size))
      break;

    msg_size = (reinterpret_cast<const int32*>(buffer_to_parse))[0];
    if (msg_size < 0 || msg_size >= ipc::MessageChannel::kMaximumMessageSize) {
      DLOG(ERROR) << "Parse message failed invalid size = " << msg_size;
      ::SetEvent(quit_event_);
      return;
    }
    if (buffer_to_parse_size < msg_size)
      break;

    proto::Message* msg = NULL;
    if (!ParseIncomingMessage(buffer_to_parse + sizeof(msg_size),
                              msg_size - sizeof(msg_size),
                              &msg)) {
      DLOG(ERROR) << "Parse message failed size = " << msg_size;
      ::SetEvent(quit_event_);
      return;
    }
    // Deliver message to listener.
    {
      scoped_ptr<proto::Message> mptr(msg);
      base::AutoLock lock(listener_lock_);
      if (listener_)
        listener_->OnMessageReceived(owner_, mptr.release());
    }
    buffer_to_parse += msg_size;
    buffer_to_parse_size -= msg_size;
  }
  DCHECK_GE(buffer_to_parse_size, 0);
  // Handle left buffer by copying to |overflow_buffer_|.
  overflow_buffer_.assign(buffer_to_parse, buffer_to_parse_size);

  ReceiveInternal();
}

void MessageChannelWin::Impl::OnSentEvent() {
  DWORD bytes_sent;
  DWORD ret = ::GetOverlappedResult(pipe_,
                                    &sent_overlapped_,
                                    &bytes_sent,
                                    TRUE);
  if (!ret)
    ::SetEvent(quit_event_);
  ::ResetEvent(on_sent_event_);

  {
    base::AutoLock lock(sending_list_lock_);
    DCHECK_EQ(bytes_sent, sending_list_.front().size());
    sending_list_.pop();
    if (!sending_list_.empty())
      ::SetEvent(send_event_);
  }
}

void MessageChannelWin::Impl::ReceiveInternal() {
  DWORD size = sizeof(incoming_buffer_);

  if (!::ReadFile(pipe_,
                  incoming_buffer_,
                  size,
                  NULL,
                  &receive_overlapped_)) {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING)
      ::SetEvent(quit_event_);
  }
}

void MessageChannelWin::Impl::CleanupPipe() {
  DCHECK_NE(pipe_, INVALID_HANDLE_VALUE);

  // Cancel overlapped IO.
  if (!::CancelIo(pipe_)) {
    DWORD error = GetLastError();
    DLOG(ERROR) << "CancelIO failed error = " << error;
  } else {
    // If pipe_ is not invalid handle, then CloseHandle will cause access
    // violation, so it should only be closed when CancelIO returns non-zero.
    ::CloseHandle(pipe_);
  }

  pipe_ = INVALID_HANDLE_VALUE;
  ::ResetEvent(on_received_event_);
  ::ResetEvent(on_sent_event_);
  ::ResetEvent(send_event_);

  // Remove all pending messages.
  {
    base::AutoLock lock(sending_list_lock_);
    while (!sending_list_.empty())
      sending_list_.pop();
  }
}

bool MessageChannelWin::Impl::PackOutgoingMessage(
    const proto::Message& message, std::string* buffer) {
  // Place holder for buffer size.
  buffer->append(sizeof(int32), 0);
  // Write message data.
  if (!message.AppendToString(buffer) ||
      (buffer->size() - sizeof(int32)) >= MessageChannel::kMaximumMessageSize) {
    return false;
  }

  // Write buffer size.
  int32 bytes = static_cast<int32>(buffer->size());
  buffer->replace(0, sizeof(bytes),
                  reinterpret_cast<char*>(&bytes), sizeof(int32));

  return true;
}

bool MessageChannelWin::Impl::ParseIncomingMessage(
    const char* buffer, int32 size, proto::Message** message) {
  DCHECK_GT(size, 0);
  *message = NULL;
  scoped_ptr<proto::Message> ret(new proto::Message());

  if (!ret->ParseFromArray(buffer, size))
    return false;

  *message = ret.release();
  return true;
}

void MessageChannelWin::Impl::SendInternal() {
  base::AutoLock lock(sending_list_lock_);
  // Send the message by pipe.
  BOOL ret = ::WriteFile(pipe_,
                         sending_list_.front().data(),
                         sending_list_.front().size(),
                         NULL,
                         &sent_overlapped_);
  if (!ret) {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING) {
      DLOG(ERROR) << "WriteFile failed error = " << error;
      ::SetEvent(quit_event_);
      return;
    }
  }
}

MessageChannelWin::MessageChannelWin(MessageChannelWin::Delegate* delegate)
    : ALLOW_THIS_IN_INITIALIZER_LIST(impl_(new Impl(delegate, this))) {
}

MessageChannelWin::~MessageChannelWin() {
  SetListener(NULL);
  ::SetEvent(impl_->quit_event_);
  impl_->Release();
}

bool MessageChannelWin::IsConnected() const {
  return base::AtomicRefCountIsOne(&impl_->is_running_);
}

bool MessageChannelWin::Send(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  if (!IsConnected())
    return false;

  std::string buffer;
  if (!impl_->PackOutgoingMessage(*message, &buffer))
    return false;

  {
    base::AutoLock lock(impl_->sending_list_lock_);
    impl_->sending_list_.push(buffer);
    if (impl_->sending_list_.size() == 1)
      ::SetEvent(impl_->send_event_);
  }
  return true;
}

void MessageChannelWin::SetListener(Listener* listener) {
  base::AutoLock lock(impl_->listener_lock_);
  if (impl_->listener_ == listener)
    return;
  if (impl_->listener_ != NULL)
    impl_->listener_->OnDetachedFromMessageChannel(this);

  impl_->listener_ = listener;
  if (listener)
    listener->OnAttachedToMessageChannel(this);
}

bool MessageChannelWin::SetHandle(HANDLE handle) {
  DCHECK(!IsConnected());

  if (IsConnected() || handle == INVALID_HANDLE_VALUE)
    return false;

  impl_->pipe_ = handle;
  if (!base::PlatformThread::CreateNonJoinable(0, impl_))
    return false;

  // Make sure the new thread has started.
  return impl_->thread_event_.Wait();
}

void MessageChannelWin::SetDelegate(Delegate* delegate) {
  base::AutoLock lock(impl_->delegate_lock_);
  impl_->delegate_ = delegate;
}

}  // namespace ipc
