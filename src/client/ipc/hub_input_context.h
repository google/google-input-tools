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

#ifndef GOOPY_IPC_HUB_INPUT_CONTEXT_H_
#define GOOPY_IPC_HUB_INPUT_CONTEXT_H_
#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

class Component;
class HotkeyList;

// A class to manage all information and resource related to an input context in
// the Hub, including:
// 1. The owner component of the input context.
// 2. All components attached to the input context.
//    The owner is always attached to the input context and never gets detached.
// 3. Active consumers of all messages that may be produced by attached
//    components.
// 4. Active hotkey Mapping.
class InputContext {
 public:
  typedef std::vector<uint32> MessageTypeVector;

  // Each component attached to an input context can have one of following
  // states.
  enum AttachState {
    // The component is not attached to the input context yet.
    NOT_ATTACHED = 0,

    // The component is going to be attached passively pending confirmation.
    // The _PASSIVE suffix is just an informal hint, which has no restrict to
    // the final attach state when the component is actually attached.
    PENDING_PASSIVE,

    // The component is going to be attached actively pending confirmation.
    // The _ACTIVE suffix is just an informal hint, which has no restrict to
    // the final attach state when the component is actually attached.
    PENDING_ACTIVE,

    // The component is attached to the input context passively, which will only
    // be set as active consumer for messages without active consumer yet.
    PASSIVE,

    // The component is attached to the input context actively, which will be
    // set as active consumer for all messages it can consume, but can be
    // replaced by other components later.
    ACTIVE,

    // The component is attached to the input context actively, which will be
    // set as active consumer for all messages it can consume and will never be
    // replaced by other components, unless the active consumer is resigned
    // explicitly or the component is detached from the input context.
    ACTIVE_STICKY,
  };

  // An interface class that the Hub implementation should implement.
  class Delegate {
   public:
    // Called when a component becomes active consumer for one or more messages.
    virtual void OnComponentActivated(InputContext* input_context,
                                      Component* component,
                                      const MessageTypeVector& messages) = 0;

    // Called when a component loses active consumer role for one or more
    // messages.
    virtual void OnComponentDeactivated(InputContext* input_context,
                                        Component* component,
                                        const MessageTypeVector& messages) = 0;

    // Called when a component is detached from the input context.
    // The |state| is the component' attach state before detaching.
    virtual void OnComponentDetached(InputContext* input_context,
                                     Component* component,
                                     AttachState state) = 0;

    // Called when active consumer of one or more message types are changed.
    virtual void OnActiveConsumerChanged(InputContext* input_context,
                                         const MessageTypeVector& messages) = 0;

    // Called when a component is no longer the active consumer of any message
    // types of an input context. the Hub may detach the component from the
    // input context.
    virtual void MaybeDetachComponent(InputContext* input_context,
                                      Component* component) = 0;

    // Called when some message types, which may be produced by attached
    // components and need consumers, don't have active consumers yet.
    virtual void RequestConsumer(InputContext* input_context,
                                 const MessageTypeVector& messages,
                                 Component* exclude) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // |owner| and |delegate| must not be NULL.
  InputContext(uint32 id, Component* owner, Delegate* delegate);
  ~InputContext();

  // Stores information of the input context into the |info| object.
  void GetInfo(proto::InputContextInfo* info) const;

  uint32 id() const { return id_; }

  Component* owner() const { return owner_; }

  Delegate* delegate() const { return delegate_; }

  // Attaches a component to the input context with a specified state.
  // This method can be called multiple times for a component to change its
  // attach state. The rules are:
  // 1. Changing the state to NOT_ATTACHED will detach the component, which is
  //    same as calling DetachComponent() method.
  // 2. Attaching a component with PENDING_PASSIVE or PENDING_ACTIVE state will
  //    just add it to the pending list without activating it.
  //    Changing the state to a pending state for an attached component is not
  //    allowed.
  // 3. Attaching a component with PASSIVE state or changing the state from
  //    a pending state to PASSIVE will set the component as active consumer for
  //    all messages which can be consumed by it but still have no active
  //    consumer.
  //    Changing from ACTIVE_STICKY to PASSIVE will not affect active consumer
  //    for any message, but will allow other components to take over active
  //    consumer roles owned by this component in the future.
  //    Changing from ACTIVE to PASSIVE has no effect.
  // 4. Attaching a component with ACTIVE state or changing the state from a
  //    pending state or PASSIVE to ACTIVE will set the component as active
  //    consumer for all messages which can be consumed by it, except if the
  //    current active consumer's state is ACTIVE_STICKY.
  //    Changing the state from ACTIVE_STICKY to ACTIVE will not affect active
  //    consumer for any message, but will allow other components to take over
  //    active consumer roles owned by this component in the future.
  // 5. Attaching a component with ACTIVE_STICKY state or changing the state
  //    from a pending state or PASSIVE to ACTIVE_STICKY will set the component
  //    as active consumer for all messages which can be consumed by it, except
  //    if the current active consumer's state is ACTIVE_STICKY. Other
  //    components will not be allowed to take over active consumer roles owned
  //    by this component.
  //    Changing the state from ACTIVE to ACTIVE_STICKY will not affect active
  //    consumer for any message, but will prevent other components from taking
  //    over active consumer roles owned by this component in the future.
  // 6. PENDING_ACTIVE and PENDING_PASSIVE have no difference for this method.
  //    The state can change from any of them to any of PASSIVE, ACTIVE and
  //    ACTIVE_STICKY.
  //
  // If |persistent| is true then |delegate_|'s MaybeDetachComponent() method
  // will never be called for this component. It has no effect for owner.
  //
  // Returns true if the component is successfully attached.
  bool AttachComponent(Component* component,
                       AttachState state,
                       bool persistent);

