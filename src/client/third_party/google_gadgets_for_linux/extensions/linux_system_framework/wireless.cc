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
#include "wireless.h"

#include <algorithm>
#include <cstddef>
#include <time.h>

#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/slot.h>

// defined in <linux/wireless.h>, but we don't want to introduce such
// dependency.
#define IW_MODE_AUTO 0
#define IW_MODE_ADHOC 1
#define IW_MODE_INFRA 2

#if !defined(NM_DBUS_SERVICE) || !defined(NM_DBUS_PATH) || \
    !defined(NM_DBUS_INTERFACE)
#error "NetworkManager.h is not available or incorrect?"
#endif

// for both nm 0.6.x and 0.7.x
static const char kNMService[] = NM_DBUS_SERVICE;
static const char kNMPath[] = NM_DBUS_PATH;
static const char kNMInterface[] = NM_DBUS_INTERFACE;

// for nm 0.6.x
static const char kNMInterfaceDevices[] =
  NM_DBUS_INTERFACE ".Devices";

// for nm 0.7.x
static const char kNMInterfaceDevice[] =
  NM_DBUS_INTERFACE ".Device";
static const char kNMInterfaceDeviceWireless[] =
  NM_DBUS_INTERFACE ".Device.Wireless";
static const char kNMInterfaceAccessPoint[] =
  NM_DBUS_INTERFACE ".AccessPoint";
static const char kNMInterfaceConnectionActive[] =
  NM_DBUS_INTERFACE ".Connection.Active";

static const char kNMServiceUserSettings[] =
  "org.freedesktop.NetworkManagerUserSettings";
static const char kNMServiceSystemSettings[] =
  "org.freedesktop.NetworkManagerSystemSettings";
static const char kNMPathSettings[] =
  "/org/freedesktop/NetworkManagerSettings";
static const char kNMInterfaceSettings[] =
  "org.freedesktop.NetworkManagerSettings";
static const char kNMInterfaceSettingsConnection[] =
  "org.freedesktop.NetworkManagerSettings.Connection";
static const char kNMInterfaceSettingsSystem[] =
  "org.freedesktop.NetworkManagerSettings.System";

namespace ggadget {
namespace framework {
namespace linux_system {

using namespace ggadget::dbus;

//#ifdef NM_DEVICE_TYPE_WIFI
static const int kDeviceTypeWifi = NM_DEVICE_TYPE_WIFI;
//#else
//static const int kDeviceTypeWifi = DEVICE_TYPE_802_11_WIRELESS;
//#endif

// Defined in nm 0.7
static const int kDeviceStateActivated = 8;
static const int kDeviceStateFailed = 9;

class Wireless::Impl {
  class WirelessAccessPoint : public WirelessAccessPointInterface {
   public:
    /**
     * @param dev_path object path to the associated device.
     * @param ap_path object path to the access point.
     * @param new_api true to use nm 0.7.x api, false to use nm 0.6.x api.
     */
    WirelessAccessPoint(Impl *impl, const std::string &dev_path,
                        const std::string &ap_path, bool new_api)
      : impl_(impl),
        dev_path_(dev_path),
        ap_path_(ap_path),
        new_api_(new_api),
        type_(WIRELESS_TYPE_ANY),
        strength_(0),
        ap_(NULL),
        on_signal_connection_(NULL) {
      if (new_api_) {
        ap_ = DBusProxy::NewSystemProxy(kNMService, ap_path,
                                        kNMInterfaceAccessPoint);
        if (ap_) {
          on_signal_connection_ = ap_->ConnectOnSignalEmit(
              NewSlot(this, &WirelessAccessPoint::OnSignal));
        }
      } else {
        ap_ = DBusProxy::NewSystemProxy(kNMService, ap_path,
                                        kNMInterfaceDevices);
        if (ap_) {
          // nm 0.6.x only emits signals on NetworkManager object.
          on_signal_connection_ = impl_->network_manager_->ConnectOnSignalEmit(
              NewSlot(this, &WirelessAccessPoint::OnSignal));
        }
      }
      if (ap_) {
        UpdateInfo();
      } else {
        DLOG("Failed to create proxy for wireless ap: %s, for device %s",
             ap_path.c_str(), dev_path.c_str());
      }
    }
    virtual ~WirelessAccessPoint() {
      if (on_signal_connection_)
        on_signal_connection_->Disconnect();
      delete ap_;
    }
    virtual void Destroy() {
      delete this;
    }
    virtual std::string GetName() const {
      return name_;
    }
    virtual Type GetType() const {
      return type_;
    }
    virtual int GetSignalStrength() const {
      return strength_;
    }
    virtual void Connect(Slot1<void, bool> *callback) {
      impl_->Connect(dev_path_, ap_path_, name_, callback);
    }
    virtual void Disconnect(Slot1<void, bool> *callback) {
      impl_->Disconnect(dev_path_, name_, callback);
    }
    std::string GetPath() const {
      return ap_path_;
    }
    bool IsValid() const {
      return ap_ && name_.length();
    }

