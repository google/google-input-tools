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

#include "ipc/multi_component_host.h"

#include <map>
#include <queue>

#include "base/atomic_ref_count.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/threading/thread_collision_warner.h"
#include "base/time.h"
#include "ipc/component.h"
#include "ipc/constants.h"
#include "ipc/message_queue.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/simple_message_queue.h"
#include "ipc/thread_message_queue_runner.h"

namespace {

// Message types used internally.
enum {
  MSG_IPC_CHANNEL_CONNECTED = ipc::MSG_SYSTEM_RESERVED_START,
  MSG_IPC_CHANNEL_CLOSED,
  MSG_IPC_HANDLE_PENDING_MESSAGE,
};

}  // namespace

namespace ipc {

////////////////////////////////////////////////////////////////////////////////
// MultiComponentHost::Host implementation.
////////////////////////////////////////////////////////////////////////////////
class MultiComponentHost::Host : public ThreadMessageQueueRunner::Delegate,
                                 public MessageQueue::Handler {
 public:
  Host(MultiComponentHost* owner, Component* component);
  virtual ~Host();

  // Initializes the object. If |create_thread| is true then a dedicate thread
  // will be created for running this component. |owner_->lock_| must have been
  // locked when calling this method.
  bool InitUnlocked(bool create_thread);

  // Finalizes the object. The runner thread, if available, will be terminated.
  // All resource will be released, but the host object itself will not be
  // deleted. Returns true when success. |owner_->lock_| must have been locked
  // when calling this method.
  bool FinalizeUnlocked();

  // Posts a message to the component's message queue. |data| == NULL means
  // it's an external message.
  void PostMessage(proto::Message* message, void* data);

  // Convenient method to post an IPC message to the host.
  void PostIPCMessage(uint32 type);

  // Wait a reply message. |owner_->lock_| must have been locked when calling
  // this method.
  bool WaitReplyUnlocked(uint32 type,
                         uint32 serial,
                         int timeout,
                         proto::Message** reply);

  uint32 id() const { return info_.id(); }
  void set_id(uint32 id) {
    info_.set_id(id);
  }
  const std::string& string_id() const { return info_.string_id(); }
  Component* component() const { return component_; }

  void PauseMessageHandling();
  void ResumeMessageHandling();
  bool IsMessageHandlingPaused();

 private:
  // Overridden from ThreadMessageQueueRunner::Delegate:
  virtual MessageQueue* CreateMessageQueue() OVERRIDE;
  virtual void DestroyMessageQueue(MessageQueue* queue) OVERRIDE;
  virtual void RunnerThreadStarted() OVERRIDE;
  virtual void RunnerThreadTerminated() OVERRIDE;

  // Overridden from MessageQueue::Handler:
  virtual void HandleMessage(proto::Message* message, void* data) OVERRIDE;

  // Handler of MSG_IPC_CHANNEL_CONNECTED
  void OnMsgIPCChannelConnected();

  // Handler of MSG_IPC_CHANNEL_CLOSED
  void OnMsgIPCChannelClosed();

  // Handler of MSG_REGISTER_COMPONENT's reply message.
  void OnMsgRegisterComponentReply(proto::Message* message);

  // Returns true if message is a reply message and should be saved in
  // |reply_stack_| other than be saved in |pending_messages_|.
  bool HandleReplyMessage(proto::Message* message);

  // Handles a message from message queue.
  // the message could be saved in reply_stack_, pending_messages_ or handled by
  // component according to the message type and whether the message handling is
  // paused and whether there are pending messages.
  void ComponentHandle(proto::Message* message);

  // Handles one pending message in |pending_messages_| queue, and post another
  // MSG_IPC_HANDLE_PENDING_MESSAGE if there are still some pending messages.
  void HandleOnePendingMessage();

  // Perform additional initialize tasks on runner thread.
  void PostInit();

  // Finalizes the host by optionally deregistering the component and removing
  // it from various maps.
  void PostFinalize();

  // Increases |wait_reply_level_| by one.
  void EnterWaitReply();

  // Decreases |wait_reply_level_| by one.
  void LeaveWaitReply();

  // Checks if we are currently inside a WaitReply() call.
  bool InsideWaitReply();

  // Deletes all messages in |pending_messages_|.
  void ClearPendingMessages();

  MultiComponentHost* owner_;

  Component* component_;

  scoped_ptr<MessageQueue> message_queue_;

  // It must be declared after |message_queue_| to make sure to be destroyed
  // first.
  scoped_ptr<ThreadMessageQueueRunner> runner_;

  proto::ComponentInfo info_;

  // A container to hold all received reply messages. Key is serial number.
  typedef std::map<uint32, proto::Message*> ReplyStack;
  ReplyStack reply_stack_;

  // The last serial pushed to |reply_stack_|, used when message handling is
  // paused.
  uint32 reply_stack_head_serial_;

  // A queue for keeping all incoming messages when message handling is paused.
  std::queue<proto::Message*> pending_messages_;

  // Level of recursive calls to WaitReply();
  base::AtomicRefCount wait_reply_level_;

  // Indicates if message handing is paused.
  base::AtomicRefCount pause_count_;

  // Indicates if there is a pending MSG_REGISTER_COMPONENT request.
  bool register_request_pending_;

  // A ThreadCollisionWarner to ensure the component related code is executed
  // on same thread.
  DFAKE_MUTEX(component_section_);

  DISALLOW_COPY_AND_ASSIGN(Host);
};

MultiComponentHost::Host::Host(MultiComponentHost* owner, Component* component)
    : owner_(owner),
      component_(component),
      reply_stack_head_serial_(0),
      wait_reply_level_(0),
      pause_count_(0),
      register_request_pending_(false) {
  DCHECK(owner_);
  DCHECK(component_);
}

MultiComponentHost::Host::~Host() {
  DCHECK(!InsideWaitReply());
  DCHECK(pending_messages_.empty());
}

bool MultiComponentHost::Host::InitUnlocked(bool create_thread) {
  owner_->lock_.AssertAcquired();
  DCHECK(!owner_->component_to_host_map_.count(component_));

  bool success = false;
  if (create_thread) {
    runner_.reset(new ThreadMessageQueueRunner(this));
    // CreateMessageQueue() and PostInit() will be called by
    // RunnerThreadStarted() from the runner thread, and it's guaranteed to be
    // called before runner_->Run() returns.
    runner_->Run();
    DCHECK(message_queue_.get());
    success = runner_->IsRunning();
  } else {
    // We need to initialize |message_queue_| and call PostInit() here if
    // there is no runner thread.
    (void) CreateMessageQueue();
    PostInit();
    success = true;
  }

  if (!success || string_id().empty() ||
      owner_->string_id_to_host_map_.count(string_id())) {
    return false;
  }

  owner_->component_to_host_map_[component_] = this;
  owner_->string_id_to_host_map_[string_id()] = this;
  return true;
}

bool MultiComponentHost::Host::FinalizeUnlocked() {
  owner_->lock_.AssertAcquired();
  DCHECK(owner_->component_to_host_map_.count(component_) &&
         owner_->component_to_host_map_[component_] == this);

  // If there is no dedicated runner thread for the component, then it cannot be
  // removed within a recursived SendWithReply() call,
  if (!runner_.get() && InsideWaitReply())
    return false;

  const uint32 id = info_.id();
  const std::string string_id = info_.string_id();

  DCHECK(!string_id.empty());
  DCHECK(owner_->string_id_to_host_map_.count(string_id) &&
         owner_->string_id_to_host_map_[string_id] == this);

  owner_->string_id_to_host_map_.erase(string_id);
  owner_->component_to_host_map_.erase(component_);

  if (id != kComponentDefault) {
    DCHECK(owner_->id_to_host_map_.count(id) &&
           owner_->id_to_host_map_[id] == this);
    owner_->id_to_host_map_.erase(id);
    if (owner_->IsChannelConnectedUnlocked())
      owner_->SendMsgDeregisterComponentUnlocked(id);
  }

  {
    // |component_->Deregistered()| might be called in PostFinalize(), so we
    // need to unlock |owner_->lock_| to avoid deadlocking.
    base::AutoUnlock auto_unlock(owner_->lock_);
    if (runner_.get()) {
      // Recursived SendWithReply() calls running on the dedicated runner thread
      // should be terminated correctly.
      // PostFinalize() will be called from the runner thread just before it's
      // being terminated.
      runner_->Quit();
      DCHECK(!runner_->IsRunning());
      runner_.reset();
    } else {
      PostFinalize();
      message_queue_.reset();
    }

    DCHECK(reply_stack_.empty());
    info_.Clear();
  }

  return true;
}

void MultiComponentHost::Host::PostMessage(proto::Message* message,
                                           void* data) {
  // This method is not protected by a lock, and may be called from any thread,
  // it must be guaranteed to be called after the host has been initialized.
  DCHECK(message_queue_.get());
  // MessageQueue::Post() might return false if the Post() is called after the
  // Quit() has been called if a component is running in a dedicate thread.
  // So just ignore false returned in this case.
  message_queue_->Post(message, data);
}

void MultiComponentHost::Host::PostIPCMessage(uint32 type) {
  proto::Message* message = new proto::Message();
  message->set_type(type);
  message->set_reply_mode(proto::Message::NO_REPLY);
  PostMessage(message, owner_);
}

bool MultiComponentHost::Host::WaitReplyUnlocked(uint32 type,
                                                 uint32 serial,
                                                 int timeout,
                                                 proto::Message** reply) {
  // This method can only be called from the runner thread.
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);
  owner_->lock_.AssertAcquired();

