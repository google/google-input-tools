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

#ifndef GOOPY_IPC_MESSAGE_QUEUE_WIN_H_
#define GOOPY_IPC_MESSAGE_QUEUE_WIN_H_
#pragma once

#include <windows.h>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "ipc/message_queue.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

// MessageQueueWin implements MessageQueue interface. the implementation
// borrows from windows message loop mechanism which starts from creating
// a dedicated window within the construction method.
//
// After construction, all messages posted (by calling PostMessage) are
// actually delivered to and dispatched by the window's message loop,
// and finally picked up by this class from the window procedure registered
// when the window is created. then the message dispatching is taken over
// from windows message loop and given to |handler_| to dispatch.
//
// caller|           | windows message loop |                       |handler_
//       |PostMessage|                      |                       |
//       |---------->|        ... ...       |                       |
//       |           |        ... ...       |                       |
//       |           |     DispatchMessage  |                       |
//       |           |        ... ...       | Handler::HandleMessage|
//       |           |     IPCWindowProc    | --------------------->| ... ...
//       |           |                      |                       |
//       |           |                      |                       |
//
//   Message convertion from and to windows message
//
// Once a IPC message is posted (by PostMessage function call) to windows
// message queue,it's converted as a 'WPARAM' of a windows message of type=
// |WM_GOOPY_IPC_MSG| and LPARAM=|NULL|, it's in the IPCWindowProc where the
// IPC message is translated back.
//
//   How to Quit
//
// If Quit() is called from any threads, the MessageQueueWin will reject any
// calls of |Post| and simply return false and delete the message.
// If the message queue is right blocked in |DoMessage|, the call of |Quit|
// from any thread also makes the |DoMessage| return quickly. it's achieved
// by posting a special message to message loop(a NULL IPC Message),
// which is treated as a quit signal right after it's converted from windows
// message to IPC message.
//
//   Cross Thread Safety
//
// the windows message queue is thread-unique, once a
// instance of this class is created in one thread, all messages posted to
// the dedicated window would be handled in the thread's context in a FIFO
// way. so no matter at which thread a message is created, it's handled and
// terminated synchroninously by handler_ in the same thread the dedicated
// window is created.

class MessageQueueWin : public MessageQueue {
 public:
  explicit MessageQueueWin(Handler* handler);

  virtual ~MessageQueueWin();

  // Init MessageQueue by creating a window for communication.
  bool Init();

  // Overriden from interface MessageQueue.
  // Refer to message_queue for details.
  virtual bool Post(proto::Message* message, void* user_data) OVERRIDE;
  virtual bool DoMessage(int* timeout) OVERRIDE;
  virtual bool DoMessageNonexclusive(int* timeout) OVERRIDE;
  virtual void Quit() OVERRIDE;
  virtual boolean InCurrentThread() OVERRIDE;
  uint32 thread_id() { return thread_id_; }

 private:
  // IPCWindowHelper is responsible for creating IPC Window that transports
  // IPC messages.
  class IPCWindowHelper;

  bool DoMessageInternal(bool exclusive, int* timeout);

  // The id of thread that the messge queue is created in.
  uint32 thread_id_;

  // Window handle working with IPC.
  // See above comments for details.
  HWND hwnd_;

  // Lock that protect this message queue.
  base::Lock lock_;

  // Indicate Quit() is called.
  bool is_quiting_;

  // Consumer of IPC messages from this message queue.
  Handler* handler_;

  // recursive level of DoMessage.
  int do_message_exclusive_level_;

  // recursive level of DoMessageNonexclusive.
  int do_message_nonexclusive_level_;

  DISALLOW_COPY_AND_ASSIGN(MessageQueueWin);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_QUEUE_WIN_H_
