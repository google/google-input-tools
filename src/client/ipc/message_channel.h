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

#ifndef GOOPY_IPC_MESSAGE_CHANNEL_H_
#define GOOPY_IPC_MESSAGE_CHANNEL_H_
#pragma once

namespace ipc {

namespace proto {
class Message;
}

// An interface class for sending and receiving messages between two processes.
class MessageChannel {
 public:
  // Implemented by the consumer of a MessageChannel to receive messages.
  // Methods of this interface may be called from different threads, so the
  // implementation must be threadsafe.
  class Listener {
   public:
    virtual ~Listener() {}

    // Called when a message is received. The message's ownership will be
    // transfered to the Listener.
    virtual void OnMessageReceived(MessageChannel* channel,
                                   proto::Message* message) = 0;

    // Called when the message channel is connected.
    virtual void OnMessageChannelConnected(MessageChannel* channel) {}

    // Called when the message channel is closed by any reason.
    virtual void OnMessageChannelClosed(MessageChannel* channel) {}

    // Called when the listener is attached to the message channel, i.e. when
    // MessageChannel::SetListener() gets called.
    virtual void OnAttachedToMessageChannel(MessageChannel* channel) {}

    // Called when the listener is detached from the message channel, i.e. when
    // another listener is attached to the message channel, or the message
    // channel is destroyed.
    virtual void OnDetachedFromMessageChannel(MessageChannel* channel) {}
  };

  enum {
    // The maximum message size in bytes. Attempting to receive a
    // message of this size or bigger results in a channel error.
    kMaximumMessageSize = 16 * 1024 * 1024,

    // Ammount of data to read at once from the pipe.
    kReadBufferSize = 4 * 1024
  };

  virtual ~MessageChannel() {}

  // Checks if the message channel is connected or not.
  virtual bool IsConnected() const = 0;

  // Sends a message asynchronously. The message will be deleted automatically.
  virtual bool Send(proto::Message* message) = 0;

  // Sets the listener object. Only one listener can be set to a message
  // channel.
  virtual void SetListener(Listener* listener) = 0;
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_CHANNEL_H_
