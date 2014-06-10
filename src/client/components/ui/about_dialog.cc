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
#include <atlbase.h>
#include <atlwin.h>
#include "components/ui/about_dialog.h"

#include "base/logging.h"
#include "components/ui/skin_ui_component_utils.h"
#include "resources/about_dialog_resource.h"

namespace ime_goopy {
namespace components {
LRESULT AboutDialog::OnInitDialog(UINT msg, WPARAM wparam, LPARAM lparam,
                                  BOOL& handled) {
  // Reinforce the focus to stay in the popup dialog, as some applications
  // like PaoPaoTang will make the popup to lose focus.
  SetFocus();
  return 0;
}

LRESULT AboutDialog::OnClose(WORD nofity, WORD id, HWND hwnd, BOOL& handled) {
  bool checked = IsDlgButtonChecked(IDC_CHECK_USER_METRICS);
  EndDialog(id);
  return 0;
}

}  // namespace components
}  // namespace ime_goopy