  DCHECK(reply);
  DCHECK_NE(kComponentDefault, info_.id());
  DCHECK_NE(0, timeout);
  DCHECK_EQ(0, reply_stack_.count(serial));

  EnterWaitReply();
  *reply = NULL;
  reply_stack_[serial] = NULL;
  uint32 old_reply_stack_head_serial = reply_stack_head_serial_;
  reply_stack_head_serial_ = serial;
  {
    base::AutoUnlock auto_unlock(owner_->lock_);
    // |info_.id()| will be set to kComponentDefault when the message channel is
    // closed, i.e. when we receive a MSG_IPC_CHANNEL_CLOSED message.
    while (message_queue_->DoMessage(timeout > 0 ? &timeout : NULL) &&
           info_.id() != kComponentDefault) {
      proto::Message* msg = reply_stack_[serial];
      if (msg) {
        DCHECK_EQ(type, msg->type());
        *reply = msg;
        break;
      }
    }
  }
  reply_stack_.erase(serial);
  reply_stack_head_serial_ = old_reply_stack_head_serial;
  LeaveWaitReply();
  return *reply != NULL;
}

inline void MultiComponentHost::Host::PauseMessageHandling() {
  base::AtomicRefCountInc(&pause_count_);
}

inline void MultiComponentHost::Host::ResumeMessageHandling() {
  DCHECK(!base::AtomicRefCountIsZero(&pause_count_));

  // We shouldn't check pending_messages_.empty() here, as it's not thread safe.
  if (!base::AtomicRefCountDec(&pause_count_))
    PostIPCMessage(MSG_IPC_HANDLE_PENDING_MESSAGE);
}

