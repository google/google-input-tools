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
#ifndef GOOPY_TSF_TSF_UTILS_H_
#define GOOPY_TSF_TSF_UTILS_H_

#include <windows.h>
#include <string>
#include "base/basictypes.h"

namespace ime_goopy {
namespace tsf {

// A untility class that manages the current language and input processor
// using text service framework.
class TSFUtils {
 public:
  // Switches to the Text Input Processor described by the language and profile.
  static void SwitchToTIP(DWORD lang, const std::wstring& profile);
  // Gets the langauge profile guid of the current Text Input Processor.
  static std::wstring GetCurrentLanguageProfile();
  // Gets the current language id.
  static DWORD GetCurrentLanguageId();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TSFUtils);
};

}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_TSF_UTILS_H_
