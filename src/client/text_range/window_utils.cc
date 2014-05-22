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

#include "text_range/window_utils.h"

namespace ime_goopy {
std::wstring GetWindowClassName(HWND window) {
  wchar_t class_name[MAX_PATH];
  if (GetClassName(window, class_name, MAX_PATH) == 0)
    return L"";
  if (_wcslwr_s(class_name, MAX_PATH) != 0)
    return L"";
  return class_name;
}

bool IsEditControl(HWND window) {
  std::wstring name = GetWindowClassName(window);
  return name == L"edit" || name.substr(0, 8) == L"richedit";
}

bool IsBrowserControl(HWND window) {
  return GetWindowClassName(window) == L"internet explorer_server";
}
}  // namespace ime_goopy
