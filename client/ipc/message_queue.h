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

#ifndef GOOPY_IPC_MESSAGE_QUEUE_H_
#define GOOPY_IPC_MESSAGE_QUEUE_H_
#pragma once

namespace ipc {

namespace proto {
class Message;
}

// An interface class for implementing an in-process asynchronous message queue.
class MessageQueue {
 public:
  // Implemented by consumers of a MessageQueue to handle messages. A
  // MessageQueue should be associated to a Handler upon construction. The
  // Handler should not be changed during a MessageQueue's whole life.
  class Handler {
   public:
    virtual ~Handler() {}

    // Called when a message is received. This method will always be called on
    // the thread that creates the MessageQueue.
    virtual void HandleMessage(proto::Message* message, void* user_data) = 0;
  };

  virtual ~MessageQueue() {}

  // Posts a message to the MessageQueue asynchronously. This method can be
  // called from any thread. Ownership of the |message| will be consumed by the
  // MessageQueue. Posting a NULL message is allowed to unblock a DoMessage()
  // call, but a NULL message will not be dispatched.
  // False will be returned when any error occurrs, e.g. if Quit() has been
  // called. |message| will be deleted immediately when returning false.
  // |user_data| is an arbitrary pointer which will be dispatched along with the
  // message, its ownership will not be consumed by the MessageQueue.
  virtual bool Post(proto::Message* message, void* user_data) = 0;

  // Wait and dispatch one message. |*timeout| is number of milliseconds to wait
  // before return, if a message is dispatched in time, then remained time will
  // be stored in |*timeout| and true will be returned. Otherwise |*timeout|
  // will be set to zero and false will be returned. NULL or negative value
  // means unlimited timeout. Zero means do not wait at all.
  // This method can only be called from the thread creating the MessageQueue,
  // but it can be called recursively from Handler::HandleMessage().
  // Returns false immediately when getting a NULL message or Quit() has been
  // called..
  virtual bool DoMessage(int* timeout) = 0;

  // Similar with DoMessage but runs in a nonexclusive way, which means messages
  // of other message queues and UI in the thread will also be dispatched. And
  // same with DoMessage, this method will not return until a message of this
  // message queue is dispatched.
  // This function can not be called from Handler::HandleMessage() in case there
  // is a call of DoMessage in the stack.
  virtual bool DoMessageNonexclusive(int* timeout) = 0;

  // Unblocks all recursive blocking DoMessage() calls. It might be called from
  // any thread.
  virtual void Quit() = 0;

  // Returns true if the message queue is running in the calling thread.
  virtual boolean InCurrentThread() { return false; }

  // Creates a new MessageQueue object and associates it to a given handler.
  // It should be implemented per platform.
  static MessageQueue* Create(Handler* handler);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_QUEUE_H_
