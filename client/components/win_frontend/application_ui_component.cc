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

#include "components/win_frontend/application_ui_component.h"

#include <atlbase.h>
#include <atlimage.h>
#include <stdlib.h>
#include <windows.h>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "common/app_const.h"
#include "ipc/constants.h"
#include "ipc/component_host.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/settings_client.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/thread_local_singleton_holder.h"

namespace ime_goopy {
namespace components {

namespace {
const uint32 kProduceMessages[] = {
  ipc::MSG_ACTIVATE_COMPONENT,
};

const uint32 kConsumeMessages[] = {
  ipc::MSG_INPUT_CONTEXT_DELETED,
  ipc::MSG_SHOW_MENU,
  ipc::MSG_SHOW_MESSAGE_BOX,
};

const char kMenuStringID[] = "com.google.input_tools.win_menu";

const int kStartMenuID = 0x100;

const wchar_t kWindowClassName[] = L"MENUHELPER";

// Loads a bitmap from the string |buffer|.
HBITMAP LoadBitMapFromString(const std::string& buffer) {
  IStream* stream = NULL;
  if (::CreateStreamOnHGlobal(0, TRUE, &stream) != S_OK) {
    DCHECK(false) << "CreateStreamOnHGlobal failed";
    return NULL;
  }
  ULONG size_written = 0;
  stream->Write(buffer.c_str(), buffer.size(), &size_written);
  DCHECK_EQ(size_written, buffer.size()) << "error writting string to istream";
  CImage image;
  image.Load(stream);
  DCHECK(!image.IsNull());
  stream->Release();
  return image.Detach();
}

// Adds the |command_list| into the |menu|. The command id of the items in
// |menu| will start with |start_command_id|.
// The HBITMAP objects created for the menu icon is put in bmps so that the
// caller can destory them when done.
// Returns the next available command id.
int AddCommandListToHMENU(const ipc::proto::CommandList& command_list,
                          int start_command_id,
                          HMENU menu,
                          std::vector<HBITMAP>* bmps) {
  DCHECK(menu);
  DCHECK(bmps);
  int command_id = start_command_id;
  for (int i = 0; i < command_list.command_size(); ++i) {
    const ipc::proto::Command& command = command_list.command(i);
    if (!command.visible())
      continue;
    std::wstring text = Utf8ToWide(command.title().text());
    if (command.has_sub_commands()) {
      HMENU sub_menu = ::CreatePopupMenu();
      command_id = AddCommandListToHMENU(
          command.sub_commands(), command_id, sub_menu, bmps);
      ::AppendMenu(menu, MF_STRING | MF_POPUP,
                   reinterpret_cast<UINT_PTR>(sub_menu),
                   text.c_str());
    } else {
      UINT menu_flag = MF_STRING;
      switch (command.state()) {
        case ipc::proto::Command::CHECKED:
          menu_flag |= MF_CHECKED;
          break;
        case ipc::proto::Command::SEPARATOR:
          menu_flag = MF_SEPARATOR;
          break;
        default:
          break;
      }
      if (!command.enabled())
        menu_flag |= MF_DISABLED;
      ::AppendMenu(menu, menu_flag, command_id, text.c_str());
      MENUITEMINFO info = {0};
      info.cbSize = sizeof(info);
      info.fMask = MIIM_DATA;
      // Store the command string id to the item data.
      info.dwItemData =
          reinterpret_cast<ULONG_PTR>(new std::string(command.id()));
      ::SetMenuItemInfo(menu, command_id, false, &info);
      ++command_id;
    }
    if (command.has_state_icon() && command.state_icon().has_normal() &&
        command.state_icon().normal().has_data()) {
      HBITMAP bmp = LoadBitMapFromString(command.state_icon().normal().data());
      if (bmp) {
        // Blend to icon with menu background color.
        CImage source_image;
        source_image.Attach(bmp);
        source_image.SetHasAlphaChannel(true);
        CImage menu_icon;
        menu_icon.Create(source_image.GetWidth(), source_image.GetHeight(), 24);
        HDC dc = menu_icon.GetDC();
        RECT rect = {0, 0, menu_icon.GetWidth(), menu_icon.GetHeight()};
        ::FillRect(dc, &rect, ::GetSysColorBrush(COLOR_MENU));
        source_image.AlphaBlend(dc, rect, rect);
        menu_icon.ReleaseDC();
        bmp = menu_icon.Detach();
        MENUITEMINFO info = {0};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_CHECKMARKS;
        info.hbmpChecked = bmp;
        info.hbmpUnchecked = bmp;
        bmps->push_back(bmp);
        ::SetMenuItemInfo(menu, ::GetMenuItemCount(menu) - 1, TRUE, &info);
      }
    }
  }
  return command_id;
}

// Get the command string id of the menu item in menu whose command id is
// |menu_id|.
std::string* GetCommandIDByMenuCommandID(HMENU menu, int menu_id) {
  for (int i = 0; i < ::GetMenuItemCount(menu); ++i) {
    if (menu_id == ::GetMenuItemID(menu, i)) {
      MENUITEMINFO info = {0};
      info.cbSize = sizeof(info);
      info.fMask = MIIM_DATA;
      ::GetMenuItemInfo(menu, i, TRUE, &info);
      return reinterpret_cast<std::string*>(info.dwItemData);
    }
    HMENU sub_menu = ::GetSubMenu(menu, i);
    if (sub_menu) {
      std::string* id = GetCommandIDByMenuCommandID(sub_menu, menu_id);
      if (id)
        return id;
    }
  }
  return NULL;
}

// Destroy the user data for menu items in |menu|.
void DestroyMenuItems(HMENU menu) {
  for (int i = 0; i < ::GetMenuItemCount(menu); ++i) {
    HMENU sub_menu = ::GetSubMenu(menu, i);
    if (sub_menu) {
      DestroyMenuItems(sub_menu);
    } else {
      MENUITEMINFO info = {0};
      info.cbSize = sizeof(info);
      info.fType = MIIM_DATA;
      ::GetMenuItemInfo(menu, i, TRUE, &info);
      delete reinterpret_cast<std::string*>(info.dwItemData);
    }
  }
}

int GetMessageBoxFlag(int button_set, int icon) {
  int flag = 0;
  switch (button_set) {
    case ipc::proto::MB_BUTTON_SET_OK:
      flag |= MB_OK;
      break;
    case ipc::proto::MB_BUTTON_SET_OKCANCEL:
      flag |= MB_OKCANCEL;
      break;
    case ipc::proto::MB_BUTTON_SET_ABORTRETRYIGNORE:
      flag |= MB_ABORTRETRYIGNORE;
      break;
    case ipc::proto::MB_BUTTON_SET_YESNOCANCEL:
      flag |= MB_YESNOCANCEL;
      break;
    case ipc::proto::MB_BUTTON_SET_YESNO:
      flag |= MB_YESNO;
      break;
    case ipc::proto::MB_BUTTON_SET_RETRYCANCEL:
      flag |= MB_RETRYCANCEL;
      break;
    case ipc::proto::MB_BUTTON_SET_CANCELTRYCONTINUE:
      flag |= MB_CANCELTRYCONTINUE;
      break;
    default:
      NOTREACHED() << "Invalid button set for message box";
      break;
  }
  switch (icon) {
    case ipc::proto::MB_ICON_NONE:
      break;
    case ipc::proto::MB_ICON_ERROR:
      flag |= MB_ICONERROR;
      break;
    case ipc::proto::MB_ICON_QUESTION:
      flag |= MB_ICONQUESTION;
      break;
    case ipc::proto::MB_ICON_WARNING:
      flag |= MB_ICONWARNING;
      break;
    case ipc::proto::MB_ICON_INFORMATION:
      flag |= MB_ICONINFORMATION;
      break;
    default:
      NOTREACHED() << "Invalid icon for message box";
      break;
  }
  return flag;
}

ipc::proto::MessageBoxButton GetMessageBoxButton(int button_code) {
  switch (button_code) {
    case IDOK:
      return ipc::proto::MB_BUTTON_OK;
    case IDCANCEL:
      return ipc::proto::MB_BUTTON_CANCEL;
    case IDABORT:
      return ipc::proto::MB_BUTTON_ABORT;
    case IDRETRY:
      return ipc::proto::MB_BUTTON_RETRY;
    case IDIGNORE:
      return ipc::proto::MB_BUTTON_IGNORE;
    case IDYES:
      return ipc::proto::MB_BUTTON_YES;
    case IDNO:
      return ipc::proto::MB_BUTTON_NO;
    case IDCLOSE:
      return ipc::proto::MB_BUTTON_CLOSE;
    case IDHELP:
      return ipc::proto::MB_BUTTON_HELP;
    default:
      NOTREACHED() << "Invalid return code of MessageBox";
      return ipc::proto::MB_BUTTON_CLOSE;
  }
}

#ifdef DEBUG
static const int MAX_LEN = 100;
bool IsRTLSystem() {
  wchar_t is_rtl[MAX_LEN] = {};
  if (::GetEnvironmentVariable(L"GOOGLE_INPUT_TOOLS_RTL", is_rtl, MAX_LEN)) {
    return strtoul(WideToUtf8(is_rtl).c_str(), NULL, 10) == 1;
  } else {
    return false;
  }
}
#else
bool IsRTLSystem() {
  // We can't detect the layout of application in ipc_console, so we use the
  // layout of shell window as the default layout.
  return ::GetWindowLong(::GetShellWindow(), GWL_EXSTYLE) &
         WS_EX_LAYOUTRTL;
}
#endif

}  // namespace

ApplicationUIComponent::ApplicationUIComponent()
    : menu_owner_(NULL) {
  settings_ = new ipc::SettingsClient(this, NULL);
  DCHECK(settings_);
  WNDCLASSEX wndclass = {0};
  wndclass.cbSize = sizeof(wndclass);
  if (!::GetClassInfoEx(_AtlBaseModule.GetModuleInstance(),
                        kWindowClassName, &wndclass)) {
    wndclass.lpszClassName = kWindowClassName;
    wndclass.hInstance = _AtlBaseModule.GetModuleInstance();
    wndclass.style = CS_IME;
    wndclass.lpfnWndProc = DefWindowProc;
    if (!RegisterClassEx(&wndclass)) {
      DLOG(ERROR) << "Can't register class with name"
                  << kWindowClassName
                  << " error_code = "
                  << GetLastError();
    }
  }
  menu_owner_ = ::CreateWindow(kWindowClassName, NULL, WS_DISABLED, 0, 0, 0, 0,
                               NULL, NULL, NULL, NULL);
  DCHECK(menu_owner_);
  // Set/Clear the style WS_EX_LAYOUTRTL to make menu layout the same with
  // system layout.
  if (IsRTLSystem()) {
    ::SetWindowLong(
        menu_owner_,
        GWL_EXSTYLE,
        ::GetWindowLong(menu_owner_, GWL_EXSTYLE) | WS_EX_LAYOUTRTL);
  } else {
    ::SetWindowLong(
        menu_owner_,
        GWL_EXSTYLE,
        ::GetWindowLong(menu_owner_, GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL);
  }
}

ApplicationUIComponent::~ApplicationUIComponent() {
  ::DestroyWindow(menu_owner_);
}

void ApplicationUIComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  std::string string_id = StringPrintf("%s_%d_%x", kMenuStringID,
                                       GetCurrentThreadId(),
                                       reinterpret_cast<uint64>(this));
  info->set_string_id(string_id);
  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);

  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
  GetSubComponentsInfo(info);
}

void ApplicationUIComponent::Handle(ipc::proto::Message* message) {
  if (HandleMessageBySubComponents(message))
    return;
  switch (message->type()) {
    case ipc::MSG_SHOW_MENU:
      OnMsgShowMenu(message);
      break;
    case ipc::MSG_INPUT_CONTEXT_DELETED:
      OnMsgInputContextDeleted(message);
      break;
    case ipc::MSG_SHOW_MESSAGE_BOX:
      OnMsgShowMessageBox(message);
      break;
    default:
      NOTREACHED() << "Invalid message type = "
                   << ipc::GetMessageName(message->type())
                   << " from icid = " << message->icid()
                   << " received by ApplicationUIComponent";
      delete message;
      break;
  }
}

void ApplicationUIComponent::OnRegistered() {
  int64 pid = 0;
  if (!settings_->GetIntegerValue(kSettingsIPCConsolePID, &pid)) {
    DLOG(ERROR) << "Failed to get the process id for ipc console";
    return;
  }
  // This setting only changes when ipc_console crashes and creates a new
  // instance.
  console_pid_ = static_cast<DWORD>(pid);
}

void ApplicationUIComponent::OnDeregistered() {
  base::AutoLock scoped_lock(lock_);
  attached_icids_.clear();
}

void ApplicationUIComponent::InputContextCreated(int icid) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_ACTIVATE_COMPONENT, icid, false));
  Send(mptr.release(), NULL);
  base::AutoLock scoped_lock(lock_);
  attached_icids_.insert(icid);
}

