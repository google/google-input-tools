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

#include "user.h"

#include <ggadget/main_loop_interface.h>
#include <ggadget/logger.h>
#include <ggadget/string_utils.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include "hal_strings.h"

namespace ggadget {
namespace framework {
namespace linux_system {

const char kProcfsInterruptsFile[] = "/proc/interrupts";

User::User()
    : input_device_state_(0),
      period_(kDefaultIdlePeriod),
      last_irq_(time(NULL)) {
  DBusProxy *proxy = DBusProxy::NewSystemProxy(kHalDBusName,
                                               kHalObjectManager,
                                               kHalInterfaceManager);
  if (!proxy) {
    DLOG("Failed to connect to DBus Hal service.");
    return;
  }

  FindDevices(proxy, kHalCapabilityInputKeyboard);
  FindDevices(proxy, kHalCapabilityInputMouse);
  delete proxy;

  input_devices_.push_back("keyboard");
  input_devices_.push_back("mouse");

#ifdef _DEBUG
  DLOG("Names of input devices:");
  for (size_t i = 0; i < input_devices_.size(); i++) {
    DLOG("%s", input_devices_[i].c_str());
  }
#endif

  // Set a timeout to check whether there are input events from these devices.
  WatchCallbackSlot *callback =
      new WatchCallbackSlot(NewSlot(this, &User::CheckInputEvents));
  GetGlobalMainLoop()->AddTimeoutWatch(10000, callback);
}

void User::FindDevices(DBusProxy *proxy, const char *capability) {
  ASSERT(proxy);
  std::vector<std::string> devices_udi;
  DBusStringArrayReceiver receiver(&devices_udi);

  if (!proxy->CallMethod("FindDeviceByCapability", true,
                         kDefaultDBusTimeout, receiver.NewSlot(),
                         MESSAGE_TYPE_STRING, capability,
                         MESSAGE_TYPE_INVALID)) {
    DLOG("Failed to get devices with capability %s", capability);
    return;
  }

  DLOG("Device capability: %s", capability);
  for (size_t i = 0; i < devices_udi.size(); i++) {
    DLOG("Device %zu: %s", i, devices_udi[i].c_str());
    GetDeviceName(devices_udi[i].c_str());
  }
}

void User::GetDeviceName(const char *device_udi) {
  DBusProxy *proxy, *parent_proxy;
  DBusStringReceiver parent, subsystem;

  proxy = DBusProxy::NewSystemProxy(kHalDBusName,
                                    device_udi,
                                    kHalInterfaceDevice);
  while (proxy) {
    proxy->CallMethod(kHalMethodGetProperty, true,
                      kDefaultDBusTimeout, parent.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalPropInfoParent,
                      MESSAGE_TYPE_INVALID);
    parent_proxy = DBusProxy::NewSystemProxy(kHalDBusName,
                                             parent.GetValue().c_str(),
                                             kHalInterfaceDevice);
    if (!parent_proxy) {
      delete proxy;
      break;
    }

    if (!parent_proxy->CallMethod(kHalMethodGetProperty, true,
                                  kDefaultDBusTimeout, subsystem.NewSlot(),
                                  MESSAGE_TYPE_STRING, kHalPropInfoSubsystem,
                                  MESSAGE_TYPE_INVALID)) {
      // Devices may still use "info.bus" instead of "info.subsystem"
      subsystem.Reset();
      parent_proxy->CallMethod(kHalMethodGetProperty, true,
                               kDefaultDBusTimeout, subsystem.NewSlot(),
                               MESSAGE_TYPE_STRING, kHalPropInfoSubsystemOld,
                               MESSAGE_TYPE_INVALID);
    }
    DLOG("Subsystem the device connected to: %s", subsystem.GetValue().c_str());

    // If the input device is on a usb bus, and the bus number is x, then we set
    // the device name as "usbx".
    if (subsystem.GetValue().compare("usb") == 0 ||
        subsystem.GetValue().compare("usb_device") == 0) {
      std::string device_name("usb");
      DBusIntReceiver bus_number;
      if (parent_proxy->CallMethod(kHalMethodGetProperty, true,
                                 kDefaultDBusTimeout, bus_number.NewSlot(),
                                 MESSAGE_TYPE_STRING,
                                 (subsystem.GetValue() + ".bus_number").c_str(),
                                 MESSAGE_TYPE_INVALID)) {
        device_name += StringPrintf("%jd", bus_number.GetValue());
        input_devices_.push_back(device_name);
      } else {
        // This parent has no information about bus number, go to the grandparent
        // unless we have already reached the root.
        if (parent.GetValue().compare(kHalObjectComputer) != 0) {
          subsystem.Reset();
          parent.Reset();
          delete proxy;
          proxy = parent_proxy;
          continue;
        }
      }
    }

    delete parent_proxy;
    delete proxy;
    proxy = NULL;
  }
}

static int GetHash(int base, const char *str) {
  while (*str) {
    base = base * 31 + static_cast<unsigned char >(*str);
    str++;
  }
  return base;
}

bool User::CheckInputEvents(int watch_id) {
  GGL_UNUSED(watch_id);
  char line[256];
  FILE *interrupt_file = fopen(kProcfsInterruptsFile, "r");
  if (interrupt_file) {
    int new_state = 0;
    while (fgets(line, sizeof(line), interrupt_file)) {
      for (size_t i = 0; i < input_devices_.size(); i++) {
        if (strcasestr(line, input_devices_[i].c_str()) != NULL) {
          new_state = GetHash(new_state, line);
          break;
        }
      }
    }
    fclose(interrupt_file);

    if (new_state != input_device_state_) {
      input_device_state_ = new_state;
      last_irq_ = time(NULL);
    }
  } else {
    // If failed to get input status, let IsUserIdle() always return false.
    last_irq_ = time(NULL);
  }

  return true;
}

User::~User() {
}

bool User::IsUserIdle() {
  return time(NULL) > last_irq_ + period_ ? true : false;
}

void User::SetIdlePeriod(time_t period) {
  period_ = period;
}

} // namespace linux_system
} // namespace framework
} // ggadget
