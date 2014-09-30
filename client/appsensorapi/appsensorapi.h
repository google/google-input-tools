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
//
// This file contains the declaration of DLL export functions.
// Normally these functions are orient to the system. Instead
// of calling these functions directly, it is better to use a
// wrapper class like AppSensorHelper.

#ifndef GOOPY_APPSENSORAPI_APPSENSORAPI_H__
#define GOOPY_APPSENSORAPI_APPSENSORAPI_H__

#include <windows.h>
#include "base/basictypes.h"

namespace ime_goopy {
class Handler;

// Declares the type name of the entries.
typedef BOOL (STDAPICALLTYPE *AppSensorInitFunc)();
typedef WNDPROC AppSensorHandleMessageFunc;
typedef BOOL (STDAPICALLTYPE *AppSensorHandleCommandFunc)(uint32, void *);
typedef BOOL (STDAPICALLTYPE *AppSensorRegisterHandlerFunc)(const Handler *);

extern "C" {
  // Declares the c-style entries of AppSensor class.
  BOOL WINAPI AppSensorInit();
  LRESULT WINAPI AppSensorHandleMessage(HWND hwnd, UINT message,
                                        WPARAM wparam, LPARAM lparam);
  BOOL WINAPI AppSensorHandleCommand(uint32 command, void *data);
  BOOL WINAPI AppSensorRegisterHandler(const Handler *handler);
}
}  // namespace ime_goopy

#endif  // GOOPY_APPSENSORAPI_APPSENSORAPI_H__
