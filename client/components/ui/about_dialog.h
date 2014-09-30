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
#ifndef GOOPY_COMPONENTS_UI_ABOUT_DIALOG_H_
#define GOOPY_COMPONENTS_UI_ABOUT_DIALOG_H_

#include <atlbase.h>
#include <atlwin.h>

#include "ipc/settings_client.h"
#include "resources/about_dialog_resource.h"


namespace ime_goopy {
namespace components {

class AboutDialog : public CDialogImpl<AboutDialog> {
 public:
  enum { IDD = IDD_ABOUT };
  BEGIN_MSG_MAP(AboutDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnClose)
    COMMAND_ID_HANDLER(IDCANCEL, OnClose)
  END_MSG_MAP()
  explicit AboutDialog(ipc::SettingsClient* settings)
      : settings_(settings) {
    DCHECK(settings_);
  }

  LRESULT OnInitDialog(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
  LRESULT OnClose(WORD nofity, WORD id, HWND hwnd, BOOL& handled);

 private:
  ipc::SettingsClient* settings_;
  DISALLOW_COPY_AND_ASSIGN(AboutDialog);
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_UI_ABOUT_DIALOG_H_
