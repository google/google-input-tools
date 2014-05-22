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

// This file contains the implementation of AppSensor class. This class
// implements the core funtionality of application detecting. It tries
// to match the file information of the current process module against
// the information in HandlerManager. If a match is found, then
// the messaging handling will be delegated to the corresponding
// Handler.

#include "appsensorapi/appsensor.h"
#include "base/logging.h"
#include "appsensorapi/handler.h"
#include "appsensorapi/handlermanager.h"
#include "appsensorapi/versionreader.h"

namespace ime_goopy {

AppSensor::AppSensor()
    : is_init_(FALSE) {
  handler_manager_.reset(new HandlerManager());
}

BOOL AppSensor::Init() {
  TCHAR filename[MAX_PATH];

  if (!is_init_) {
    is_init_ = GetActiveProcessFilename(MAX_PATH, filename) &&
        VersionReader::GetVersionInfo(filename, &version_info_);
  }
  return is_init_;
}

BOOL AppSensor::GetActiveProcessFilename(DWORD buff_size,
                                         TCHAR* const filename) {
  DCHECK(filename != NULL);
  return ::GetModuleFileName(NULL, filename, buff_size);
}

LRESULT AppSensor::HandleMessage(HWND hwnd, UINT message,
                                 WPARAM wparam, LPARAM lparam) {
  // Delegates to HandlerManager if initialzing is completed.
  return !is_init_ ? FALSE :
      handler_manager_->HandleMessage(version_info_, hwnd, message,
                                      wparam, lparam);
}

BOOL AppSensor::HandleCommand(uint32 command, void *data) {
  // Delegates to HandlerManager if initialzing is completed.
  return !is_init_ ? FALSE :
      handler_manager_->HandleCommand(version_info_, command, data);
}

BOOL AppSensor::RegisterHandler(const Handler* handler) {
  if (handler == NULL) return FALSE;
  return !is_init_ ? FALSE : handler_manager_->AddHandler(handler);
}
}  // namespace ime_goopy
