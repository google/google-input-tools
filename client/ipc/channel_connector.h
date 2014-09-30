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

#ifndef GOOPY_IPC_CHANNEL_CONNECTOR_H_
#define GOOPY_IPC_CHANNEL_CONNECTOR_H_
#pragma once

#include <string>

#include "base/compiler_specific.h"
#include "ipc/hub.h"
#include "ipc/message_channel.h"
#include "ipc/message_util.h"

// #define CHANNEL_VERBOSE_DEBUG

namespace ipc {

// A Hub::Connector implementation that connects a MessageChannel to a Hub.
class ChannelConnector : public Hub::Connector,
                         public MessageChannel::Listener {
 public:
  // The ChannelConnector object registers itself to the |channel| object as the
  // listener, it'll delete itself whenever it's detached from the |channel|
  // object. The |channel| object should be deleted by somebody else.
  ChannelConnector(Hub* hub, MessageChannel* channel)
      : hub_(hub),
        channel_(channel),
        attached_(false) {
    channel->SetListener(this);
  }

  // Implementation of Hub::Connector.
  virtual bool Send(proto::Message* message) OVERRIDE {
#ifdef CHANNEL_VERBOSE_DEBUG
    std::string text;
    PrintMessageToString(*message, &text, false);
#endif
    bool result = channel_->Send(message);
    if (!result && !channel_->IsConnected())
      hub_->Detach(this);
#ifdef CHANNEL_VERBOSE_DEBUG
    DLOG(INFO) << "Sent to " << this << " ("
               << (result ? "success" : "fail") << "):\n"
               << text;
#endif
    return result;
  }

  virtual void Attached() OVERRIDE {
    attached_ = true;
  }

  virtual void Detached() OVERRIDE {
    attached_ = false;
  }

  // Implementation of MessageChannel::Listener.
  virtual void OnMessageReceived(MessageChannel* channel,
                                 proto::Message* message) OVERRIDE {
    DCHECK(attached_);
#ifdef CHANNEL_VERBOSE_DEBUG
    std::string text;
    PrintMessageToString(*message, &text, false);
    DLOG(INFO) << "Dispatch from " << this << ":\n" << text;
#endif
    hub_->Dispatch(this, message);
  }

  virtual void OnMessageChannelConnected(MessageChannel* channel) OVERRIDE {
    DLOG(WARNING) << "Channel connected:" << this;
    DCHECK(!attached_);
    DCHECK_EQ(channel, channel_);
    hub_->Attach(this);
  }

  virtual void OnMessageChannelClosed(MessageChannel* channel) OVERRIDE {
    DLOG(WARNING) << "Channel closed:" << this;
    DCHECK(attached_);
    DCHECK_EQ(channel, channel_);
    hub_->Detach(this);
  }

  virtual void OnDetachedFromMessageChannel(MessageChannel* channel) OVERRIDE {
    DLOG(WARNING) << "Channel detached:" << this;
    DCHECK_EQ(channel, channel_);
    if (attached_)
      hub_->Detach(this);
    delete this;
  }

 private:
  virtual ~ChannelConnector() {
  }

  Hub* hub_;
  MessageChannel* channel_;
  bool attached_;

  DISALLOW_COPY_AND_ASSIGN(ChannelConnector);
};

}  // namespace ipc

#endif  // CHANNEL_CONNECTOR_H_