inline bool MultiComponentHost::Host::IsMessageHandlingPaused() {
  return !base::AtomicRefCountIsZero(&pause_count_);
}

MessageQueue* MultiComponentHost::Host::CreateMessageQueue() {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);
#if defined(OS_WIN)
  message_queue_.reset(MessageQueue::Create(this));
#else
  message_queue_.reset(new SimpleMessageQueue(this));
#endif
  DCHECK(message_queue_.get());
  return message_queue_.get();
}

void MultiComponentHost::Host::DestroyMessageQueue(MessageQueue* queue) {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);
  DCHECK_EQ(message_queue_.get(), queue);
  message_queue_.reset();
}

void MultiComponentHost::Host::RunnerThreadStarted() {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);

  PostInit();
}

void MultiComponentHost::Host::RunnerThreadTerminated() {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);

  PostFinalize();
}

void MultiComponentHost::Host::HandleMessage(proto::Message* message,
                                             void* data) {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);
  DCHECK(message);

  // The reply message should be saved to |reply_stack_| immediately
  // if message handling is paused and serial is on the top of
  // |reply_stack_|, which means ComponentBase::SendWithReplyNonRecursive
  // is waiting for the reply message.
  // If it's not the case, a reply message should be saved in |reply_stack_| in
  // |ComponentHandle| or |HandleOnePendingMessage| by calling
  // |HandleReplyMessage|, or be delayed until all pending messages before it
  // are handled if any.
  const uint32 serial = message->serial();
  if (message->reply_mode() == proto::Message::IS_REPLY &&
      IsMessageHandlingPaused() &&
      serial == reply_stack_head_serial_ &&
      reply_stack_.count(serial) &&
      !reply_stack_[serial]) {
    reply_stack_[serial] = message;
    return;
  }

  scoped_ptr<proto::Message> mptr(message);
  switch (mptr->type()) {
    case MSG_IPC_CHANNEL_CONNECTED:
      DCHECK_EQ(owner_, data);
      OnMsgIPCChannelConnected();
      break;
    case MSG_IPC_CHANNEL_CLOSED:
      DCHECK_EQ(owner_, data);
      OnMsgIPCChannelClosed();
      break;
    case MSG_IPC_HANDLE_PENDING_MESSAGE:
      DCHECK_EQ(owner_, data);
      HandleOnePendingMessage();
      break;
    case MSG_REGISTER_COMPONENT:
      DCHECK(!data);
      DCHECK_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
      OnMsgRegisterComponentReply(mptr.release());
      break;
    default:
      DCHECK(!data);
      ComponentHandle(mptr.release());
      break;
  }
}

