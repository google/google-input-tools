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
// This file contains the implementation of DLL export functions.

#include "appsensorapi/appsensorapi.h"
#include "appsensorapi/appsensor.h"
#include "appsensorapi/handlermanager.h"
#include "appsensorapi/versionreader.h"

using ime_goopy::AppSensor;
using ime_goopy::Handler;

// This stores an instance of AppSensor. Any C-style functions will
// be wrapped to the methods of this instance.
AppSensor* app_sensor = NULL;

// Defines the DLL extrance point.
BOOL WINAPI DllMain(HINSTANCE hinst_dll, DWORD reason, LPVOID reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      // Allocate memory for the AppSensor.
      app_sensor = new AppSensor();
      break;
    case DLL_PROCESS_DETACH:
      delete app_sensor;
      app_sensor = NULL;
      break;
    // We ignore the thread requests since the sensor should live within
    // the whole process.
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
  }
  return TRUE;
}

// Defines the entrance of AppSensor.Init.
BOOL WINAPI AppSensorInit() {
  if (app_sensor == NULL) return FALSE;
  return app_sensor->Init();
}

// Defines the entrance of AppSensor.HandleMessage.
BOOL WINAPI AppSensorHandleMessage(HWND hwnd, UINT message,
                                   WPARAM wparam, LPARAM lparam) {
  if (app_sensor == NULL) return FALSE;
  return app_sensor->HandleMessage(hwnd, message, wparam, lparam);
}

// Defines the entrance of AppSensor.HandleCommand.
BOOL WINAPI AppSensorHandleCommand(uint32 command,
                                   void *data) {
  if (app_sensor == NULL) return FALSE;
  return app_sensor->HandleCommand(command, data);
}

// Defines the entrance of AppSensor.RegisterHandler.
BOOL WINAPI AppSensorRegisterHandler(const Handler* handler) {
  if (app_sensor == NULL) return FALSE;
  return app_sensor->RegisterHandler(handler);
}
