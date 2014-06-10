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
#include "components/ui/skin_ui_component_utils.h"

#include <atlimage.h>
#include <shlwapi.h>
#include <windows.h>

#include "base/logging.h"
#include "base/stringprintf.h"
#pragma push_macro("LOG")
#pragma push_macro("DLOG")
#include "third_party/google_gadgets_for_linux/ggadget/view_host_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/menu_builder.h"
#pragma pop_macro("DLOG")
#pragma pop_macro("LOG")

namespace ime_goopy {
namespace components {
namespace {

void SaveBitmapToString(HBITMAP bmp, std::string* buffer) {
  DCHECK(buffer);
  CImage image;
  image.Attach(bmp, CImage::DIBOR_TOPDOWN);
  image.SetHasAlphaChannel(true);
  IStream* stream = NULL;
  if (::CreateStreamOnHGlobal(0, TRUE, &stream) != S_OK) {
    stream->Release();
    buffer->clear();
    return;
  }
  image.Save(stream, Gdiplus::ImageFormatPNG);
  image.Detach();
  STATSTG stat = {0};
  stream->Stat(&stat, STATFLAG_NONAME);
  int size = stat.cbSize.LowPart;  // LowPart is enough for a icon.
  char* buf = new char[size];
  LARGE_INTEGER zero = {0};
  stream->Seek(zero, STREAM_SEEK_SET, NULL);
  ULONG size_read = 0;
  stream->Read(buf, size, &size_read);
  DCHECK_EQ(size_read, size);
  stream->Release();
  buffer->assign(buf, size);
}

static const char kMenuCommandPrefix[] = "menu_command.";

}  // namespace

void SkinUIComponentUtils::MenuInterfaceToCommandList(
    const ggadget::win32::MenuBuilder* menu,
    ipc::proto::CommandList* command_list) {
  for (int i = 0; i < menu->GetItemCount(); ++i) {
    int style = 0;
    std::string text;
    HBITMAP icon = NULL;
    int command_id = 0;
    ggadget::win32::MenuBuilder* sub_menu = NULL;
    menu->GetMenuItem(i, &text, &style, &icon, &command_id, &sub_menu);
    ipc::proto::Command* command = command_list->add_command();
    command->mutable_title()->set_text(text);
    command->set_state(ipc::proto::Command::NORMAL);
    if (icon) {
      SaveBitmapToString(
          icon,
          command->mutable_state_icon()->mutable_normal()->mutable_data());
    }
    if (sub_menu) {
      command->set_id("");
      MenuInterfaceToCommandList(sub_menu, command->mutable_sub_commands());
    } else {
      if (style & ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED)
        command->set_state(ipc::proto::Command::CHECKED);
      if (style & ggadget::MenuInterface::MENU_ITEM_FLAG_GRAYED)
        command->set_enabled(false);
      if (style & ggadget::MenuInterface::MENU_ITEM_FLAG_SEPARATOR)
        command->set_state(ipc::proto::Command::SEPARATOR);
      std::string id = StringPrintf("%s%d", kMenuCommandPrefix, command_id);
      command->set_id(id);
    }
  }
}

void SkinUIComponentUtils::ExecuteMenuCommand(
    const ggadget::win32::MenuBuilder* menu,
    const std::string& id) {
  DCHECK_GE(id.length(), arraysize(kMenuCommandPrefix));
  const char* text = id.c_str() + arraysize(kMenuCommandPrefix) - 1;
  int command_id = static_cast<int>(strtoul(text, NULL, 0 /* base 10 */));
  bool success = menu->OnCommand(command_id);
  DCHECK(success);
}

Point<int> SkinUIComponentUtils::GetCursorPosOnView(
    ggadget::ViewHostInterface *view_host) {
  POINT cursor_pos;
  HWND window = reinterpret_cast<HWND>(view_host->GetNativeWidget());
  ::GetCursorPos(&cursor_pos);
  ::ScreenToClient(window, &cursor_pos);
  return Point<int>(cursor_pos);
}

void SkinUIComponentUtils::SetCursorPosOnView(
    ggadget::ViewHostInterface *view_host, Point<int> cursor_pos) {
  HWND window = reinterpret_cast<HWND>(view_host->GetNativeWidget());
  POINT cursor = cursor_pos.toPOINT();
  ::ClientToScreen(window, &cursor);
  ::SetCursorPos(cursor.x, cursor.y);
}

Rect<int> SkinUIComponentUtils::GetScreenRectAtPoint(Point<int> pt) {
  HMONITOR monitor = ::MonitorFromPoint(pt.toPOINT(), MONITOR_DEFAULTTONEAREST);
  MONITORINFO monitor_info;
  monitor_info.cbSize = sizeof(monitor_info);
  if (GetMonitorInfo(monitor, &monitor_info)) {
    return Rect<int>(monitor_info.rcWork);
  } else {
    return Rect<int>(-1, -1, -1, -1);
  }
}

}  // namespace components
}  // namespace ime_goopy
