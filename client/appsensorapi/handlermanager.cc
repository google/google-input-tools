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

// This file contains the definition of HandlerManager class, which
// manages all available handlers which are used to identify the handler
// running process. The handlers can be added or removed from the manager
// class. It also provides delegated methods to invoke the customized rules
// when a matching handler is found.

#include "appsensorapi/handlermanager.h"
#include "base/logging.h"
#include "appsensorapi/handler.h"
#include "appsensorapi/versionreader.h"

namespace ime_goopy {
bool HandlerManager::AddHandler(const Handler* handler) {
  return handler_set_.insert(handler).second;
}

bool HandlerManager::RemoveHandler(const Handler* handler) {
  return handler_set_.erase(handler) > 0;
}

const Handler *HandlerManager::GetHandlerBySize(uint64 size) {
  // Assign default value to last modified time.
  VersionInfo info;
  info.modified_time = kUnspecifiedFileTime;
  info.file_size = size;
  // Delegate to GetHandlerByInfo
  return GetHandlerByInfo(info);
}

const Handler *HandlerManager::GetHandlerByInfo(const VersionInfo &target) {
  // Iterate all handlers in the hash set.
  for (HandlerSet::iterator handler_set_iter = handler_set_.begin();
       handler_set_iter != handler_set_.end();
       ++handler_set_iter) {
    const VersionInfo *handler = (*handler_set_iter)->version_info();

    // Check if the size or the last modified time doesn't match.
    if (handler->file_size != kUnspecifiedFileSize &&
        target.file_size != handler->file_size)
      continue;
    if (handler->modified_time != kUnspecifiedFileTime &&
        target.modified_time != handler->modified_time)
      continue;

    // A flag to indicate if all conditions have been met.
    bool is_match = true;

    // Iterate through all items in the version map.
    for (FileInfoMap::const_iterator file_info_iter =
             handler->file_info.begin();
         file_info_iter != handler->file_info.end();
         ++file_info_iter) {
      FileInfoMap::const_iterator find_result =
          target.file_info.find(file_info_iter->first);
      // Check if any item in the current handler doesn't match with
      // the realistic version info.
      if (find_result == target.file_info.end() ||
          find_result->second != file_info_iter->second) {
        is_match = false;
        break;
      }
    }
    if (!is_match) continue;
    // Find a match, assign the pointer to the return handler's pointer.
    return *handler_set_iter;
  }
  return NULL;
}

bool HandlerManager::HandleCommand(const VersionInfo &version_info,
                                   uint32 command,
                                   void *data) {
  const Handler *handler = GetHandlerByInfo(version_info);
  if (handler != NULL) {
    return handler->HandleCommand(command, data);
  }
  return false;
}

LRESULT HandlerManager::HandleMessage(const VersionInfo &version_info,
                                      HWND hwnd,
                                      UINT message,
                                      WPARAM wparam,
                                      LPARAM lparam) {
  const Handler *handler = GetHandlerByInfo(version_info);
  if (handler != NULL) {
    return handler->HandleMessage(hwnd, message, wparam, lparam);
  }
  return false;
}

}  // namespace ime_goopy
