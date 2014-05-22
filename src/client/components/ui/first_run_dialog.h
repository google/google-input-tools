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
#ifndef GOOPY_COMPONENTS_UI_FIRST_RUN_DIALOG_H_
#define GOOPY_COMPONENTS_UI_FIRST_RUN_DIALOG_H_

#include "base/resource_bundle.h"
#include "common/atl.h"
#include "ipc/settings_client.h"
#include "resources/about_dialog_resource.h"


namespace ime_goopy {
namespace components {

class FirstRunDialog : public CDialogImpl<FirstRunDialog> {
 public:
  enum { IDD = IDD_FIRST_RUN };
  BEGIN_MSG_MAP(FirstRunDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnClose)
    COMMAND_ID_HANDLER(IDNO, OnClose)
    COMMAND_ID_HANDLER(IDCANCEL, OnClose)
    COMMAND_ID_HANDLER(IDC_DETAIL, OnDetail)
  END_MSG_MAP()
  explicit FirstRunDialog(ipc::SettingsClient* settings)
    : settings_(settings) {
    DCHECK(settings_);
  }

  LRESULT OnInitDialog(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
  LRESULT OnClose(WORD nofity, WORD id, HWND hwnd, BOOL& handled);
  LRESULT OnDetail(WORD nofity, WORD id, HWND hwnd, BOOL& handled);

 private:
  CHyperLink detail_;
  ipc::SettingsClient* settings_;
  DISALLOW_COPY_AND_ASSIGN(FirstRunDialog);
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_UI_FIRST_RUN_DIALOG_H_
