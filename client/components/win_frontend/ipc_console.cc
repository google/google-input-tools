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

#include "components/win_frontend/ipc_console.h"

#include "base/at_exit.h"
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/scoped_ptr.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "base/threading/platform_thread.h"
#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/atl.h"
#include "common/process_quit_controller.h"
#include "common/registry.h"
#include "common/scheduler.h"
#include "common/security_util_win.h"
#include "components/plugin_manager/plugin_manager_component.h"
#include "components/settings_store/settings_store_win.h"
#include "components/keyboard_input/keyboard_input_component.h"
#include "components/ui/skin_ui_component.h"
#include "ipc/direct_message_channel.h"
#include "ipc/hub_host.h"
#include "ipc/message_channel_server_win.h"
#include "ipc/multi_component_host.h"
#include "ipc/settings_client.h"

CAppModule _Module;

using ipc::HubHost;
using ipc::MessageChannelServerWin;
using ipc::MessageChannel;
using ipc::DirectMessageChannel;
using ipc::MultiComponentHost;
using ime_goopy::ProcessQuitController;
using ime_goopy::Scheduler;

namespace {
#ifdef DEBUG
static int GetTimerOut() {
  wchar_t factor[MAX_PATH] = {0};
  if (GetEnvironmentVariable(L"INPUTTOOLS_TIME_OUT", factor, MAX_PATH)) {
    return _wtoi(factor);
  } else {
    return 60000;  // 60 seconds.
  }
}
static const int kWaitTimeout = GetTimerOut();
#else
static const int kWaitTimeout = 60000; // 60 seconds.
#endif


#ifdef DEBUG
static int GetTimerFactor() {
  wchar_t factor[MAX_PATH] = {0};
  if (GetEnvironmentVariable(L"GOOPY_TIMER_FACTOR", factor, MAX_PATH)) {
    return _wtoi(factor);
  } else {
    return 60;
  }
}
static const int kTimerFactor = GetTimerFactor();
#else
static const int kTimerFactor = 60;
#endif
const int kPingDelayStart = 2 * 60 * kTimerFactor;    // 2 hours.
const int kPingDefaultInterval = 2 * 60 * kTimerFactor;   // 2 hours.
const int kPingMaxInterval = 24 * 60 * kTimerFactor;   // 1 day.

class SimpleAtExitManager : public base::AtExitManager {
 public:
  SimpleAtExitManager() : AtExitManager(true) {
  }
};

SimpleAtExitManager at_exit_manager;

// This class waits for quit signal then informs multi_component_host host to
// quit.
class WaitQuitThread : public base::PlatformThread::Delegate {
 public:
  WaitQuitThread(ProcessQuitController* process_quit_controller,
                 ipc::MultiComponentHost* host)
      : host_(host),
        process_quit_controller_(process_quit_controller),
        thread_handle_(NULL),
        quit_received_(true, false) {
    DCHECK(host && process_quit_controller_);
  }

  virtual ~WaitQuitThread() {
    ::base::PlatformThread::Join(thread_handle_);
  }

  bool Start() {
    return base::PlatformThread::Create(NULL, this, &thread_handle_);
  }

  bool WaitForQuit() {
    return quit_received_.Wait();
  }

 protected:
  virtual void ThreadMain() OVERRIDE {

    if (process_quit_controller_->WaitQuitSignal()) {
      host_->QuitWaitingComponents();
      quit_received_.Signal();
    }
  }

 private:
  ime_goopy::ProcessQuitController* process_quit_controller_;
  ipc::MultiComponentHost* host_;
  base::PlatformThreadHandle thread_handle_;
  base::WaitableEvent quit_received_;
  DISALLOW_COPY_AND_ASSIGN(WaitQuitThread);
};

}  // namespace

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR lpstrCmdLine,
                     int nCmdShow) {
  DWORD session_id = 0;
  ::ProcessIdToSessionId(::GetCurrentProcessId(), &session_id);
  std::wstring event_name = ime_goopy::WideStringPrintf(
      L"%s%d", ime_goopy::kIPCConsoleEventNamePrefix, session_id);

  ProcessQuitController process_quit_controller(
      ime_goopy::kIPCConsoleQuitEventName);

  // Calls CreateEvent to notify management service of start, fail to create
  // implies another instance has been started.
  HANDLE event = ::CreateEvent(NULL, FALSE, FALSE, event_name.c_str());
  if (!event || ::GetLastError() == ERROR_ALREADY_EXISTS)
    return -1;

#ifdef DEBUG
  {
    // Log path is $TEMP\googleinputtools\module-pid.log
    char path[MAX_PATH] = {0};
    ::GetTempPathA(MAX_PATH, path);
    ::PathAppendA(path, "googleinputtools");
    ::CreateDirectoryA(path, NULL);
    char module[MAX_PATH] = {0};
    if (::GetModuleFileNameA(NULL, module, MAX_PATH) == 0) {
      module[0] = 0;
    }
    char* module_basename = ::PathFindFileNameA(module);
    CStringA logfile;
    logfile.Format("%s-%d.log", module_basename, GetCurrentProcessId());
    ::PathAppendA(path, logfile);
    logging::InitLogging(path,
                         logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                         logging::DONT_LOCK_LOG_FILE,
                         logging::DELETE_OLD_LOG_FILE);
    logging::SetLogItems(false, true, true, false);
    ime_shared::VLog::SetFromEnvironment(L"INPUT_TOOLS_VMODULE",
                                         L"INPUT_TOOLS_VLEVEL");
  }
#endif
  scoped_ptr<HubHost> hub(new HubHost());
  hub->Run();

  scoped_ptr<MultiComponentHost> ime_host(new MultiComponentHost(true));
  // Adds settings component.
  ime_goopy::components::SettingsStoreWin* settings_component =
      new ime_goopy::components::SettingsStoreWin(
          ime_goopy::AppUtils::OpenUserRegistry());
  ime_host->AddComponent(settings_component);
  // Adds plugin manager component.
  ime_goopy::components::PluginManagerComponent* plugin_manager_component =
      new ime_goopy::components::PluginManagerComponent();
  ime_host->AddComponent(plugin_manager_component);
  // Adds ui component.
  ime_goopy::components::SkinUIComponent* skin_ui_component =
      new ime_goopy::components::SkinUIComponent();
  ime_host->AddComponent(skin_ui_component);
  // Adds Keyboard Input component.
  ime_goopy::components::KeyboardInputComponent* keyboard_input_component =
      new ime_goopy::components::KeyboardInputComponent();
  ime_host->AddComponent(keyboard_input_component);

  // Connects ime_host to hub.
  scoped_ptr<DirectMessageChannel> ime_hub_channel(
      new DirectMessageChannel(hub.get()));
  ime_host->SetMessageChannel(ime_hub_channel.get());

  int timeout = kWaitTimeout;
  if (!ime_host->WaitForComponents(&timeout))
    DLOG(ERROR) << "Wait components ready timeout";

  // Starts listening to quit event.
  scoped_ptr<WaitQuitThread> quit_thread(
      new WaitQuitThread(&process_quit_controller, ime_host.get()));
  quit_thread->Start();

  // Starts listening to other processes.
  scoped_ptr<MessageChannelServerWin> server(
      new MessageChannelServerWin(hub.get()));
  server->Initialize();

  // Wait until installer or other processes inform this process to quit.
  quit_thread->WaitForQuit();

  server.reset();
  ime_host.reset();

  // Inform the process quits completely.
  process_quit_controller.SignalQuitFinished();
  quit_thread.reset();

  return 0;
}
