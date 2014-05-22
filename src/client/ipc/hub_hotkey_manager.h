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

#ifndef GOOPY_IPC_HUB_HOTKEY_MANAGER_H_
#define GOOPY_IPC_HUB_HOTKEY_MANAGER_H_
#pragma once

#include <map>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

class Component;
class HotkeyList;
class HubImpl;
class InputContext;

// A built-in component for handling hotkey related messages.
class HubHotkeyManager : public Hub::Connector {
 public:
  typedef proto::Message Message;

  explicit HubHotkeyManager(HubImpl* hub);
  ~HubHotkeyManager();

  // Implementation of Hub::Connector interface
  virtual bool Send(Message* message) OVERRIDE;

 private:
  // A structure to hold necessary information of a pending keyboard event sent
  // by an application component. We use these information to send reply message
  // back to the application.
  struct PendingKeyEvent {
    // Id of the application component, which sent the MSG_SEND_KEY_EVENT
    // message.
    uint32 app_id;

    // Serial number of the message.
    uint32 serial;
  };

  typedef std::map<uint32, PendingKeyEvent> PendingKeyEventMap;

  // A structure to hold data related to hotkey processing of an input context.
  struct InputContextData {
    proto::KeyEvent previous_key_event;

    // A map holding information of all pending keyboard events of an input
    // context. Key is the serial number of the MSG_PROCESS_KEY_EVENT message
    // sent to the input method component.
    PendingKeyEventMap pending_key_events;
  };

  // A map type to store hotkey processing related data of all input contexts.
  // Key is input context id.
  typedef std::map<uint32, InputContextData> InputContextDataMap;

  bool OnMsgInputContextGotFocus(Component* source, Message* message);
  bool OnMsgActiveConsumerChanged(Component* source, Message* message);
  bool OnMsgAttachToInputContext(Component* source, Message* message);
  bool OnMsgDetachedFromInputContext(Component* source, Message* message);

  bool OnMsgSendKeyEvent(Component* source, Message* message);
  bool OnMsgAddHotkeyList(Component* source, Message* message);
  bool OnMsgRemoveHotkeyList(Component* source, Message* message);
  bool OnMsgCheckHotkeyConflict(Component* source, Message* message);
  bool OnMsgActivateHotkeyList(Component* source, Message* message);
  bool OnMsgDeactivateHotkeyList(Component* source, Message* message);
  bool OnMsgQueryActiveHotkeyList(Component* source, Message* message);

  bool OnMsgProcessKeyEventReply(Component* source, Message* message);


  // Helper functions

  // Gets the InputContextData object for a specified input context.
  // If |create_new| is true, then a new object will be created when necessary.
  InputContextData* GetInputContextData(uint32 icid, bool create_new);

  // Creates a new message with given information. Serial number will be set
  // automatically for non-reply messages.
  Message* NewMessage(uint32 type,
                      Message::ReplyMode reply_mode,
                      uint32 target,
                      uint32 icid);

  // Replies a pending key event sent from a specified application component.
  void ReplyPendingKeyEvent(
      uint32 app_id, uint32 icid, uint32 serial, bool result);

  // Discards all pending key events of a specified input context by replying
  // them with false return value.
  void DiscardAllPendingKeyEvents(uint32 icid);

  // Deletes all data of a specified input context.
  void DeleteInputContextData(uint32 icid);

  // Matches a key event in all active hotkey lists of a specified input
  // context and the global input context.
  // If any hotkey is matched, then messages associated to the hotkey will be
  // dispatched and true will be returned. Otherwise false will be returned.
  bool MatchHotkey(InputContext* input_context,
                   InputContextData* data,
                   const proto::KeyEvent& key);

  // Matches a key event in a set of HotkeyList objects.
  // If any hotkey is matched, then messages associated to the hotkey will be
  // dispatched and true will be returned. Otherwise false will be returned.
  bool MatchHotkeyInHotkeyLists(
      const std::vector<const HotkeyList*>& hotkey_lists,
      const proto::KeyEvent& previous_key,
      const proto::KeyEvent& current_key);

  // Dispatches messages associated to a specified hotkey.
  void DispatchHotkeyMessages(uint32 owner_id, const proto::Hotkey& hotkey);

  // The Component id representing the hotkey manager itself.
  uint32 self_;

  // Weak pointer to our owner.
  HubImpl* hub_;

  InputContextDataMap input_context_data_;

  // Counter for generating serial numbers of outgoing messages.
  uint32 message_serial_;

  DISALLOW_COPY_AND_ASSIGN(HubHotkeyManager);
};


}  // namespace hub
}  // namespace ipc


#endif  // GOOPY_IPC_HUB_HOTKEY_MANAGER_H_
