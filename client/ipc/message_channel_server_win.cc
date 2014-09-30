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

#include "ipc/message_channel_server_win.h"

#include <aclapi.h>
#include <sddl.h>
#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "common/security_util_win.h"
#include "common/vistautil.h"
#include "ipc/channel_connector.h"
#include "ipc/hub.h"
#include "ipc/message_channel_win.h"
#include "ipc/message_channel_win_consts.h"

namespace ipc {

// Implementation of MessageChannelServerWin.
MessageChannelServerWin::MessageChannelServerWin(Hub* hub)
    : hub_(hub),
      shared_mem_handle_(NULL),
      shared_memory_name_(kWinIPCSharedMemoryName),
      server_name_(kWinIPCServerName) {
  DCHECK(hub_);
}

MessageChannelServerWin::MessageChannelServerWin(
    Hub* hub,
    const std::wstring& shared_memory_name,
    const std::wstring& server_name)
    : hub_(hub),
      shared_mem_handle_(NULL),
      shared_memory_name_(shared_memory_name),
      server_name_(server_name) {
}

MessageChannelServerWin::~MessageChannelServerWin() {
  if (shared_mem_handle_ != NULL)
    ::CloseHandle(shared_mem_handle_);
  // Pipe server should stop before removing channels.
  // Or else the pipe server may start the channel in OnPipeConnected after the
  // channel has been removed.
  if (pipe_server_.get())
    pipe_server_->Stop();

  // Remove all remaining channels.
  // |channels_lock_| has to be released before SetDelegate. because SetDelegate
  // will acquire another lock which may currently be
  // held by |MessageChannelWin| running |delegate_->OnChannelClosed| which is
  // acquiring |channels_lock_|, so there is a deadlock.
  while (true) {
    MessageChannelWin* channel_to_delete = NULL;
    {
      base::AutoLock lock(channels_lock_);
      if (channels_.empty())
        break;
      std::set<MessageChannelWin*>::iterator iter = channels_.begin();
      channel_to_delete = *iter;
      channels_.erase(iter);
    }
    if (channel_to_delete) {
      channel_to_delete->SetDelegate(NULL);
      delete channel_to_delete;
    }
  }
}

bool MessageChannelServerWin::Initialize() {
  // Create shared memory then store session id into it.
  DWORD session_id;
  ::ProcessIdToSessionId(GetCurrentProcessId(), &session_id);

  // Set security attributes to proper state for inter-process communication.
  SECURITY_ATTRIBUTES sa = {0};
  ime_goopy::GetIPCFileMapReadOnlySecurityAttributes(&sa);

  // Creating file mapping and storing session id is not atomic.
  // If a client is connecting between the time slot file mapping is created and
  // session id is stored, client will not have a valid pipe name to connect, it
  // will take the client another 50ms to reconnect.
  shared_mem_handle_ = ::CreateFileMapping(
      INVALID_HANDLE_VALUE,
      &sa,
      PAGE_READWRITE,
      0,
      sizeof(DWORD),
      shared_memory_name_.c_str());
  ime_goopy::ReleaseIPCSecurityAttributes(&sa);
  if (!shared_mem_handle_) {
    DLOG(ERROR) << "CreateFileMapping failed with error = "
                << ::GetLastError();
    return false;
  }

  // Store session id into the shared memory.
  DWORD* data = reinterpret_cast<DWORD*>(
      ::MapViewOfFile(shared_mem_handle_,
                      FILE_MAP_WRITE,
                      0,
                      0,
                      sizeof(DWORD)));
  if (!data) {
    DLOG(ERROR) << "MapViewOfFile failed with error ="
                << ::GetLastError();
    return false;
  }

  *data = session_id;

  ::UnmapViewOfFile(data);

  // Start pipe server, which will listen to incoming pipe
  // connections from clients.
  wchar_t pipe_name[MAX_PATH] = {0};
  ::_snwprintf_s(
      pipe_name, MAX_PATH, L"%s%d%s",
      kWinIPCPipeNamePrefix, session_id,
      server_name_.c_str());
  pipe_server_.reset(new PipeServerWin(pipe_name, this));

  return pipe_server_->Start();
}

void MessageChannelServerWin::OnPipeConnected(HANDLE pipe) {
  // |connector| & |channel| will be deleted by hub_ when it is detached.
  MessageChannelWin* channel = new MessageChannelWin(this);
  {
    base::AutoLock lock(channels_lock_);
    DCHECK(!channels_.count(channel));
    channels_.insert(channel);
  }
  // Channel connector will manage its life cycle itself.
  new ChannelConnector(hub_, channel);
  channel->SetHandle(pipe);
}

void MessageChannelServerWin::OnChannelClosed(MessageChannelWin* channel) {
  scoped_ptr<MessageChannelWin> channel_to_delete;
  {
    base::AutoLock lock(channels_lock_);
    std::set<MessageChannelWin*>::iterator iter = channels_.find(channel);
    if (iter != channels_.end()) {
      channels_.erase(iter);
      channel_to_delete.reset(channel);
    }
  }
}

}  // namespace ipc
