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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_USER_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_USER_H__

#include <time.h>
#include <vector>
#include <string>
#include <ggadget/framework_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/dbus/dbus_proxy.h>

namespace ggadget {
namespace framework {
namespace linux_system {

using namespace ggadget::dbus;

class User : public UserInterface {
 public:
  User();
  ~User();

  /*
   * We do this by checking whether there are any interrupts during the last
   * certain period of time. The default value of the period is 1 minute.To
   * change the value, @see SetIdlePeriod.
   */
  virtual bool IsUserIdle();
  virtual void SetIdlePeriod(time_t period);
  virtual time_t GetIdlePeriod() const { return period_; }

 private:
  static const time_t kDefaultIdlePeriod = 60;

  void FindDevices(DBusProxy *proxy, const char *capability);
  void GetDeviceName(const char *device_udi);
  bool CheckInputEvents(int watch_id);

  std::vector<std::string> input_devices_;
  int input_device_state_;
  time_t period_;
  time_t last_irq_;
};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_USER_H__
