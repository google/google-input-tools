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

// This header is meant to be included in multiple passes, hence no traditional
// header guard.

#if !defined(DECLARE_IPC_MSG)
#error This file should not be included directly.
#endif

// Messages defined below must be sorted in asending order by id, otherwise bad
// thing may happen.
// Messages are grouped by sections, each section occupies a certain amount of
// additional id range, in case we want to add more messages in the future.
//
// Basic rules of message handling:
// 1. When receiving a message with reply_mode == NEED_REPLY, the message
//    consumer must send back a reply message, regardless of the original
//    message definition. If the original message definition does not require a
//    reply message, then a reply message with a boolean value should be sent
//    back indicating if the message has been handled successfully.
// 2. A reply message should always have explicit target set. So when sending a
//    reply message, its target must be set to the original message's source id.
// 3. Hub will just ignore a message's icid if its target is set to a explicit
//    comopnent id other than kComponentDefault or kComponentBroadcast.
// 4. If a message's target is kComponentDefault or kComponentBroadcast, then
//    Hub will determine the actual target based on its icid.

DECLARE_IPC_MSG(0, INVALID)

//////////////////////////////////////////////////////////////////////////////
// Messages for managing components.
//////////////////////////////////////////////////////////////////////////////

// Component -> Hub
// Register one or more components to hub.
//
// reply_mode: NEED_REPLY, the source component must wait for the reply.
// source: kComponentDefault
//    The component has no id yet before sending this message.
// target: kComponentDefault
//    This message will be processed by Hub.
// icid: kInputContextNone.
//    This message may not be bound to any input context.
// payload: component_info
//    One or more ComponentInfo objects. Multiple components can be registered
//    at the same time, then they will share the same connection to hub.
//
// Reply message:
// reply_mode: IS_REPLY.
// source: kComponentDefault, indicating hub.
// target: kComponentDefault
// icid: kInputContextNone.
// payload: component_info
//    The same ComponentInfo objects with component ids filled in.
//
// If a component was failed to register by any reason, kComponentDefault will
// be returned as the id.
DECLARE_IPC_MSG(0x0001, REGISTER_COMPONENT)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when a component is created.
//
// reply_mode: NO_REPLY.
// source: kComponentDefault
// target: kComponentBroadcast
// icid: kInputContextNone
// payload: component_info
//    The ComponentInfo object of the newly created component.
DECLARE_IPC_MSG(0x0002, COMPONENT_CREATED)

// Component -> Hub
// Deregister one or more components from hub.
//
// reply_mode: NO_REPLY, the source component may not wait for the reply.
// source: kComponentDefault
// target: kComponentDefault
//    This message will be processed by Hub.
// icid: kInputContextNone.
//    This message may not be bound to any input context.
// payload: The ids of components to be deregistered shall be stored in the
//    uint32 array.
//
// Reply message: (Only available if the source component wants it)
// reply_mode: IS_REPLY.
// source: kComponentDefault, indicating hub.
// target: kComponentDefault
// icid: kInputContextNone.
// payload: Boolean values indicating whether or not the components were
//    deregistered successfully will be stored in the boolean array.
DECLARE_IPC_MSG(0x0003, DEREGISTER_COMPONENT)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when a component is deleted.
//
// reply_mode: NO_REPLY.
// source: kComponentDefault
// target: kComponentBroadcast
// icid: kInputContextNone
// payload: Id of the deleted component is stored in the uint32 array.
DECLARE_IPC_MSG(0x0004, COMPONENT_DELETED)

// Component -> Hub
// Query information of registered components.
//
// reply_mode: NEED_REPLY, the source component must wait for the reply.
// source: Id of the component sending this message.
// target: kComponentDefault
//    This message will be processed by Hub.
// icid: kInputContextNone.
//    This message may not be bound to any input context.
// payload: component_info
//    One or more template ComponentInfo objects. For each object, the
//    available fields form an AND matching rule. A registered component
//    will be treated as matched, if it matches one of the template
//    ComponentInfo objects.
//
// Reply message:
// reply_mode: IS_REPLY.
// source: kComponentDefault, indicating hub.
// target: Id of the component sending the original message.
// icid: kInputContextNone.
// payload: component_info
//    ComponentInfo objects of all matched components.
DECLARE_IPC_MSG(0x0005, QUERY_COMPONENT)


//////////////////////////////////////////////////////////////////////////////
// Messages for managing input context.
//////////////////////////////////////////////////////////////////////////////

// Component(App) -> Hub
// Create a new input context.
//
// reply_mode: NEED_REPLY, the source component must wait for the reply to get
//   the id of the newly created input context.
// source: Id of the component which wants to create a new input context.
// target: kComponentDefault
//    This message will be processed by Hub.
// icid: kInputContextNone
// payload: No payload
//    TODO(suzhe): Add payload for describing content type of the input field.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which wants to create a new input context.
// icid: An unique id of the newly created input context.
// payload: A boolean value true, indicating the input context has been
//    created successfully.
//
// The source component will receive the reply message as soon as Hub
// creates the input context.
// Hub won't attach other components to the input context automatically,
// so before actually using the input context, the source component may want
// to send a MSG_REQUEST_CONSUMER message to Hub to find out consumer
// components for certain messages and attach them to the input context.
//
// If any error occurred, the reply message will contain an Error payload and
// the returned icid will be kInputContextNone.
DECLARE_IPC_MSG(0x0020, CREATE_INPUT_CONTEXT)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when an input context is created.
//
// reply_mode: NO_REPLY.
// source: kComponentDefault
// target: kComponentBroadcast
// icid: kInputContextNone
// payload: input_context_info
//    The information of the newly created input context.
DECLARE_IPC_MSG(0x0021, INPUT_CONTEXT_CREATED)

// Component(App) -> Hub
// Delete an existing input context.
//
// reply_mode: NO_REPLY
// source: Id of the component which wants to delete an input context
// target: kComponentDefault
//    This message will be processed by Hub.
// icid: Id of the input context to be deleted
//    It must be owned by the source component.
// payload: No payload
//
// Reply message (Only available if the source component wants one):
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which wants to delete an input context
// icid: Id of the input context being deleted.
// payload: A boolean value true, indicating the input context has been
//    deleted successfully.
//
// An input context can only be deleted by its owner.
//
// If any error occurred and a reply message was requested, the reply message
// will contain an Error payload.
DECLARE_IPC_MSG(0x0022, DELETE_INPUT_CONTEXT)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when an input context is deleted.
//
// reply_mode: NO_REPLY.
// source: kComponentDefault
// target: kComponentBroadcast
// icid: kInputContextNone
// payload: The first item of the uint32 array is the id of the input context
//    being deleted.
DECLARE_IPC_MSG(0x0023, INPUT_CONTEXT_DELETED)

