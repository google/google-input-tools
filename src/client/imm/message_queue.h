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

#ifndef GOOPY_IMM_MESSAGE_QUEUE_H__
#define GOOPY_IMM_MESSAGE_QUEUE_H__

#include "base/basictypes.h"
#include "imm/context_locker.h"
#include "imm/immdev.h"

namespace ime_goopy {
namespace imm {
// MessageQueue is used to send messages to IMM. There are two typical usages
// of this class. One is to attach to a transmsglist, this is usually used in
// ImeToAsciiEx, the added message will be stored in the transmsglist, and the
// count of the messages will be returned when Detach(). Another usage is
// sending messages to IMM on our own initiative, in this case the messages
// will be stored in the internal vector and will be send to IMM when Send() is
// called.
template <class ImmLockPolicy, class ImmMessagePolicy>
class MessageQueueT {
 public:
  explicit MessageQueueT(HIMC himc)
      : himc_(himc), transmsg_(NULL), transmsg_count_(0) {
  }

  void Attach(LPTRANSMSGLIST transmsg) {
    // Make sure there is no unsent messages before attaching.
    Send();
    transmsg_ = transmsg;
  }

  // Detach from a message list, return the message count in the list.
  int Detach() {
    LPTRANSMSGLIST transmsg = transmsg_;
    transmsg_ = NULL;
    int transmsg_count = transmsg_count_;
    transmsg_count_ = 0;

    // If we are not using the extended message vector, means that the
    // TRANSMSGLIST is big enough, simply return the count in TRANSMSGLIST.
    if (!messages_.size()) return transmsg_count;

    // If transmsg_ is not big enough to store all messages, those extra
    // messages are stored temporarily in messages_ vector. In this case, all
    // messages should be stored in the message buffer of the context.
    // Generally, transmsg_ can contains 256 messages, but this number is not
    // documented, so there are chances that it may be full.

    // Prepare message buffer.
    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    // If anything wrong, return the message count in TRANSMSGLIST, the extra
    // messages will be saved in vector so they can be sent out later.
    if (!context.get()) return transmsg_count;

    int size = static_cast<int>(transmsg_count + messages_.size()) *
               sizeof(TRANSMSG);
    HIMCCLockerT<TRANSMSG, ImmLockPolicy> message_buffer(
        &context->hMsgBuf, size);
    if (!message_buffer.get()) return transmsg_count;

    // Copy messages from transmsg_ to message buffer.
    CopyMemory(message_buffer.get(),
               &transmsg->TransMsg,
               sizeof(TRANSMSG) * transmsg_count);

    // Copy the rest messages from messages_ to message buffer.
    int count_in_vector = static_cast<int>(messages_.size());
    for (int i = 0; i < count_in_vector; i++) {
      message_buffer[i + transmsg_count].message = messages_[i].message;
      message_buffer[i + transmsg_count].wParam = messages_[i].wParam;
      message_buffer[i + transmsg_count].lParam = messages_[i].lParam;
    }
    messages_.clear();
    return transmsg_count + count_in_vector;
  }

  void AddMessage(UINT message, WPARAM wparam, LPARAM lparam) {
    // If transmsg_ is not big enough, we store extra messages in messages_
    // vector.
    if (transmsg_ && transmsg_count_ < transmsg_->uMsgCount) {
      transmsg_->TransMsg[transmsg_count_].message = message;
      transmsg_->TransMsg[transmsg_count_].wParam = wparam;
      transmsg_->TransMsg[transmsg_count_].lParam = lparam;
      transmsg_count_++;
    } else {
      TRANSMSG transmsg = {message, wparam, lparam};
      messages_.push_back(transmsg);
    }
  }

  // Send the messages to context if not attached to a message list.
  bool Send() {
    // Don't send if attached to a TRANSMSGLIST, these messages will be sent via
    // the buffer provided by ImeToAsciiEx.
    if (transmsg_) return false;
    if (!messages_.size()) return false;

    // Prepare message buffer.
    HIMCLockerT<INPUTCONTEXT, ImmLockPolicy> context(himc_);
    if (!context.get()) return false;

    int size = static_cast<int>(messages_.size()) * sizeof(TRANSMSG);
    HIMCCLockerT<TRANSMSG, ImmLockPolicy> message_buffer(
        &context->hMsgBuf, size);
    if (!message_buffer.get()) return false;

    // Copy to message buffer.
    int count_in_vector = static_cast<int>(messages_.size());
    for (int i = 0; i < count_in_vector; i++) {
      message_buffer[i].message = messages_[i].message;
      message_buffer[i].wParam = messages_[i].wParam;
      message_buffer[i].lParam = messages_[i].lParam;
    }
    context->dwNumMsgBuf = count_in_vector;
    messages_.clear();

    // Send to IMM.
    if (!ImmMessagePolicy::ImmGenerateMessage(himc_)) return false;
    return true;
  }

 private:
  HIMC himc_;
  LPTRANSMSGLIST transmsg_;
  vector<TRANSMSG> messages_;
  int transmsg_count_;
};

class WindowsImmMessagePolicy {
 public:
  inline static BOOL ImmGenerateMessage(HIMC himc) {
    return ::ImmGenerateMessage(himc);
  }
};

typedef MessageQueueT<WindowsImmLockPolicy, WindowsImmMessagePolicy>
  MessageQueue;
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_MESSAGE_QUEUE_H__
