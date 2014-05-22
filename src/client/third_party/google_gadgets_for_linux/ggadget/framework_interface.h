/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_FRAMEWORK_INTERFACE_H__
#define GGADGET_FRAMEWORK_INTERFACE_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/slot.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;
class Slot;

namespace framework {

/**
 * @defgroup FrameworkInterfaces Framework interfaces
 * @ingroup Interfaces
 * @{
 */

/**
 * These modes are for framework.BrowseForFile() and framework.BrowseForFiles().
 * There is no C++ interface for the methods. The script methods are registered
 * in framework extension modules.
 * The prototype of the methods are like:
 *   - string BrowseForFile(string filter, optional string title,
 *                          optional int mode, optional string default_name);
 * and
 *   - collection BrowseForFiles(string filter, optional string title,
 *                               optional int mode);
 */
enum BrowseForFileMode {
  /**
   * Default mode.
   */
  BROWSE_FILE_MODE_OPEN,
  /**
   * Indicates the caller wants folder(s) instead of file(s).
   * If this flag is specified, the filter parameter may be ignored (depending
   * on the underlying system).
   */
  BROWSE_FILE_MODE_FOLDER,
  /**
   * Indicates the caller wants to get a filename to save as.
   * Behavior is undefined if this mode is used in BrowseForFiles().
   */
  BROWSE_FILE_MODE_SAVEAS
};

class FileSystemInterface;
class AudioclipInterface;
class WirelessInterface;

/** Interface for retrieving the information of the machine. */
class MachineInterface {
 protected:
  virtual ~MachineInterface() {}

 public:
  /** Retrieves the BIOS Serial number. */
  virtual std::string GetBiosSerialNumber() const = 0;
  /** Retrieves the machine's manuafacturere name. */
  virtual std::string GetMachineManufacturer() const = 0;
  /** Retrieves the machine's model. */
  virtual std::string GetMachineModel() const = 0;
  /** Retrieves the machine's architecture. */
  virtual std::string GetProcessorArchitecture() const = 0;
  /** Retrieves the number of processors running the gadget. */
  virtual int GetProcessorCount() const = 0;
  /** Retrieves the family name of the processor. */
  virtual int GetProcessorFamily() const = 0;
  /** Retrieves the model number of the processor. */
  virtual int GetProcessorModel() const = 0;
  /** Retrieves the processor's name. */
  virtual std::string GetProcessorName() const = 0;
  /** Gets the speed of the processor, in MHz. */
  virtual int GetProcessorSpeed() const = 0;
  /** Retrieves the step designation of the processor. */
  virtual int GetProcessorStepping() const = 0;
  /** Gets the processor's vendor name. */
  virtual std::string GetProcessorVendor() const = 0;
};

/** Interface for retrieving the information of the memory. */
class MemoryInterface {
 protected:
  virtual ~MemoryInterface() {}

 public:
  /** Gets the total number of bytes of virtual memory. */
  virtual int64_t GetTotal() = 0;
  /** Gets the total number of bytes of virtual memory currently free. */
  virtual int64_t GetFree() = 0;
  /** Gets the number of bytes of virtual memory currently in use. */
  virtual int64_t GetUsed() = 0;
  /** Gets the number of bytes of physical memory currently free. */
  virtual int64_t GetFreePhysical() = 0;
  /** Gets the total number of bytes of physical memory. */
  virtual int64_t GetTotalPhysical() = 0;
  /** Gets the number of bytes of physical memory currently in use. */
  virtual int64_t GetUsedPhysical() = 0;
};

/** Interface for retrieving the information about the network. */
class NetworkInterface {
 protected:
  virtual ~NetworkInterface() {}

 public:
  /** The network connection type. */
  enum ConnectionType {
    CONNECTION_TYPE_UNKNOWN = -1,
    CONNECTION_TYPE_802_3 = 0,
    CONNECTION_TYPE_802_5 = 1,
    CONNECTION_TYPE_FDDI = 2,
    CONNECTION_TYPE_WAN = 3,
    CONNECTION_TYPE_LOCAL_TALK = 4,
    CONNECTION_TYPE_DIX = 5,
    CONNECTION_TYPE_ARCNET_RAW = 6,
    CONNECTION_TYPE_ARCNET_878_2 = 7,
    CONNECTION_TYPE_ATM = 8,
    CONNECTION_TYPE_WIRELESS_WAN = 9,
    CONNECTION_TYPE_IRDA = 10,
    CONNECTION_TYPE_BPC = 11,
    CONNECTION_TYPE_CO_WAN = 12,
    CONNECTION_TYPE_1394 = 13,
    CONNECTION_TYPE_INFINI_BAND = 14,
    CONNECTION_TYPE_TUNNEL = 15,
    CONNECTION_TYPE_NATIVE_802_11 = 16,
    CONNECTION_TYPE_XDSL = 17,
    CONNECTION_TYPE_BLUETOOTH = 18,
  };

