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

#include <NetworkManager.h>
#include <ggadget/string_utils.h>
#include <ggadget/main_loop_interface.h>
#include "network.h"

namespace ggadget {
namespace framework {
namespace linux_system {

//#ifdef NM_DEVICE_TYPE_WIFI
static const int kDeviceTypeWifi = NM_DEVICE_TYPE_WIFI;
//#else
//static const int kDeviceTypeWifi = DEVICE_TYPE_802_11_WIRELESS;
//#endif

//#ifdef NM_DEVICE_TYPE_ETHERNET
static const int kDeviceTypeEthernet = NM_DEVICE_TYPE_ETHERNET;
//#else
//static const int kDeviceTypeEthernet = DEVICE_TYPE_802_3_ETHERNET;
//#endif

static const int kDeviceTypeUnknown = 0;

Network::Network()
  : is_new_api_(false),
    is_online_(true), // treats online by default
    connection_type_(CONNECTION_TYPE_802_3),
    physcial_media_type_(PHYSICAL_MEDIA_TYPE_UNSPECIFIED),
    network_manager_(NULL),
    on_signal_connection_(NULL) {
  network_manager_ = DBusProxy::NewSystemProxy(NM_DBUS_SERVICE, NM_DBUS_PATH,
                                               NM_DBUS_INTERFACE);
  if (!network_manager_) {
    DLOG("Network Manager is not available.");
    return;
  }

  // Checks if it's nm 0.7 or above. nm 0.6.x doesn't have introspectable data
  // nor these method and signal.
  if (network_manager_->GetMethodInfo("GetDevices", NULL, NULL, NULL, NULL) &&
      network_manager_->GetSignalInfo("StateChanged", NULL, NULL)) {
    DLOG("network manager 0.7 or above is available.");
    is_new_api_ = true;
    int state;
    if (network_manager_->GetProperty("State").v().ConvertToInt(&state)) {
      is_online_ = (state == NM_STATE_CONNECTED);
    }
  } else {
    DLOG("network manager 0.6.x might be used.");
    DBusIntReceiver result;
    if (network_manager_->CallMethod("state", true, kDefaultDBusTimeout,
                                     result.NewSlot(), MESSAGE_TYPE_INVALID)) {
      is_online_ = (result.GetValue() == NM_STATE_CONNECTED);
    }
  }

  on_signal_connection_ = network_manager_->ConnectOnSignalEmit(
      NewSlot(this, &Network::OnSignal));

  if (is_online_) {
    Update();
  } else {
    connection_type_ = CONNECTION_TYPE_UNKNOWN;
    physcial_media_type_ = PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
  }
}

Network::~Network() {
  if (on_signal_connection_)
    on_signal_connection_->Disconnect();
  delete network_manager_;
}

void Network::OnSignal(const std::string &name, int argc, const Variant *argv) {
  DLOG("Got signal from network manager: %s", name.c_str());
  bool need_update = false;
  // nm 0.6.x uses "StateChange", 0.7.x uses "StateChanged".
  if (name == "StateChange" || name == "StateChanged") {
    int state;
    if (argc >= 1 && argv[0].ConvertToInt(&state)) {
      is_online_ = (state == NM_STATE_CONNECTED);
      DLOG("Network is %s.", is_online_ ? "connected" : "disconnected");
      if (is_online_) {
        need_update = true;
      } else {
        connection_type_ = CONNECTION_TYPE_UNKNOWN;
        physcial_media_type_ = PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
      }
    }
  } else if (name == "PropertiesChanged" || name == "DeviceAdded" ||
             name == "DeviceRemoved" || name == "DeviceNowActive" ||
             name == "DeviceNoLongerActive") {
    need_update = is_online_;
  }
  if (need_update)
    Update();
}

void Network::Update() {
  DLOG("Update network information.");
  StringVector devices;
  DBusStringArrayReceiver result(&devices);

  if (network_manager_->CallMethod(is_new_api_ ? "GetDevices" : "getDevices",
                                   true, kDefaultDBusTimeout, result.NewSlot(),
                                   MESSAGE_TYPE_INVALID) && devices.size()) {
    std::string dev_interface(NM_DBUS_INTERFACE);
    dev_interface.append(is_new_api_ ? ".Device" : ".Devices");
    for(StringVector::iterator it = devices.begin();
        it != devices.end(); ++it) {
      DLOG("Found network device: %s", it->c_str());
      DBusProxy *dev = DBusProxy::NewSystemProxy(NM_DBUS_SERVICE, *it,
                                                 dev_interface);
      if (dev) {
        bool active = false;
        int type = kDeviceTypeUnknown;
        if (is_new_api_) {
          int state;
          if (dev->GetProperty("State").v().ConvertToInt(&state))
            active = (state == 8); // NM_DEVICE_STATE_ACTIVATED
        } else {
          DBusBooleanReceiver result;
          if (dev->CallMethod("getLinkActive", true, kDefaultDBusTimeout,
                              result.NewSlot(), MESSAGE_TYPE_INVALID))
            active = result.GetValue();
        }

        if (active) {
          if (is_new_api_) {
            dev->GetProperty("DeviceType").v().ConvertToInt(&type);
          } else {
            DBusIntReceiver result;
            if (dev->CallMethod("getType", true, kDefaultDBusTimeout,
                                result.NewSlot(), MESSAGE_TYPE_INVALID))
              type = static_cast<int>(result.GetValue());
          }
          DLOG("device %s is active, type: %d", it->c_str(), type);
        }
        delete dev;

        if (active) {
          if (type == kDeviceTypeEthernet) {
            connection_type_ = CONNECTION_TYPE_802_3;
            physcial_media_type_ = PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
          } else if (type == kDeviceTypeWifi) {
            connection_type_ = CONNECTION_TYPE_NATIVE_802_11;
            physcial_media_type_ = PHYSICAL_MEDIA_TYPE_NATIVE_802_11;
          } else {
            connection_type_ = CONNECTION_TYPE_UNKNOWN;
            physcial_media_type_ = PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
          }

          // No need to check more devices.
          if (connection_type_ != CONNECTION_TYPE_UNKNOWN)
            break;
        }
      } else {
        DLOG("Failed to create dbus object for device: %s", it->c_str());
      }
    }
  }

  // Always return 802.3 type if the connection type is unknown.
  if (connection_type_ == CONNECTION_TYPE_UNKNOWN)
    connection_type_ = CONNECTION_TYPE_802_3;
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
