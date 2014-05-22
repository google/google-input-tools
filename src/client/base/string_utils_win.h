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

#ifndef I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_STRING_UTILS_H_
#define I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_STRING_UTILS_H_

#include <string>

namespace ime_goopy {

std::string WideToUtf8(const std::wstring& wide);
std::wstring Utf8ToWide(const std::string& utf8);

}  // namespace ime_goopy
#endif  // I18N_INPUT_ENGINE_LIB_PUBLIC_WIN_STRING_UTILS_H_
