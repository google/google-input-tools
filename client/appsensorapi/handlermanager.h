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

// This file contains the declaration of HandlerManager class. This class
// manages all available handlers which are used to identify the current
// running process. The handlers can be added or removed from the manager
// class. It also provides delegated methods to nivoke the customized rules
// when a matching handler is found.

#ifndef GOOPY_APPSENSORAPI_HANDLERMANAGER_H__
#define GOOPY_APPSENSORAPI_HANDLERMANAGER_H__

#include <windows.h>
#include <hash_set>
#include "base/basictypes.h"

namespace ime_goopy {

struct VersionInfo;
class Handler;

class HandlerManager {
 public:
  HandlerManager() { }

  // Adds one handler under the management.
  //
  // Note: The owner of the handler should ensure this handler to be valid
  // during the life time of this class.
  //
  // handler: The handler to add.
  // retval: True if success, false if the handler is already added.
  bool AddHandler(const Handler *handler);

  // Removes specified handler from the management set.
  //
  // Note: If a handler should be removed during the life time of the manager
  // class, it should be removed from the manager before releasing the
  // allocated memory. Otherwise the result is undetermined.
  //
  // handler: The handler to remove.
  // retval: True if success, false if the handler is not previously in
  //         the set.
  bool RemoveHandler(const Handler *handler);

  // Returns a pointer to the handler matched by size.
  //
  // size: The size to see if any fingprints match with.
  // returned_handler: The handler's pointer to be returned.
  // retval: The handler to the application which is the required size.
  const Handler *GetHandlerBySize(uint64 size);

  // Returns a pointer to the handler matched by specified version info.
  //
  // The version info can be partly filled, i.e., only some fields are
  // assigned as non-zero, or the version map contains a subset of all
  // available blocks.
  //
  // A value of zero is treated as the default value of any integer fields.
  // It would be considered as matching only if all fields containing
  // non-default values are equal.
  //
  // condition: The version info to see if any handlers match with.
  // returned_handler: The handler's pointer to be returned.
  // retval: The handler to the application which meets with the specific
  // condition.
  const Handler *GetHandlerByInfo(const VersionInfo &condition);

  // Returns the number of handlers added in this class.
  int GetCount() { return handler_set_.size(); }

  // Dispatches the given command to the handler that matches the given version
  // information, if any.
  //
  // condition: The version info to see if any handlers match with.
  // command: The identify of user-defined command.
  // data: Any additional data to the command.
  // retval: True if a match is found, false otherwise.
  bool HandleCommand(const VersionInfo &condition,
                     uint32 command,
                     void *data);

  // Dispatches the given message to the handler that matches the given
  // version, if any.
  // See the Windows API for reference.
  //
  // condition: The version info to see if any handlers match with.
  // hwnd: The handle of the window the message is sent to.
  // message: The message id.
  // wparam: The parameter according to the concrete message.
  // lparam: The parameter according to the concrete message.
  // retval: The processing result.
  LRESULT HandleMessage(const VersionInfo &condition,
                        HWND hwnd,
                        UINT message,
                        WPARAM wparam,
                        LPARAM lparam);
 private:
  // Define a hash set to store all pointers to handler instances.
  typedef hash_set<const Handler*> HandlerSet;

  // Stores all pointers to the handlers for management.
  HandlerSet handler_set_;
  DISALLOW_EVIL_CONSTRUCTORS(HandlerManager);
};

}  // namespace ime_goopy

#endif  // GOOPY_APPSENSORAPI_HANDLERMANAGER_H__
