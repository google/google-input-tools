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

#ifndef GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_UTILS_H_
#define GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_UTILS_H_

#include <string>
#include <vector>

namespace ime_goopy {
namespace components {

class PluginManagerUtils {
 public:
  // List all the plugin files in |path|, including the sub directory.
  static bool ListPluginFile(const std::string& path,
                             std::vector<std::string>* files);
};
}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_PLUGIN_MANAGER_PLUGIN_MANAGER_UTILS_H_