  // Detaches a comopnent from the input context.
  // Returns true if the component is successfully detached.
  bool DetachComponent(Component* component);

  // Gets the attach state of a component.
  AttachState GetComponentAttachState(Component* component) const {
    ComponentMap::const_iterator i = attached_components_.find(component);
    return i != attached_components_.end() ? i->second.state : NOT_ATTACHED;
  }

  // Checks if a component is really attached.
  bool IsComponentReallyAttached(Component* component) const {
    return IsAttachedState(GetComponentAttachState(component));
  }

  // Checks if a component is in pending state, either PENDING_ACTIVE or
  // PENDING_PASSIVE.
  bool IsComponentPending(Component* component) const {
    return IsPendingState(GetComponentAttachState(component));
  }

  // Checks if a component is in PENDING_ACTIVE state.
  bool IsComponentPendingActive(Component* component) const {
    return GetComponentAttachState(component) == PENDING_ACTIVE;
  }

  // Checks if a component is in PENDING_PASSIVE state.
  bool IsComponentPendingPassive(Component* component) const {
    return GetComponentAttachState(component) == PENDING_PASSIVE;
  }

  bool IsComponentPersistent(Component* component) const {
    ComponentMap::const_iterator i = attached_components_.find(component);
    return i != attached_components_.end() ? i->second.persistent: false;
  }

  void SetComponentPersistent(Component* component, bool persistent) {
    ComponentMap::iterator i = attached_components_.find(component);
    if (i != attached_components_.end() && i->first != owner_)
      i->second.persistent = persistent;
  }

  // Checks if an attached component is assigned as the active consumer for at
  // least one message type.
  bool IsComponentActive(Component* component) const;

  // Checks if an attached component is redundant. A component is redundant if
  // it's not active consumer for any message that others may produce and need
  // consumer.
  bool IsComponentRedundant(Component* component) const;

  // Finds out all redundant components and calls delegate's
  // MaybeDetachComponent method for them.
  void MaybeDetachRedundantComponents();

  // Checks if any attached component may produce the specified message type.
  // If |include_pending| is true then components with PENDING attach state will
  // be checked as well.
  bool MayProduce(uint32 message_type, bool include_pending) const;

  // Checks if any attached component may consume the specified message type.
  // If |include_pending| is true then components with PENDING attach state will
  // be checked as well.
  bool MayConsume(uint32 message_type, bool include_pending) const;

  // Checks if there is an active consumer for a specified message type.
  bool HasActiveConsumer(uint32 message_type) const {
    return GetActiveConsumer(message_type) != NULL;
  }

  // Gets the active consumer for a specified message type.
  Component* GetActiveConsumer(uint32 message_type) const {
    ConsumerMap::const_iterator i = active_consumers_.find(message_type);
    return i != active_consumers_.end() ? i->second : NULL;
  }

  // Assigns a component as the active consumer for one or more message types.
  // The component must be already attached to the input context, and the attach
  // state must be one of PASSIVE, ACTIVE and ACTIVE_STICKY.
  // This method will fail if the current active consumer's attach state is
  // ACTIVE_STICKY.
  // Returns true when success.
  bool AssignActiveConsumer(Component* component,
                            const uint32* messages,
                            size_t size);

  // Resigns a component as the active consumer for one or more message types.
  // The component must be already attached to the input context, and the attach
  // state must be one of PASSIVE, ACTIVE and ACTIVE_STICKY.
  // Another attached component, which can consume the message type, may be
  // assigned as the active consumer automatically.
  // This method also prevents the component from consuming the message types in
  // the future. AssignActiveConsumer() must be called in order to revert this
  // effect.
  // Returns true when success.
  bool ResignActiveConsumer(Component* component,
                            const uint32* messages,
                            size_t size);