// This is a bi-direction message that can be produced and consumed by both
// Hub and a component.
//
// Hub -> Component(IME,UI)
// Asks a component to attach itself to an input context.
//
// reply_mode: NEED_REPLY
// source: kComponentDefault
// target: Id of the component to be attached to the input context
// icid: Id of the input context to be attached
// payload:
//    input_context_info: Information of the input context.
//
// Reply message:
// reply_mode: IS_REPLY
// source: Id of the component attached to the input context
// target: kComponentDefault
// icid: id of the input context which the component is attached to
// payload: A boolean 'true' value when success. Any other payload indicating
//    a failure.
//
// Hub sends this message to a component when it wants to attach the
// comopnent to an input context for consuming some messages. Upon receiving
// this message, the component may perform necessary initialization task,
// such as, creating an internal object to serve this input context.
// When finishing the initialization task successfully, the component must
// reply a boolean 'true' to Hub. Then a MSG_COMPONENT_ACTIVATED message
// will be sent to the component containing message types that the component
// is assigned to consume.
//
// Note that, all components will be attached to the default input context
// (kInputContextNone) implicitly without involving this message.
//
// If a component is attached in this way, it might be detached automatically
// later when it's not necessary anymore.
//
// Hub won't attach a component to an input context implicitly without
// asking the component (except the default input context), so if a component
// can't consume this message, it can only attach to an input context by
// sending MSG_ATTACH_TO_INPUT_CONTEXT or MSG_ACTIVATE_COMPONENT message to
// attach or activate itself explicitly.
//
//
// Component(IME,UI) -> Hub
// Asks Hub to attach a component to an input context.
//
// reply_mode: NO_REPLY
// source: Id of the component which wants to attach to a specified input
//    context.
// target: kComponentDefault
// icid: Id of the input context which the component wants to be attached to
// payload: No payload
//
// Reply message: (only available when the component requires it)
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component sending out the original request.
// icid: Id of the input context which the target component is attached to
// payload: A boolean 'true' value when success. Any other payload indicating
//    a failure.
//
// A component will be attached to an input context passively, so that it will
// only be assigned as the active consumer for the messages which do not have
// active consumers yet. In order to activate a component for a specified
// input context, making the component the active consumer for all messages it
// can consume, A MSG_ACTIVATE_COMPONENT must be sent to Hub.
//
// If a component is attached in this way, it won't be detached automatically
// unless it sends a MSG_DETACH_FROM_INPUT_CONTEXT message to Hub
// explicitly.
DECLARE_IPC_MSG(0x0024, ATTACH_TO_INPUT_CONTEXT)

// Component -> Hub
// Detachs a component itself from an input context.
//
// reply_mode: NO_REPLY
// source: id of the component which wants to detach itself from a specified
//    input context
// target: kComponentDefault
// icid: id of the input context which the component wants to be detached from
// payload: No payload
//
// Once the component is detached successfully, a
// MSG_DETACHED_FROM_INPUT_CONTEXT message will be sent to the component.
DECLARE_IPC_MSG(0x0025, DETACH_FROM_INPUT_CONTEXT)

// Hub -> Component(IME,UI)
// Inform a component when it gets detached from an input context.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: id of the component which is just detached from the input context
// icid: id of the input context
// payload: No payload
//
// Hub sends this message to a component when it detachs a component from
// an input context. Upon receiving this message, the component should release
// all resources associated to the input context.
// A component may be detached from an input context because of:
// 1. The input context is destroyed
// 2. The component is replaced by another component with the same
// functionality.
DECLARE_IPC_MSG(0x0026, DETACHED_FROM_INPUT_CONTEXT)

// Component -> Hub
// Queries the information of a specified input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message
// target: kComponentDefault
// icid: Id of the input context to be queried
// payload: No payload
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent out the original message
// icid: Id of the input context being queried
// payload: An InputContextInfo object.
DECLARE_IPC_MSG(0x0027, QUERY_INPUT_CONTEXT)

// Component(App) -> Hub
// Give input focus to an input context.
//
// reply_mode: NO_REPLY
// source: id of the component which owns the input context to be focused
// target: kComponentDefault
// icid: id of the input context to be focused
// payload: No payload
//
// Reply message (Only available if the source component wants one):
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent out the original message
// icid: Id of the input context being focused
// payload: A boolean value true, indicating the input context has been
//    focused successfully.
//
// This message asks Hub to give input focus to an input context. Upon
// receiving this message, Hub will move input focus from the current
// focused input context to the specified input context and broadcast
// MSG_INPUT_CONTEXT_LOST_FOCUS and MSG_INPUT_CONTEXT_GOT_FOCUS messages to
// inform other components about the focus movement.
DECLARE_IPC_MSG(0x0028, FOCUS_INPUT_CONTEXT)

// Hub -> Component(IME,UI) (Broadcast)
// Inform that an input context just got input focus.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: id of the input context which just got input focus
// payload: No payload
//
// Any components caring about input focus change should consume this message
// rather than MSG_FOCUS_INPUT_CONTEXT message.
// A component must attach to the input context in order to receive this
// message.
DECLARE_IPC_MSG(0x0029, INPUT_CONTEXT_GOT_FOCUS)

// Component(App) -> Hub
// Remove input focus from an input context.
//
// reply_mode: NO_REPLY
// source: id of the component which owns the input context to be blurred
// target: kComponentDefault
// icid: id of the input context to be blurred
// payload: No payload
//
// Reply message (Only available if the source component wants one):
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent out the original message
// icid: Id of the input context being blurred
// payload: A boolean value true, indicating the input context has been
//    blurred successfully.
//
// This message asks Hub to remove input focus from an input context.
// Upon receiving this message, Hub will remove input focus from the
// specified input context if it's currently focused. And broadcast
// MSG_INPUT_CONTEXT_LOST_FOCUS message to inform other components about
// the focus change.
DECLARE_IPC_MSG(0x002A, BLUR_INPUT_CONTEXT)

// Hub -> Component(IME,UI) (Broadcast)
// Inform that an input context just lost input focus.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// original_target: kComponentBroadcast, indicating it's a broadcast message.
// icid: id of the input context which just lost input focus
// payload: No payload
//
// Any components caring about input focus change should consume this message
// rather than MSG_BLUR_INPUT_CONTEXT message.
// A component must attach to the input context in order to receive this
// message.
DECLARE_IPC_MSG(0x002B, INPUT_CONTEXT_LOST_FOCUS)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when a component is attached to an input
// context.
//
// reply_mode: NO_REPLY.
// source: kComponentDefault
// target: kComponentBroadcast
// icid: kInputContextNone
// payload:
// 1. uint32[0]: Id of the input context which a component is attached to.
// 2. uint32[1]: Id of the component being attached to the input context.
DECLARE_IPC_MSG(0x002C, COMPONENT_ATTACHED)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when a component is detached from an
// input context.
//
// reply_mode: NO_REPLY.
// source: kComponentDefault
// target: kComponentBroadcast
// icid: kInputContextNone
// payload:
// 1. uint32[0]: Id of the input context which a component is detached from.
// 2. uint32[1]: Id of the component being detached from the input context.
//
// This message will not be broadcasted when an input context is being
// deleted.
DECLARE_IPC_MSG(0x002D, COMPONENT_DETACHED)

// TODO(suzhe): Add messages for updating the information of an input context,
// such as the input type and allowed character set.


//////////////////////////////////////////////////////////////////////////////
// Messages for managing active consumers attached to input contexts.
//////////////////////////////////////////////////////////////////////////////

