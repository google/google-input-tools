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

#include "components/ui/skin_ui_component.h"

#include <windows.h>

#include "base/string_utils_win.h"
#include "common/app_const.h"
#include "common/shellutils.h"
#include "components/ui/about_dialog.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "third_party/google_gadgets_for_linux/ggadget/math_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"
#include "third_party/google_gadgets_for_linux/ggadget/view_host_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/menu_builder.h"

namespace ime_goopy {
namespace components {
namespace {

static const int kStartMenuID = 0x100;

void LaunchAboutDialog(const char* menu_text, ipc::SettingsClient* settings) {
  AboutDialog dialog(settings);
  dialog.DoModal(::GetForegroundWindow(), 0);
}

void LaunchHelp(const char* menu_text) {
  ::MessageBox(NULL, L"Help", L"Help", MB_OK);
}

RECT GetViewRect(const ggadget::View* view) {
  RECT rect = { 0 };
  if (!view || !view->GetViewHost())
    return rect;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  view->GetViewHost()->GetWindowPosition(&x, &y);
  view->GetViewHost()->GetWindowSize(&w, &h);
  rect.left = x;
  rect.right = x + w;
  rect.top = y;
  rect.bottom = y + h;
  return rect;
}
} // namespace

void SkinUIComponent::ConstructIMEMenu(ggadget::MenuInterface* menu) {
  tool_bar_->AddImeCommandListToMenuInterface(menu);
  // TODO(synch): Add Skin specific menu items.
  menu->AddItem(NULL, 0, NULL, NULL,  // A seperator.
                ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
  std::wstring text = L"about";
  menu->AddItem(WideToUtf8(text).c_str(), 0, NULL,
                ggadget::NewSlot(LaunchAboutDialog, settings_),
                ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
  text = L"help";
  menu->AddItem(WideToUtf8(text).c_str(), 0, NULL,
                ggadget::NewSlot(LaunchHelp),
                ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
}

bool SkinUIComponent::OnShowContextMenu(ggadget::MenuInterface* menu) {
  DCHECK(menu);
  ggadget::win32::MenuBuilder* menu_builder =
      ggadget::down_cast<ggadget::win32::MenuBuilder*>(menu);
  int item_count = menu_builder->GetItemCount();
  ipc::proto::CommandList command_list;
  menu_builder->PreBuildMenu(kStartMenuID);
  SkinUIComponentUtils::MenuInterfaceToCommandList(menu_builder, &command_list);
  ipc::proto::Rect rect;
  ggadget::Rectangle rectangle = menu_builder->GetPositionHint();
  rect.set_x(rectangle.x);
  rect.set_y(rectangle.y);
  rect.set_width(rectangle.w);
  rect.set_height(rectangle.h);
  if (ShellUtils::CheckWindows8()) {
    // On windows8, the composing window and status windows are shown in
    // ipc console process with ui_access=true, and the menu will be shown in
    // the application process, so the composing window or status bar will
    // cover the menu(because windows in process with ui_access=true will
    // always be shown above windows in process with ui_access=false), so we
    // change the hint rect to the corresponding view to avoid overlapping.
    ::RECT composing_rect = GetViewRect(skin_->GetComposingView());
    ::RECT status_rect = GetViewRect(skin_->GetMainView());
    ::RECT hint_rect = { 0 };
    hint_rect.left = rect.x();
    hint_rect.right = rect.x() + rect.width();
    hint_rect.top = rect.y();
    hint_rect.bottom = rect.y() + rect.height();
    if (hint_rect.right == hint_rect.left)
      hint_rect.right++;
    if (hint_rect.top == hint_rect.bottom)
      hint_rect.bottom++;
    ::RECT tmp = { 0 };
    if (::IntersectRect(&tmp, &composing_rect, &hint_rect)) {
      hint_rect = composing_rect;
    } else if (::IntersectRect(&tmp, &status_rect, &hint_rect)) {
      hint_rect = status_rect;
    }
    rect.set_x(hint_rect.left);
    rect.set_y(hint_rect.top);
    rect.set_width(hint_rect.right - hint_rect.left);
    rect.set_height(hint_rect.bottom - hint_rect.top);
  }
  std::string id = ShowMenu(rect, command_list);
  if (!id.empty())
    SkinUIComponentUtils::ExecuteMenuCommand(menu_builder, id);
  return false;
}

void SkinUIComponent::ShowFirstRun() {
  // TODO: implement this.
}

}  // namespace components
}  // namespace ime_goopy
