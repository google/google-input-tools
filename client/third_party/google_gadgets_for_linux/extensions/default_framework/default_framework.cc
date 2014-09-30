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

#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/slot.h>
#include <ggadget/framework_interface.h>
#include <ggadget/file_system_interface.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/audioclip_interface.h>
#include <ggadget/locales.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_file_system.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/system_utils.h>

#define Initialize default_framework_LTX_Initialize
#define Finalize default_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    default_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace default_framework {

class DefaultMachine : public MachineInterface {
 public:
  virtual std::string GetBiosSerialNumber() const { return "Unknown"; }
  virtual std::string GetMachineManufacturer() const { return "Unknown"; }
  virtual std::string GetMachineModel() const { return "Unknown"; }
  virtual std::string GetProcessorArchitecture() const { return "Unknown"; }
  virtual int GetProcessorCount() const { return 0; }
  virtual int GetProcessorFamily() const { return 0; }
  virtual int GetProcessorModel() const { return 0; }
  virtual std::string GetProcessorName() const { return "Unknown"; }
  virtual int GetProcessorSpeed() const { return 0; }
  virtual int GetProcessorStepping() const { return 0; }
  virtual std::string GetProcessorVendor() const { return "Unknown"; }
};

class DefaultMemory : public MemoryInterface {
 public:
  virtual int64_t GetTotal() { return 0; }
  virtual int64_t GetFree() { return 0; }
  virtual int64_t GetUsed() { return 0; }
  virtual int64_t GetFreePhysical() { return 0; }
  virtual int64_t GetTotalPhysical() { return 0; }
  virtual int64_t GetUsedPhysical() { return 0; }
};

class DefaultPerfmon : public PerfmonInterface {
 public:
  virtual Variant GetCurrentValue(const char *counter_path) {
    GGL_UNUSED(counter_path);
    return Variant(0); 
  }
  virtual int AddCounter(const char *counter_path, CallbackSlot *slot) {
    GGL_UNUSED(counter_path);
    delete slot;
    return -1;
  }
  virtual void RemoveCounter(int id) { GGL_UNUSED(id); }
};

class DefaultPower : public PowerInterface {
 public:
  virtual bool IsCharging() { return false; }
  virtual bool IsPluggedIn() { return true; }
  virtual int GetPercentRemaining() { return 0; }
  virtual int GetTimeRemaining() { return 0; }
  virtual int GetTimeTotal() { return 0; }
};

class DefaultProcesses : public ProcessesInterface {
 public:
  virtual void Destroy() { }
  virtual int GetCount() const { return 0; }
  virtual ProcessInfoInterface *GetItem(int index) {
    GGL_UNUSED(index);
    return NULL; 
  }
};

class DefaultProcess : public ProcessInterface {
 public:
  virtual ProcessesInterface *EnumerateProcesses() { return &default_processes_; }
  virtual ProcessInfoInterface *GetForeground() { return NULL; }
  virtual ProcessInfoInterface *GetInfo(int pid) {
    GGL_UNUSED(pid);
    return NULL;
  }
 private:
  DefaultProcesses default_processes_;
};

class DefaultWireless : public WirelessInterface {
 public:
  virtual bool IsAvailable() const { return false; }
  virtual bool IsConnected() const { return false; }
  virtual bool EnumerationSupported() const { return false; }
  virtual int GetAPCount() const { return 0; }
  virtual WirelessAccessPointInterface *GetWirelessAccessPoint(int index) {
    GGL_UNUSED(index);
    return NULL;
  }
  virtual std::string GetName() const { return "Unknown"; }
  virtual std::string GetNetworkName() const  { return "Unknwon"; }
  virtual int GetSignalStrength() const { return 0; }
  virtual void ConnectAP(const char *ap_name, Slot1<void, bool> *callback) {
    GGL_UNUSED(ap_name);
    if (callback) {
      (*callback)(false);
      delete callback;
    }
  }
  virtual void DisconnectAP(const char *ap_name, Slot1<void, bool> *callback) {
    GGL_UNUSED(ap_name);
    if (callback) {
      (*callback)(false);
      delete callback;
    }
  }
};

class DefaultNetwork : public NetworkInterface {
 public:
  virtual bool IsOnline() { return true; }
  virtual ConnectionType GetConnectionType() {
    return NetworkInterface::CONNECTION_TYPE_802_3;
  }
  virtual PhysicalMediaType GetPhysicalMediaType() {
    return NetworkInterface::PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
  }
  virtual WirelessInterface *GetWireless() {
    return &wireless_;
  }

  DefaultWireless wireless_;
};

