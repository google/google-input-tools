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
#include "common/process_quit_controller.h"

#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "common/security_util_win.h"

static const DWORD kWaitingTime = 100;  // In milliseconds.

namespace ime_goopy {

ProcessQuitController::ProcessQuitController(const std::wstring& quit_name)
    : quit_name_(quit_name),
      is_process_to_quit_(true),
      quit_event_(NULL),
      quit_finish_event_(NULL) {
  ::ProcessIdToSessionId(::GetCurrentProcessId(), &session_id_);

  SECURITY_ATTRIBUTES security_attributes = {0};
  GetIPCSecurityAttributes(&security_attributes);

  std::wstring quit_finish_event_name =
      WideStringPrintf(L"%s_finish_%d", quit_name_.c_str(), session_id_);
  quit_finish_event_.Reset(::CreateEvent(
      &security_attributes, false, false, quit_finish_event_name.c_str()));

  std::wstring quit_event_name =
      WideStringPrintf(L"%s_%d", quit_name_.c_str(), session_id_);
  quit_event_.Reset(::CreateEvent(
      &security_attributes, false, false, quit_event_name.c_str()));
  ReleaseIPCSecurityAttributes(&security_attributes);
}

ProcessQuitController::ProcessQuitController(
    const std::wstring& quit_name, DWORD session_id)
    : quit_name_(quit_name),
      quit_event_(NULL),
      quit_finish_event_(NULL),
      session_id_(session_id),
      is_process_to_quit_(false) {
}

ProcessQuitController::~ProcessQuitController() {
}

bool ProcessQuitController::Quit() {
  DCHECK(!is_process_to_quit_);
  std::wstring quit_event_name =
      WideStringPrintf(L"%s_%d", quit_name_.c_str(), session_id_);
  quit_event_.Reset(::OpenEvent(
      EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, quit_event_name.c_str()));
  if (!quit_event_.Get())
    return false;

  if (quit_event_.Get()) {
    ::SetEvent(quit_event_.Get());
    return true;
  }
  return false;
}

bool ProcessQuitController::WaitProcessFinish() {
  DCHECK(!is_process_to_quit_);

  std::wstring quit_finish_event_name =
      WideStringPrintf(L"%s_finish_%d", quit_name_.c_str(), session_id_);

  while (true) {
    quit_finish_event_.Reset(NULL);
    quit_finish_event_.Reset(::OpenEvent(
        EVENT_MODIFY_STATE | SYNCHRONIZE,
        FALSE,
        quit_finish_event_name.c_str()));

    if (!quit_finish_event_.Get()) {
      // Process may has quited.
      return true;
    }
    DWORD ret = ::WaitForSingleObject(quit_finish_event_.Get(), kWaitingTime);
    if (ret == WAIT_TIMEOUT) {
      continue;
    } else if (ret == WAIT_OBJECT_0) {
      return true;
    } else {
      break;
    }
  }

  return false;
}

bool ProcessQuitController::WaitQuitSignal() {
  DCHECK(is_process_to_quit_);
  if (!quit_event_.Get())
    return false;
  return ::WaitForSingleObject(quit_event_.Get(), INFINITE) ==
      WAIT_OBJECT_0;
}

bool ProcessQuitController::SignalQuitFinished() {
  DCHECK(is_process_to_quit_);
  if (quit_finish_event_.Get()) {
    ::SetEvent(quit_finish_event_.Get());
    return true;
  }
  return false;
}

}  // namespace ime_goopy
