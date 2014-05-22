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

#include "ipc/pipe_server_win.h"

#include <string>

#include "base/atomicops.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "common/security_util_win.h"
#include "ipc/message_channel_win_consts.h"

static const uint32 kMaxMsgBufSize = 4096;
static const uint32 kPipeTimeout = 1000;

namespace ipc {

class PipeServerWin::Impl : public base::PlatformThread::Delegate {
 public:
  Impl(const std::wstring& name,
       PipeServerWin::Delegate* delegate);

  virtual ~Impl();

  virtual void ThreadMain() OVERRIDE;

  // Called to create a named pipe,
  // return INVALID_HANDLE_VALUE if failed, pipe handle if success.
  HANDLE CreatePipe();

  // Called to asynchronously connect a client.
  // Return false if connect failed.
  bool ConnectToClient(HANDLE pipe);

  // Used by |ConnectNamedPipe| to asynchronous connecting a client.
  OVERLAPPED connected_overlapped_;

  // Signaled if one pipe is connected.
  HANDLE connected_event_;

  // Signaled if |Stop| is called.
  HANDLE quit_event_;

  // Thread handle of worker thread.
  base::PlatformThreadHandle thread_;

  // Id of worker thread.
  volatile DWORD thread_id_;

  // Signaled when thread started.
  base::WaitableEvent thread_event_;

  // Name used as part of pipe name.
  std::wstring name_;

  // Actual consumer of named pipe.
  PipeServerWin::Delegate* delegate_;
};

// Implementation of PipeServerWin::Impl.
PipeServerWin::Impl::Impl(const std::wstring& name,
                          PipeServerWin::Delegate* delegate)
    : connected_event_(CreateEvent(NULL, TRUE, FALSE, NULL)),
      quit_event_(CreateEvent(NULL, FALSE, FALSE, NULL)),
      thread_(NULL),
      thread_id_(0),
      thread_event_(false, false),
      name_(name),
      delegate_(delegate) {
  DCHECK(delegate_);
  memset(&connected_overlapped_, 0, sizeof(connected_overlapped_));
  connected_overlapped_.hEvent = connected_event_;
}

PipeServerWin::Impl::~Impl() {
  ::CloseHandle(quit_event_);
  ::CloseHandle(connected_event_);
}

void PipeServerWin::Impl::ThreadMain() {
  HANDLE pipe = CreatePipe();
  if (pipe == INVALID_HANDLE_VALUE) {
    thread_event_.Signal();
    return;
  }
  thread_id_ = ::GetCurrentThreadId();
  thread_event_.Signal();

  while (true) {
    // Start another pipe for next client.
    if (pipe == INVALID_HANDLE_VALUE) {
      pipe = CreatePipe();
      DCHECK(pipe != INVALID_HANDLE_VALUE);
    }

    // Waiting for client connection.
    if (!ConnectToClient(pipe)) {
      // The reason why the failure doesn't cause a loop break is
      // the client may close the recently connected pipe just before server
      // call |ConnectToClient|.
      ::CloseHandle(pipe);
      pipe = INVALID_HANDLE_VALUE;
      continue;
    }

    HANDLE event_handles[2] = {0};
    event_handles[0] = quit_event_;
    event_handles[1] = connected_event_;

    DWORD ret = ::WaitForMultipleObjects(arraysize(event_handles),
                                         event_handles,
                                         FALSE,
                                         INFINITE);
    if (ret == WAIT_OBJECT_0) {
      // |Stop| is called.
      break;
    } else if (ret == WAIT_OBJECT_0 + 1) {
      // pipe connected
      ::ResetEvent(connected_event_);
      // Connect pipe channel to hub.
      delegate_->OnPipeConnected(pipe);
      pipe = INVALID_HANDLE_VALUE;
    } else {
      DWORD error = ::GetLastError();
      NOTREACHED() << "WaitForMultipleObjects failed error = " << error;
      break;
    }
  }

  if (pipe != INVALID_HANDLE_VALUE) {
    if (!::CancelIo(pipe)) {
      DWORD error = ::GetLastError();
      if (error != ERROR_NOT_FOUND)
        DLOG(ERROR) << "CancelIO failed with error = " << error;
    }
    ::CloseHandle(pipe);
  }
  thread_id_ = 0;
}

HANDLE PipeServerWin::Impl::CreatePipe() {
  // Get proper security attributes for inter-process communication.
  SECURITY_ATTRIBUTES security_attributes = {0};
  if (!ime_goopy::GetIPCSecurityAttributes(&security_attributes)) {
    DLOG(ERROR) << "GetIPCSecurityAttributes failed";
    return INVALID_HANDLE_VALUE;
  }

  // Create asynchronous named pipe.
  HANDLE pipe = CreateNamedPipe(
      name_.c_str(),
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
      PIPE_UNLIMITED_INSTANCES,
      kMaxMsgBufSize,
      kMaxMsgBufSize,
      kPipeTimeout,
      &security_attributes);

  ime_goopy::ReleaseIPCSecurityAttributes(&security_attributes);

  if (pipe == INVALID_HANDLE_VALUE) {
    DWORD error = ::GetLastError();
    DLOG(ERROR) << "CreateNamedPipe failed error = " << error;
  }
  return pipe;
}

bool PipeServerWin::Impl::ConnectToClient(HANDLE pipe) {
  ::ConnectNamedPipe(pipe, &connected_overlapped_);
  DWORD error = ::GetLastError();
  if (error == ERROR_PIPE_CONNECTED) {
    // According to experiment, overlapped event will not be fired in this case.
    ::SetEvent(connected_event_);
  } else if (error != ERROR_IO_PENDING) {
    DLOG(ERROR) << "ConnectNamedPipe failed error = " << error;
    return false;
  }
  return true;
}

PipeServerWin::PipeServerWin(const std::wstring& name,
                             PipeServerWin::Delegate* delegate)
    : ALLOW_THIS_IN_INITIALIZER_LIST(impl_(new Impl(name, delegate))) {
}

PipeServerWin::~PipeServerWin() {
  Stop();
}

bool PipeServerWin::Start() {
  DCHECK_EQ(impl_->thread_id_, 0);

  if (::wcsncmp(impl_->name_.c_str(),
                kWinIPCPipeNamePrefix,
                ::wcslen(kWinIPCPipeNamePrefix)) != 0) {
    return false;
  }

  // Make sure old thread has been terminated.
  Stop();

  impl_->thread_event_.Reset();
  if (!base::PlatformThread::Create(0, impl_.get(), &impl_->thread_))
    return false;

  // Make sure the new thread has started.
  return impl_->thread_event_.Wait();
}

void PipeServerWin::Stop() {
  DCHECK_NE(::GetCurrentThreadId(), impl_->thread_id_);

  if (impl_->thread_) {
    ::SetEvent(impl_->quit_event_);
    base::PlatformThread::Join(impl_->thread_);
    impl_->thread_ = NULL;
    ::ResetEvent(impl_->quit_event_);
  }
}

}  // namespace ipc