// Component -> Hub
// Activates one or more components for a specified input context.
//
// reply_mode: NO_REPLY
// source: id of the component sending this message.
// target: kComponentDefault
// icid: id of the input context which components should be activated for.
// payload: If there is no payload then the source component itself will be
//    activated. Otherwise ids/string_ids of components to be activated
//    should be stored in uint32/string array,
//
// Reply message (Only available if the source component wants one):
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent out the original message
// icid: id of the input context which components got activated for.
// payload: boolean values indicating if components are activated correctly.
//
// Activating a component means: assigning the component as the active
// consumer of all messages it can consume. But a component can't override
// active consumer roles owned by the owner of the input context or Hub,
// unless they give up the role explicitly.
//
// When activating a component which is not attached to the input context yet,
// Hub will try to attach the comopnent to the input context first. If the
// component consumes MSG_ATTACH_TO_INPUT_CONTEXT message, then Hub will
// send this message to the component and attach and activate it when
// receiving a valid reply.
// When a component is activated for any message types, then a
// MSG_COMPONENT_ACTIVATED message will be sent to it.
DECLARE_IPC_MSG(0x0040, ACTIVATE_COMPONENT)

// Component -> Hub
// Assigns a component as the active consumer of one or more message types to
// a specified input context.
//
// reply_mode: NO_REPLY
// source: id of the component sending this message
// target: kComponentDefault
// icid: id of the input context which the source component will be assigned
//    to.
// payload: message types to assign should be stored in uint32 array.
//
// A component can only assign itself to an input context. When the component
// successfully gets active consumer roles for one or more messages, Hub
// will send MSG_COMPONENT_ACTIVATED message to it.
DECLARE_IPC_MSG(0x0041, ASSIGN_ACTIVE_CONSUMER)

// Component -> Hub
// Resigns active consumer roles of one or more message types from a specified
// input context.
//
// reply_mode: NO_REPLY
// source: id of the component sending this message
// target: kComponentDefault
// icid: id of the input context which the source component will be resigned
//    from.
// payload: message types to resign should be stored in uint32 array.
//
// A component can only resign itself from an input context. When the
// component is successfully resigned from active consumer roles for one or
// more messages, Hub will send MSG_COMPONENT_DEACTIVATED message to it.
DECLARE_IPC_MSG(0x0042, RESIGN_ACTIVE_CONSUMER)

// Component -> Hub
// Queries active consumers of one or more message types for a specific input
// context.
//
// reply_mode: NEED_REPLY
// source: id of the component sending this message
// target: kComponentDefault
// icid: id of the input context to be queried.
// payload: message types to be queried should be stored in uint32 array.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: id of the component which sent the original message
// icid: id of the input context being queried.
// payload: ids of active consumers of each message are stored in uint32 array.
// If there is no active comsumer for a specific message type, then a
// kComponentBroadcast will be returned instead.
DECLARE_IPC_MSG(0x0043, QUERY_ACTIVE_CONSUMER)

// Hub -> Component
// Informs a component that it has been activated (assigned as active consumer)
// for one or more message types.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: id of the component which was activated.
// icid: id of the input context which the component was activated for
// payload: activated message types are stored in uint32 array
DECLARE_IPC_MSG(0x0044, COMPONENT_ACTIVATED)

// Hub -> Component
// Informs a component that it has been deactivated for one or more message
// types.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: id of the component which was deactivated.
// icid: id of the input context which the component was deactivated for
// payload: deactivated message types are stored in uint32 array
DECLARE_IPC_MSG(0x0045, COMPONENT_DEACTIVATED)

// Component -> Hub
// Asks Hub to look up components which can consume specified message
// types, and attaches them to the input context as active consumers.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: id of the component which needs consumers.
// target: kComponentDefault
// icid: id of the input context which consumers should be attached to
// payload: message types requiring consumers should be stored in uint32 array
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload:
// 1. boolean[0]: indicating if the message was handled correctly or not.
// 2. uint32: Requested message types that already have active consumers.
//
// For a component, not all messages produced by it may need consumers. For
// example, some messages may just for informing other components for updated
// status, and some messages may be optional. So Hub won't automatically
// attach a component to an input context, unless another attached component
// asks for it explicitly by sending this message. Usually this message should
// only be sent to Hub once as soon as the component is attached to the
// input context. If more than one of this messages are sent, only the last
// one will take effect.
//
// Hub will only care about message types that do not have active consumer yet.
// When receiving this message, Hub will find out all candidate components for
// consuming the requested message types that do not have active consumer yet,
// and sends out asynchronous requests to ask them to attach themselves to the
// specified input context. As this process happens asynchronously, if the
// component sending this request wants to know whether or not its request gets
// fulfilled, it can do following things:
// 1. Request for this message's reply message, which contains all requested
// message types that already have active consumers.
// 2. Monitor ACTIVE_CONSUMER_CHANGED message so that it could be notified when
// other requested message types have active consumers.
DECLARE_IPC_MSG(0x0046, REQUEST_CONSUMER)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub when any messages' active consumers of
// an input context are changed.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: id of the input context whose active message consumers are changed
// payload:
// 1. uint32: Message types whose active consumers have been changed.
// 2. boolean: Boolean values indicating if corresponding message types have
// active consumer or not. A false means the old active consumer of
// corresponding message type was just deactivated.
//
// A component must attach to the input context in order to receive this
// message.
DECLARE_IPC_MSG(0x0047, ACTIVE_CONSUMER_CHANGED)


//////////////////////////////////////////////////////////////////////////////
// Messages for handling Keyboard events.
//////////////////////////////////////////////////////////////////////////////

// Component(App) -> Hub
// Sends a keyboard event to Hub for processing.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the keyboard event is bound to
// payload: A KeyEvent object
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload: A boolean value indicating if the keyboard event was processed or
// not.
//
// This message is handled by Hub itself. For each keyboard event,
// following steps will be performed:
// 1. The keyboard event will be matched against activated hotkeys. If any
// hotkey is matched, then messages bound to the hotkey will be dispatched and
// true will be returned to the application component sending the original
// keyboard event.
// 2. If the keyboard event doesn't match with any hotkey, then it'll be
// forward to the active input method component attached the input context by
// sending a MSG_PROCESS_KEY_EVENT.
// 3. When the input method replies the MSG_PROCESS_KEY_EVENT event, Hub
// will forward the result back to the application component.
DECLARE_IPC_MSG(0x0060, SEND_KEY_EVENT)

// Hub -> Component(IME) or Component(App) -> Component(IME)
// Sends a keyboard event to an input method.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the keyboard event is bound to
// payload: A KeyEvent object
//
// Reply message:
// reply_mode: IS_REPLY
// source: Id of the component which handled the message.
// target: Id of the component which sent the original message.
// icid: Id of the input context.
// payload: A boolean value indicating if the keyboard event was processed or
// not.
//
// If the application send a key event to Hub by using MSG_SEND_KEY_EVENT,
// then Hub will forward the key event to the input method by using
// MSG_PROCESS_KEY_EVENT. The application may also send this message directly
// to bypass hotkey processing in Hub.
DECLARE_IPC_MSG(0x0061, PROCESS_KEY_EVENT)

// Component(IME,UI) -> Component(App)
// Synthesizes a keyboard event.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the keyboard event is bound to
// payload: A KeyEvent object
//
// Application components should consume this message to synthesize a system
// keyboard event from the given KeyEvent object. The synthesized system
// keyboard event will be processed by the application like a real event, i.e.
// it will be sent back to Hub or the input method component as a
// MSG_SEND_KEY_EVENT or MSG_PROCESS_KEY_EVENT.
//
// This message is useful for a virtual keyboard component to generate fake
// keyboard events.
DECLARE_IPC_MSG(0x0062, SYNTHESIZE_KEY_EVENT)


//////////////////////////////////////////////////////////////////////////////
// Messages for handling Composition text.
//////////////////////////////////////////////////////////////////////////////

