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
//
// This class contains the definition of default behavior of Handler
// class. This class has a message processing method and a invoke method for
// general propose. The inherit classes can override these method to provide
// their own handling logic.
//
#ifndef GOOPY_APPSENSORAPI_HANDLER_H__
#define GOOPY_APPSENSORAPI_HANDLER_H__
#include "base/scoped_ptr.h"
#include "appsensorapi/versionreader.h"

namespace ime_goopy {
// Defines the default value of file_size and modified_time. The handler
// manager should ignore this field if the value equals to the default value.
extern const size_t kUnspecifiedFileSize;
extern const uint64 kUnspecifiedFileTime;

class Handler {
 public:
  // Initializes the VersionInfo structure by default values.
  // An inherit class should initialize the version_info_ variable in this
  // constructor with proper value to identify the application.
  Handler();

  // Uses the virtual deconstructor here to prevent this class from being
  // instantiated directly.
  virtual ~Handler() = 0;

  // Processes windows message. The meaning of its definition and return value
  // consists with the Windows API. In this implementation, the method always
  // returns false to indicate that the message is not processed and need futher
  // processing.
  virtual LRESULT HandleMessage(HWND hwnd, UINT message,
                                WPARAM wparam, LPARAM lparam) const;
  // Invokes any user-defined commands via this method. This method takes only
  // an integer and a void-type pointer. Any other commands should be able to
  // convert into this general representation.
  //
  // The return value indicates whether the operation is success or not.
  //
  // Here is an example to override this method for multiple commands.
  //
  //   MyHandler::HandleCommand(uint32 command, void *data) {
  //     switch(command) {
  //     case CMD_ADD_NEW_WORD:
  //       Dictionary *dict = (Dictionary *) data;
  //       dict->AddWord("Henry Ou");
  //       break;
  //     case CMD_ENABLE_IME_FOR_PROCESS:
  //       HKL current_layout = GetKeyboardLayout(0);
  //       ActiveateKeyboardLayout(current_layout, KLF_SETFORPROCESS);
  //       break;
  //     default:
  //       return FALSE;
  //     }
  //     return TRUE;
  //   }
  //
  virtual BOOL HandleCommand(uint32 command, void *data) const;

  // Returns the VersionInfo stored in this class.
  const VersionInfo *version_info() const { return &version_info_; }

 protected:
  // This variable stores the version info of specified condition. The condition
  // will be compared to the realistic environment by HandlerManager.
  // If they match, the methods in this class will be emitted.
  VersionInfo version_info_;
  DISALLOW_EVIL_CONSTRUCTORS(Handler);
};
}
#endif  // GOOPY_APPSENSORAPI_HANDLER_H__
