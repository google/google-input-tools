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
#ifndef GOOPY_APPSENSORAPI_HANDLERS_WOW_H__
#define GOOPY_APPSENSORAPI_HANDLERS_WOW_H__

#include "appsensorapi/common.h"
#include "appsensorapi/handlers/common_handlers.h"

namespace ime_goopy {
namespace handlers {

class WowHandler : public NoCandidateWindowHandler {
 public:
  WowHandler() {
    version_info_.file_info[FileInfoKey::kProductName] =
        TEXT("World of Warcraft");
  }
  BOOL HandleCommand(uint32 command, void *data) const {
    switch(command) {
      case CMD_SHOULD_ASSEMBLE_COMPOSITION: {
        // See bug 4597941.
        // Goopy IME's UI design is to display caret selected candidate in line
        // while using windows tsf framework. But the application
        // "World of Warcraft" uses its own UI and gets the compositon string
        // from system, which causes user can not see the raw composition that
        // was inputed. So assembling composition operation in the application
        // "World of Warcraft" should not be processed even the tsf framework
        // is used.
        bool* should_assemble_composition = static_cast<bool*>(data);
        *should_assemble_composition = false;
        break;
      }
      default: {
        return FALSE;
      }
    }
    return TRUE;
  }
};
}  // namespace handlers
}  // namespace ime_goopy
#endif  // GOOPY_APPSENSORAPI_HANDLERS_WOW_H__
