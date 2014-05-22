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

#include <sys/utsname.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include "machine.h"
#include "hal_strings.h"
#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>

namespace ggadget {
namespace framework {
namespace linux_system {

using namespace ggadget::dbus;

static const char* kKeysInMachineInfo[] = {
  "cpu family", "model", "stepping",
  "vendor_id", "model name", "cpu MHz"
};

// Represents the file names for reading CPU info.
static const char kCPUInfoFile[] = "/proc/cpuinfo";

Machine::Machine() {
  InitArchInfo();
  InitProcInfo();
  DBusProxy *proxy = DBusProxy::NewSystemProxy(kHalDBusName,
                                               kHalObjectComputer,
                                               kHalInterfaceDevice);
  if (!proxy) {
    DLOG("Failed to connect to DBus Hal service.");
    return;
  }

  DBusStringReceiver string_receiver;

  if (!proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                         string_receiver.NewSlot(),
                         MESSAGE_TYPE_STRING, kHalPropSystemUUID,
                         MESSAGE_TYPE_INVALID)) {
    /** The Hal specification changed one time. */
    proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                      string_receiver.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalPropSystemUUIDOld,
                      MESSAGE_TYPE_INVALID);
  }
  serial_number_ = string_receiver.GetValue();
  string_receiver.Reset();

  /** get machine vendor */
  if (!proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                         string_receiver.NewSlot(),
                         MESSAGE_TYPE_STRING, kHalPropSystemVendor,
                         MESSAGE_TYPE_INVALID)) {
    /** The Hal specification changed one time. */
    proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                      string_receiver.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalPropSystemVendorOld,
                      MESSAGE_TYPE_INVALID);
  }
  machine_vendor_ = string_receiver.GetValue();
  string_receiver.Reset();

  /** get machine model*/
  if (!proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                         string_receiver.NewSlot(),
                         MESSAGE_TYPE_STRING, kHalPropSystemProduct,
                         MESSAGE_TYPE_INVALID)) {
    /** The Hal specification changed one time. */
    proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                      string_receiver.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalPropSystemProductOld,
                      MESSAGE_TYPE_INVALID);
  }
  machine_model_ = string_receiver.GetValue();

  delete proxy;
}

Machine::~Machine() {
}

std::string Machine::GetBiosSerialNumber() const {
  return serial_number_;
}

std::string Machine::GetMachineManufacturer() const {
  return machine_vendor_;
}

std::string Machine::GetMachineModel() const {
  return machine_model_;
}

std::string Machine::GetProcessorArchitecture() const {
  return sysinfo_[CPU_ARCH];
}

int Machine::GetProcessorCount() const {
  return cpu_count_;
}

int Machine::GetProcessorFamily() const {
  return static_cast<int>(strtol(sysinfo_[CPU_FAMILY].c_str(), NULL, 10));
}

int Machine::GetProcessorModel() const {
  return static_cast<int>(strtol(sysinfo_[CPU_MODEL].c_str(), NULL, 10));
}

std::string Machine::GetProcessorName() const {
  return sysinfo_[CPU_NAME];
}

int Machine::GetProcessorSpeed() const {
  return static_cast<int>(strtol(sysinfo_[CPU_SPEED].c_str(), NULL, 10));
}

int Machine::GetProcessorStepping() const {
  return static_cast<int>(strtol(sysinfo_[CPU_STEPPING].c_str(), NULL, 10));
}

std::string Machine::GetProcessorVendor() const {
  return sysinfo_[CPU_VENDOR];
}

void Machine::InitArchInfo() {
  utsname name;
  if (uname(&name) == -1) { // indicates error when -1 is returned.
    sysinfo_[CPU_ARCH] = "";
    return;
  }

  sysinfo_[CPU_ARCH] = std::string(name.machine);
}

void Machine::InitProcInfo() {
  FILE* fp = fopen(kCPUInfoFile, "r");
  if (fp == NULL)
    return;

  char line[1001] = { 0 };
  cpu_count_ = 0;
  std::string key, value;

  // get the processor count
  while (fgets(line, sizeof(line) - 1, fp)) {
    if (!SplitString(line, ":", &key, &value))
      continue;

    key = TrimString(key);
    value = TrimString(value);

    if (key == "processor") {
      cpu_count_ ++;
      continue;
    }

    if (cpu_count_ > 1) continue;

    for (size_t i = 0; i < arraysize(kKeysInMachineInfo); ++i) {
      if (key == kKeysInMachineInfo[i]) {
        sysinfo_[i] = value;
        break;
      }
    }
  }

  fclose(fp);
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
