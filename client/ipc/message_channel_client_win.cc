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

#include "ipc/message_channel_client_win.h"

#include <windows.h>
#include <string>

#include "base/atomicops.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/message_channel_win.h"
#include "ipc/message_channel_win_consts.h"

// TODO(haicsun): don't use loop.
static const DWORD kRetryConnectInterval = 100;  // In milliseconds.

namespace ipc {

class MessageChannelClientWin::Impl : public base::PlatformThread::Delegate,
                                      public MessageChannelWin::Delegate {
 public:
  Impl(MessageChannel::Listener* listener,
       const std::wstring& shared_memory_name,
       const std::wstring& server_name);
  virtual ~Impl();

  // Overridden from |base::PlatformThread::Delegate|:
  virtual void ThreadMain() OVERRIDE;

  // Overridden from |MessageChannelWin::Delegate|:
  virtual void OnChannelClosed(MessageChannelWin* channel) OVERRIDE;

  // Signaled when thread started.
  base::WaitableEvent thread_event_;

  // Thread handle of worker thread.
  base::PlatformThreadHandle thread_;

  // Id of worker thread.
  volatile DWORD thread_id_;

  // Signaled if |Stop| is called.
  HANDLE quit_event_;

  // Signaled if server pipe can't be connected.
  HANDLE reconnect_event_;

  // The message channel is created by worker thread and used by listener_ to
  // deliver messages.
  scoped_ptr<MessageChannelWin> channel_;

  // Shared memory's name which is created by server used to get session
  // information.
  std::wstring shared_memory_name_;

  // Server pipe name.
  std::wstring server_name_;

  // Consumer of |channel_|.
  MessageChannel::Listener* listener_;
};

// Implementation of MessageChannelClientWin::Impl.
MessageChannelClientWin::Impl::Impl(MessageChannel::Listener* listener,
                                    const std::wstring& shared_memory_name,
                                    const std::wstring& server_name)
    : thread_event_(false, false),
      thread_(NULL),
      thread_id_(0),
      quit_event_(CreateEvent(NULL, FALSE, FALSE, NULL)),
      reconnect_event_(CreateEvent(NULL, FALSE, FALSE, NULL)),
      shared_memory_name_(shared_memory_name),
      server_name_(server_name),
      listener_(listener) {
}

MessageChannelClientWin::Impl::~Impl() {
  if (channel_.get()) {
    channel_->SetDelegate(NULL);
    channel_.reset(NULL);
  }
  ::CloseHandle(quit_event_);
  ::CloseHandle(reconnect_event_);
}

void MessageChannelClientWin::Impl::ThreadMain() {
  // Notify the thread has been started.
  thread_id_ = ::GetCurrentThreadId();
  thread_event_.Signal();

  HANDLE event_handles[2];
  event_handles[0] = quit_event_;
  event_handles[1] = reconnect_event_;

  bool retry = false;
  while (true) {
    DWORD ret = ::WaitForMultipleObjects(
        arraysize(event_handles),
        event_handles,
        FALSE,
        retry ? kRetryConnectInterval : INFINITE);
    if (ret == WAIT_OBJECT_0) {
      // |quit_event_| is signaled.
      break;
    } else if (ret == (WAIT_OBJECT_0 + 1) || ret == WAIT_TIMEOUT) {
      // |reconnect event_| is signaled or need to reconnect.

      // Get pipe name from shared memory.
      // Local file mapping only takes effect for current session.
      HANDLE shared_mem_handle = ::OpenFileMapping(
          FILE_MAP_READ,
          FALSE,
          shared_memory_name_.c_str());
      if (shared_mem_handle == NULL) {
        // Server is not ready.
        DWORD error = ::GetLastError();
        if (error != ERROR_FILE_NOT_FOUND)
          DLOG(ERROR) << "Can't open file mapping error = " << error;
        retry = true;
        continue;
      }

      DWORD* data = reinterpret_cast<DWORD*>(
          ::MapViewOfFile(shared_mem_handle,
                          FILE_MAP_READ,
                          0,
                          0,
                          sizeof(DWORD)));
      if (!data) {
        DWORD error = ::GetLastError();
        DLOG(ERROR) << "Can't open file mapping error = " << error;
        ::CloseHandle(shared_mem_handle);
        retry = true;
        continue;
      }

      DWORD session_id = *data;
      ::UnmapViewOfFile(data);
      ::CloseHandle(shared_mem_handle);

      // Start connecting server pipe.
      wchar_t pipe_name[MAX_PATH] = {0};
      ::_snwprintf_s(pipe_name,
                     MAX_PATH,
                     L"%s%d%s",
                     kWinIPCPipeNamePrefix,
                     session_id,
                     server_name_.c_str());

      HANDLE pipe = ::CreateFile(pipe_name,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);
      if (pipe == INVALID_HANDLE_VALUE) {
        DWORD error = ::GetLastError();
        if (error != ERROR_FILE_NOT_FOUND)
          DLOG(ERROR) << "CreateFile failed error = " << error;
        retry = true;
        continue;
      }
      retry = false;
      // If connect successfully, initialized the message channel.
      if (!channel_.get())
        channel_.reset(new MessageChannelWin(this));
      channel_->SetListener(listener_);
      channel_->SetHandle(pipe);
    } else {
      DWORD error = ::GetLastError();
      NOTREACHED() << "WaitForMultipleObjects failed error = " << error;
      break;
    }
  }
  thread_id_ = 0;
}

void MessageChannelClientWin::Impl::OnChannelClosed(
    MessageChannelWin* channel) {
  DCHECK_EQ(channel, channel_.get());
  ::SetEvent(reconnect_event_);
}

// Implementation of MessageChannelClientWin.
MessageChannelClientWin::MessageChannelClientWin(
    MessageChannel::Listener* listener)
    : impl_(new Impl(listener, kWinIPCSharedMemoryName, kWinIPCServerName)) {
}

MessageChannelClientWin::MessageChannelClientWin(
    MessageChannel::Listener* listener,
    const std::wstring& shared_memory_name,
    const std::wstring& server_name)
    : impl_(new Impl(listener, shared_memory_name, server_name)) {
}

MessageChannelClientWin::~MessageChannelClientWin() {
  Stop();
}

bool MessageChannelClientWin::Start() {
  DCHECK_EQ(impl_->thread_id_, 0);

  // Make sure old thread has been terminated.
  Stop();

  ::ResetEvent(impl_->reconnect_event_);
  impl_->thread_event_.Reset();
  if (!base::PlatformThread::Create(0, impl_.get(), &impl_->thread_))
    return false;
  ::SetEvent(impl_->reconnect_event_);

  return impl_->thread_event_.Wait();
}

void MessageChannelClientWin::Stop() {
  DCHECK_NE(::GetCurrentThreadId(), impl_->thread_id_);

  if (impl_->thread_) {
    ::SetEvent(impl_->quit_event_);
    base::PlatformThread::Join(impl_->thread_);
    impl_->thread_ = NULL;
    ::ResetEvent(impl_->quit_event_);
  }
}

}  // namespace ipc
