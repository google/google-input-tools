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

// A set of exported functions of the plugin dynamic library, which are
// implemented by the wrapper.
#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_EXPORTS_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_EXPORTS_H_

#include "components/plugin_wrapper/callbacks.h"
#include "components/plugin_wrapper/plugin_wrapper.h"

extern "C" {
// Lists the components in the plugin. The ComponentInfo of the components will
// be stored in a MessagePayload object and serialized to |buffer|. The |buffer|
// must be deleted by calling FreeBufferProc.
// Returns the count of ComponentInfo objects listed in |buffer|.
typedef int (API_CALL* ListComponentsProc)(
    char** buffer, int* size);
static const char ListComponentsProcName[] = "ListComponents";

// Creates an instance of the component identified by |id|.
typedef ComponentInstance (API_CALL* CreateInstanceProc)(
    ComponentCallbacks callbacks,
    const char* id);
static const char kCreateInstanceProcName[] = "CreateInstance";

// Destroys the component |instance|.
typedef void (API_CALL* DestroyInstanceProc)(ComponentInstance instance);
static const char kDestroyInstanceProcName[] = "DestroyInstance";

// Gets the ipc::proto::ComponentInfo of the component |instance|, this function
// will creates a byte buffer and serialize the protobuf in the |buffer|.
// |buffer| must be free by calling FreeBufferProc.
typedef void (API_CALL* GetInfoProc)(ComponentInstance instance,
                                     char** buffer,
                                     int* buffer_length);
static const char kGetInfoProcName[] = "GetInfo";

// Notifies the component instance that it has been registered in the ipc hub.
typedef void (API_CALL* RegisteredProc)(ComponentInstance instance, int id);
static const char kRegisteredProcName[] = "Registered";

// Notifies the component instance that it has been deregistered in the ipc hub.
typedef void (API_CALL* DeregisteredProc)(ComponentInstance instance);
static const char kDeregisteredProcName[] = "Deregistered";

// Sends a message to the component |instance|. The message should be serialized
// to |message_buffer|.
typedef void (API_CALL* HandleMessageProc)(ComponentInstance instance,
                                           const char* message_buffer,
                                           int buffer_length);
static const char kHandleMessageProcName[] = "HandleMessage";

// Frees the buffer created in the DLL.
typedef void (API_CALL* FreeBufferProc)(char* buffer);
static const char kFreeBufferProcName[] = "FreeBuffer";
};

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_EXPORTS_H_
