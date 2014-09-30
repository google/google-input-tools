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

// A set of callback functions needed by the plugin component, which are
// implemented by the wrapper.
#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_CALLBACKS_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_CALLBACKS_H_

#include "base/logging.h"
#include "components/plugin_wrapper/plugin_wrapper.h"

extern "C" {
typedef bool (API_CALL* SendCallback)(
    ComponentOwner owner,
    const char* message_buf, int length,
    uint32* serial);
typedef bool (API_CALL* SendWithReplyCallback)(
    ComponentOwner owner,
    const char* message_buf, int length,
    int time_out,
    char** reply_buf, int* reply_length);
typedef void (API_CALL* PauseMessageHandlingCallback)(ComponentOwner owner);
typedef void (API_CALL* ResumeMessageHandlingCallback)(ComponentOwner owner);
typedef bool (API_CALL* RemoveComponentCallback)(ComponentOwner owner,
                                                 ComponentInstance instance);
typedef void (API_CALL* FreeBufferCallback)(char* buffer);
};

// A set of callback functions that will be called in the plugin.
struct ComponentCallbacks {
  ComponentOwner owner;
  // Sends a message to the PluginComponentStub.
  SendCallback send;
  // Sends a message to the PluginComponentStub and waits for reply.
  SendWithReplyCallback send_with_reply;
  PauseMessageHandlingCallback pause_message_handling;
  ResumeMessageHandlingCallback resume_message_handling;
  // Removes the component from the host.
  RemoveComponentCallback remove_component;
  // Frees a buffer created outside the plugin.
  FreeBufferCallback free_buffer;
};

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_CALLBACKS_H_