void MultiComponentHost::Host::OnMsgIPCChannelConnected() {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);

  // Multiple MSG_IPC_CHANNEL_CONNECTED might be posted to us at the same time,
  // but we should only handle one of them.
  if (register_request_pending_ || info_.id() != kComponentDefault)
    return;

  base::AutoLock auto_lock(owner_->lock_);

  // The message channel may have been disconnected again when we get this
  // message.
  if (!owner_->IsChannelConnectedUnlocked())
    return;

  proto::Message* message = new proto::Message();
  message->set_type(MSG_REGISTER_COMPONENT);
  message->set_reply_mode(proto::Message::NEED_REPLY);
  proto::MessagePayload* payload = message->mutable_payload();
  payload->add_component_info()->CopyFrom(info_);

  const bool success = owner_->SendInternalUnlocked(message, NULL);
  DCHECK(success);

  register_request_pending_ = true;
}

void MultiComponentHost::Host::OnMsgIPCChannelClosed() {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);

  // We are now ready to be registered again.
  register_request_pending_ = false;

  if (info_.id() == kComponentDefault)
    return;

  {
    base::AutoLock auto_lock(owner_->lock_);
    const uint32 id = info_.id();
    DCHECK(owner_->id_to_host_map_.count(id) &&
           owner_->id_to_host_map_[id] == this);
    owner_->id_to_host_map_.erase(id);
    info_.set_id(kComponentDefault);
  }

  // |component_->Deregistered()| must be called without locking
  // |owner_->lock_|, because the component may access the host in this method.
  component_->Deregistered();

  // It makes no sense to dispatch pending messages anymore.
  ClearPendingMessages();
}

void MultiComponentHost::Host::OnMsgRegisterComponentReply(
    proto::Message* message) {
  DFAKE_SCOPED_RECURSIVE_LOCK(component_section_);

  scoped_ptr<proto::Message> mptr(message);

  const proto::MessagePayload& payload = message->payload();
  DCHECK_EQ(1, payload.component_info_size());

  const proto::ComponentInfo& info = payload.component_info(0);
  DCHECK_EQ(info.string_id(), info_.string_id());

  const uint32 id = info.id();
  DCHECK_EQ(info.id(), info_.id());

  register_request_pending_ = false;

  // id will be kComponentDefault if this component cannot be registered to Hub,
  // because of e.g. its string_id conflicts with an existing component.
  component_->Registered(id);

  owner_->OnComponentRegistered(component_);
}

bool MultiComponentHost::Host::HandleReplyMessage(proto::Message* message) {
  const uint32 serial = message->serial();
  if (message->reply_mode() == proto::Message::IS_REPLY &&
      reply_stack_.count(serial)) {
    if (!reply_stack_[serial]) {
      reply_stack_[serial] = message;
      return true;
    } else {
      DLOG(ERROR) << "Multiple reply messages with same serial received.";
    }
  }
  return false;
}

void MultiComponentHost::Host::ComponentHandle(proto::Message* message) {
  if (!pending_messages_.empty() || IsMessageHandlingPaused()) {
    pending_messages_.push(message);
    return;
  }
  if (!HandleReplyMessage(message))
    component_->Handle(message);
}