  /** The network connection physical media type. */
  enum PhysicalMediaType {
    /** None of the following. */
    PHYSICAL_MEDIA_TYPE_UNSPECIFIED = 0,
    /**
     * A wireless LAN network through a miniport driver that conforms to the
     * 802.11 interface.
     */
    PHYSICAL_MEDIA_TYPE_WIRELESS_LAN = 1,
    /** A DOCSIS-based cable network. */
    PHYSICAL_MEDIA_TYPE_CABLE_MODEM = 2,
    /** Standard phone lines. */
    PHYSICAL_MEDIA_TYPE_PHONE_LINE = 3,
    /** Wiring that is connected to a power distribution system. */
    PHYSICAL_MEDIA_TYPE_POWER_LINE = 4,
    /** A Digital Subscriber line (DSL) network. */
    PHYSICAL_MEDIA_TYPE_DSL = 5,
    /** A Fibre Channel interconnect. */
    PHYSICAL_MEDIA_TYPE_FIBRE_CHANNEL = 6,
    /** An IEEE 1394 (firewire) bus. */
    PHYSICAL_MEDIA_TYPE_1394 = 7,
    /** A Wireless WAN link. */
    PHYSICAL_MEDIA_TYPE_WIRELESS_WAN = 8,
    /** 802.11. */
    PHYSICAL_MEDIA_TYPE_NATIVE_802_11 = 9,
    /** Blue tooth. */
    PHYSICAL_MEDIA_TYPE_BLUETOOTH = 10,
  };

 public:
  /** Detects whether the network connection is on. */
  virtual bool IsOnline() = 0;
  /** Gets the type of the connection. */
  virtual ConnectionType GetConnectionType() = 0;
  /** Gets the type of the physical media. */
  virtual PhysicalMediaType GetPhysicalMediaType() = 0;

  /**
   * Gets the Wireless object containing information about the system's
   * wireless connection.
   */
  virtual WirelessInterface *GetWireless() = 0;
};

/**
 * Interface for emulating Windows Perfmon API.
 */
class PerfmonInterface {
 protected:
  virtual ~PerfmonInterface() {}

 public:
  /**
   * Callback for perfmon counter.
   *
   * The first parameter is the counter_path, the second parameter is the new
   * value of the counter_path.
   */
  typedef Slot2<void, const char *, const Variant &> CallbackSlot;

  /** Get the current value for the specified counter. */
  virtual Variant GetCurrentValue(const char *counter_path) = 0;

  /**
   * Add a performance counter.
   *
   * @param counter_path Path of the counter to be monitored.
   * @param slot A slot to be called when the value of the monitored
   * counter is changed. The slot is owned by the PerfmonInterface instance,
   * and shall be deleted when removing the counter.
   *
   * @return an unique id of the counter, which can be used to remove the
   * counter. if returns -1 means failed to add counter, and the slot will be
   * deleted immediately.
   */
  virtual int AddCounter(const char *counter_path, CallbackSlot *slot) = 0;

  /**
   * Remove a performance counter, previouslly added by AddCounter() function.
   */
  virtual void RemoveCounter(int id) = 0;
};

/** Interface for retrieving the information of the power and battery status. */
class PowerInterface {
 protected:
  virtual ~PowerInterface() {}

 public:
  /** Gets whether the battery is charning.  */
  virtual bool IsCharging() = 0;
  /** Gets whether the computer is plugged in. */
  virtual bool IsPluggedIn() = 0;
  /** Gets the remaining battery power in percentage. */
  virtual int GetPercentRemaining() = 0;
  /**
   * Gets the estimated time, in seconds, left before the battery will need to
   * be charged.
   */
  virtual int GetTimeRemaining() = 0;
  /**
   * Gets the estimated time, in seconds, the bettery will work when fully
   * charged.
   */
  virtual int GetTimeTotal() = 0;
};

/** Interface for retrieving the process information. */
class ProcessInfoInterface {
 protected:
  virtual ~ProcessInfoInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  /** Gets the process Id. */
  virtual int GetProcessId() const = 0;
  /** Gets the path of the running process. */
  virtual std::string GetExecutablePath() const = 0;
};

/** Interface for enumerating the processes. */
class ProcessesInterface {
 protected:
  virtual ~ProcessesInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  /** Get the number of processes. */
  virtual int GetCount() const = 0;
  /**
   * Get the process information according to the index.
   * @return the information of the process.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessInfoInterface *GetItem(int index) = 0;
};

/** Interface for retrieving the information of the processes. */
class ProcessInterface {
 protected:
  virtual ~ProcessInterface() {}