void ApplicationUIComponent::OnMsgInputContextDeleted(
    ipc::proto::Message* message) {
  base::AutoLock scoped_lock(lock_);
  int icid = message->payload().input_context_info().id();
  attached_icids_.erase(icid);
  ReplyTrue(message);
}

void ApplicationUIComponent::OnMsgShowMenu(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  {
    base::AutoLock scoped_lock(lock_);
    if (!attached_icids_.count(mptr->icid())) {
      NOTREACHED() << "MSG_SHOW_MENU with invalid icid";
      return;
    }
  }
  int target = mptr->source();
  HMENU menu = ::CreatePopupMenu();
  std::vector<HBITMAP> bmps;
  for (int i = 0; i < mptr->payload().command_list_size(); ++i) {
    AddCommandListToHMENU(mptr->payload().command_list(i),
                          kStartMenuID,
                          menu,
                          &bmps);
  }
  int x = mptr->payload().input_caret().rect().x();
  int y = mptr->payload().input_caret().rect().y();
  int w = mptr->payload().input_caret().rect().width();
  int h = mptr->payload().input_caret().rect().height();
  TPMPARAMS menu_params = {0};
  menu_params.cbSize = sizeof(TPMPARAMS);
  menu_params.rcExclude.left = x;
  menu_params.rcExclude.top = y;
  menu_params.rcExclude.right = x + w;
  menu_params.rcExclude.bottom = y + h;
  DWORD flag = TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_TOPALIGN |
               TPM_RETURNCMD;
  // If we don't set TPM_LAYOUTRTL, the menu layout will be determined by
  // menu_owner_ and the application's default layout.
  if (IsRTLSystem())
    flag |= TPM_LAYOUTRTL;
  // TrackPopupMenuEx will return when user click on the menu item, or the user
  // cancel the menu by clicking elsewhere or switch between windows.
  int ret_code =
      ::TrackPopupMenuEx(menu,
                         flag,
                         x, y + h,
                         menu_owner_,
                         &menu_params);
  // As the major components in the framework lives in the process of
  // ipc_console, we need to allow the ipc console process to set foreground
  // window in case some component in the ipc console process needs to pop up
  // a window and get focused. As the API AllowSetForegroundWindow() only take
  // effect before receiving next input, so we need to call it every time there
  // is a chance that ipc_console will pop up UI. And there are 3 ways for user
  // to trigger an UI popup: using menu or button on status bar, or hotkeys.
  // So we call this API in ShouldProcessKey and win menu component, because
  // clicking on the status bar won't generate input to the application.
  ::AllowSetForegroundWindow(console_pid_);

  ipc::ConvertToReplyMessage(mptr.get());
  mptr->mutable_payload()->Clear();
  mptr->mutable_payload()->add_string();
  if (ret_code != 0) {
    std::string* id = GetCommandIDByMenuCommandID(menu, ret_code);
    if (id)
      mptr->mutable_payload()->mutable_string(0)->assign(*id);
    else
      NOTREACHED() << "Invalid command id = " << ret_code;
  }
  Send(mptr.release(), NULL);
  DestroyMenuItems(menu);
  for (int i = 0; i < bmps.size(); ++i)
    ::DeleteObject(bmps[i]);
  ::DestroyMenu(menu);
}

