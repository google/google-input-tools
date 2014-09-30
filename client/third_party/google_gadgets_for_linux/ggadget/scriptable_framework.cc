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

#include <map>
#include "scriptable_framework.h"
#include "audioclip_interface.h"
#include "file_manager_interface.h"
#include "framework_interface.h"
#include "gadget_interface.h"
#include "image_interface.h"
#include "scriptable_array.h"
#include "scriptable_file_system.h"
#include "scriptable_image.h"
#include "signals.h"
#include "string_utils.h"
#include "unicode_utils.h"
#include "view.h"
#include "event.h"
#include "scriptable_event.h"
#include "gadget_consts.h"
#include "permissions.h"
#include "system_utils.h"
#include "small_object.h"

namespace ggadget {
namespace framework {

// Default argument list for methods that has single optional slot argument.
static const Variant kDefaultArgsForSingleSlot[] = {
  Variant(static_cast<Slot *>(NULL))
};

// Default argument list for methods whose second argument is an optional slot.
static const Variant kDefaultArgsForSecondSlot[] = {
  Variant(), Variant(static_cast<Slot *>(NULL))
};

// Implementation of ScriptableAudio
class ScriptableAudioclip : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0xa9f42ea54e2a4d13, ScriptableInterface);
  ScriptableAudioclip(AudioclipInterface *clip)
      : clip_(clip) {
    ASSERT(clip);
    clip->ConnectOnStateChange(
        NewSlot(this, &ScriptableAudioclip::OnStateChange));
  }

  virtual void DoClassRegister() {
    RegisterProperty("balance",
                     NewSlot(&AudioclipInterface::GetBalance,
                             &ScriptableAudioclip::clip_),
                     NewSlot(&AudioclipInterface::SetBalance,
                             &ScriptableAudioclip::clip_));
    RegisterProperty("currentPosition",
                     NewSlot(&AudioclipInterface::GetCurrentPosition,
                             &ScriptableAudioclip::clip_),
                     NewSlot(&AudioclipInterface::SetCurrentPosition,
                             &ScriptableAudioclip::clip_));
    RegisterProperty("duration",
                     NewSlot(&AudioclipInterface::GetDuration,
                             &ScriptableAudioclip::clip_),
                     NULL);
    RegisterProperty("error",
                     NewSlot(&AudioclipInterface::GetError,
                             &ScriptableAudioclip::clip_),
                     NULL);
    RegisterProperty("src",
                     NewSlot(&AudioclipInterface::GetSrc,
                             &ScriptableAudioclip::clip_),
                     NewSlot(&AudioclipInterface::SetSrc,
                             &ScriptableAudioclip::clip_));
    RegisterProperty("state",
                     NewSlot(&AudioclipInterface::GetState,
                             &ScriptableAudioclip::clip_),
                     NULL);
    RegisterProperty("volume",
                     NewSlot(&AudioclipInterface::GetVolume,
                             &ScriptableAudioclip::clip_),
                     NewSlot(&AudioclipInterface::SetVolume,
                             &ScriptableAudioclip::clip_));
    RegisterClassSignal("onstatechange",
                        &ScriptableAudioclip::onstatechange_signal_);
    RegisterMethod("play", NewSlot(&AudioclipInterface::Play,
                                   &ScriptableAudioclip::clip_));
    RegisterMethod("pause", NewSlot(&AudioclipInterface::Pause,
                                    &ScriptableAudioclip::clip_));
    RegisterMethod("stop", NewSlot(&AudioclipInterface::Stop,
                                   &ScriptableAudioclip::clip_));
  }

  ~ScriptableAudioclip() {
    clip_->Destroy();
    clip_ = NULL;
  }

  void OnStateChange(AudioclipInterface::State state) {
    onstatechange_signal_(this, state);
  }

  Connection *ConnectOnStateChange(Slot *slot) {
    return onstatechange_signal_.ConnectGeneral(slot);
  }