class DefaultFileSystem : public FileSystemInterface {
 public:
  virtual DrivesInterface *GetDrives() { return NULL; }
  virtual std::string BuildPath(const char *path, const char *name) {
    GGL_UNUSED(path);
    GGL_UNUSED(name);
    return "";
  }
  virtual std::string GetDriveName(const char *path) {
    GGL_UNUSED(path);
    return "";
  }
  virtual std::string GetParentFolderName(const char *path) {
    GGL_UNUSED(path);
    return "";
  }
  virtual std::string GetFileName(const char *path) {
    GGL_UNUSED(path);
    return "";
  }
  virtual std::string GetBaseName(const char *path) {
    GGL_UNUSED(path);
    return "";
  }
  virtual std::string GetExtensionName(const char *path) {
    GGL_UNUSED(path);
    return ""; 
  }
  virtual std::string GetAbsolutePathName(const char *path) {
    GGL_UNUSED(path);
    return "";
  }
  virtual std::string GetTempName() { return ""; }
  virtual bool DriveExists(const char *drive_spec) {
    GGL_UNUSED(drive_spec);
    return false;
  }
  virtual bool FileExists(const char *file_spec) {
    GGL_UNUSED(file_spec);
    return false;
  }
  virtual bool FolderExists(const char *folder_spec) {
    GGL_UNUSED(folder_spec);
    return false;
  }
  virtual DriveInterface *GetDrive(const char *drive_spec) {
    GGL_UNUSED(drive_spec);
    return NULL;
  }
  virtual FileInterface *GetFile(const char *file_path) {
    GGL_UNUSED(file_path);
    return NULL;
  }
  virtual FolderInterface *GetFolder(const char *folder_path) {
    GGL_UNUSED(folder_path);
    return NULL; 
  }
  virtual FolderInterface *GetSpecialFolder(SpecialFolder special_folder) {
    GGL_UNUSED(special_folder);
    return NULL;
  }
  virtual bool DeleteFile(const char *file_spec, bool force) {
    GGL_UNUSED(file_spec);
    GGL_UNUSED(force);
    return false;
  }
  virtual bool DeleteFolder(const char *folder_spec, bool force) {
    GGL_UNUSED(folder_spec);
    GGL_UNUSED(force);
    return false;
  }
  virtual bool MoveFile(const char *source, const char *dest) {
    GGL_UNUSED(source);
    GGL_UNUSED(dest);
    return false;
  }
  virtual bool MoveFolder(const char *source, const char *dest) {
    GGL_UNUSED(source);
    GGL_UNUSED(dest);
    return false;
  }
  virtual bool CopyFile(const char *source, const char *dest,
                        bool overwrite) {
    GGL_UNUSED(source);
    GGL_UNUSED(dest);
    GGL_UNUSED(overwrite);
    return false;
  }
  virtual bool CopyFolder(const char *source, const char *dest,
                          bool overwrite) {
    GGL_UNUSED(source);
    GGL_UNUSED(dest);
    GGL_UNUSED(overwrite);
    return false;
  }

  virtual FolderInterface *CreateFolder(const char *path) {
    GGL_UNUSED(path);
    return NULL;
  }
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite,
                                              bool unicode) {
    GGL_UNUSED(filename);
    GGL_UNUSED(overwrite);
    GGL_UNUSED(unicode);
    return NULL;
  }
  virtual TextStreamInterface *OpenTextFile(const char *filename,
                                            IOMode mode,
                                            bool create,
                                            Tristate format) {
    GGL_UNUSED(filename);
    GGL_UNUSED(mode);
    GGL_UNUSED(create);
    GGL_UNUSED(format);
    return NULL;
  }
  virtual BinaryStreamInterface *CreateBinaryFile(const char *filename,
                                                  bool overwrite) {
    GGL_UNUSED(filename);
    GGL_UNUSED(overwrite);
    return NULL;
  }
  virtual BinaryStreamInterface *OpenBinaryFile(const char *filename,
                                                IOMode mode,
                                                bool create) {
    GGL_UNUSED(filename);
    GGL_UNUSED(mode);
    GGL_UNUSED(create);
    return NULL;
  }
  virtual TextStreamInterface *GetStandardStream(StandardStreamType type,
                                                 bool unicode) {
    GGL_UNUSED(type);
    GGL_UNUSED(unicode);
    return NULL;
  }
  virtual std::string GetFileVersion(const char *filename) {
    GGL_UNUSED(filename);
    return "";
  }
};

class DefaultAudio : public AudioInterface {
 public:
  virtual AudioclipInterface * CreateAudioclip(const char *src) {
    GGL_UNUSED(src);
    return NULL; 
  }
};

class DefaultRuntime : public RuntimeInterface {
 public:
  virtual std::string GetAppName() const {
    return "Google Desktop";
  }
  virtual std::string GetAppVersion() const {
    return GGL_API_VERSION;
  }
  virtual std::string GetOSName() const {
    return "";
  }
  virtual std::string GetOSVersion() const {
    return "";
  }
};

class DefaultCursor : public CursorInterface {
 public:
  virtual void GetPosition(int *x, int *y)  {
    if (x) *x = 0;
    if (y) *y = 0;
  }
};

class DefaultScreen : public ScreenInterface {
 public:
  virtual void GetSize(int *width, int *height) {
    if (width) *width = 0;
    if (height) *height = 0;
  }
};

class DefaultUser : public UserInterface {
 public:
  virtual bool IsUserIdle() {
    return false;
  }
  virtual void SetIdlePeriod(time_t period) {
    GGL_UNUSED(period);
  }
  virtual time_t GetIdlePeriod() const {
    return 0;
  }
};