   private:
    void OnSignal(const std::string &signal, int argc, const Variant *argv) {
      DLOG("Signal received for ap %s: %s", ap_path_.c_str(), signal.c_str());
      // Only care about strength change. Other information are not likely to
      // be changed.
      if (signal == "WirelessNetworkStrengthChanged") { // nm 0.6.x
        std::string dev_path;
        std::string ap_path;
        if (argc == 3 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_ && argv[1].ConvertToString(&ap_path) &&
            ap_path == ap_path_ && argv[2].type() == Variant::TYPE_INT64) {
          strength_ = VariantValue<int>()(argv[2]);
        }
      } else if (signal == "DeviceStrengthChanged") { // nm 0.6.x
        std::string dev_path;
        if (impl_->device_ && impl_->device_->GetActiveAPPath() == ap_path_ &&
            argc == 2 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_ && argv[1].type() == Variant::TYPE_INT64) {
          strength_ = VariantValue<int>()(argv[1]);
        }
      } else if (signal == "PropertiesChanged") { // nm 0.7.x
        if (argc == 1 && argv[0].type() == Variant::TYPE_SCRIPTABLE) {
          ScriptableInterface *props =
              VariantValue<ScriptableInterface*>()(argv[0]);
          Variant var = props->GetProperty("Strength").v();
          if (var.type() == Variant::TYPE_INT64)
            strength_ = VariantValue<int>()(var);
        }
      }
    }