// Component(IME) -> Hub
// Sets the current composition text.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the composition text is bound to.
// payload: A Composition object
//
// This message is sent by the IME to Hub whenever the composition text is
// changed. If the payload doesn't contain a composition object, then the
// current composition text will be cleared.
// The input method should always send out an empty MSG_SET_COMPOSITION
// message to ckear the current composition explicity after inserting a text
// into the application.
//
// Hub caches a copy of the current composition text of each input context,
// and broadcasts a MSG_COMPOSITION_CHANGED message to the input context
// whenever its composition text gets changed.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0080, SET_COMPOSITION)

// Component(App) -> Component(IME)
// Asks the input method to cancel the current composition session.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose composition should be cancelled.
// payload: No payload
//
// This message is sent by the application to the IME whenever the application
// wants to abandon the current composition text. When receiving this message,
// the IME should reset states associated to the given icid to their initial
// value quietly. The IME should not send any result text to the application,
// except for sending out an empty MSG_SET_COMPOSITION message.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0081, CANCEL_COMPOSITION)

// Component(App) -> Component(IME)
// Asks the input method to complete the current composition session.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose composition should be completed.
// payload: No payload
//
// This message is sent by the application to the IME whenever the application
// wants to confirm the current composition text. When receiving this message,
// the IME should finish the ongoing composition session associated to the
// given icid by sending a MSG_INSERT_TEXT containing the result text and an
// empty MSG_SET_COMPOSITION message to clean the current composition text.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0082, COMPLETE_COMPOSITION)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub whenever the composition text of an input
// context gets changed.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: Id of the input context whose composition text gets changed.
// payload: A composition object or empty if the composition text gets cleared.
//
// The application component may watch this message if it wants to show the
// composition text inline. A UI component may also watch this message to show
// the composition text in a separated UI.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0083, COMPOSITION_CHANGED)

// Component(App,UI) -> Hub
// Queries the current composition text of an input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose composition text will be returned.
// payload: No payload.
//
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message.
// icid: Id of the input context whose composition text is returned.
// payload: A composition object or empty if there is no composition text.
//
// Any component can send this message to Hub to retrieve the current
// compoosition text of an input context.
DECLARE_IPC_MSG(0x0084, QUERY_COMPOSITION)

//////////////////////////////////////////////////////////////////////////////
// Messages for inserting content to applications.
//////////////////////////////////////////////////////////////////////////////

// Component(IME,Virtual Keyboard) -> Component(App)
// Inserts a text into the document associated to a specified input context.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the text should be inserted into.
// payload: A Composition object
//
// This message is sent by the IME (or Virtual Keyboard) to the application
// whenever a text should be inserted into the document.
// Applications must be able to handle this message in order to accept results
// from input methods. Though applications supporting inline composition may
// clear the current composition text before inserting a text, input methods
// should always send an empty MSG_SET_COMPOSITION message to clear the
// current composition explicity when necessary.
DECLARE_IPC_MSG(0x00A0, INSERT_TEXT)


//////////////////////////////////////////////////////////////////////////////
// Messages for handling candidate lists.
//////////////////////////////////////////////////////////////////////////////

// Component(IME) -> Hub
// Sets the current toplevel candidate list.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the candidate list is bound to.
// payload: A CandidateList object
//
// Though a CandidateList may contain cascaded CandidateLists, only one
// toplevel CandidateList is allowed. This message is for setting the toplevel
// CandidateList object.
//
// This message is sent by the IME to Hub whenever the current candidate list is
// changed.
//
// Hub caches a copy of the current candidate list of each input context, and
// broadcasts a MSG_CANDIDATE_LIST_CHANGED message to the input context whenever
// its candidate list gets changed.
//
// When receiving this message, the candidate UI or application should replace
// the old CandidateList object (if available) with the new one provided by
// the message. If the |auto_show| flag of this candidate list is false, then
// the candidate UI will not show it until the owner input method requests it
// explicitly by sending a MSG_SHOW_CANDIDATE_LIST message.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C0, SET_CANDIDATE_LIST)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub whenever the candidate list of an input
// context gets changed.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: Id of the input context whose candidate list gets changed.
// payload: A candidate list object or empty if the candidate list gets cleared.
//
// This message will only be triggered by a MSG_SET_CANDIDATE_LIST message.
//
// The application component may watch this message if it wants to show the
// candidate list by itself. A UI component may also watch this message to show
// the candidate list in a separated UI.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C1, CANDIDATE_LIST_CHANGED)

// Component(IME) -> Hub
// Sets the current selected candidate in the candidate list.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the candidate list is bound to.
// payload:
// 1. uint32[0]: Id of the current selected candidate list.
// 2. uint32[1]: Index of the current selected candidate in the candidate list,
//               if the value is >= number of candidates in the candidate list,
//               then the current selected candidate will be unselected.
//
// This message is sent by the IME to Hub whenever it wants to select a
// candidate (as well as a candidate list when necessary).
//
// Each candidate list has a selected candidate, but when there are more than
// one visible cascaded candidate lists, only one candidate in one candidate
// list can be selected actively. This message is for this purpose.
//
// The actively selected candidate and its owner candidate list may be displayed
// in a different style than selected candidates in other candidate lists.
//
// Hub caches the selected candidate information and broadcasts a
// MSG_SELECTED_CANDIDATE_CHANGED message to the input context whenever the
// selected candidate gets changed.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C2, SET_SELECTED_CANDIDATE)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub whenever the selected candidate of an
// input context gets changed.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: Id of the input context whose selected candidate gets changed.
// payload:
// 1. uint32[0]: Id of the current selected candidate list.
// 2. uint32[1]: Index of the current selected candidate in the candidate list,
//               if the value is >= number of candidates in the candidate list,
//               then the current selected candidate will be unselected.
//
// The application component should watch this message if it shows the candidate
// list by itself.
// A UI component should also watch this message if it shows a separated
// candidate UI.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C3, SELECTED_CANDIDATE_CHANGED)