  AudioclipInterface *clip_;
  Signal2<void, ScriptableAudioclip *, AudioclipInterface::State>
      onstatechange_signal_;
};

class ScriptableAudio::Impl : public SmallObject<> {
 public:
  Impl(AudioInterface *audio, GadgetInterface *gadget)
    : audio_(audio), gadget_(gadget) {
  }

  ScriptableAudioclip *Open(const char *src, Slot *method) {
    if (!src || !*src) {
      delete method;
      return NULL;
    }

    std::string src_str;
    if (strstr(src, "://")) {
      const Permissions *perm = gadget_->GetPermissions();
      // If it's not a local file uri, then only allow it when gadget has the
      // permission to access network.
      if (strncasecmp(src, kFileUrlPrefix, arraysize(kFileUrlPrefix) - 1) &&
          (!perm || !perm->IsRequiredAndGranted(Permissions::NETWORK))) {
        LOG("No permission to access %s", src);
        return NULL;
      }
      src_str = src;
    } else if (IsAbsolutePath(src)) {
      // It's ok to play files in local filesystem.
      src_str = std::string(kFileUrlPrefix) + src;
    } else {
      // src may be a relative file name under the base path of the gadget.
      std::string extracted_file;
      if (!gadget_->GetFileManager()->ExtractFile(src, &extracted_file))
        return NULL;
      src_str = std::string(kFileUrlPrefix) + extracted_file;
    }

    AudioclipInterface *clip = audio_->CreateAudioclip(src_str.c_str());
    if (clip) {
      ScriptableAudioclip *scriptable_clip = new ScriptableAudioclip(clip);
      if (method)
        scriptable_clip->ConnectOnStateChange(method);
      return scriptable_clip;
    }

    delete method;
    return NULL;
  }

  ScriptableAudioclip *Play(const char *src, Slot *method) {
    ScriptableAudioclip *clip = Open(src, method);
    if (clip)
      clip->clip_->Play();
    return clip;
  }

  void Stop(ScriptableAudioclip *clip) {
    if (clip)
      clip->clip_->Stop();
  }

  AudioInterface *audio_;
  GadgetInterface *gadget_;
};

ScriptableAudio::ScriptableAudio(AudioInterface *audio,
                                 GadgetInterface *gadget)
    : impl_(new Impl(audio, gadget)) {
}

void ScriptableAudio::DoRegister() {
  RegisterMethod("open", NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::Open),
                                                kDefaultArgsForSecondSlot));
  RegisterMethod("play", NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::Play),
                                                kDefaultArgsForSecondSlot));
  RegisterMethod("stop", NewSlot(impl_, &Impl::Stop));
}

ScriptableAudio::~ScriptableAudio() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableRuntime
ScriptableRuntime::ScriptableRuntime(RuntimeInterface *runtime) {
  RegisterProperty("appName",
                   NewSlot(runtime, &RuntimeInterface::GetAppName), NULL);
  RegisterProperty("appVersion",
                   NewSlot(runtime, &RuntimeInterface::GetAppVersion), NULL);
  RegisterProperty("osName",
                   NewSlot(runtime, &RuntimeInterface::GetOSName), NULL);
  RegisterProperty("osVersion",
                   NewSlot(runtime, &RuntimeInterface::GetOSVersion), NULL);
}

ScriptableRuntime::~ScriptableRuntime() {
}