 public:
  /**
   * An enumeration of process IDs for all processes on the system.
   * @return the enumeration of the currently running processes.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessesInterface *EnumerateProcesses() = 0;
  /*
   * Gets the information of the foreground process.
   * @return the information of the foreground process.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessInfoInterface *GetForeground() = 0;
  /**
   * Gets the information of the specified process according to the process ID.
   * @return the information of the specified process.
   *     The user must call @c Destroy() after using this object.
   */
  virtual ProcessInfoInterface *GetInfo(int pid) = 0;
};

/** Interface for retrieving the information of the wireless access point. */
class WirelessAccessPointInterface {
 protected:
  virtual ~WirelessAccessPointInterface() {}

 public:
  enum Type {
    WIRELESS_TYPE_INFRASTRUCTURE = 0,
    WIRELESS_TYPE_INDEPENDENT = 1,
    WIRELESS_TYPE_ANY = 2,
  };

 public:
  virtual void Destroy() = 0;

 public:
  /** Gets the name of the access point. */
  virtual std::string GetName() const = 0;
  /** Gets the type of the wireless service. */
  virtual Type GetType() const = 0;
  /**
   * Gets the signal strength of the access point, expressed as a
   * percentage.
   */
  virtual int GetSignalStrength() const = 0;
  /**
   * Connects to this access point and, if the @c callback is specified and
   * non-null, calls the method with a boolean status.
   */
  virtual void Connect(Slot1<void, bool> *callback) = 0;
  /**
   * Disconnects from this access point and, if the @c callback is specified
   * and non-null, calls the method with a boolean status.
   */
  virtual void Disconnect(Slot1<void, bool> *callback) = 0;
};

/** Interface for retrieving the information of the wireless network. */
class WirelessInterface {
 protected:
  virtual ~WirelessInterface() {}

 public:
  /** Gets whether the wireless is available. */
  virtual bool IsAvailable() const = 0;
  /** Gets whether the wireless is connected. */
  virtual bool IsConnected() const = 0;
  /** Get whether the enumeration of wireless access points is supported. */
  virtual bool EnumerationSupported() const = 0;

  /**
   * Get the count of the wireless access points.
   */
  virtual int GetAPCount() const = 0;

  /**
   * Get information of an access points.
   * @return the @c WirelessAccessPointInterface pointer,
   *     or @c NULL if the access points don't support enumeration
   *     or index out of range.
   *     The user must call @c Destroy of the returned pointer after use
   *     if the return value is not @c NULL.
   */
  virtual WirelessAccessPointInterface *GetWirelessAccessPoint(int index) = 0;

  /** Get the name of the wireless adapter. */
  virtual std::string GetName() const = 0;

  /** Get the name of the network. */
  virtual std::string GetNetworkName() const = 0;

  /** Get the wireless connection's signal strength, in percentage. */
  virtual int GetSignalStrength() const = 0;

  /** Connects to a specific access point. */
  virtual void ConnectAP(const char *ap_name, Slot1<void, bool> *callback) = 0;

  /** Disconnects from a specific access point. */
  virtual void DisconnectAP(const char *ap_name,
                            Slot1<void, bool> *callback) = 0;
};


/** Interface for creating an Audioclip instance. */
class AudioInterface {
 protected:
  virtual ~AudioInterface() {}

 public:
  virtual AudioclipInterface * CreateAudioclip(const char *src) = 0;
};

/** Interface for quering information about gadget's runtime environment. */
class RuntimeInterface {
 protected:
  virtual ~RuntimeInterface() {}

 public:
  /** Get the name of this application, i.e. "Google Desktop". */
  virtual std::string GetAppName() const = 0;

  /**
   * Get the version of the running version of Google Desktop.
   * Example: "1.2.0.1". In future releases, this property may be useful
   * for conditionally using new API while still supporting older versions.
   * */
  virtual std::string GetAppVersion() const = 0;

  /** Get the name of the computer's operating system (OS). */
  virtual std::string GetOSName() const = 0;

  /** Get the operating system version. */
  virtual std::string GetOSVersion() const = 0;
};

/** Interface for querying cursor position on the screen. */
class CursorInterface {
 protected:
  virtual ~CursorInterface() {}

 public:
  virtual void GetPosition(int *x, int *y) = 0;
};

class ScreenInterface {
 protected:
  virtual ~ScreenInterface() {}

 public:
  virtual void GetSize(int *width, int *height) = 0;
};

class UserInterface {
 protected:
  virtual ~UserInterface() {}

 public:
  /**
   * Check whether user is idle.
   * @return @c true if the user is idle.
   */
  virtual bool IsUserIdle() = 0;

  /**
   * Set a value so that  if user does nothing during the last @a period
   * seconds, the user is considered to be idle.
   */
  virtual void SetIdlePeriod(time_t period) = 0;
  virtual time_t GetIdlePeriod() const = 0;
};

/** @} */

} // namespace framework
} // namespace ggadget

#endif // GGADGET_FRAMEWORK_INTERFACE_H__