static DefaultMachine g_machine_;
static DefaultMemory g_memory_;
static DefaultNetwork g_network_;
static DefaultPower g_power_;
static DefaultProcess g_process_;
static DefaultFileSystem g_filesystem_;
static DefaultAudio g_audio_;
static DefaultRuntime g_runtime_;
static DefaultCursor g_cursor_;
static DefaultScreen g_screen_;
static DefaultPerfmon g_perfmon_;
static DefaultUser g_user_;

static ScriptableBios g_script_bios_(&g_machine_);
static ScriptableCursor g_script_cursor_(&g_cursor_);
static ScriptableMachine g_script_machine_(&g_machine_);
static ScriptableMemory g_script_memory_(&g_memory_);
static ScriptableNetwork g_script_network_(&g_network_);
static ScriptablePower g_script_power_(&g_power_);
static ScriptableProcess g_script_process_(&g_process_);
static ScriptableProcessor g_script_processor_(&g_machine_);
static ScriptableScreen g_script_screen_(&g_screen_);
static ScriptableUser g_script_user_(&g_user_);

static std::string DefaultGetFileIcon(const char *filename) {
  GGL_UNUSED(filename);
  return std::string("");
}

static std::string DefaultBrowseForFile(const char *filter) {
  GGL_UNUSED(filter);
  return std::string("");
}

static ScriptableArray *DefaultBrowseForFiles(const char *filter) {
  GGL_UNUSED(filter);
  return new ScriptableArray();
}

static Date DefaultLocalTimeToUniversalTime(const Date &date) {
  return date;
}

} // namespace default_framework
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::default_framework;

extern "C" {
  bool Initialize() {
    LOGI("Initialize default_framework extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize default_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  GadgetInterface *gadget) {
    LOGI("Register default_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();

    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    // ScriptableAudio is per gadget, so create a new instance here.
    ScriptableAudio *script_audio = new ScriptableAudio(&g_audio_, gadget);
    reg_framework->RegisterVariantConstant("audio", Variant(script_audio));
    reg_framework->RegisterMethod("BrowseForFile",
                                  NewSlot(DefaultBrowseForFile));
    reg_framework->RegisterMethod("BrowseForFiles",
                                  NewSlot(DefaultBrowseForFiles));

    // ScriptableGraphics is per gadget, so create a new instance here.
    ScriptableGraphics *script_graphics = new ScriptableGraphics(gadget);
    reg_framework->RegisterVariantConstant("graphics",
                                           Variant(script_graphics));

    reg_framework->RegisterVariantConstant("runtime", Variant(&g_runtime_));

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    ResultVariant prop = framework->GetProperty("system");
    if (prop.v().type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable<UINT64_C(0x002bf7e456d94f52)>();
      reg_framework->RegisterVariantConstant("system", Variant(system));
    } else {
      system = VariantValue<ScriptableInterface *>()(prop.v());
    }

    if (!system) {
      LOG("Failed to retrieve or add framework.system object.");
      return false;
    }

    RegisterableInterface *reg_system = system->GetRegisterable();
    if (!reg_system) {
      LOG("framework.system object is not registerable.");
      return false;
    }

    // ScriptableFileSystem is per gadget, so create a new instance here.
    ScriptableFileSystem *script_filesystem =
        new ScriptableFileSystem(&g_filesystem_, gadget);
    reg_system->RegisterVariantConstant("filesystem",
                                        Variant(script_filesystem));

    reg_system->RegisterVariantConstant("bios",
                                        Variant(&g_script_bios_));
    reg_system->RegisterVariantConstant("cursor",
                                        Variant(&g_script_cursor_));
    reg_system->RegisterVariantConstant("machine",
                                        Variant(&g_script_machine_));
    reg_system->RegisterVariantConstant("memory",
                                        Variant(&g_script_memory_));
    reg_system->RegisterVariantConstant("network",
                                        Variant(&g_script_network_));
    reg_system->RegisterVariantConstant("power",
                                        Variant(&g_script_power_));
    reg_system->RegisterVariantConstant("process",
                                        Variant(&g_script_process_));
    reg_system->RegisterVariantConstant("processor",
                                        Variant(&g_script_processor_));
    reg_system->RegisterVariantConstant("screen",
                                        Variant(&g_script_screen_));
    reg_system->RegisterVariantConstant("user",
                                        Variant(&g_script_user_));

    reg_system->RegisterMethod("getFileIcon", NewSlot(DefaultGetFileIcon));
    reg_system->RegisterMethod("languageCode", NewSlot(GetSystemLocaleName));
    reg_system->RegisterMethod("localTimeToUniversalTime",
                               NewSlot(DefaultLocalTimeToUniversalTime));

    // ScriptablePerfmon is per gadget, so create a new instance here.
    ScriptablePerfmon *script_perfmon =
        new ScriptablePerfmon(&g_perfmon_, gadget);

    reg_system->RegisterVariantConstant("perfmon", Variant(script_perfmon));

    return true;
  }
}
