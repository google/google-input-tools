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

// This file contains the declaration of AppSensor class. This class
// implements the core funtionality of application detecting. It tries
// to match the file information of the current process module against
// the information in HandlerManager. If a match is found, then
// the messaging handling will be delegated to the corresponding
// Handler.

#ifndef GOOPY_APPSENSORAPI_APPSENSOR_H__
#define GOOPY_APPSENSORAPI_APPSENSOR_H__
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "appsensorapi/versionreader.h"

namespace ime_goopy {
class Handler;
class HandlerManager;
class VersionReader;

class AppSensor {
 public:
  AppSensor();

  // Obtains the process module and store in this class.
  BOOL Init();

  // Handles Windows Message. Refer to the Windows API for more details.
  LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // Handles other command for general propose.
  BOOL HandleCommand(uint32 command, void *data);

  // Registers a handler in the sensitive framework.
  BOOL RegisterHandler(const Handler *handler);

  // Returns the instance of handler manager.
  HandlerManager *handler_manager() const {
    return handler_manager_.get();
  }
 private:
  // Returns the file name of current process module.
  BOOL GetActiveProcessFilename(DWORD buff_size, TCHAR* const filename);

  // Stores the HandlerManager to manage all known handlers.
  scoped_ptr<HandlerManager> handler_manager_;

  // Be used to store the file information of the current process module.
  VersionInfo version_info_;

  // Flag to indicate if the class is initialized. It is used to avoid
  // receiving message from system before initializing.
  BOOL is_init_;

  DISALLOW_EVIL_CONSTRUCTORS(AppSensor);
};
}

#endif  // GOOPY_APPSENSORAPI_APPSENSOR_H__
