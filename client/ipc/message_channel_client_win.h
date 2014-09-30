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

#ifndef GOOPY_IPC_MESSAGE_CHANNEL_CLIENT_WIN_H_
#define GOOPY_IPC_MESSAGE_CHANNEL_CLIENT_WIN_H_

#include <string>
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "ipc/message_channel.h"

namespace ipc {

// MessageChannelClientWin is responsible for creating MessageChannelWin
// instance for MultiComponentHost's use.
// A worker thread is created to monitor the status of server side of message
// channel and to connect if server side is ready.
// if the channel is broken, worker thread will be notified and then start
// reconnecting to create a new one.
class MessageChannelClientWin {
 public:
  explicit MessageChannelClientWin(MessageChannel::Listener * listener);

  // shared_memory_name :
  //   Shared memory created by MessageChannelServerWin to store information
  //   needed to connect the server pipe.
  // server_name :
  //   Name of server pipe created by MessageChannelServerWin.
  MessageChannelClientWin(MessageChannel::Listener * listener,
                          const std::wstring& shared_memory_name,
                          const std::wstring& server_name);


  ~MessageChannelClientWin();

  // Start the worker thread.
  // return false if worker thread can't be created or is already running.
  // This method is not thread-safe, caller must alternately call |Start| and
  // |Stop|.
  bool Start();

  // Stop the worker thread, return after it terminates.
  // This method is not thread-safe.
  void Stop();

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_COPY_AND_ASSIGN(MessageChannelClientWin);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_CHANNEL_CLIENT_WIN_H_
