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

#ifndef GOOPY_IPC_MESSAGE_CHANNEL_SERVER_WIN_H_
#define GOOPY_IPC_MESSAGE_CHANNEL_SERVER_WIN_H_

#include <windows.h>
#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "ipc/hub.h"
#include "ipc/message_channel_win.h"
#include "ipc/pipe_server_win.h"

namespace ipc {

class Hub;

// MessageChannelServerWin maintains the creation of message channel, after a
// message channel is connected, it will be attached to hub, then hub is
// responsible for deleting it.
//
// Since client may not have enough privilege to know which session it belongs
// to, so MessageChannelServerWin will create a shared memory to inform clients
// of such information. To create the shared memory, process must have
// PROCESS_QUERY_INFORMATION privilege and other necessary privileges to create
// the shared memory.
class MessageChannelServerWin : public PipeServerWin::Delegate,
                                MessageChannelWin::Delegate {
 public:
  // Server name and shared memory name are set to default.
  explicit MessageChannelServerWin(Hub* hub);

  // shared_memory_name :
  //   Shared memory created to store information needed by client to connect
  //   the pipe.
  // server_name :
  //   Name of server pipe to create.
  MessageChannelServerWin(Hub* hub,
                          const std::wstring& shared_memory_name,
                          const std::wstring& server_name);

  virtual ~MessageChannelServerWin();

  bool Initialize();

 private:
  // Overridden from |PipeServerWin::Delegate|:
  virtual void OnPipeConnected(HANDLE pipe) OVERRIDE;

  // Overridden from |MessageChannelWin::Delegate|:
  virtual void OnChannelClosed(MessageChannelWin* channel) OVERRIDE;

  scoped_ptr<PipeServerWin> pipe_server_;
  Hub* hub_;
  HANDLE shared_mem_handle_;
  std::set<MessageChannelWin*> channels_;
  base::Lock channels_lock_;
  std::wstring shared_memory_name_;
  std::wstring server_name_;
  DISALLOW_COPY_AND_ASSIGN(MessageChannelServerWin);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_CHANNEL_SERVER_WIN_H_