  // Sets messages which may be produced by a component and required to have
  // consumers. It's a per component setting, and will only be valid when the
  // component is attached to the input context. If |already_have_consumer| is
  // not NULL, then it'll be filled with all message types that already have
  // consumers, thus no need to send out requests for them anymore.
  void SetMessagesNeedConsumer(Component* component,
                               const uint32* messages,
                               size_t size,
                               std::vector<uint32>* already_have_consumers);

  // Gets all attached consumers for a specified message type. The active
  // consumer will always be the first one.
  // If |include_pending| is true then components with PENDING attach state will
  // be checked as well.
  size_t GetAllConsumers(uint32 message_type,
                         bool include_pending,
                         std::vector<Component*>* consumers) const;

  // Gets IDs of all attached consumers for a specified message type. The active
  // consumer will always be the first one.
  // If |include_pending| is true then components with PENDING attach state will
  // be checked as well.
  size_t GetAllConsumersID(uint32 message_type,
                           bool include_pending,
                           std::vector<uint32>* ids) const;

  // Gets messages which may be produced by attached components and required to
  // have consumers but no attached consumer yet.
  // If |include_pending| is true then components with pending states will be
  // checked as well.
  size_t GetAllMessagesNeedConsumer(bool include_pending,
                                    MessageTypeVector* messages) const;

  // Gets the active hotkey list of an attached component.
  const HotkeyList* GetComponentActiveHotkeyList(Component* component) const;

  // Sets the active hotkey list of an attached component.
  void SetComponentActiveHotkeyList(Component* component, uint32 id);

  // Unsets the active hotkey list of an attached component.
  void UnsetComponentActiveHotkeyList(Component* component);

  // Called when a hotkey list of an attached component is updated.
  void ComponentHotkeyListUpdated(Component* component, uint32 id);

  // Called when a hotkey list of an attached component is removed.
  void ComponentHotkeyListRemoved(Component* component, uint32 id);

  const std::vector<const HotkeyList*>& GetAllActiveHotkeyLists() {
    if (!active_hotkey_lists_valid_)
      InitializeActiveHotkeyLists();
    return active_hotkey_lists_;
  }

  static bool IsPendingState(AttachState state) {
    return state == PENDING_PASSIVE || state == PENDING_ACTIVE;
  }

  static bool IsAttachedState(AttachState state) {
    return state == PASSIVE || state == ACTIVE || state == ACTIVE_STICKY;
  }

  // TODO(suzhe): Add:
  // 1. Hotkey support
  // 2. Support for different input types, such as text, password, phone, etc.
  // 3. Store input caret information.

 private:
  typedef std::set<uint32> MessageTypeSet;

  struct ComponentState {
    ComponentState()
        : state(NOT_ATTACHED),
          persistent(false),
          hotkey_list_id(0),
          hotkey_list_set(false) {
    }

    // Attach state of the component.
    AttachState state;

    // If it's true then |delegate_|'s MaybeDetachComponent() method will never
    // be called for this component.
    bool persistent;

    // Messages that the component doesn't want to consume.
    MessageTypeSet resigned_consumer;

    // Messages that the component may produce and need others to consume.
    MessageTypeSet need_consumer;

    // ID of the component's hotkey list, which is activated for this input
    // context.
    uint32 hotkey_list_id;

    // Indicates if the value of |hotkey_list_id| is valid or not.
    bool hotkey_list_set;
  };

  typedef std::map<Component*, ComponentState> ComponentMap;
  typedef std::map<uint32, Component*> ConsumerMap;

  // Finds a consumer for a specified message type.
  Component* FindConsumer(uint32 message_type, Component* exclude) const;

  void ActivateForMessages(Component* component,
                           const MessageTypeVector& messages,
                           bool active);

  void DeactivateForMessages(Component* component,
                             const MessageTypeVector& messages);

  void CheckAndRequestConsumer(const uint32* messages,
                               int size,
                               Component* exclude);

  // Checks if a message need consumer.
  bool MessageNeedConsumer(uint32 message, Component* exclude) const;

  void InvalidateActiveHotkeyLists();

  void InitializeActiveHotkeyLists();

  uint32 id_;

  // |owner_| will be set to NULL when an input context object is being
  // destroyed.
  Component* owner_;

  Delegate* delegate_;

  ComponentMap attached_components_;

  ConsumerMap active_consumers_;

  bool active_hotkey_lists_valid_;

  std::vector<const HotkeyList*> active_hotkey_lists_;

  DISALLOW_COPY_AND_ASSIGN(InputContext);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_INPUT_CONTEXT_H_
