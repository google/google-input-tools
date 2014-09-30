/*
  Copyright 2008 Google Inc.

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

#ifndef GGADGET_XDG_ICON_THEME_H__
#define GGADGET_XDG_ICON_THEME_H__

#include <string>

namespace ggadget {
namespace xdg {

void AddIconDir(const std::string &dir);
std::string LookupIcon(const std::string &icon_name,
                       const std::string &theme_name,
                       int size);
std::string LookupIconInDefaultTheme(const std::string &icon_name, int size);
void EnableSvgIcon(bool);

} // namespace xdg
} // namespace ggadget

#endif // GGADGET_XDG_ICON_THEME_H__