void MultiComponentHost::Host::HandleOnePendingMessage() {
  if (IsMessageHandlingPaused() || pending_messages_.empty())
    return;

  proto::Message* message = pending_messages_.front();
  pending_messages_.pop();

  // We should post MSG_IPC_HANDLE_PENDING_MESSAGE before handling the message
  // otherwise if we use SendWithReply when handling the message, the reply
  // will be cached forever.
  if (!pending_messages_.empty())
    PostIPCMessage(MSG_IPC_HANDLE_PENDING_MESSAGE);

  if (!HandleReplyMessage(message))
    component_->Handle(message);
}

void MultiComponentHost::Host::PostInit() {
  component_->GetInfo(&info_);
  info_.set_id(kComponentDefault);
}

void MultiComponentHost::Host::PostFinalize() {
  if (info_.id() != kComponentDefault)
    component_->Deregistered();
  ClearPendingMessages();
}

inline void MultiComponentHost::Host::EnterWaitReply() {
  base::AtomicRefCountInc(&wait_reply_level_);
}

inline void MultiComponentHost::Host::LeaveWaitReply() {
  DCHECK(!base::AtomicRefCountIsZero(&wait_reply_level_));
  base::AtomicRefCountDec(&wait_reply_level_);
}

inline bool MultiComponentHost::Host::InsideWaitReply() {
  return !base::AtomicRefCountIsZero(&wait_reply_level_);
}

void MultiComponentHost::Host::ClearPendingMessages() {
  while (!pending_messages_.empty()) {
    delete pending_messages_.front();
    pending_messages_.pop();
  }
}

////////////////////////////////////////////////////////////////////////////////
// MultiComponentHost implementation.
////////////////////////////////////////////////////////////////////////////////

MultiComponentHost::MultiComponentHost(bool create_thread)
    : create_thread_(create_thread),
      channel_(NULL),
      serial_count_(0),
      components_ready_(false, false) {
}

MultiComponentHost::~MultiComponentHost() {
  RemoveAllComponents();
  // All components must have been removed before destroying the component host.
  DCHECK(component_to_host_map_.empty());
  if (channel_)
    channel_->SetListener(NULL);
}

void MultiComponentHost::SetMessageChannel(MessageChannel* channel) {
  DCHECK(channel);
  channel->SetListener(this);
}

bool MultiComponentHost::AddComponent(Component* component) {
  DCHECK(component);
  base::AutoLock auto_lock(lock_);
  if (GetHostByComponentUnlocked(component)) {
    DLOG(ERROR) << "Try to add a component that has already been added.";
    return false;
  }

  Host* host = new Host(this, component);
  if (!host->InitUnlocked(create_thread_)) {
    DLOG(ERROR) << "Failed to initialize the component.";
    delete host;
    return false;
  }

  component->DidAddToHost(this);
  wait_components_.insert(component);

  // Registers the component asynchronously.
  if (IsChannelConnectedUnlocked())
    host->PostIPCMessage(MSG_IPC_CHANNEL_CONNECTED);

  return true;
}

bool MultiComponentHost::RemoveComponent(Component* component) {
  DCHECK(component);
  base::AutoLock auto_lock(lock_);
  Host* host = GetHostByComponentUnlocked(component);
  if (!host) {
    DLOG(ERROR) << "Try to remove a nonexistent component.";
    return false;
  }

  if (!RemoveHostUnlocked(host)) {
    DLOG(ERROR) << "Failed to finalize the component.";
    return false;
  }
  wait_components_.erase(component);
  return true;
}

bool MultiComponentHost::Send(Component* component,
                              proto::Message* message,
                              uint32* serial) {
  DCHECK(component);
  DCHECK(message);

  scoped_ptr<proto::Message> mptr(message);
  if (IsInternalMessage(message->type()))
    return false;

  base::AutoLock auto_lock(lock_);
  Host* host = GetHostByComponentUnlocked(component);

  // the component must have been registered before sending any message.
  if (!host || host->id() == kComponentDefault)
    return false;

  message->set_source(host->id());
  return SendInternalUnlocked(mptr.release(), serial);
}

