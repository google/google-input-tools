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
#ifndef GOOPY_COMMON_PROCESS_QUIT_CONTROLLER_H_
#define GOOPY_COMMON_PROCESS_QUIT_CONTROLLER_H_

#include <windows.h>
#include <string>

#include "base/logging.h"
#include "base/scoped_handle.h"

namespace ime_goopy {

// This class is for inter-process communication use in such scenarios.
// Process A is waiting for instruction from Process B, if Process B wants
// Process A to quit, it will fire a signal, then waits for Process A completing
// the quit command. After receiving the signal, process A will clean up itself
// and then fire a signal to inform Process B that it finishes.
class ProcessQuitController {
 public:
  // Constructor for quit target process.
  explicit ProcessQuitController(const std::wstring& quit_name);
  // Constructor for quit commander process.
  ProcessQuitController(const std::wstring& quit_name, DWORD session_id);
  ~ProcessQuitController();

  // Called by administrative process that fires the quit event to inform
  // another process owns the quit event to quit.
  // Returns true if signal exists.
  bool Quit();

  // Called by administrative process fire the quit event to wait until the
  // target process finish.
  bool WaitProcessFinish();

  // Called by process receiving quit event to quit itself.
  bool WaitQuitSignal();

  // Called by process receiving quit event to inform that quiting finished.
  bool SignalQuitFinished();

 private:
  ScopedHandle quit_event_;
  ScopedHandle quit_finish_event_;
  std::wstring quit_name_;
  DWORD session_id_;
  bool is_process_to_quit_;
  DISALLOW_COPY_AND_ASSIGN(ProcessQuitController);
};

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_PROCESS_QUIT_CONTROLLER_H_