// Component(IME) -> Hub
// Sets the visibility of a candidate list.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the candidate list is bound to.
// payload:
// 1. uint32[0]: Id of the candidate list whose visibility will be changed.
// 2. boolean[0]: Indicates if the candidate list should be visible or not.
//
// This message is sent by the IME whenever it wants to show or hide a candidate
// list.
//
// A candidate list can be shown only if it's the toplevel candidate list or
// all parent candidate lists are visible.
//
// Hub will broadcast a MSG_CANDIDATE_LIST_VISIBILITY_CHANGED message whenever a
// candidate list's visibility gets changed.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C4, SET_CANDIDATE_LIST_VISIBILITY)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub whenever the visibility of a candidate
// list of an input context gets changed.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: Id of the input context whose candidate list's visibility gets changed.
// payload:
// 1. uint32[0]: Id of the candidate list.
// 2. boolean[0]: Indicates if the candidate list should be visible or not.
//
// The application component should watch this message if it shows the candidate
// list by itself.
// A UI component should also watch this message if it shows a separated
// candidate UI.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C5, CANDIDATE_LIST_VISIBILITY_CHANGED)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list when it's shown.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: A uint32 integer indicating the id of the candidate list.
//
// This message is sent by the candidate UI to the owner input method whenever
// a candidate list is shown by any reason.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C6, CANDIDATE_LIST_SHOWN)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list when it's shown.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: A uint32 integer indicating the id of the candidate list.
//
// This message is sent by the candidate UI to the owner input method whenever
// a candidate list is hidden by any reason.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C7, CANDIDATE_LIST_HIDDEN)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list to flip to the next page.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: A uint32 integer indicating the id of the candidate list.
//
// This message is sent by the candidate UI to the owner input method whenever
// the user clicks candidate window's page down button.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C8, CANDIDATE_LIST_PAGE_DOWN)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list to flip to the previous
// page.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: A uint32 integer indicating the id of the candidate list.
//
// This message is sent by the candidate UI to the owner input method whenever
// the user clicks candidate window's page up button.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00C9, CANDIDATE_LIST_PAGE_UP)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list to scroll to a specific
// candidate.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: Two uint32 integers:
// uint32(0) = id of the candidate list;
// uint32(1) = desired candidate index for |page_start|.
//
// This message is sent by the candidate UI to the owner input method whenever
// the user clicks or drags candidate window's scroll bar.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00CA, CANDIDATE_LIST_SCROLL_TO)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list to resize the current page.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: Three uint32 integers:
// uint32(0) = id of the candidate list;
// uint32(1) = desired |page_width|;
// uint32(2) = desired |page_height|;
//
// This message is sent by the candidate UI to the owner input method when the
// candidate UI cannot display the whole page for some reasons, e.g. the
// screen size is too small. So that the input method must adjust the current
// page size in order to handle page flipping correctly.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00CB, CANDIDATE_LIST_PAGE_RESIZE)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list when the user selects a
// candidate by mouse.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the candidate list.
// icid: Id of the input context which the candidate list is bound to.
// payload: Two uint32 integers and one boolean:
// uint32(0) = id of the candidate list;
// uint32(1) = index of the candidate in the candidate list;
// boolean(0) = indicates if the user wants to commit the candidate. False
// means the user just selected the candidate by right click or opening its
// command menu.
//
// This message is sent by the candidate UI to the owner input method when the
// user selects a candidate.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00CC, SELECT_CANDIDATE)

// Component(App,UI) -> Component(IME)
// Informs the owner component of a candidate list when the user triggers a
// command associated to a candidate.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the command.
// icid: Id of the input context which the candidate list is bound to.
// payload: Three uint32 integers:
// uint32(0) = id of the candidate list;
// uint32(1) = index of the candidate in the candidate list;
// string(0) = id of the command being triggered.
//
// This message is sent by the candidate UI to the owner input method when the
// user triggers a command associated to a candidate.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x00CD, DO_CANDIDATE_COMMAND)

// Component(App,UI) -> Hub
// Queries the current candidate list of an input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose candidate list will be returned.
// payload: No payload.
//
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message.
// icid: Id of the input context whose candidate list is returned.
// payload:
// 1. A candidate list object or empty if there is no candidate list.
// 2. uint32[0]: Id of the candidate list containing the actively selected
//               candidate. (Optional)
//
// Any component can send this message to Hub to retrieve the current
// candidate list of an input context.
DECLARE_IPC_MSG(0x00CE, QUERY_CANDIDATE_LIST)


//////////////////////////////////////////////////////////////////////////////
// Messages for managing Input Caret information.
//////////////////////////////////////////////////////////////////////////////

// Component(App) -> Component(IME,UI)/Hub (Broadcast)
// Informs that the input caret's position on the screen or text direction has
// been changed.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentBroadcast
// icid: Id of the input context which the input caret is bound to.
// payload: An InputCaret object.
//
// The application broadcasts this message whenever the input caret's on
// screen position or text direction is changed. Hub will cache the input caret
// information internally, so that any component can query it afterwards.
//
// TODO(suzhe): add support of this message in hub_input_context_manager.cc
DECLARE_IPC_MSG(0x00E0, UPDATE_INPUT_CARET)

// Component -> Hub
// Query the input caret information of a specified input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the input caret is bound to.
// payload: No payload.
//
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload: An InputCaret object.
//
// Though Hub support of querying the information of any input context, it only
// makes sense to query the focused one.
//
// TODO(suzhe): add support of this message in hub_input_context_manager.cc
DECLARE_IPC_MSG(0x00E1, QUERY_INPUT_CARET)


//////////////////////////////////////////////////////////////////////////////
// Messages for accessing document contents.
//////////////////////////////////////////////////////////////////////////////

// Component(IME) -> Component(App)/Hub
//
// TODO(suzhe): define this message.
DECLARE_IPC_MSG(0x0100, GET_DOCUMENT_INFO)

// Component(IME) -> Component(App)/Hub
//
// TODO(suzhe): define this message.
DECLARE_IPC_MSG(0x0101, GET_DOCUMENT_CONTENT_IN_RANGE)


//////////////////////////////////////////////////////////////////////////////
// Messages for managing global CommandLists.
//////////////////////////////////////////////////////////////////////////////

// Component -> Hub
// Sets component's command list for a specified input context.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the command list is bound to.
// payload: A CommandList object or empty to clear the registered CommandList.
//
// This message is used for registering a command list to a specified input
// context, which may be displayed on the screen by a UI in a system defined
// way. On Windows commands may be displayed as buttons on a toolbar, on
// Mac commands may be displayed as menu items of system language menu.
//
// Any components can register commands to any input contexts. Commands
// registered to kInputContextNone are so called global commands, which will
// always be displayed by the UI regardless of the current focused input
// context. Commands registered to a real input context will only be displayed
// when the input context is focused.
//
// A component can only register a CommandList object to an input context if it
// has been attached to this input context. And the registered CommandList
// object will be removed automatically when the component is detached from the
// input context.
//
// A component should only register at most one CommandList object to an input
// context, but can reigster multiple CommandList objects to different input
// contexts. So a CommandList object can be identified by its owner component
// id and associated input context id.
//
// Commands in a CommandList can be organized in a tree hierarchy by attaching
// one or more commands to another command as its sub commands.
DECLARE_IPC_MSG(0x0120, SET_COMMAND_LIST)

// Component -> Hub
// Updates one or more commands previous registered to a specified input context
// by MSG_SET_COMMAND_LIST.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the command list is bound to.
// payload: A CommandList object containing the updated commands.
//
// This message is used for updating one or more registered command in the
// registered CommandList tree hierarchy. The update will be performed by
// replacing the old Command objects by the new Command objects with same ids.
//
// Commands in this message's CommandList object do not need to be organized
// into the same hierarchy as the registered ones.
DECLARE_IPC_MSG(0x0121, UPDATE_COMMANDS)

// Component -> Hub
// Query all CommandList objects registered to a specified input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose command lists will be returned.
// payload: No payload.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload: CommandList objects.
//
// This message returns all CommandList objects registered to a specified input
// context by all components attached to it.
DECLARE_IPC_MSG(0x0122, QUERY_COMMAND_LIST)

// Hub -> Components (Broadcast)
// A broadcast message produced by Hub whenever any command list registered to a
// specified input context has been changed.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: kComponentBroadcast
// icid: id of the input context whose command list has been changed.
// payload:
// 1. The latest CommandList objects registered to the input context. A
// CommandList with no command means that the original CommandList has been
// removed from the input context.
// 2. A set of boolean values indicating if anything has been changed in the
// corresponding CommandList object.
//
// A UI component may watch this message to update the commands displayed on
// screen whenever anything has been changed.
//
// This message only applies to kInputContextNone and the focused input context.
DECLARE_IPC_MSG(0x0123, COMMAND_LIST_CHANGED)

