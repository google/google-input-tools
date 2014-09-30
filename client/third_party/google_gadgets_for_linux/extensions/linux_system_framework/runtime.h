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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_RUNTIME_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_RUNTIME_H__

#include <vector>
#include <string>
#include <ggadget/sysdeps.h>
#include <ggadget/framework_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/dbus/dbus_proxy.h>

namespace ggadget {
namespace framework {
namespace linux_system {

using namespace ggadget::dbus;

class Runtime : public RuntimeInterface {
 public:
  Runtime();
  ~Runtime();

  virtual std::string GetAppName() const { return "Google Desktop"; }
  virtual std::string GetAppVersion() const { return GGL_API_VERSION; }
  virtual std::string GetOSName() const { return os_name_; }
  virtual std::string GetOSVersion() const { return os_version_; }

 private:
  std::string os_name_;
  std::string os_version_;
};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_RUNTIME_H__
