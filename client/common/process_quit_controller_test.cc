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
#include <windows.h>
#include "common/process_quit_controller.h"
#include "base/commandlineflags.h"
#include "base/scoped_handle.h"
#include <gtest/gunit.h>

DEFINE_bool(master, true, "master process");
static const wchar_t kTestProcessStartedEventName[] =
    L"Global\\com_google_ime_test_started";
static const wchar_t kTestQuitEventName[] = L"Global\\test_quit_event";

namespace ime_goopy {

TEST(ProcessQuitControllerTest, test) {
  DWORD session_id = 0;
  ::ProcessIdToSessionId(::GetCurrentProcessId(), &session_id);
  ProcessQuitController controller(kTestQuitEventName, session_id);
  EXPECT_TRUE(controller.Quit());
  EXPECT_TRUE(controller.WaitProcessFinish());
}

}  // namespace ime_goopy

using ime_goopy::ProcessQuitController;

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_master) {
    wchar_t module_name[MAX_PATH] = {0};
    if (!::GetModuleFileName(NULL, module_name, MAX_PATH))
      return -1;

    SHELLEXECUTEINFO execute_info = {0};
    execute_info.cbSize = sizeof(execute_info);
    execute_info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    execute_info.lpVerb = L"open";
    execute_info.lpFile = module_name;
    execute_info.lpParameters = L"--master=false";
    execute_info.nShow = SW_HIDE;
    if (!::ShellExecuteEx(&execute_info))
      return -1;
    ScopedHandle started_event(
        ::CreateEvent(NULL, false, false, kTestProcessStartedEventName));
    ::WaitForSingleObject(started_event.Get(), INFINITE);
    return RUN_ALL_TESTS();
  } else {
    ProcessQuitController controller(kTestQuitEventName);
    ScopedHandle started_event(::OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,
                                          FALSE,
                                          kTestProcessStartedEventName));
    ::SetEvent(started_event.Get());
    if (!controller.WaitQuitSignal() || !controller.SignalQuitFinished())
      return -1;
    return 0;
  }
}