// Implementation of ScriptableNetwork
class ScriptableWirelessAccessPoint : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0xcf8c688383b54c43, ScriptableInterface);
  ScriptableWirelessAccessPoint(WirelessAccessPointInterface *ap)
      : ap_(ap) {
    ASSERT(ap);
  }

  virtual void DoClassRegister() {
    RegisterProperty("name",
                     NewSlot(&WirelessAccessPointInterface::GetName,
                             &ScriptableWirelessAccessPoint::ap_),
                     NULL);
    RegisterProperty("type",
                     NewSlot(&WirelessAccessPointInterface::GetType,
                             &ScriptableWirelessAccessPoint::ap_),
                     NULL);
    RegisterProperty("signalStrength",
                     NewSlot(&WirelessAccessPointInterface::GetSignalStrength,
                             &ScriptableWirelessAccessPoint::ap_),
                     NULL);
    RegisterMethod("connect",
                   NewSlotWithDefaultArgs(
                       NewSlot(&ScriptableWirelessAccessPoint::Connect),
                       kDefaultArgsForSingleSlot));
    RegisterMethod("disconnect",
                   NewSlotWithDefaultArgs(
                       NewSlot(&ScriptableWirelessAccessPoint::Disconnect),
                       kDefaultArgsForSingleSlot));
  }

  virtual ~ScriptableWirelessAccessPoint() {
    ap_->Destroy();
    ap_ = NULL;
  }

  void Connect(Slot *method) {
    ap_->Connect(method ? new SlotProxy1<void, bool>(method) : NULL);
  }

  void Disconnect(Slot *method) {
    ap_->Disconnect(method ? new SlotProxy1<void, bool>(method) : NULL);
  }

  WirelessAccessPointInterface *ap_;
};

class ScriptableWireless : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x1838DCFED2E146F3, ScriptableInterface);
  explicit ScriptableWireless(WirelessInterface *wireless)
    : wireless_(wireless) {
    ASSERT(wireless_);
    RegisterProperty(
        "available",
        NewSlot(wireless_, &WirelessInterface::IsAvailable), NULL);
    RegisterProperty(
        "connected",
        NewSlot(wireless_, &WirelessInterface::IsConnected), NULL);
    RegisterProperty(
        "enumerateAvailableAccessPoints",
        NewSlot(this, &ScriptableWireless::EnumerateAvailableAPs), NULL);
    RegisterProperty(
        "enumerationSupported",
        NewSlot(wireless_, &WirelessInterface::EnumerationSupported), NULL);
    RegisterProperty(
        "name",
        NewSlot(wireless_, &WirelessInterface::GetName), NULL);
    RegisterProperty(
        "networkName",
        NewSlot(wireless_, &WirelessInterface::GetNetworkName), NULL);
    RegisterProperty(
        "signalStrength",
        NewSlot(wireless_, &WirelessInterface::GetSignalStrength), NULL);
    RegisterMethod(
        "connect",
        NewSlotWithDefaultArgs(NewSlot(this, &ScriptableWireless::ConnectAP),
                               kDefaultArgsForSecondSlot));
    RegisterMethod(
        "disconnect",
        NewSlotWithDefaultArgs(NewSlot(this, &ScriptableWireless::DisconnectAP),
                               kDefaultArgsForSecondSlot));
  }

  ScriptableArray *EnumerateAvailableAPs() {
    ScriptableArray *array = new ScriptableArray();
    int count = wireless_->GetAPCount();
    ASSERT(count >= 0);
    for (int i = 0; i < count; ++i) {
      WirelessAccessPointInterface *ap = wireless_->GetWirelessAccessPoint(i);
      if (ap)
        array->Append(Variant(new ScriptableWirelessAccessPoint(ap)));
    }
    return array;
  }

  void ConnectAP(const char *ap_name, Slot *method) {
    wireless_->ConnectAP(ap_name,
                         method ? new SlotProxy1<void, bool>(method) : NULL);
  }

  void DisconnectAP(const char *ap_name, Slot *method) {
    wireless_->DisconnectAP(ap_name,
                            method ? new SlotProxy1<void, bool>(method) : NULL);
  }

  WirelessInterface *wireless_;
};

class ScriptableNetwork::Impl : public SmallObject<> {
 public:
  Impl(NetworkInterface *network)
    : network_(network), scriptable_wireless_(network_->GetWireless()) {
  }

  NetworkInterface *network_;
  ScriptableWireless scriptable_wireless_;
};

