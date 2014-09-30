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

#ifndef I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_SECURITY_UTILS_H_
#define I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_SECURITY_UTILS_H_

#include <AccCtrl.h>

namespace ime_goopy {

// Sets low integrity to 'handle'.
// This function has no ATL dependency.
//
// See http://msdn.microsoft.com/en-us/library/bb250462(VS.85).aspx for more
// details on setting low integrity to system objects.
bool SetHandleLowIntegrity(HANDLE handle, SE_OBJECT_TYPE type);

}  // namespace i18n_input
#endif  // I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_SECURITY_UTILS_H_
