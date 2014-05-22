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

#include "ipc/hub_host.h"

#include "base/synchronization/waitable_event.h"
#include "ipc/message_types.h"
#include "ipc/simple_message_queue.h"

namespace {

// Private data for MessageQueue::Post to post a message synchronously
// the event will be signaled when the message is handled
struct ControlMessageUserData {
  explicit ControlMessageUserData(void* data)
      : private_data(data), message_handled_event(false, false) {
  }
  void* private_data;
  base::WaitableEvent message_handled_event;
};

// System reserved message type definition used by attach & detach
enum {
  MSG_ATTACH_HUBHOST = ipc::MSG_SYSTEM_RESERVED_START,
  MSG_DETACH_HUBHOST,
};

}  // namespace

namespace ipc {

HubHost::HubHost() {
}

HubHost::~HubHost() {
  Quit();
}

void HubHost::Attach(Hub::Connector* connector) {
  DCHECK(connector);

  ControlMessageUserData user_data(reinterpret_cast<void*>(connector));
  bool ok;
  {
    base::AutoLock locker(runner_lock_);
    DCHECK(message_queue_.get());

    // Post an attach control message to runner and wait until it's handled
    scoped_ptr<proto::Message> ctrl_msg(new proto::Message());
    DCHECK(ctrl_msg.get());
    ctrl_msg->set_type(MSG_ATTACH_HUBHOST);

    if (message_queue_->InCurrentThread()) {
      HandleMessage(ctrl_msg.release(), reinterpret_cast<void*>(&user_data));
    } else {
      ok = message_queue_->Post(
        ctrl_msg.release(), reinterpret_cast<void*>(&user_data));
      DCHECK(ok);
    }
  }
  ok = user_data.message_handled_event.Wait();
  DCHECK(ok);
}

void HubHost::Detach(Hub::Connector* connector) {
  DCHECK(connector);

  ControlMessageUserData user_data(reinterpret_cast<void*>(connector));
  bool ok;
  {
    base::AutoLock locker(runner_lock_);
    DCHECK(message_queue_.get());

    // Post a detach control message to runner and wait until it's handled
    scoped_ptr<proto::Message> ctrl_msg(new proto::Message());
    DCHECK(ctrl_msg.get());
    ctrl_msg->set_type(MSG_DETACH_HUBHOST);

    if (message_queue_->InCurrentThread()) {
      HandleMessage(ctrl_msg.release(), reinterpret_cast<void*>(&user_data));
    } else {
      ok = message_queue_->Post(
        ctrl_msg.release(), reinterpret_cast<void*>(&user_data));
      DCHECK(ok);
    }
    DCHECK(ok);
  }

  ok = user_data.message_handled_event.Wait();
  DCHECK(ok);
}

bool HubHost::Dispatch(Hub::Connector* connector, proto::Message* message) {
  DCHECK(connector && message);
  scoped_ptr<proto::Message> mptr(message);

  if (mptr->type() >= MSG_SYSTEM_RESERVED_START &&
      mptr->type() <= MSG_SYSTEM_RESERVED_END)
    return false;

  base::AutoLock locker(runner_lock_);
  DCHECK(message_queue_.get());
  return message_queue_->Post(mptr.release(),
                              reinterpret_cast<void*>(connector));
}

void HubHost::Run() {
  base::AutoLock locker(runner_lock_);

  // Return if already running
  if (message_queue_runner_.get())
    return;

  message_queue_runner_.reset(new ThreadMessageQueueRunner(this));
  DCHECK(message_queue_runner_.get());
  message_queue_runner_->Run();
}

void HubHost::Quit() {
  base::AutoLock locker(runner_lock_);

  if (message_queue_runner_.get()) {
    message_queue_runner_->Quit();
    message_queue_runner_.reset();
  }
}

MessageQueue* HubHost::CreateMessageQueue() {
  DCHECK(!message_queue_.get());
#if defined(OS_WIN)
  message_queue_.reset(MessageQueue::Create(this));
#else
  message_queue_.reset(new SimpleMessageQueue(this));
#endif
  return message_queue_.get();
}

void HubHost::DestroyMessageQueue(MessageQueue* queue) {
  DCHECK(queue);
  DCHECK_EQ(queue, message_queue_.get());
  message_queue_.reset();
}

void HubHost::RunnerThreadStarted() {
  hub_impl_.reset(new hub::HubImpl());
  DCHECK(hub_impl_.get());
}

void HubHost::RunnerThreadTerminated() {
  hub_impl_.reset();
}

void HubHost::HandleMessage(proto::Message* message, void* user_data) {
  DCHECK(message && user_data);
  scoped_ptr<proto::Message> mptr(message);

  switch (mptr->type()) {
    case MSG_ATTACH_HUBHOST: {
      // Attach to hub
      ControlMessageUserData* ctrl_data =
          reinterpret_cast<ControlMessageUserData*>(user_data);
      hub_impl_->Attach(
          reinterpret_cast<Connector*>(ctrl_data->private_data));
      ctrl_data->message_handled_event.Signal();
      break;
    }
    case MSG_DETACH_HUBHOST: {
      // Detach from hub
      ControlMessageUserData* ctrl_data =
          reinterpret_cast<ControlMessageUserData*>(user_data);
      hub_impl_->Detach(
          reinterpret_cast<Connector*>(ctrl_data->private_data));
      ctrl_data->message_handled_event.Signal();
      break;
    }
    default:
      // Hub IPC messages
      hub_impl_->Dispatch(
          reinterpret_cast<Hub::Connector*>(user_data), mptr.release());
      break;
  }
}

}  // namespace ipc