void ApplicationUIComponent::OnMsgShowMessageBox(ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(message);
  {
    base::AutoLock scoped_lock(lock_);
      if (!attached_icids_.count(mptr->icid())) {
      NOTREACHED() << "MSG_SHOW_MESSAGE_BOX with invalid icid";
      return;
    }
  }
  DCHECK(mptr->has_payload() && mptr->payload().string_size() > 1);
  if (!mptr->has_payload() || mptr->payload().string_size() < 2) {
    ReplyError(mptr.release(), ipc::proto::Error::INVALID_PAYLOAD,
               "Invalid payload for MSG_SHOW_CONTEXT_MENU");
    return;
  }
  std::wstring title = Utf8ToWide(mptr->payload().string(0));
  std::wstring text = Utf8ToWide(mptr->payload().string(1));
  int message_box_flag = GetMessageBoxFlag(
      mptr->payload().uint32_size() ? mptr->payload().uint32(0) :
                                      ipc::proto::MB_BUTTON_SET_OK,
      mptr->payload().uint32_size() > 1 ? mptr->payload().uint32(1) :
                                          ipc::proto::MB_ICON_NONE);
  int ret = ::MessageBox(::GetForegroundWindow(),
                         text.c_str(), title.c_str(), message_box_flag);
  ipc::ConvertToReplyMessage(mptr.get());
  mptr->mutable_payload()->Clear();
  mptr->mutable_payload()->add_uint32(GetMessageBoxButton(ret));
  Send(mptr.release(), NULL);
}

ApplicationUIComponent* ApplicationUIComponent::GetThreadLocalInstance() {
  using ggadget::win32::ThreadLocalSingletonHolder;
  ApplicationUIComponent* thread_local_component =
      ThreadLocalSingletonHolder<ApplicationUIComponent>::GetValue();
  if (!thread_local_component) {
    thread_local_component = new ApplicationUIComponent();
    bool result = ThreadLocalSingletonHolder<ApplicationUIComponent>::SetValue(
        thread_local_component);
    DCHECK(result);
  }
  return thread_local_component;
}

void ApplicationUIComponent::ClearThreadLocalInstance() {
  using ggadget::win32::ThreadLocalSingletonHolder;
  ApplicationUIComponent* thread_local_component =
      ThreadLocalSingletonHolder<ApplicationUIComponent>::GetValue();
  if (thread_local_component) {
    delete thread_local_component;
    bool result =
        ThreadLocalSingletonHolder<ApplicationUIComponent>::SetValue(NULL);
    DCHECK(result);
  }
}

}  // namespace components
}  // namespace ime_goopy
