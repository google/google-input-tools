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
#ifndef GOOPY_IPC_MESSAGE_CHANNEL_WIN_H_
#define GOOPY_IPC_MESSAGE_CHANNEL_WIN_H_

#include <string>
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "ipc/message_channel.h"

namespace ipc {

class MessageChannelWin : public MessageChannel {
 public:
  // An interface that should be implemented by the owner of message channel
  // objects to let the owner know when a message channel is closed, so that
  // the owner can decide to destroy or reconnect the message channel.
  class Delegate {
   public:
    virtual ~Delegate() {}
    // Should not call |SetHandle| directly from this method.
    virtual void OnChannelClosed(MessageChannelWin* channel) = 0;
  };

  // |delegate| is optional.
  explicit MessageChannelWin(Delegate* delegate);

  virtual ~MessageChannelWin();

  // Overridden from MessageChannel:
  virtual bool IsConnected() const OVERRIDE;
  virtual bool Send(proto::Message* message) OVERRIDE;
  virtual void SetListener(Listener* listener) OVERRIDE;

  // Sets the working pipe. It can only be called where there is no working
  // pipe.
  // The |pipe_handle| must be a handle of a pipe opened with
  // PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED open mode and
  // PIPE_TYPE_BYTE|PIPE_READMODE_BYTE pipe mode.
  // Return false if creating thread fails.
  // SetHandle is not thread safe and should only be called from same thread.
  bool SetHandle(HANDLE pipe_handle);

  // Set the delegate of message channel. must be called from other thread than
  // the worker thread.
  void SetDelegate(Delegate* delegate);

 private:
  class Impl;
  Impl* impl_;

  DISALLOW_COPY_AND_ASSIGN(MessageChannelWin);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_CHANNEL_WIN_H_
