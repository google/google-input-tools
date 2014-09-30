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

#ifndef GOOPY_BASE_WIN_WINDOWS_VERSION_H_
#define GOOPY_BASE_WIN_WINDOWS_VERSION_H_
#pragma once

namespace base {
namespace win {

// NOTE: Keep these in order so callers can do things like
// "if (GetWinVersion() > WINVERSION_2000) ...".  It's OK to change the values,
// though.
enum Version {
  VERSION_PRE_2000 = 0,  // Not supported
  VERSION_2000 = 1,      // Not supported
  VERSION_XP = 2,
  VERSION_SERVER_2003 = 3,
  VERSION_VISTA = 4,
  VERSION_2008 = 5,
  VERSION_WIN7 = 6,
};

// Returns the running version of Windows.
Version GetVersion();

// Returns the major and minor version of the service pack installed.
void GetServicePackLevel(int* major, int* minor);

}  // namespace win
}  // namespace base

#endif  // GOOPY_BASE_WIN_WINDOWS_VERSION_H_
