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

#include "ipc/direct_message_channel.h"

#include "base/atomicops.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

////////////////////////////////////////////////////////////////////////////////
// Implementation of DirectMessageChannel::Impl
////////////////////////////////////////////////////////////////////////////////
class DirectMessageChannel::Impl : public Hub::Connector {
 public:
  Impl(DirectMessageChannel* owner, Hub* hub);
  virtual ~Impl();

  void SetListener(MessageChannel::Listener* listener);
  MessageChannel::Listener* GetListener() const;

  // Overridden from Hub::Connector:
  virtual bool Send(proto::Message* message) OVERRIDE;
  virtual void Attached() OVERRIDE;
  virtual void Detached() OVERRIDE;

  // Weak pointer to our owner.
  DirectMessageChannel* owner_;

  // Weak pointer to Hub instance.
  Hub* hub_;

  // Weak pointer to our listener instance.
  base::subtle::AtomicWord listener_;

  // Indicates if we have attached to hub.
  base::subtle::Atomic32 attached_;

  // Ensures that SetListener() can only be called from one thread.
  base::Lock set_listener_lock_;
};

DirectMessageChannel::Impl::Impl(DirectMessageChannel* owner, Hub* hub)
  : owner_(owner),
    hub_(hub),
    listener_(NULL),
    attached_(false) {
  DCHECK(hub_);
}

DirectMessageChannel::Impl::~Impl() {
  SetListener(NULL);
}

void DirectMessageChannel::Impl::SetListener(
    MessageChannel::Listener* listener) {
  base::AutoLock locker(set_listener_lock_);

  if (GetListener() == listener)
    return;

  MessageChannel::Listener* old_listener = GetListener();
  if (old_listener) {
    hub_->Detach(this);
    old_listener->OnDetachedFromMessageChannel(owner_);
  }

  base::subtle::Release_Store(
      &listener_, reinterpret_cast<base::subtle::AtomicWord>(listener));

  if (listener) {
    listener->OnAttachedToMessageChannel(owner_);
    hub_->Attach(this);
  }
}

MessageChannel::Listener* DirectMessageChannel::Impl::GetListener() const {
  return reinterpret_cast<MessageChannel::Listener*>(
      base::subtle::Acquire_Load(&listener_));
}

bool DirectMessageChannel::Impl::Send(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  MessageChannel::Listener* listener = GetListener();
  if (listener) {
    listener->OnMessageReceived(owner_, mptr.release());
    return true;
  }
  return false;
}

void DirectMessageChannel::Impl::Attached() {
  base::subtle::Release_Store(&attached_, true);
  MessageChannel::Listener* listener = GetListener();
  if (listener)
    listener->OnMessageChannelConnected(owner_);
}

void DirectMessageChannel::Impl::Detached() {
  base::subtle::Release_Store(&attached_, false);
  MessageChannel::Listener* listener = GetListener();
  if (listener)
    listener->OnMessageChannelClosed(owner_);
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of DirectMessageChannel
////////////////////////////////////////////////////////////////////////////////
DirectMessageChannel::DirectMessageChannel(Hub* hub)
  : ALLOW_THIS_IN_INITIALIZER_LIST(impl_(new Impl(this, hub))) {
}

DirectMessageChannel::~DirectMessageChannel() {
}

bool DirectMessageChannel::IsConnected() const {
  return base::subtle::Acquire_Load(&impl_->attached_);
}

bool DirectMessageChannel::Send(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  return IsConnected() && impl_->hub_->Dispatch(impl_.get(), mptr.release());
}

void DirectMessageChannel::SetListener(Listener* listener) {
  impl_->SetListener(listener);
}

}  // namespace ipc
