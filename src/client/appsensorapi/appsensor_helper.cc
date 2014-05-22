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

#include "appsensorapi/appsensor_helper.h"

#include "appsensorapi/appsensor.h"
#include "appsensorapi/handlermanager.h"

namespace ime_goopy {
AppSensor* AppSensorHelper::sensor_ = NULL;
AppSensor *AppSensorHelper::Instance() {
  if (!sensor_) {
    sensor_ = new AppSensor;
  }
  return sensor_;
}
}