bool MultiComponentHost::SendWithReply(Component* component,
                                       proto::Message* message,
                                       int timeout,
                                       proto::Message** reply) {
  DCHECK(component);
  DCHECK(message);
  if (reply)
    *reply = NULL;

  scoped_ptr<proto::Message> mptr(message);
  const uint32 type = message->type();
  if (IsInternalMessage(type))
    return false;

  base::AutoLock auto_lock(lock_);
  Host* host = GetHostByComponentUnlocked(component);

  // the component must have been registered before sending any message.
  if (!host || host->id() == kComponentDefault)
    return false;

  message->set_source(host->id());
  const bool need_reply = (message->reply_mode() == proto::Message::NEED_REPLY);
  uint32 serial = 0;
  if (!SendInternalUnlocked(mptr.release(), &serial))
    return false;

  // Just returns ture if no reply message is needed.
  if (!need_reply || !reply)
    return true;

  if (timeout == 0)
    return false;

  return host->WaitReplyUnlocked(type, serial, timeout, reply);
}

void MultiComponentHost::PauseMessageHandling(Component* component) {
  base::AutoLock auto_lock(lock_);
  Host* host = GetHostByComponentUnlocked(component);
  if (!host)
    return;
  host->PauseMessageHandling();
}

void MultiComponentHost::ResumeMessageHandling(Component* component) {
  base::AutoLock auto_lock(lock_);
  Host* host = GetHostByComponentUnlocked(component);
  if (!host)
    return;
  host->ResumeMessageHandling();
}

void MultiComponentHost::OnMessageReceived(MessageChannel* channel,
                                           proto::Message* message) {
  base::AutoLock auto_lock(lock_);
  scoped_ptr<proto::Message> mptr(message);

  DCHECK_EQ(channel_, channel);
  DCHECK(message);

  Host* host = NULL;

  // We need to handle reply message of MSG_REGISTER_COMPONENT specially,
  // because at this point, the component is not actually registered, and we
  // need to find it out by the string id.
  if (message->type() == MSG_REGISTER_COMPONENT) {
    DCHECK_EQ(proto::Message::IS_REPLY, message->reply_mode());
    DCHECK_EQ(1, message->payload().component_info_size());
    const proto::ComponentInfo& info = message->payload().component_info(0);
    const std::string& strid = info.string_id();
    const uint32 id = info.id();
    StringIdToHostMap::const_iterator i = string_id_to_host_map_.find(strid);

    // The component may have been removed when we receive the reply message.
    // In such case, we need to let Hub know it.
    if (i == string_id_to_host_map_.end()) {
      if (id != kComponentDefault)
        SendMsgDeregisterComponentUnlocked(id);
    } else {
      host = i->second;

      // We need to add this component to |id_to_host_map_| immediately, in case
      // more messages will be sent to this component before this reply message
      // gets handled.
      if (id != kComponentDefault) {
        DCHECK(!id_to_host_map_.count(id));
        id_to_host_map_[id] = host;
        DCHECK_EQ(kComponentDefault, host->id());
        // Set the component host id just after adding it to id_to_host_map_, to
        // make sure that it will be remove correctly when remove component is
        // called before the register component message is processed.
        host->set_id(id);
      }
    }
  } else {
    IdToHostMap::const_iterator i = id_to_host_map_.find(message->target());
    if (i != id_to_host_map_.end())
      host = i->second;
  }

  if (host) {
    host->PostMessage(mptr.release(), NULL);
    return;
  }

#if !defined(NDEBUG)
  std::string text;
  PrintMessageToString(*message, &text, true);
  DLOG(ERROR) << "Failed to find target component for message:" << text;
#endif
}

void MultiComponentHost::OnMessageChannelConnected(MessageChannel* channel) {
  base::AutoLock auto_lock(lock_);
  DCHECK(channel_);
  DCHECK(channel_->IsConnected());
  OnChannelConnectedUnlocked();
}

void MultiComponentHost::OnMessageChannelClosed(MessageChannel* channel) {
  base::AutoLock auto_lock(lock_);
  DCHECK(channel_);
  DCHECK(!channel_->IsConnected());
  OnChannelClosedUnlocked();
}