ScriptableNetwork::ScriptableNetwork(NetworkInterface *network)
  : impl_(new Impl(network)) {
  RegisterProperty(
      "online", NewSlot(network, &NetworkInterface::IsOnline), NULL);
  RegisterProperty(
      "connectionType",
      NewSlot(network, &NetworkInterface::GetConnectionType), NULL);
  RegisterProperty(
      "physicalMediaType",
      NewSlot(network, &NetworkInterface::GetPhysicalMediaType), NULL);
  RegisterConstant("wireless", &impl_->scriptable_wireless_);
}

ScriptableNetwork::~ScriptableNetwork() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptablePerfmon
class ScriptablePerfmon::Impl : public SmallObject<> {
 public:
  struct Counter {
    int id;
    EventSignal signal;
  };

  Impl(PerfmonInterface *perfmon, GadgetInterface *gadget)
    : perfmon_(perfmon), gadget_(gadget) {
    ASSERT(perfmon_);
    ASSERT(gadget_);
  }

  ~Impl() {
    for (CounterMap::iterator it = counters_.begin();
         it != counters_.end(); ++it) {
      perfmon_->RemoveCounter(it->second->id);
      delete it->second;
    }
  }

  void AddCounter(const char *path, Slot *slot) {
    ASSERT(path && *path && slot);

    std::string str_path(path);
    CounterMap::iterator it = counters_.find(str_path);
    if (it != counters_.end()) {
      // Remove the old one.
      perfmon_->RemoveCounter(it->second->id);
      delete it->second;
      counters_.erase(it);
    }

    Counter *counter = new Counter;
    static_cast<Signal*>(&counter->signal)->ConnectGeneral(slot);
    counter->id = perfmon_->AddCounter(path, NewSlot(this, &Impl::Call));

    if (counter->id >= 0)
      counters_[str_path] = counter;
    else
      delete counter;
  }

  void RemoveCounter(const char *path) {
    ASSERT(path && *path);
    std::string str_path(path);
    CounterMap::iterator it = counters_.find(str_path);
    if (it != counters_.end()) {
      perfmon_->RemoveCounter(it->second->id);
      delete it->second;
      counters_.erase(it);
    }
  }

  void Call(const char *path, const Variant &value) {
    ASSERT(path && *path);
    std::string str_path(path);
    CounterMap::iterator it = counters_.find(str_path);
    if (it != counters_.end()) {
      PerfmonEvent event(value);
      ScriptableEvent scriptable_event(&event, NULL, NULL);
      View *view = gadget_->GetMainView();
      ASSERT(view);
      view->FireEvent(&scriptable_event, it->second->signal);
    }
  }

  typedef LightMap<std::string, Counter *> CounterMap;
  CounterMap counters_;
  PerfmonInterface *perfmon_;
  GadgetInterface *gadget_;
};

ScriptablePerfmon::ScriptablePerfmon(PerfmonInterface *perfmon,
                                     GadgetInterface *gadget)
  : impl_(new Impl(perfmon, gadget)) {
}

void ScriptablePerfmon::DoRegister() {
  RegisterMethod("currentValue",
                 NewSlot(impl_->perfmon_, &PerfmonInterface::GetCurrentValue));
  RegisterMethod("addCounter",
                 NewSlot(impl_, &Impl::AddCounter));
  RegisterMethod("removeCounter",
                 NewSlot(impl_, &Impl::RemoveCounter));
}

ScriptablePerfmon::~ScriptablePerfmon() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableProcess
class ScriptableProcess::Impl : public SmallObject<> {
 public:
  Impl(ProcessInterface *process)
    : process_(process) {
    ASSERT(process_);
  }

  std::string EncodeProcessInfo(ProcessInfoInterface *proc_info) {
    if (!proc_info)
      return "null";

    std::string path = proc_info->GetExecutablePath();
    UTF16String utf16_path;
    ConvertStringUTF8ToUTF16(path.c_str(), path.size(), &utf16_path);
    return StringPrintf(
        "{\"processId\":%d,\"executablePath\":%s}", proc_info->GetProcessId(),
        EncodeJavaScriptString(utf16_path.c_str(), '"').c_str());
  }

