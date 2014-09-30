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
#ifndef GOOPY_APPSENSORAPI_HANDLERS_CROSS_FIRE_HANDLER_H__
#define GOOPY_APPSENSORAPI_HANDLERS_CROSS_FIRE_HANDLER_H__

#include "base/logging.h"
#include "appsensorapi/common.h"
#include "appsensorapi/handler.h"
#include "common/shellutils.h"

namespace ime_goopy {
namespace handlers {

class CrossFireHandler : public ime_goopy::Handler {
 public:
  CrossFireHandler() {
    version_info_.file_info[FileInfoKey::kOriginalFilename] =
        TEXT("Client.EXE");
  }
  // Invokes any user-defined commands via this method.
  // data should be a pointer of CrossFireHandlerData.
  BOOL HandleCommand(uint32 command, void *data) const {
    switch (command) {
      case CMD_SHOULD_USE_FALLBACK_UI: {
        // See bug 5517252.
        // The game "CrossFire" on windows XP will refresh the screen frequently
        // so that the layered window may flash. So we use default fallback UI
        // instead of skin UI for compatibility.
        UI_Selection_Data* ui_selection_data =
            reinterpret_cast<UI_Selection_Data*>(data);
        DCHECK(ui_selection_data);
        ui_selection_data->should_use_default_ui = false;
        wchar_t exe_path[MAX_PATH] = {0};
        wchar_t exe_name[MAX_PATH] = {0};
        ::GetModuleFileName(NULL, exe_path, MAX_PATH);
        _wsplitpath(exe_path, NULL, NULL, exe_name, NULL);
        // Check if the exe file name match the game because the file info check
        // is too weak. We can't check the game window name because the
        // UI_Window will be created before the "CrossFire" window is created
        // if set as default IME.
        if (_wcsicmp(exe_name, L"crossfire") == 0) {
          if (!ShellUtils::CheckWindowsVista()) {
            ui_selection_data->should_use_default_ui = true;
          }
        } else {
          return FALSE;
        }
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
#endif  // GOOPY_APPSENSORAPI_HANDLERS_CROSS_FIRE_HANDLER_H__
