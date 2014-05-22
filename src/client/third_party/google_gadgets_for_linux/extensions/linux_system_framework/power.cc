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

#include "power.h"

#include "hal_strings.h"
#include <ggadget/logger.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/slot.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>

namespace ggadget {
namespace framework {
namespace linux_system {

Power::Power()
  : is_charging_(false),
    is_plugged_in_(false),
    percent_remaining_(0),
    time_remaining_(0),
    charge_level_design_(0),
    charge_level_current_(0),
    charge_level_rate_(0),
    battery_(NULL),
    battery_signal_connection_(NULL),
    ac_adapter_(NULL),
    ac_adapter_signal_connection_(NULL) {
  DBusProxy *hal = DBusProxy::NewSystemProxy(kHalDBusName, kHalObjectManager,
                                             kHalInterfaceManager);
  if (!hal) {
    DLOG("Failed to access Hal.");
    return;
  }

  std::vector<std::string> strlist;
  DBusStringArrayReceiver strlist_receiver(&strlist);
  // Initialize battery.
  if (hal->CallMethod(kHalMethodFindDeviceByCapability, true,
                      kDefaultDBusTimeout, strlist_receiver.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalCapabilityBattery,
                      MESSAGE_TYPE_INVALID) && strlist.size()) {
    for (size_t i = 0; i < strlist.size(); ++i) {
      DBusProxy *battery = DBusProxy::NewSystemProxy(kHalDBusName,
                                                     strlist[i].c_str(),
                                                     kHalInterfaceDevice);
      if (battery) {
        DLOG("Found battery %s", strlist[i].c_str());
        DBusStringReceiver str_receiver;
        battery->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                            str_receiver.NewSlot(),
                            MESSAGE_TYPE_STRING, kHalPropBatteryType,
                            MESSAGE_TYPE_INVALID);
        if (!battery_ || str_receiver.GetValue() == "primary")
          battery_ = battery;
        else
          delete battery;
      }
    }
    if (battery_) {
      battery_signal_connection_ = battery_->ConnectOnSignalEmit(
          NewSlot(this, &Power::OnBatterySignal));
      LoadBatteryInfo();
    }
  }

  // Initialize ac_adapter.
  strlist.clear();
  if (hal->CallMethod(kHalMethodFindDeviceByCapability, true,
                      kDefaultDBusTimeout, strlist_receiver.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalCapabilityACAdapter,
                      MESSAGE_TYPE_INVALID) && strlist.size()) {
    ac_adapter_ = DBusProxy::NewSystemProxy(kHalDBusName,
                                            strlist[0].c_str(),
                                            kHalInterfaceDevice);
    if (ac_adapter_) {
      DLOG("Found AC adapter %s", strlist[0].c_str());
      ac_adapter_signal_connection_ =
          ac_adapter_->ConnectOnSignalEmit(
              NewSlot(this, &Power::OnAcAdapterSignal));
      LoadAcAdapterInfo();
    }
  }

  if (!battery_)
    DLOG("No battery found.");
  if (!ac_adapter_)
    DLOG("No AC adapter found.");

  delete hal;
}

Power::~Power() {
  if (battery_signal_connection_)
    battery_signal_connection_->Disconnect();
  if (ac_adapter_signal_connection_)
    ac_adapter_signal_connection_->Disconnect();
  delete battery_;
  delete ac_adapter_;
  battery_ = NULL;
  ac_adapter_ = NULL;
}

void Power::OnBatterySignal(const std::string &name,
                            int argc, const Variant *argv) {
  GGL_UNUSED(argc);
  GGL_UNUSED(argv);
  if (name == kHalSignalPropertyModified)
    LoadBatteryInfo();
}

void Power::OnAcAdapterSignal(const std::string &name,
                              int argc, const Variant *argv) {
  GGL_UNUSED(argc);
  GGL_UNUSED(argv);
  if (name == kHalSignalPropertyModified)
    LoadAcAdapterInfo();
}

void Power::LoadBatteryInfo() {
  if (!battery_) return;
  DLOG("Load battery info.");

  battery_->CallMethod(kHalMethodGetProperty, false, kDefaultDBusTimeout,
                       is_charging_.NewSlot(), MESSAGE_TYPE_STRING,
                       kHalPropBatteryRechargableIsCharging,
                       MESSAGE_TYPE_INVALID);
  battery_->CallMethod(kHalMethodGetProperty, false, kDefaultDBusTimeout,
                       percent_remaining_.NewSlot(), MESSAGE_TYPE_STRING,
                       kHalPropBatteryChargeLevelPercentage,
                       MESSAGE_TYPE_INVALID);
  battery_->CallMethod(kHalMethodGetPropertyInt, false, kDefaultDBusTimeout,
                       time_remaining_.NewSlot(), MESSAGE_TYPE_STRING,
                       kHalPropBatteryRemainingTime,
                       MESSAGE_TYPE_INVALID);
  battery_->CallMethod(kHalMethodGetProperty, false, kDefaultDBusTimeout,
                       charge_level_design_.NewSlot(), MESSAGE_TYPE_STRING,
                       kHalPropBatteryChargeLevelDesign,
                       MESSAGE_TYPE_INVALID);
  battery_->CallMethod(kHalMethodGetProperty, false, kDefaultDBusTimeout,
                       charge_level_current_.NewSlot(), MESSAGE_TYPE_STRING,
                       kHalPropBatteryChargeLevelCurrent,
                       MESSAGE_TYPE_INVALID);
  battery_->CallMethod(kHalMethodGetProperty, false, kDefaultDBusTimeout,
                       charge_level_rate_.NewSlot(), MESSAGE_TYPE_STRING,
                       kHalPropBatteryChargeLevelRate,
                       MESSAGE_TYPE_INVALID);
}

void Power::LoadAcAdapterInfo() {
  if (!ac_adapter_) return;
  DLOG("Load ac adapter info.");

  ac_adapter_->CallMethod(kHalMethodGetProperty, false, kDefaultDBusTimeout,
                          is_plugged_in_.NewSlot(),
                          MESSAGE_TYPE_STRING, kHalPropACAdapterPresent,
                          MESSAGE_TYPE_INVALID);
}

bool Power::IsCharging() {
  return is_charging_.GetValue();
}

bool Power::IsPluggedIn() {
  return is_plugged_in_.GetValue() || !battery_;
}

int Power::GetPercentRemaining() {
  if (percent_remaining_.GetValue() > 0)
    return static_cast<int>(percent_remaining_.GetValue());

  if (charge_level_design_.GetValue() > 0)
    return static_cast<int>(charge_level_current_.GetValue() * 100 /
        charge_level_design_.GetValue());

  return 0;
}

int Power::GetTimeRemaining() {
  if (time_remaining_.GetValue() > 0)
    return static_cast<int>(time_remaining_.GetValue());

  if (charge_level_rate_.GetValue() > 0) {
    // If the battery is charging then return the remaining time to full.
    // else return the remaining time to empty.
    if (IsCharging()) {
      return static_cast<int>(
          (charge_level_design_.GetValue() -
           charge_level_current_.GetValue()) /
          charge_level_rate_.GetValue());
    } else {
      return static_cast<int>(charge_level_current_.GetValue() /
                              charge_level_rate_.GetValue());
    }
  }
  return 0;
}

int Power::GetTimeTotal() {
  if (charge_level_rate_.GetValue() > 0)
    return static_cast<int>(charge_level_design_.GetValue() /
                            charge_level_rate_.GetValue());
  return 0;
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