  ScriptableArray *EnumerateProcesses() {
    ScriptableArray *array = new ScriptableArray();
    ProcessesInterface *processes = process_->EnumerateProcesses();
    if (processes) {
      int count = processes->GetCount();
      for (int i = 0; i < count; i++) {
        ProcessInfoInterface *proc_info = processes->GetItem(i);
        if (proc_info)
          array->Append(Variant(proc_info->GetProcessId()));
      }
      processes->Destroy();
    }
    return array;
  }

  JSONString GetForegroundProcess() {
    return JSONString(EncodeProcessInfo(process_->GetForeground()));
  }

  JSONString GetProcessInfo(int pid) {
    return JSONString(EncodeProcessInfo(process_->GetInfo(pid)));
  }

  ProcessInterface *process_;
};

ScriptableProcess::ScriptableProcess(ProcessInterface *process)
  : impl_(new Impl(process)) {
  RegisterProperty("enumerateProcesses",
                   NewSlot(impl_, &Impl::EnumerateProcesses), NULL);
  RegisterProperty("foreground",
                   NewSlot(impl_, &Impl::GetForegroundProcess), NULL);
  RegisterMethod("getInfo",
                 NewSlot(impl_, &Impl::GetProcessInfo));
}

ScriptableProcess::~ScriptableProcess() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptablePower
ScriptablePower::ScriptablePower(PowerInterface *power) {
  ASSERT(power);
  RegisterProperty(
      "charging",
      NewSlot(power, &PowerInterface::IsCharging), NULL);
  RegisterProperty(
      "percentRemaining",
      NewSlot(power, &PowerInterface::GetPercentRemaining), NULL);
  RegisterProperty(
      "pluggedIn",
      NewSlot(power, &PowerInterface::IsPluggedIn), NULL);
  RegisterProperty(
      "timeRemaining",
      NewSlot(power, &PowerInterface::GetTimeRemaining), NULL);
  RegisterProperty(
      "timeTotal",
      NewSlot(power, &PowerInterface::GetTimeTotal), NULL);
}

// Implementation of ScriptableMemory
ScriptableMemory::ScriptableMemory(MemoryInterface *memory) {
  ASSERT(memory);
  RegisterProperty("free", NewSlot(memory, &MemoryInterface::GetFree), NULL);
  RegisterProperty("total", NewSlot(memory, &MemoryInterface::GetTotal), NULL);
  RegisterProperty("used", NewSlot(memory, &MemoryInterface::GetUsed), NULL);
  RegisterProperty("freePhysical",
                   NewSlot(memory, &MemoryInterface::GetFreePhysical), NULL);
  RegisterProperty("totalPhysical",
                   NewSlot(memory, &MemoryInterface::GetTotalPhysical), NULL);
  RegisterProperty("usedPhysical",
                   NewSlot(memory, &MemoryInterface::GetUsedPhysical), NULL);
}

// Implementation of ScriptableBios
ScriptableBios::ScriptableBios(MachineInterface *machine) {
  ASSERT(machine);
  RegisterProperty(
      "serialNumber",
      NewSlot(machine, &MachineInterface::GetBiosSerialNumber), NULL);
}

// Implementation of ScriptableMachine
ScriptableMachine::ScriptableMachine(MachineInterface *machine) {
  ASSERT(machine);
  RegisterProperty(
      "manufacturer",
      NewSlot(machine, &MachineInterface::GetMachineManufacturer), NULL);
  RegisterProperty(
      "model",
      NewSlot(machine, &MachineInterface::GetMachineModel), NULL);
}