    void UpdateInfo() {
      if (ap_) {
        int mode = IW_MODE_AUTO;
        if (new_api_) { // nm 0.7.x
          ResultVariant prop = ap_->GetProperty("Ssid");
          if (prop.v().type() == Variant::TYPE_SCRIPTABLE) {
            name_ =
                Impl::ParseSSID(VariantValue<ScriptableInterface*>()(prop.v()));
          }
          prop = ap_->GetProperty("Mode");
          if (prop.v().type() == Variant::TYPE_INT64) {
            mode = VariantValue<int>()(prop.v());
          }
          prop = ap_->GetProperty("Strength");
          if (prop.v().type() == Variant::TYPE_INT64) {
            strength_ = VariantValue<int>()(prop.v());
          }
        } else {  // nm 0.6.x
          DBusStringReceiver str_receiver;
          DBusIntReceiver int_receiver;
          if (ap_->CallMethod("getName", true, kDefaultDBusTimeout,
                              str_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
            name_ = str_receiver.GetValue();
          }
          if (ap_->CallMethod("getMode", true, kDefaultDBusTimeout,
                              int_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
            mode = static_cast<int>(int_receiver.GetValue());
          }
          if (ap_->CallMethod("getStrength", true, kDefaultDBusTimeout,
                              int_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
            strength_ = static_cast<int>(int_receiver.GetValue());
          }
        }

        if (mode == IW_MODE_ADHOC)
          type_ = WIRELESS_TYPE_INDEPENDENT;
        else if (mode == IW_MODE_INFRA)
          type_ = WIRELESS_TYPE_INFRASTRUCTURE;
        else
          type_ = WIRELESS_TYPE_ANY;
      }
    }

   private:
    Impl *impl_;
    std::string dev_path_;
    std::string ap_path_;
    bool new_api_;

    std::string name_;
    Type type_;
    int strength_;

    DBusProxy *ap_;
    Connection *on_signal_connection_;
  };

  class WirelessDevice {
   public:
    WirelessDevice(Impl *impl, const std::string &dev_path, bool new_api)
      : impl_(impl),
        dev_path_(dev_path),
        new_api_(new_api),
        valid_(false),
        connected_(false),
        dev_(NULL),
        dev_wireless_(NULL),
        active_ap_(NULL),
        on_dev_signal_connection_(NULL),
        on_wireless_signal_connection_(NULL),
        connect_callback_(NULL) {
      if (new_api) {
        dev_ = DBusProxy::NewSystemProxy(kNMService, dev_path,
                                         kNMInterfaceDevice);
        if (dev_) {
          dev_wireless_ =
              DBusProxy::NewSystemProxy(kNMService, dev_path,
                                        kNMInterfaceDeviceWireless);
          if (dev_wireless_) {
            on_dev_signal_connection_ = dev_->ConnectOnSignalEmit(
                NewSlot(this, &WirelessDevice::OnSignal));
            on_wireless_signal_connection_ = dev_wireless_->ConnectOnSignalEmit(
                NewSlot(this, &WirelessDevice::OnSignal));
            UpdateInfo();
          } else {
            delete dev_;
            dev_ = NULL;
          }
        }
      } else {
        dev_ = DBusProxy::NewSystemProxy(kNMService, dev_path,
                                         kNMInterfaceDevices);
        if (dev_) {
          // nm 0.6.x only emits signals on NetworkManager object.
          on_dev_signal_connection_ =
              impl_->network_manager_->ConnectOnSignalEmit(
                  NewSlot(this, &WirelessDevice::OnSignal));
          UpdateInfo();
        }
      }

      if (!dev_) {
        DLOG("Failed to create proxy for wireless device: %s",
             dev_path.c_str());
      }
    }
    ~WirelessDevice() {
      if (on_dev_signal_connection_)
        on_dev_signal_connection_->Disconnect();
      if (on_wireless_signal_connection_)
        on_wireless_signal_connection_->Disconnect();
      delete active_ap_;
      delete dev_;
      delete dev_wireless_;
      delete connect_callback_;
    }
    std::string GetDevicePath() const {
      return dev_path_;
    }
    std::string GetName() const {
      return name_;
    }
    std::string GetNetworkName() const {
      return active_ap_ ? active_ap_->GetName() : std::string();
    }
    int GetSignalStrength() const {
      return active_ap_ ? active_ap_->GetSignalStrength() : 0;
    }
    int GetAPCount() const {
      return static_cast<int>(access_points_.size());
    }
    WirelessAccessPoint *GetWirelessAccessPoint(int index) const {
      if (index >= 0 && index < static_cast<int>(access_points_.size())) {
        return new WirelessAccessPoint(impl_, dev_path_,
                                       access_points_[index], new_api_);
      }
      return NULL;
    }
    bool IsConnected() const {
      return connected_;
    }
    bool EnumerationSupported() const {
      return access_points_.size() != 0;
    }
    bool IsValid() const {
      return valid_;
    }
    void SetConnectCallback(Slot1<void, bool> *callback) {
      delete connect_callback_;
      connect_callback_ = callback;
    }
    std::string GetActiveAPPath() const {
      return active_ap_ ? active_ap_->GetPath() : std::string();
    }

   private:
    void AddAccessPoint(const std::string &ap) {
      StringVector::iterator it =
          std::find(access_points_.begin(), access_points_.end(), ap);
      if (it == access_points_.end()) {
        DLOG("Access point %s added to device %s",
             ap.c_str(), dev_path_.c_str());
        access_points_.push_back(ap);
      }
    }
    void RemoveAccessPoint(const std::string &ap) {
      StringVector::iterator it =
          std::find(access_points_.begin(), access_points_.end(), ap);
      if (it != access_points_.end()) {
        DLOG("Access point %s removed from device %s",
             ap.c_str(), dev_path_.c_str());
        access_points_.erase(it);
      }
    }

    void OnSignal(const std::string &signal, int argc, const Variant *argv) {
      DLOG("Signal received for dev %s: %s", dev_path_.c_str(), signal.c_str());
      bool connect_performed = false;
      if (signal == "DeviceNowActive") { // nm 0.6.x
        std::string dev_path;
        if (argc >= 1 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_) {
          connect_performed = true;
          connected_ = true;
        }
      } else if (signal == "DeviceNoLongerActive") { // nm 0.6.x
        std::string dev_path;
        if (argc >= 1 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_) {
          connected_ = false;
        }
      } else if (signal == "DeviceActivationFailed") { // nm 0.6.x
        std::string dev_path;
        if (argc >= 1 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_) {
          connected_ = false;
          connect_performed = true;
        }
      } else if (signal == "WirelessNetworkAppeared") { // nm 0.6.x
        std::string dev_path;
        std::string net_path;
        if (argc >= 2 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_ && argv[1].ConvertToString(&net_path)) {
          AddAccessPoint(net_path);
        }
      } else if (signal == "WirelessNetworkDisappeared") { // nm 0.6.x
        std::string dev_path;
        std::string net_path;
        if (argc >= 2 && argv[0].ConvertToString(&dev_path) &&
            dev_path == dev_path_ && argv[1].ConvertToString(&net_path)) {
          RemoveAccessPoint(net_path);
        }
      } else if (signal == "StateChanged") { // nm 0.7.x
        int new_state;
        if (argc >= 1 && argv[0].ConvertToInt(&new_state)) {
          connected_ = (new_state == kDeviceStateActivated);
          connect_performed = (new_state == kDeviceStateActivated ||
                               new_state == kDeviceStateFailed);
        }
      } else if (signal == "AccessPointAdded") { // nm 0.7.x
        std::string ap_path;
        if (argc >= 1 && argv[0].ConvertToString(&ap_path)) {
          AddAccessPoint(ap_path);
        }
      } else if (signal == "AccessPointRemoved") { // nm 0.7.x
        std::string ap_path;
        if (argc >= 1 && argv[0].ConvertToString(&ap_path)) {
          RemoveAccessPoint(ap_path);
        }
      }
      // Is it necessary to handl PropertiesChanged signal?

      if (!connected_) {
        delete active_ap_;
        active_ap_ = NULL;
      }

      if (connect_performed) {
        UpdateActiveAP();
        if (connect_callback_) {
          (*connect_callback_)(connected_);
          delete connect_callback_;
          connect_callback_ = NULL;
        }
      }
    }

    void UpdateConnected() {
      connected_ = false;
      if (new_api_ && dev_ && dev_wireless_) {
        ResultVariant prop = dev_->GetProperty("State");
        if (prop.v().type() == Variant::TYPE_INT64) {
          connected_ = (VariantValue<int>()(prop.v()) == kDeviceStateActivated);
        }
      } else if (!new_api_ && dev_) {
        DBusBooleanReceiver bool_receiver;
        if (dev_->CallMethod("getLinkActive", true, kDefaultDBusTimeout,
                             bool_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
          connected_ = bool_receiver.GetValue();
        }
      }
    }
    void UpdateName() {
      name_.clear();
      if (new_api_ && dev_ && dev_wireless_) {
        ResultVariant prop = dev_->GetProperty("Interface");
        if (prop.v().type() == Variant::TYPE_STRING) {
          name_ = VariantValue<std::string>()(prop.v());
        }
      } else if (!new_api_ && dev_) {
        DBusStringReceiver str_receiver;
        if (dev_->CallMethod("getName", true, kDefaultDBusTimeout,
                             str_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
          name_ = str_receiver.GetValue();
        }
      }
    }
    void UpdateAccessPoints() {
      DBusStringArrayReceiver aplist_receiver(&access_points_);
      if (new_api_ && dev_ && dev_wireless_) {
        dev_wireless_->CallMethod("GetAccessPoints", true,
                                   kDefaultDBusTimeout,
                                   aplist_receiver.NewSlot(),
                                   MESSAGE_TYPE_INVALID);
      } else if (!new_api_ && dev_) {
        dev_->CallMethod("getNetworks", true, kDefaultDBusTimeout,
                         aplist_receiver.NewSlot(), MESSAGE_TYPE_INVALID);
      }
    }
    void UpdateActiveAP() {
      delete active_ap_;
      active_ap_ = NULL;
      if (!connected_ || !dev_)
        return;
      if (new_api_ && dev_wireless_) {
        ResultVariant prop = dev_wireless_->GetProperty("ActiveAccessPoint");
        if (prop.v().type() == Variant::TYPE_STRING) {
          std::string ap_path = VariantValue<std::string>()(prop.v());
          active_ap_ =
              new WirelessAccessPoint(impl_, dev_path_, ap_path, new_api_);
        }
      } else if (!new_api_) {
        DBusStringReceiver str_receiver;
        if (dev_->CallMethod("getActiveNetwork", true, kDefaultDBusTimeout,
                             str_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
          std::string ap_path = str_receiver.GetValue();
          active_ap_ =
              new WirelessAccessPoint(impl_, dev_path_, ap_path, new_api_);
        }
      }
      if (!active_ap_ || !active_ap_->IsValid()) {
        connected_ = false;
        delete active_ap_;
        active_ap_ = NULL;
      }
    }

    void UpdateInfo() {
      valid_ = false;
      if (new_api_ && dev_ && dev_wireless_) {
        ResultVariant prop = dev_->GetProperty("DeviceType");
        if (prop.v().type() == Variant::TYPE_INT64) {
          int type = VariantValue<int>()(prop.v());
          valid_ = (type == kDeviceTypeWifi);
        }
      } else if (!new_api_ && dev_) {
        DBusIntReceiver int_receiver;
        if (dev_->CallMethod("getType", true, kDefaultDBusTimeout,
                             int_receiver.NewSlot(), MESSAGE_TYPE_INVALID)) {
          valid_ = (int_receiver.GetValue() == kDeviceTypeWifi);
        }
      }

      if (valid_) {
        UpdateConnected();
        UpdateName();
        UpdateAccessPoints();
        UpdateActiveAP();
        DLOG("Found wireless device: %s, interface: %s",
             dev_path_.c_str(), name_.c_str());
      } else {
        DLOG("%s is not a wireless device", dev_path_.c_str());
      }
    }

   private:
    Impl *impl_;
    std::string dev_path_;
    bool new_api_;
    std::string name_;
    bool valid_;
    bool connected_;

    StringVector access_points_;
    DBusProxy *dev_;
    DBusProxy *dev_wireless_;
    WirelessAccessPoint *active_ap_;
    Connection *on_dev_signal_connection_;
    Connection *on_wireless_signal_connection_;
    Slot1<void, bool> *connect_callback_;
  };

 public:
  Impl()
    : new_api_(false),
      device_(NULL),
      network_manager_(NULL),
      on_signal_connection_(NULL) {
    network_manager_ =
        DBusProxy::NewSystemProxy(kNMService, kNMPath, kNMInterface);
    if (!network_manager_) {
      DLOG("Network Manager is not available.");
      return;
    }

    // Checks if it's nm 0.7 or above. nm 0.6.x doesn't have introspectable data
    // nor these method and signal.
    if (network_manager_->GetMethodInfo("GetDevices", NULL, NULL, NULL, NULL) &&
        network_manager_->GetSignalInfo("StateChanged", NULL, NULL)) {
      DLOG("network manager 0.7 or above is available.");
      new_api_ = true;
    } else {
      DLOG("network manager 0.6.x might be used.");
    }

    on_signal_connection_ = network_manager_->ConnectOnSignalEmit(
        NewSlot(this, &Impl::OnSignal));
    UpdateWirelessDevice();
  }
  ~Impl() {
    if (on_signal_connection_)
      on_signal_connection_->Disconnect();
    delete device_;
    delete network_manager_;
  }
  bool IsAvailable() {
    return device_ != NULL;
  }
  bool IsConnected() {
    return device_ ? device_->IsConnected() : false;
  }
  bool EnumerationSupported() {
    return device_ ? device_->EnumerationSupported() : false;
  }
  int GetAPCount() {
    return device_ ? device_->GetAPCount() : 0;
  }
  WirelessAccessPointInterface* GetWirelessAccessPoint(int index) {
    return device_ ? device_->GetWirelessAccessPoint(index) : NULL;
  }
  std::string GetName() {
    return device_ ? device_->GetName() : std::string();
  }
  std::string GetNetworkName() {
    return device_ ? device_->GetNetworkName() : std::string();
  }
  int GetSignalStrength() {
    return device_ ? device_->GetSignalStrength() : 0;
  }

  void ConnectAP(const char *ap_name, Slot1<void, bool> *callback) {
    if (device_ && ap_name && *ap_name) {
      if (device_->GetNetworkName() == ap_name) {
        if (callback) {
          (*callback)(true);
          delete callback;
          return;
        }
      } else {
        int count = device_->GetAPCount();
        for (int i = 0; i < count; ++i) {
          WirelessAccessPoint *ap = device_->GetWirelessAccessPoint(i);
          if (ap && ap->GetName() == ap_name) {
            Connect(device_->GetDevicePath(), ap->GetPath(), ap->GetName(),
                    callback);
            delete ap;
            return;
          }
          delete ap;
        }
      }
    }
    if (callback) {
      (*callback)(false);
      delete callback;
    }
  }

  void DisconnectAP(const char *ap_name, Slot1<void, bool> *callback) {
    if (device_ && ap_name && *ap_name) {
      Disconnect(device_->GetDevicePath(), ap_name, callback);
      return;
    }
    if (callback) {
      (*callback)(false);
      delete callback;
    }
  }

 private:
  void UpdateWirelessDevice() {
    delete device_;
    device_ = NULL;

    StringVector dev_list;
    DBusStringArrayReceiver str_receiver(&dev_list);
    if (network_manager_->CallMethod(new_api_ ? "GetDevices" : "getDevices",
                                     true, kDefaultDBusTimeout,
                                     str_receiver.NewSlot(),
                                     MESSAGE_TYPE_INVALID)) {
      for (StringVector::iterator it = dev_list.begin();
           it != dev_list.end(); ++it) {
        WirelessDevice *dev = new WirelessDevice(this, *it, new_api_);
        if (dev->IsValid()) {
          device_ = dev;
          return;
        }
        delete dev;
      }
    }
  }
  void OnSignal(const std::string &signal, int argc, const Variant *argv) {
    DLOG("Signal received for nm %s: %s",
         new_api_ ? "0.7.x" : "0.6.x", signal.c_str());
    // Both nm 0.6.x and 0.7.x
    if (signal == "DeviceAdded" || signal == "DeviceRemoved") {
      UpdateWirelessDevice();
    } else if (signal == "PropertiesChanged") {
      if (argc >= 1 && argv[0].type() == Variant::TYPE_SCRIPTABLE) {
        ScriptableInterface *props =
            VariantValue<ScriptableInterface*>()(argv[0]);
        ResultVariant prop;
        prop = props->GetProperty("WirelessEnabled");
        if (prop.v().type() == Variant::TYPE_BOOL) {
          DLOG("Wireless %s",
               VariantValue<bool>()(prop.v()) ? "enabled" : "disabled");
        }
      }
    }
  }

  void Connect(const std::string &dev_path, const std::string &ap_path,
               const std::string &ap_name, Slot1<void, bool> *callback) {
    if (device_) {
      DLOG("Connect to device: %s, ap: %s, ssid: %s",
           dev_path.c_str(), ap_path.c_str(), ap_name.c_str());
      if (new_api_) {
        std::string service;
        std::string connection;
        if (GetConnection(ap_name, &service, &connection)) {
          Variant argv[4] = {
            Variant(service), Variant(connection),
            Variant(dev_path), Variant(ap_path)
          };
          if (network_manager_->CallMethod("ActivateConnection", false, -1,
                                           NULL, 4, argv)) {
            device_->SetConnectCallback(callback);
            return;
          }
        }
      } else {
        if (network_manager_->CallMethod("setActiveDevice", false, -1,
                                         NULL, MESSAGE_TYPE_OBJECT_PATH,
                                         dev_path.c_str(),
                                         MESSAGE_TYPE_STRING,
                                         ap_name.c_str(),
                                         MESSAGE_TYPE_INVALID)) {
          device_->SetConnectCallback(callback);
          return;
        }
      }
    }
    if (callback) {
      (*callback)(false);
      delete callback;
    }
  }

  void Disconnect(const std::string &dev_path, const std::string &ap_name,
                  Slot1<void, bool> *callback) {
    bool result = false;
    DLOG("Disconnect from device: %s", dev_path.c_str());
    if (device_ && device_->GetNetworkName() == ap_name) {
      if (new_api_) {
        result = DeactivateConnection(dev_path);
      } else {
        // There is no way to disconnect a specific wifi device.
        // So disable wifi and enable it again.
        int ret1 = 0, ret2 = 0;
        ret1 = network_manager_->CallMethod("setWirelessEnabled", false, -1,
                                            NULL, MESSAGE_TYPE_BOOLEAN, false,
                                            MESSAGE_TYPE_INVALID);
        if (ret1) {
          ret2 = network_manager_->CallMethod("setWirelessEnabled", false, -1,
                                              NULL, MESSAGE_TYPE_BOOLEAN, true,
                                              MESSAGE_TYPE_INVALID);
        }
        result = ret1 && ret2;
      }
    }
    if (callback) {
      (*callback)(result);
      delete callback;
    }
  }

  // only for nm 0.7.x, helper class for DeactivateConnection().
  class DeactivateConnectionWorker {
   public:
    DeactivateConnectionWorker(Impl *impl, const std::string &dev_path)
      : impl_(impl), dev_path_(dev_path), result_(false) {
    }
    bool GetResult() const {
      return result_;
    }
    bool Callback(int i, const Variant &elm) {
      GGL_UNUSED(i);
      if (elm.type() == Variant::TYPE_STRING) {
        std::string con_path = VariantValue<std::string>()(elm);
        DBusProxy *con =
            DBusProxy::NewSystemProxy(kNMService, con_path,
                                      kNMInterfaceConnectionActive);
        if (con) {
          ResultVariant devices = con->GetProperty("Devices");
          delete con;
          if (devices.v().type() == Variant::TYPE_SCRIPTABLE) {
            ScriptableInterface *ptr =
                VariantValue<ScriptableInterface*>()(devices.v());
            result_ = false;
            if (ptr) {
              ptr->EnumerateElements(
                  NewSlot(this,
                          &DeactivateConnectionWorker::MatchDeviceCallback));
            }
            if (result_) {
              Variant argv[1] = { Variant(con_path) };
              impl_->network_manager_->CallMethod("DeactivateConnection",
                                                  false, -1, NULL, 1, argv);
              // Is it possible that a device has multiple active connections?
              return false;
            }
          }
        }
      }
      return true;
    }
    bool MatchDeviceCallback(int i, const Variant &dev) {
      GGL_UNUSED(i);
      std::string dev_path;
      if (dev.ConvertToString(&dev_path) && dev_path == dev_path_) {
        result_ = true;
        return false;
      }
      return true;
    }

   private:
    Impl *impl_;
    const std::string &dev_path_;
    bool result_;
  };

  /**
   * Only for nm 0.7.x, deactivate all connections of a specific device.
   */
  bool DeactivateConnection(const std::string &dev_path) {
    ResultVariant active_cons=
        network_manager_->GetProperty("ActiveConnections");
    if (active_cons.v().type() == Variant::TYPE_SCRIPTABLE) {
      ScriptableInterface *ptr =
          VariantValue<ScriptableInterface*>()(active_cons.v());
      if (ptr) {
        DeactivateConnectionWorker worker(this, dev_path);
        ptr->EnumerateElements(
            NewSlot(&worker, &DeactivateConnectionWorker::Callback));
        return worker.GetResult();
      }
    }
    return false;
  }

  static std::string GetSSIDFromSettings(ScriptableInterface *settings) {
    if (settings) {
      ResultVariant value = settings->GetProperty("802-11-wireless");
      if (value.v().type() == Variant::TYPE_SCRIPTABLE) {
        ScriptableInterface *ptr =
            VariantValue<ScriptableInterface*>()(value.v());
        if (ptr) {
          ResultVariant ssid = ptr->GetProperty("ssid");
          if (ssid.v().type() == Variant::TYPE_SCRIPTABLE)
            return ParseSSID(VariantValue<ScriptableInterface*>()(ssid.v()));
        }
      }
    }
    return std::string();
  }

  /** Only for nm 0.7.x. */
  static bool FindConnectionInSettings(DBusProxy *settings,
                                       const std::string &ssid,
                                       std::string *connection) {
    StringVector connections;
    DBusStringArrayReceiver receiver(&connections);
    if (!settings->CallMethod("ListConnections", true, kDefaultDBusTimeout,
                              receiver.NewSlot(), MESSAGE_TYPE_INVALID))
      return false;

    for (StringVector::iterator it = connections.begin();
         it != connections.end(); ++it) {
      DBusProxy *con =
          DBusProxy::NewSystemProxy(settings->GetName(), *it,
                                    kNMInterfaceSettingsConnection);
      if (con) {
        DBusScriptableReceiver receiver;
        con->CallMethod("GetSettings", true, kDefaultDBusTimeout,
                        receiver.NewSlot(), MESSAGE_TYPE_INVALID);
        // con is not required anymore.
        delete con;
        std::string con_ssid = GetSSIDFromSettings(receiver.GetValue());
        if (con_ssid == ssid) {
          *connection = *it;
          DLOG("A connection for access point %s has been found "
               "in %s, path %s", ssid.c_str(), settings->GetName().c_str(),
               it->c_str());
          return true;
        }
      }
    }
    return false;
  }

  /**
   * Only for nm 0.7.x. This piece of code doesn't work yet, due to the
   * PolicyKit issue. The AddConnection call will always fail due to lack of
   * privilege.
   */
#if 0
  static bool CreateNewConnectionInSettings(DBusProxy *settings,
                                            const std::string &ssid) {
    typedef SharedScriptable<0xa03f49b2cab5479a> ScriptableDict;
    DBusProxy *sys = settings->NewInterfaceProxy(kNMInterfaceSettingsSystem);
    if (sys) {
      ScriptableDict *data = new ScriptableDict();
      ScriptableDict *connection = new ScriptableDict();
      ScriptableDict *wireless = new ScriptableDict();
      ScriptableArray *ssid_array =
          ScriptableArray::Create(ssid.begin(), ssid.end());
      connection->RegisterVariantConstant("id", Variant("GGL Auto " + ssid));
      connection->RegisterVariantConstant("type", Variant("802-11-wireless"));
      wireless->RegisterVariantConstant("mode", Variant("infrastructure"));
      wireless->RegisterVariantConstant("ssid", Variant(ssid_array));
      data->RegisterVariantConstant("connection", Variant(connection));
      data->RegisterVariantConstant("802-11-wireless", Variant(wireless));

      data->Ref();
      Variant argv[1] = { Variant(data) };
      int id = sys->CallMethod("AddConnection", true, kDefaultDBusTimeout,
                               NULL, 1, argv);
      data->Unref();
      delete sys;
      return id != 0;
    }
    return false;
  }
#endif

  /**
   * Only for nm 0.7.x, gets a connection for specific access point.
   * Currently there is no way to create default connection for an access point
   * via nm's dbus api.
   * So we try to return an arbitrary wireless connection if no connection
   * matches the ssid exactly. Hope it works in most cases.
   */
  static bool GetConnection(const std::string &ssid,
                            std::string *service, std::string *connection) {
    // Try user settings first.
    static const char *kNMSettingsServices[] = {
      kNMServiceUserSettings,
      kNMServiceSystemSettings,
      NULL
    };

    for (size_t i = 0; kNMSettingsServices[i]; ++i) {
      DBusProxy *settings =
          DBusProxy::NewSystemProxy(kNMSettingsServices[i],
                                    kNMPathSettings,
                                    kNMInterfaceSettings);
      if (settings) {
        if (FindConnectionInSettings(settings, ssid, connection)) {
          *service = kNMSettingsServices[i];
          delete settings;
          return true;
        }
#if 0
        // Only system settings has add connection method.
        if (kNMSettingsServices[i] == kNMServiceSystemSettings) {
          if (CreateNewConnectionInSettings(settings, ssid)) {
            if (FindConnectionInSettings(settings, ssid, connection)) {
              *service = kNMSettingsServices[i];
              delete settings;
              return true;
            }
          }
        }
#endif
        delete settings;
      }
    }

    DLOG("No connection for access point %s found.", ssid.c_str());
    return false;
  }

  static bool EnumerateSSIDCallback(int i, const Variant &byte,
                                    std::string *ssid) {
    GGL_UNUSED(i);
    if (byte.type() == Variant::TYPE_INT64) {
      ssid->push_back(VariantValue<char>()(byte));
      return true;
    }
    ssid->clear();
    return false;
  }

  static std::string ParseSSID(ScriptableInterface *ssid_array) {
    std::string ssid;
    if (ssid_array) {
      ssid_array->EnumerateElements(
          NewSlot(&Impl::EnumerateSSIDCallback, &ssid));
    }
    return ssid;
  }

 private:
  bool new_api_;

  WirelessDevice *device_;
  DBusProxy *network_manager_;
  Connection *on_signal_connection_;
};

Wireless::Wireless()
  : impl_(new Impl()) {
}
Wireless::~Wireless() {
  delete impl_;
  impl_ = NULL;
}
bool Wireless::IsAvailable() const {
  return impl_->IsAvailable();
}
bool Wireless::IsConnected() const {
  return impl_->IsConnected();
}
bool Wireless::EnumerationSupported() const {
  return impl_->EnumerationSupported();
}
int Wireless::GetAPCount() const {
  return impl_->GetAPCount();
}
WirelessAccessPointInterface *Wireless::GetWirelessAccessPoint(int index) {
  return impl_->GetWirelessAccessPoint(index);
}
std::string Wireless::GetName() const {
  return impl_->GetName();
}
std::string Wireless::GetNetworkName() const {
  return impl_->GetNetworkName();
}
int Wireless::GetSignalStrength() const {
  return impl_->GetSignalStrength();
}
void Wireless::ConnectAP(const char *ap_name, Slot1<void, bool> *callback) {
  impl_->ConnectAP(ap_name, callback);
}
void Wireless::DisconnectAP(const char *ap_name, Slot1<void, bool> *callback) {
  impl_->DisconnectAP(ap_name, callback);
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
