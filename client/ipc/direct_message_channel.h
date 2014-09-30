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

#ifndef GOOPY_IPC_DIRECT_MESSAGE_CHANNEL_H_
#define GOOPY_IPC_DIRECT_MESSAGE_CHANNEL_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "ipc/message_channel.h"

namespace ipc {

class Hub;

// A MessageChannel implementation that can be connected to Hub directly.
// It'll attach to the given hub object automatically when a listener is set to
// it, and detach when the listener is set to NULL.
// The given hub object must outlive this channel object.
class DirectMessageChannel : public MessageChannel {
 public:
  explicit DirectMessageChannel(Hub* hub);
  virtual ~DirectMessageChannel();

  // Overridden from MessageChannel:
  virtual bool IsConnected() const OVERRIDE;
  virtual bool Send(proto::Message* message) OVERRIDE;
  virtual void SetListener(Listener* listener) OVERRIDE;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(DirectMessageChannel);
};

}  // namespace ipc


#endif  // GOOPY_IPC_DIRECT_MESSAGE_CHANNEL_H_