// Implementation of ScriptableProcessor
ScriptableProcessor::ScriptableProcessor(MachineInterface *machine) {
  ASSERT(machine);
  RegisterProperty(
      "architecture",
      NewSlot(machine, &MachineInterface::GetProcessorArchitecture), NULL);
  RegisterProperty(
      "count",
      NewSlot(machine, &MachineInterface::GetProcessorCount), NULL);
  RegisterProperty(
      "family",
      NewSlot(machine, &MachineInterface::GetProcessorFamily), NULL);
  RegisterProperty(
      "model",
      NewSlot(machine, &MachineInterface::GetProcessorModel), NULL);
  RegisterProperty(
      "name",
      NewSlot(machine, &MachineInterface::GetProcessorName), NULL);
  RegisterProperty(
      "speed",
      NewSlot(machine, &MachineInterface::GetProcessorSpeed), NULL);
  RegisterProperty(
      "stepping",
      NewSlot(machine, &MachineInterface::GetProcessorStepping), NULL);
  RegisterProperty(
      "vendor",
      NewSlot(machine, &MachineInterface::GetProcessorVendor), NULL);
}

// Implementation of ScriptableCursor
class ScriptableCursor::Impl : public SmallObject<> {
 public:
  Impl(CursorInterface *cursor) : cursor_(cursor) {
    ASSERT(cursor_);
  }

  JSONString GetPosition() {
    int x, y;
    cursor_->GetPosition(&x, &y);
    return JSONString(StringPrintf("{\"x\":%d,\"y\":%d}", x, y));
  }

  CursorInterface *cursor_;
};

ScriptableCursor::ScriptableCursor(CursorInterface *cursor)
  : impl_(new Impl(cursor)) {
  RegisterProperty("position", NewSlot(impl_, &Impl::GetPosition), NULL);
}

ScriptableCursor::~ScriptableCursor() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableScreen
class ScriptableScreen::Impl : public SmallObject<> {
 public:
  Impl(ScreenInterface *screen) : screen_(screen) {
    ASSERT(screen_);
  }

  JSONString GetSize() {
    int width, height;
    screen_->GetSize(&width, &height);
    return JSONString(StringPrintf("{\"width\":%d,\"height\":%d}",
                                   width, height));
  }

  ScreenInterface *screen_;
};

ScriptableScreen::ScriptableScreen(ScreenInterface *screen)
  : impl_(new Impl(screen)) {
  RegisterProperty("size", NewSlot(impl_, &Impl::GetSize), NULL);
}

ScriptableScreen::~ScriptableScreen() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableUser
ScriptableUser::ScriptableUser(UserInterface *user) {
  RegisterProperty("idle",
                   NewSlot(user, &UserInterface::IsUserIdle), NULL);
  /* RegisterProperty("idle_period",
                   NewSlot(user, &UserInterface::GetIdlePeriod),
                   NewSlot(user, &UserInterface::SetIdlePeriod)); */
}

ScriptableUser::~ScriptableUser() {
}

// Implementation of ScriptableGraphics
class ScriptableGraphics::Impl : public SmallObject<> {
 public:
  Impl(GadgetInterface *gadget) : gadget_(gadget) {
    ASSERT(gadget_);
  }

  JSONString CreatePoint() {
    return JSONString("{\"x\":0,\"y\":0}");
  }

  JSONString CreateSize() {
    return JSONString("{\"height\":0,\"width\":0}");
  }

  ScriptableImage *LoadImage(const Variant &image_src) {
    View *view = gadget_->GetMainView();
    ASSERT(view);
    ImageInterface *image = view->LoadImage(image_src, false);
    return image ? new ScriptableImage(image) : NULL;
  }

  GadgetInterface *gadget_;
};

ScriptableGraphics::ScriptableGraphics(GadgetInterface *gadget)
    : impl_(new Impl(gadget)) {
}

void ScriptableGraphics::DoRegister() {
  RegisterMethod("createPoint", NewSlot(impl_, &Impl::CreatePoint));
  RegisterMethod("createSize", NewSlot(impl_, &Impl::CreateSize));
  RegisterMethod("loadImage", NewSlot(impl_, &Impl::LoadImage));
}

ScriptableGraphics::~ScriptableGraphics() {
  delete impl_;
  impl_ = NULL;
}

} // namespace framework
} // namespace ggadget