// Component(UI) -> Component
// Informs the owner component of a command when it's triggered by the user.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component owning the command.
// icid: Id of the input context which the command is registered to.
// payload: A string value containing the command id being triggered.
DECLARE_IPC_MSG(0x0124, DO_COMMAND)


//////////////////////////////////////////////////////////////////////////////
// Messages for managing HotkeyLists.
//////////////////////////////////////////////////////////////////////////////

// Component -> Hub
// Registers one or more hotkey lists to Hub.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload: HotkeyList objects.
//
// This message is sent by a component to register one or more hotkey lists to
// Hub. A hotkey list must be registered before being activated.
DECLARE_IPC_MSG(0x0140, ADD_HOTKEY_LIST)

// Component -> Hub
// Removes one or more registered hotkey lists from Hub.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload: uint32 ids of hotkey lists to be removed.
//
// This message is sent by a component to remove one or more previously
// registered hotkey lists from Hub.
DECLARE_IPC_MSG(0x0141, REMOVE_HOTKEY_LIST)

// Component -> Hub
// TODO(suzhe): define this message and implement it in hub_hotkey_manager.cc.
DECLARE_IPC_MSG(0x0142, CHECK_HOTKEY_CONFLICT)

// Component -> Hub
// Activates a registered hotkey list on a specified input context.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context on which the hotkey list should be activated.
// payload: A uint32 id of the hotkey list to be activated.
DECLARE_IPC_MSG(0x0143, ACTIVATE_HOTKEY_LIST)

// Component -> Hub
// Deactivates the currently activated hotkey list on a specified input context.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context from which the hotkey list should be
//    deactivated.
// payload: None.
DECLARE_IPC_MSG(0x0144, DEACTIVATE_HOTKEY_LIST)

// Component -> Hub
// Queries the currently activated hotkey lists on a specified input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context to be queried.
// payload: None.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload: All HotkeyList objects currently activated on the input context.
DECLARE_IPC_MSG(0x0145, QUERY_ACTIVE_HOTKEY_LIST)

// Hub -> Components (Broadcast)
// TODO(suzhe): define this message and implement it in hub_hotkey_manager.cc.
DECLARE_IPC_MSG(0x0146, ACTIVE_HOTKEY_LIST_UPDATED)


//////////////////////////////////////////////////////////////////////////////
// Messages for managing keyboard input methods.
//////////////////////////////////////////////////////////////////////////////

// Component -> Hub (Input Method Manager)
// Lists all running keyboard input method components currently running.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which will be checked to find out all
//    suitable input methods for it.
// payload: No payload
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload:
//    component_info: ComponentInfo objects of all keyboard input methods.
//    boolean: boolean values indicating if the input method is suitable for
//             the input context.
DECLARE_IPC_MSG(0x0160, LIST_INPUT_METHODS)

// Component -> Hub (Input Method Manager)
// Switches the active input method of an input context to a specified one.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose input method should be switched.
// payload:
//    string: the string_id of the new input method component, or
//    uint32: the id of the new input method component.
DECLARE_IPC_MSG(0x0161, SWITCH_TO_INPUT_METHOD)

// Component -> Hub (Input Method Manager)
// Switches the active input method of an input context to the next one in the
// list.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose input method should be switched.
// payload: No payload
DECLARE_IPC_MSG(0x0162, SWITCH_TO_NEXT_INPUT_METHOD_IN_LIST)

// Component -> Hub (Input Method Manager)
// Switches the active input method of an input context to the previous one
// used by the input context.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context whose input method should be switched.
// payload: No payload
DECLARE_IPC_MSG(0x0163, SWITCH_TO_PREVIOUS_INPUT_METHOD)

// Hub (Input Method Manager) -> Components (Broadcast)
// A broadcast message produced by Hub when an input method is activated for
// an input context.
//
// reply_mode: NO_REPLY
// source: kComponentDefault
// target: id of the component receiving this message.
// icid: Id of the input context which the input method is activated for
// payload:
//    component_info: A ComponentInfo object of the active input method.
DECLARE_IPC_MSG(0x0164, INPUT_METHOD_ACTIVATED)

// Component -> Hub (Input Method Manager)
// Queries the active input method information of an input context.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: Id of the input context which will be queried.
// payload: No payload
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message
// icid: Id of the input context
// payload:
//    component_info: A ComponentInfo object of the active input method.
DECLARE_IPC_MSG(0x0165, QUERY_ACTIVE_INPUT_METHOD)


//////////////////////////////////////////////////////////////////////////////
// Messages for accessing settings store.
// TODO(suzhe): implement settings component to support these messages.
//////////////////////////////////////////////////////////////////////////////

// Component -> Component(Settings Store)
// Sets the value of one or more specified settings.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload:
// 1. string: keys of the settings to set.
// 2. variable: values of the settings. Using A variable with type == NONE to
// delete the value assoicated to the key.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// icid: kInputContextNone
// target: Id of the component which sent the original message.
// payload:
// 1. boolean: a set of results which indicate if corresponding value was set
// successfully or not.
//
// This message can set multiple settings at once, but it can only handle
// single-value settings. Use MSG_SETTINGS_SET_ARRAY_VALUE for multi-value
// settings.
//
// In order to keep best portability, a key should only contain characters in
// ranges: [a-z], [A-Z], [0-9] or symbols: [-_/]. Any chars not in these ranges
// will be replaced by '_'.
DECLARE_IPC_MSG(0x0180, SETTINGS_SET_VALUES)

// Component -> Component(Settings Store)
// Gets the value of one or more specified settings.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload:
// 1. string: keys of the settings to get.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message.
// icid: kInputContextNone
// payload:
// 1. string: keys of the settings.
// 2. variable: values of the settings. If a key is not found then an empty
// variable with type == NONE will be returned.
//
// This message can get multiple settings at once, but it can only handle
// single-value settings. Use MSG_SETTINGS_GET_ARRAY_VALUE for multi-value
// settings.
DECLARE_IPC_MSG(0x0181, SETTINGS_GET_VALUES)

// Component -> Component(Settings Store)
// Sets the values of a specified multi-value setting.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload:
// 1. string[0]: key of the setting to set.
// 2. variable: values of the setting.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// icid: kInputContextNone
// target: Id of the component which sent the original message.
// payload:
// 1. boolean[0]: true if array value was set successfully.
//
// This message can only set one multi-value setting at once.
DECLARE_IPC_MSG(0x0182, SETTINGS_SET_ARRAY_VALUE)

// Component -> Component(Settings Store)
// Gets the values of a specified multi-value setting.
//
// reply_mode: NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload:
// 1. string[0]: key of the setting to get.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message.
// icid: kInputContextNone
// payload:
// 1. string[0]: key of the setting.
// 2. variable: values of the setting. If the key is not found then an empty
// variable with type == NONE will be returned.
//
// This message can only get one multi-value setting at once.
DECLARE_IPC_MSG(0x0183, SETTINGS_GET_ARRAY_VALUE)

// Component -> Component(Settings Store)
// Starts monitoring the change of one or more settings.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload:
// 1. string: keys of settings to start monitoring. Adding a '*' character at
// the end of a key to monitor all keys with the same prefix.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message.
// icid: kInputContextNone
// payload:
// 1. boolean[0]: true if the observer was added successfully.
//
// This message registers the source component to the settings store component
// for monitoring the change of specified settings. The source component must be
// able to consume MSG_SETTINGS_CHANGED message, to receive changes.
DECLARE_IPC_MSG(0x0184, SETTINGS_ADD_CHANGE_OBSERVER)

