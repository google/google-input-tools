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

#ifndef GOOPY_IPC_HUB_INPUT_METHOD_MANAGER_H_
#define GOOPY_IPC_HUB_INPUT_METHOD_MANAGER_H_
#pragma once

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"

namespace ipc {
namespace hub {

class Component;
class HubImpl;
class InputContext;

// A built-in component for handling input context related messages.
class HubInputMethodManager : public Hub::Connector {
 public:
  explicit HubInputMethodManager(HubImpl* hub);
  virtual ~HubInputMethodManager();

  // Implementation of Hub::Connector interface
  virtual bool Send(proto::Message* message);

 private:
  class InputMethodSwitchingData;
  typedef std::map<uint32 /*icid*/, InputMethodSwitchingData*>
          InputMethodSwitchingDataMap;
  // Handlers for broadcast messages that we can consume.
  bool OnMsgComponentCreated(proto::Message* message);
  bool OnMsgComponentDeleted(proto::Message* message);
  bool OnMsgInputContextCreated(proto::Message* message);
  bool OnMsgInputContextDeleted(proto::Message* message);
  bool OnMsgComponentAttached(proto::Message* message);
  bool OnMsgActiveConsumerChanged(proto::Message* message);

  // Handlers for other messages that we can consume.
  bool OnMsgListInputMethods(Component* source, proto::Message* message);
  bool OnMsgSwitchToInputMethod(Component* source, proto::Message* message);
  bool OnMsgSwitchToNextInputMethodInList(Component* source,
                                          proto::Message* message);
  bool OnMsgSwitchToPreviousInputMethod(Component* source,
                                        proto::Message* message);
  bool OnMsgQueryActiveInputMethod(Component* source, proto::Message* message);

  // Handlers for replying messages.
  bool OnMsgCancelCompositionReply(Component* source, proto::Message* message);

  // Gets the current input method component which is really being used by a
  // specified input context.
  Component* GetCurrentInputMethod(const InputContext* ic) const;

  // Gets the previous input method component used by a specified input context.
  Component* GetPreviousInputMethod(const InputContext* ic) const;

  // Gets the next input method in the input method list suitable for this input
  // context.
  Component* GetNextInputMethodInList(const InputContext* ic) const;

  // Sets the current input method component of a specified input context.
  // It records the current input method as previous one and validates the new
  // input method and calls ActivateInputMethod() to activate it.
  bool SwitchToInputMethod(InputContext* ic, Component* input_method);

  bool SwitchToNextInputMethodInList(InputContext* ic);

  bool SwitchToPreviousInputMethod(InputContext* ic);

  // Checks if a given component is an input method or not.
  bool IsInputMethod(Component* component) const;

  // Validates an input method to see if it's suitable for a specified input
  // context.
  bool ValidateInputMethod(Component* input_method,
                           const InputContext* ic) const;

  // Creates an input method switching data indicates that the input context
  // |ic| is switching input method to |new_input_method|.
  void CreateSwitchingData(InputContext* ic, Component* new_input_method);

  // Updates the input method switching data.
  // This function will match the switching data with |icid| and |component_id|
  // and update the state with |state_mask| if matched. And if the switching
  // process is finished, the switching data will be deleted.
  void UpdateSwitchingData(uint32 icid, uint32 component_id, int state_mask);

  // Deletes the input context switching data. The cached messages will be
  // discarded if |discard_cache| is true.
  void DeleteSwitchingData(uint32 icid, bool discard_cache);

  bool SwitchToInputMethodAfterCancelComposition(InputContext* ic,
                                                 Component* input_method);

  // The Component object representing the input context manager itself.
  Component* self_;

  // Weak pointer to our owner.
  HubImpl* hub_;

  // Stores ids of all running input method components. It should be sorted
  // ascendingly.
  std::vector<uint32> all_input_methods_;

  // Whether or not to use the same global input method component for all input
  // contexts.
  bool use_global_input_method_;

  // Stores the current input method component used by each input context.
  std::map<uint32, std::string> current_input_methods_;

  // Stores the previous input method component used by each input context.
  std::map<uint32, std::string> previous_input_methods_;

  InputMethodSwitchingDataMap switching_data_;

  DISALLOW_COPY_AND_ASSIGN(HubInputMethodManager);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_INPUT_METHOD_MANAGER_H_
