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

#ifndef GOOPY_IPC_MULTI_COMPONENT_HOST_H_
#define GOOPY_IPC_MULTI_COMPONENT_HOST_H_
#pragma once

#include <map>
#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "ipc/component_host.h"
#include "ipc/message_channel.h"

namespace ipc {
class MessageQueue;

// A ComponentHost implementation to host multiple components.
class MultiComponentHost : public ComponentHost,
                           public MessageChannel::Listener {
 public:
  // If |create_thread| is true then each component will be run on its
  // dedicated thread, otherwise the component will be run on the thread where
  // it's added.
  explicit MultiComponentHost(bool create_thread);

  virtual ~MultiComponentHost();

  // Sets the message channel used for sending/receiving messages to/from Hub.
  void SetMessageChannel(MessageChannel* channel);

  // |create_thread_| is a readonly property.
  bool create_thread() const { return create_thread_; }

  // Overridden from ComponentHost:
  virtual bool AddComponent(Component* component) OVERRIDE;
  virtual bool RemoveComponent(Component* component) OVERRIDE;
  virtual bool Send(Component* component,
                    proto::Message* message,
                    uint32* serial) OVERRIDE;
  virtual bool SendWithReply(Component* component,
                             proto::Message* message,
                             int timeout,
                             proto::Message** reply) OVERRIDE;
  virtual void PauseMessageHandling(Component* component) OVERRIDE;
  virtual void ResumeMessageHandling(Component* component) OVERRIDE;
  virtual bool WaitForComponents(int* timeout);
  virtual void QuitWaitingComponents();

 private:
  // A class to host one component.
  class Host;

  typedef std::map<uint32, Host*> IdToHostMap;
  typedef std::map<Component*, Host*> ComponentToHostMap;
  typedef std::map<std::string, Host*> StringIdToHostMap;

  // Overridden from MessageChannel::Listener:
  virtual void OnMessageReceived(MessageChannel* channel,
                                 proto::Message* message) OVERRIDE;
  virtual void OnMessageChannelConnected(MessageChannel* channel) OVERRIDE;
  virtual void OnMessageChannelClosed(MessageChannel* channel) OVERRIDE;
  virtual void OnAttachedToMessageChannel(MessageChannel* channel) OVERRIDE;
  virtual void OnDetachedFromMessageChannel(MessageChannel* channel) OVERRIDE;

  // Internal implementation of Send() method. |lock_| should be locked when
  // calling this method.
  bool SendInternalUnlocked(proto::Message* message, uint32* serial);

  // Convenient method to send MSG_DEREGISTER_COMPONENT message. |lock_| should
  // be locked when calling this method.
  bool SendMsgDeregisterComponentUnlocked(uint32 id);

  // Convenient method to check if |channel_| is connected. |lock_| should be
  // locked when calling this method.
  bool IsChannelConnectedUnlocked() const;

  // Called when |channel_| is connected. |lock_| should be locked when calling
  // this method.
  void OnChannelConnectedUnlocked();

  // Called when |channel_| is closed or detached. |lock_| should be locked when
  // calling this method.
  void OnChannelClosedUnlocked();

  // Gets host of a given component.
  Host* GetHostByComponentUnlocked(Component* component);

  // Removes and destroys the given |host| object. |lock_| should be locked when
  // calling this method. Return false if the host cannot be removed and
  // destroyed because of nested call to SendWithReply() method.
  bool RemoveHostUnlocked(Host* host);

  void RemoveAllComponents();

  // Checks if the given message type is used by us internally.
  static bool IsInternalMessage(uint32 type);

  // Called when a component is registered to Hub.
  void OnComponentRegistered(const Component* component);

  // Indicates if we should run components on their dedicated threads.
  bool create_thread_;

  // MessageChannel for sending/receiving messages to/from Hub.
  MessageChannel* channel_;

  // Map from component ids to corresponding Host objects. It only contains
  // Host objects of registered components, and it does not own them.
  IdToHostMap id_to_host_map_;

  // Map from component objects to corresponding Host objects. It owns the Host
  // objects.
  ComponentToHostMap component_to_host_map_;

  // Map from component string ids to corresponding Host objects. It doesn not
  // own the Host objects.
  StringIdToHostMap string_id_to_host_map_;

  // A counter to generate unique serial number.
  uint32 serial_count_;

  mutable base::Lock lock_;

  // A set of components that need to be wait for registered.
  std::set<const Component*> wait_components_;

  base::WaitableEvent components_ready_;

  DISALLOW_COPY_AND_ASSIGN(MultiComponentHost);
};

}  // namespace ipc

#endif  // GOOPY_IPC_MULTI_COMPONENT_HOST_H_