// Component -> Component(Settings Store)
// Stops monitoring the change of one or more settings.
//
// reply_mode: NO_REPLY or NEED_REPLY
// source: Id of the component sending this message.
// target: kComponentDefault
// icid: kInputContextNone
// payload:
// 1. string: keys of settings to stop monitoring, which should be exactly same
// as the keys used when adding the observer.
//
// Reply message:
// reply_mode: IS_REPLY
// source: kComponentDefault
// target: Id of the component which sent the original message.
// icid: kInputContextNone
// payload:
// 1. boolean[0]: true if the observer was removed successfully.
DECLARE_IPC_MSG(0x0185, SETTINGS_REMOVE_CHANGE_OBSERVER)

// Component(Settings Store) -> Component
// Informs a component the change of a setting.
//
// reply_mode: NO_REPLY
// source: Id of the component sending this message.
// target: Id of the component monitoring the settings change.
// icid: kInputContextNone
// payload:
// 1. string[0]: key of the setting whose value(s) has been changed.
// 2. variable: new value(s) of the changed setting.
//
// This message only contains the change of one setting. Multiple messages
// will be sent if more than one settings were changed.
//
// The component changing the setting will not receive this message, even if
// it's monitoring the change this setting.
DECLARE_IPC_MSG(0x0186, SETTINGS_CHANGED)


//////////////////////////////////////////////////////////////////////////////
// Messages for controlling visibility of various UI elements.
//////////////////////////////////////////////////////////////////////////////

// Component(App) -> Component(UI)
// Asks the UI component to show the composition UI.
//
// reply_mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the composition UI is for.
// payload: No payload.
//
// The application component should send this message to the UI component to
// show the external composition UI, if it cannot show composition text inline.
// The UI component should remember the visible state of the composition UI upon
// receiving this message until the input context loses focus or receives a
// MSG_HIDE_COMPOSITION_UI
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0200, SHOW_COMPOSITION_UI)

// Component(App) -> Component(UI)
// Asks the UI component to hide the composition UI.
//
// reply_mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the composition UI is for.
// payload: No payload.
//
// The application component should send this message to the UI component to
// hide the external composition UI, if it shows composition text inline.
// The UI component should remember the visible state of the composition UI upon
// receiving this message until the input context loses focus or receives a
// MSG_SHOW_COMPOSITION_UI
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0201, HIDE_COMPOSITION_UI)

// Component(App) -> Component(UI)
// Asks the UI component to show the candidate list UI.
//
// reply_mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the candidate list UI is for.
// payload: No payload.
//
// The application component should send this message to the UI component to
// show the external candidate list UI, if it cannot show candidate list by
// itself.
// The UI component should remember the visible state of the candidate list UI
// upon receiving this message until the input context loses focus or receives a
// MSG_HIDE_CANDIDATE_LIST_UI
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0202, SHOW_CANDIDATE_LIST_UI)

// Component(App) -> Component(UI)
// Asks the UI component to hide the candidate list UI.
//
// reply_mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the candidate list UI is for.
// payload: No payload.
//
// The application component should send this message to the UI component to
// hide the external candidate list UI, if it shows candidate list by itself.
// The UI component should remember the visible state of the candidate list UI
// upon receiving this message until the input context loses focus or receives a
// MSG_SHOW_CANDIDATE_LIST_UI
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0203, HIDE_CANDIDATE_LIST_UI)

// Component(App) -> Component(UI)
// Asks the UI component to show the toolbar UI.
//
// reply_mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the toolbar UI is for.
// payload: No payload.
//
// The application component should send this message to the UI component to
// show the external toolbar UI.
// The UI component should remember the visible state of the toolbar UI upon
// receiving this message until the input context loses focus or receives a
// MSG_HIDE_TOOLBAR_UI
//
// By default the toolbar UI is visible when the input context gets focused.
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0204, SHOW_TOOLBAR_UI)

// Component(App) -> Component(UI)
// Asks the UI component to hide the toolbar UI.
//
// reply_mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which the toolbar UI is for.
// payload: No payload.
//
// The application component should send this message to the UI component to
// hide the external toolbar UI.
// The UI component should remember the visible state of the toolbar UI upon
// receiving this message until the input context loses focus or receives a
// MSG_SHOW_TOOLBAR_UI
//
// Note: this message only applies to the focused input context.
DECLARE_IPC_MSG(0x0205, HIDE_TOOLBAR_UI)

// Component(IME, App)  -> Components
// (mostly be used in CJK)
// Inform application a conversion status has been changed, like punctuation,
// shape and so on.
//
// reply mode: NO_REPLY
// source: Id of the component sending this message.
// target: kComponentBroadcast
// icid: Id of the input context which has conversion status changed.
// payload:
// 1. boolean(0): true for native mode.
// 2. boolean(1): true for full shape mode, false for half shape mode.
// 3. boolean(2): true for full punctuation mode, false or half punctuation
// mode.
DECLARE_IPC_MSG(0x0206, CONVERSION_MODE_CHANGED)

// Component (IME) -> Components (App, UI)
// Enable/Disable fake inline composition (In windows).
//
// Reply mode: NO_REPLY
// source: Id of the application component sending this message.
// target: kComponentDefault
// icid: Id of the input context which fake inline composition is for.
// payload:
// 1. boolean(0): true for enable and false for disable.
DECLARE_IPC_MSG(0x0207, ENABLE_FAKE_INLINE_COMPOSITION)

//////////////////////////////////////////////////////////////////////////////
// Messages for managing global timers
//////////////////////////////////////////////////////////////////////////////
DECLARE_IPC_MSG(0x0220, SET_TIMER)

DECLARE_IPC_MSG(0x0221, KILL_TIMER)

DECLARE_IPC_MSG(0x0222, NOTIFY_TIMER)

//////////////////////////////////////////////////////////////////////////////
// Miscellaneous messages
//////////////////////////////////////////////////////////////////////////////
DECLARE_IPC_MSG(0x0240, BEEP)

DECLARE_IPC_MSG(0x0241, HUB_SERVER_QUIT)

//////////////////////////////////////////////////////////////////////////////
// Messages for plugin component management.
//////////////////////////////////////////////////////////////////////////////

// Hub or Component -> Component(Plugin Manager),
// Queries the component info of all the plug-in components.
// TODO(synch): support template matching.
//
// reply_mode: NEED_REPLY
// source: kComponentDefault or ID of the component who sends this message.
// target: ID of the plugin manager component.
// icid: kInputContextNone.
//
// Reply message:
// reply_mode: IS_REPLY.
// source: ID of the plugin manager component.
// target: Id of the component sending the original message.
// icid: kInputContextNone.
// payload:
//   component_info: ComponentInfo objects of all matched components.
//
// This message is similar with QUERY_COMPONENT but it retrieves all the
// plug-in components, no matter whether they are registered or not, and
// this message will not include built-in components.
DECLARE_IPC_MSG(0x0260, PLUGIN_QUERY_COMPONENTS)

