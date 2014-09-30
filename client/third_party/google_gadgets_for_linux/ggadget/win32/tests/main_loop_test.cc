/* Copyright 2011 Google Inc.

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
#include <string.h>
#include <windows.h>
#include <iostream>
#include <string>
#include "ggadget/main_loop_interface.h"
#include "ggadget/scoped_ptr.h"
#include "ggadget/tests/main_loop_test.h"
#include "ggadget/win32/main_loop.h"
#include "ggadget/win32/tests/main_loop_test_utility.h"
#include "unittest/gtest.h"

namespace {

static const wchar_t* kSlaveFlag = L"--slave";

bool CreateTestProcess(LPCWSTR process_path,
                       LPWSTR command_line) {
  PROCESS_INFORMATION process_information;
  memset(&process_information, 0, sizeof(process_information));
  STARTUPINFO startup_info;
  memset(&startup_info, 0, sizeof(startup_info));

  BOOL ret = CreateProcessW(process_path,
                            command_line,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &startup_info,
                            &process_information);
  return ret != 0;
}

}  // namespace

namespace ggadget {
namespace win32 {

class SimpleCallback : public ggadget::WatchCallbackInterface {
 public:
  SimpleCallback();
  virtual ~SimpleCallback();
  virtual bool Call(ggadget::MainLoopInterface *main_loop, int watch_id);
  virtual void OnRemove(ggadget::MainLoopInterface *main_loop, int watch_id);

 private:
  // Signal to notify another process callback is called.
  HANDLE called_event_;
  // Signal to notify another process callback is removed.
  HANDLE removed_event_;
  // Signal by another process to notify process to exit.
  HANDLE quit_event_;
  DISALLOW_EVIL_CONSTRUCTORS(SimpleCallback);
};

SimpleCallback::SimpleCallback() {
  called_event_ = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE,
                            kTestWatchCalledEvent);
  removed_event_ = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE,
                             kTestWatchRemovedEvent);
  quit_event_ = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE,
                          kTestProcessQuitEvent);
}

SimpleCallback::~SimpleCallback() {
  CloseHandle(removed_event_);
  CloseHandle(called_event_);
  CloseHandle(quit_event_);
}

bool SimpleCallback::Call(ggadget::MainLoopInterface *main_loop,
                          int watch_id) {
  SetEvent(called_event_);
  BOOL ret = WaitForSingleObject(quit_event_, 0);
  if (ret == WAIT_OBJECT_0 || ret == WAIT_ABANDONED_0) {
    // Another thread wants to terminate this process.
    PostQuitMessage(0);
  }
  return true;
}

void SimpleCallback::OnRemove(ggadget::MainLoopInterface *main_loop,
                              int watch_id) {
  SetEvent(removed_event_);
}

}  // namespace win32
}  // namespace ggadget


TEST(MainLoopTest, InterProcessTimeoutWatch) {
  HANDLE called_event = CreateEvent(NULL, FALSE, FALSE, kTestWatchCalledEvent);
  HANDLE removed_event = CreateEvent(NULL, FALSE, FALSE,
                                     kTestWatchRemovedEvent);
  HANDLE quit_event = CreateEvent(NULL, TRUE, FALSE, kTestProcessQuitEvent);
  wchar_t executable_path[MAX_PATH] = { 0 };
  wchar_t command_line[MAX_PATH] = { 0 };
  GetModuleFileName(NULL, executable_path, sizeof(executable_path));
  swprintf(command_line, MAX_PATH,  L"\"%s\" %s", executable_path, kSlaveFlag);
  ASSERT_TRUE(CreateTestProcess(executable_path, command_line));

  int remaining_waiting_times = 10;
  while (remaining_waiting_times) {
    if (WaitForSingleObject(called_event, kWaitTimeout) == WAIT_OBJECT_0)
      remaining_waiting_times--;
  }
  // Quit test process.
  SetEvent(quit_event);
  // timer should be removed.
  ASSERT_EQ(WaitForSingleObject(removed_event, kWaitTimeout), WAIT_OBJECT_0);

  CloseHandle(quit_event);
  CloseHandle(called_event);
  CloseHandle(removed_event);
}

TEST(MainLoopTest, TimeoutWatch) {
  ggadget::win32::MainLoop main_loop;
  TimeoutWatchTest(&main_loop);
}

static int StartMainloopInSlaveMode() {
  // In slave mode, run a windows message loop.
  ggadget::scoped_ptr<ggadget::win32::MainLoop> main_loop(
      new ggadget::win32::MainLoop());

  ggadget::scoped_ptr<ggadget::win32::SimpleCallback> simple_callback(
      new ggadget::win32::SimpleCallback());

  int watch_id = main_loop->AddTimeoutWatch(10, simple_callback.get());

  MSG msg;
  BOOL ret;
  while ((ret = GetMessage(&msg, NULL, 0, 0)) != -1) {
    if (ret == 0) {
      // WM_QUIT received.
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  // Trigger |OnRemove| of callback.
  main_loop.reset();
  return ret;
}

int wmain(int argc, wchar_t *argv[]) {
  if (argc >= 2 && !wcscmp(argv[argc - 1], kSlaveFlag)) {
    return StartMainloopInSlaveMode();
  } else {
    // In master mode, run all unit tests.
    testing::ParseGTestFlags(&argc, argv);
    return RUN_ALL_TESTS();
  }
}