void MultiComponentHost::OnAttachedToMessageChannel(MessageChannel* channel) {
  base::AutoLock auto_lock(lock_);
  DCHECK_NE(channel_, channel);

  MessageChannel* old_channel = channel_;
  if (old_channel) {
    base::AutoUnlock auto_unlock(lock_);
    // OnDetachedFromMessageChannel() will be called when removing listener
    // from old channel.
    old_channel->SetListener(NULL);
  }

  DCHECK(!channel_);
  channel_ = channel;
  if (channel_->IsConnected())
    OnChannelConnectedUnlocked();
}

void MultiComponentHost::OnDetachedFromMessageChannel(MessageChannel* channel) {
  base::AutoLock auto_lock(lock_);
  DCHECK_EQ(channel_, channel);

  channel_ = NULL;
  if (channel->IsConnected())
    OnChannelClosedUnlocked();
}

bool MultiComponentHost::SendInternalUnlocked(proto::Message* message,
                                              uint32* serial) {
  lock_.AssertAcquired();

  scoped_ptr<proto::Message> mptr(message);
  if (!IsChannelConnectedUnlocked())
    return false;

  if (message->reply_mode() != proto::Message::IS_REPLY)
    message->set_serial(++serial_count_);

  if (serial)
    *serial = message->serial();

  return channel_->Send(mptr.release());
}

bool MultiComponentHost::SendMsgDeregisterComponentUnlocked(uint32 id) {
  if (!IsChannelConnectedUnlocked())
    return false;

  proto::Message* message = new proto::Message();
  message->set_type(MSG_DEREGISTER_COMPONENT);
  message->set_reply_mode(proto::Message::NO_REPLY);
  message->mutable_payload()->add_uint32(id);
  return SendInternalUnlocked(message, NULL);
}

bool MultiComponentHost::IsChannelConnectedUnlocked() const {
  return channel_ && channel_->IsConnected();
}

void MultiComponentHost::OnChannelConnectedUnlocked() {
  for (ComponentToHostMap::const_iterator i = component_to_host_map_.begin();
       i != component_to_host_map_.end(); ++i) {
    i->second->PostIPCMessage(MSG_IPC_CHANNEL_CONNECTED);
  }
}

void MultiComponentHost::OnChannelClosedUnlocked() {
  for (IdToHostMap::const_iterator i = id_to_host_map_.begin();
       i != id_to_host_map_.end(); ++i) {
    i->second->PostIPCMessage(MSG_IPC_CHANNEL_CLOSED);
  }
}

MultiComponentHost::Host* MultiComponentHost::GetHostByComponentUnlocked(
    Component* component) {
  ComponentToHostMap::const_iterator i = component_to_host_map_.find(component);
  return (i != component_to_host_map_.end()) ? i->second : NULL;
}

bool MultiComponentHost::RemoveHostUnlocked(Host* host) {
  Component* component = host->component();
  DCHECK(component);
  if (!host->FinalizeUnlocked())
    return false;
  component->DidRemoveFromHost();
  delete host;
  return true;
}

void MultiComponentHost::RemoveAllComponents() {
  base::AutoLock auto_lock(lock_);
  while (!component_to_host_map_.empty()) {
    if (!RemoveHostUnlocked(component_to_host_map_.begin()->second))
      NOTREACHED();
  }
}

// static
bool MultiComponentHost::IsInternalMessage(uint32 type) {
  switch (type) {
    case MSG_REGISTER_COMPONENT:
    case MSG_DEREGISTER_COMPONENT:
      return true;
    default:
      return type >= MSG_SYSTEM_RESERVED_START &&
             type <= MSG_SYSTEM_RESERVED_END;
  }
}

void MultiComponentHost::OnComponentRegistered(const Component* component) {
  base::AutoLock lock(lock_);
  std::set<const Component*>::iterator it = wait_components_.find(component);
  if (it == wait_components_.end())
    return;
  wait_components_.erase(it);
  if (wait_components_.empty())
    components_ready_.Signal();
}

bool MultiComponentHost::WaitForComponents(int* timeout) {
  if (!IsChannelConnectedUnlocked())
    return false;
  base::Time start = base::Time::Now();
  bool success = false;
  if (timeout) {
    success = components_ready_.TimedWait(
        base::TimeDelta::FromMilliseconds(*timeout));
    *timeout -= (base::Time::Now() - start).InMilliseconds();
  } else {
    success = components_ready_.Wait();
  }
  return success;
}

void MultiComponentHost::QuitWaitingComponents() {
  components_ready_.Signal();
}

}  // namespace ipc
