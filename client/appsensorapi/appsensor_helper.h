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

// This class contains the declaration of AppSensorHelper class,
// which provides a sigleton wrapper for AppSensor class.

#ifndef GOOPY_APPSENSORAPI_APPSENSOR_HELPER_H__
#define GOOPY_APPSENSORAPI_APPSENSOR_HELPER_H__

#include <windows.h>
#include "base/basictypes.h"
#include "appsensorapi/appsensor.h"

namespace ime_goopy {
class Handler;

class AppSensorHelper {
  public:
    // Return the instance to the singleton class
    static AppSensor *Instance();

  private:
    AppSensorHelper() {}
    static AppSensor* sensor_;
    DISALLOW_COPY_AND_ASSIGN(AppSensorHelper);
};
}  // namespace ime_goopy
#endif  // GOOPY_APPSENSORAPI_APPSENSOR_HELPER_H__