// Component -> Component(Plugin Manager),
// Starts several plug-in components and attaches it to the hub.
//
// reply_mode: NEED_REPLY.
// source: ID of the component sends the message.
// target: ID of the plugin manager.
// icid: kInputContextNone.
// payload:
//    string: the string ids of the components that need to be started.
//
// ReplyMessage:
// reply_mode: IS_REPLY.
// source: ID of the plugin manager.
// target: ID of the component sends the message.
// icid: kInputContextNone.
// payload: Boolean values indicates whether the corresponding component is
// started.
DECLARE_IPC_MSG(0x0261, PLUGIN_START_COMPONENTS)

// Component -> Component(Plugin Manager)
// Terminates one or more component.
// reply_mode: NO_REPLY or NEED_REPLY.
// source: ID of the component sends the message.
// target: ID of the plugin manager.
// icid: kInputContextNone.
// payload:
//    string: the string ids of the components that need to be terminated.
//
// ReplyMessage:
// reply_mode: IS_REPLY.
// source: ID of the plugin manager.
// target: ID of the component sends the message.
// icid: kInputContextNone.
// payload: Boolean values indicates whether the corresponding component is
// terminated.
DECLARE_IPC_MSG(0x0262, PLUGIN_STOP_COMPONENTS)

// Component -> Component(Plugin Manager)
// Unloads a plug-in (the plugin manager will stop all the component
// in the plugin.)
// reply_mode:NEED_REPLY.
// source: ID of the component sends the message.
// target: ID of the plugin manager.
// icid: kInputContextNone.
// payload:
//    string: the path of the plugin.
//
// Reply message:
// reply_mode: IS_REPLY.
// source: ID of the plugin manager.
// target: ID of the component sends the message.
// icid: kInputContextNone.
// payload: a boolean value indicates whether the plugin is unloaded.
//
// Installer can send this message when updating a plugin.
DECLARE_IPC_MSG(0x0263, PLUGIN_UNLOAD)

// Component -> Component(Plugin Manager)
// Notifies Plugin component manager that one or more plugin is installed.
// The plugin manager should scan the plugin and update the component
// information.
//
// reply_mode: NO_REPLY.
// source: ID of the component sends the message.
// target: ID of the plugin manager.
// icid: kInputContextNone.
// payload: one or more string each contains the path of a plugin file.
// Installer can use this message to notified the plugin manager. The plug-in
// manager will re-start the component terminated by MSG_PLUGIN_UNLOAD and
// send MSG_PLUGIN_COMPONENTS_CHANGED after receiving the message.
DECLARE_IPC_MSG(0x0264, PLUGIN_INSTALLED)

// Component(Plugin Manager) -> Component (Broadcast)
// Notifies other components that some installed component has changed.
//
// reply_mode: NO_REPLY
// source: ID of the plugin manager.
// target: kComponentBroadcast.
// icid: kInputContextNone.
// payload: no payload.
DECLARE_IPC_MSG(0x0265, PLUGIN_CHANGED)

//////////////////////////////////////////////////////////////////////////////
// Messages for Application UI component (A component that handles all UI
// interactions that must be run in the applications process).
//////////////////////////////////////////////////////////////////////////////
// Component(UI) -> Component (Application UI component)
// Let Application UI Component shows a menu. The Application UI will send
// MSG_DO_COMMAND or MSG_DO_CANDIDATE_COMMAND if a menu item is clicked.
//
// reply_mode: NEED_REPLY
// source: Id of UI.
// target: Id of the Application UI component.
// icid: Id of the input context in which user triggers a context menu.
// payload:
//   command_lists: a command list that should be shown in the menu.
//   input_caret: a location hint of the context menu.
// reply message:
// reply_mode: IS_REPLY
// source: Id of Application UI component.
// target: Id of the UI.
// icid: Id of the input context in which user triggers a context menu.
// payload: a string value which is the id of the command which is triggered, an
//     empty id means the user cancel the menu.
DECLARE_IPC_MSG(0x0280, SHOW_MENU)

// Component -> Component(Application UI component)
// Let Appication UI component shows a modal message box, and reply with the
// id of the button user clicked.
// The message box will block the application of the target input context.
//
// reply_mode: NEED_REPLY
// source: Id of component who wants to show a message box
// target: Id of the Application UI component.
// icid: Id of the input context in which the component needs a message box.
// payload:
//   string(0): the utf8 encoded title of the message box.
//   string(1): the utf8 encoded message that will be shown on the message box.
//   int32(0): The set of buttons on the message box. See MessageBoxButtonSet.
//   int32(1): The icon on the message box. See MessageBoxIcon.
// reply message:
// reply_mode: IS_REPLY
// source: Id of Application UI component.
// target: Id of the UI.
// icid: Id of the input context in which the component needs a message box.
// payload:
//   int32(0): The button that the use chooses. See MessageBoxButton.
DECLARE_IPC_MSG(0x281, SHOW_MESSAGE_BOX)

// Component -> Component(Virtual keyboard UI)
// Notifies virtual keyboard UI component how the virtual keyboard layout are
// displayed.
//
// reply_mode: NO_REPLY
// source: Id of component who send the message.
// target: kComponentDefault
// icid: Id of the input context in which the keyboard layout is displayed.
// payload:
//   VirtualKeyboard: a virtual keybaord object that contains the information
//   of virtual keyboard.
//
DECLARE_IPC_MSG(0x0300, SET_KEYBOARD_LAYOUT)

// Component -> Component(Virtual keyboard UI)
// Sent to change the virtual keyboard UI key state - the key pressed and the
// tab should be displayed.
//
// reply_mode: NO_REPLY
// source: Id of component who send the message.
// target: kComponentDefault
// icid: Id of the input context in which the keyboard layout is displayed.
// payload:
//   virtual_key(0 : n): optional, a number of virtual keys needs to be updated.
//   the whole view of keyboard layout should have been set by
//   MSG_SET_KEYBOARD_LAYOUT.
//   boolean(0): optional, true to keep other key states, false to clear, no
//   value provided will be considered as false.
//   boolean(1): optional, True to show the virtual keyboard window, false to
//   hide, omitted to keep the original setting.
//
DECLARE_IPC_MSG(0x0301, CHANGE_KEYBOARD_STATE)

// Component(Virtual keyboard UI) -> Component(Broadcast)
// A broadcast message that notifies the state of virtual keyboard has changed
// by user interaction.
//
// reply_mode: NO_REPLY.
// source: Id of virtual keyboard ui component.
// target: kComponentBroadcast
// icid: the input context id where keyboard state changed.
// payload:
//  key_event: a key event generated by user mouse event on the virtual
//  keyboard.
//
DECLARE_IPC_MSG(0x0302, VIRTUAL_KEYBOARD_STATE_CHANGED)

//////////////////////////////////////////////////////////////////////////////
// Mark of the end of predefined messages.
// Do NOT forget to increase it when adding more predefined messages.
//////////////////////////////////////////////////////////////////////////////
DECLARE_IPC_MSG(0x0303, END_OF_PREDEFINED_MESSAGE)


//////////////////////////////////////////////////////////////////////////////
// Message range reserved for internal usage.
//////////////////////////////////////////////////////////////////////////////
DECLARE_IPC_MSG(0x8000, SYSTEM_RESERVED_START)
DECLARE_IPC_MSG(0xFFFF, SYSTEM_RESERVED_END)

//////////////////////////////////////////////////////////////////////////////
// Message range for 3rd parties.
//////////////////////////////////////////////////////////////////////////////
DECLARE_IPC_MSG(0x10000, USER_DEFINED_START)
