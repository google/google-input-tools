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
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_file_system.h>
#include <ggadget/permissions.h>
#include <ggadget/gadget_interface.h>

#include "file_system.h"
#include "runtime.h"
#include "memory.h"
#include "perfmon.h"
#include "process.h"

#ifdef HAVE_DBUS_LIBRARY
#include "machine.h"
#include "power.h"
#include "user.h"
#ifdef HAVE_NETWORK_MANAGER
#include "network.h"
#endif
#endif

#define Initialize linux_system_framework_LTX_Initialize
#define Finalize linux_system_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    linux_system_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace linux_system{

static Runtime *g_runtime_ = NULL;
static Memory *g_memory_ = NULL;
static Process *g_process_ = NULL;
static FileSystem *g_filesystem_ = NULL;
static Perfmon *g_perfmon_ = NULL;

static ScriptableRuntime *g_script_runtime_ = NULL;
static ScriptableMemory *g_script_memory_ = NULL;
static ScriptableProcess *g_script_process_ = NULL;

#ifdef HAVE_DBUS_LIBRARY
static Machine *g_machine_ = NULL;
static Power *g_power_ = NULL;
static User *g_user_ = NULL;

static ScriptableBios *g_script_bios_ = NULL;
static ScriptableMachine *g_script_machine_ = NULL;
static ScriptablePower *g_script_power_ = NULL;
static ScriptableProcessor *g_script_processor_ = NULL;
static ScriptableUser *g_script_user_ = NULL;

#ifdef HAVE_NETWORK_MANAGER
static Network *g_network_ = NULL;
static ScriptableNetwork *g_script_network_ = NULL;
#endif
#endif

} // namespace linux_system
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

extern "C" {
  bool Initialize() {
    LOGI("Initialize linux_system_framework extension.");

    g_runtime_ = new Runtime;
    g_memory_ = new Memory;
    g_process_ = new Process;
    g_filesystem_ = new FileSystem;
    g_perfmon_ = new Perfmon;

    g_script_runtime_ = new ScriptableRuntime(g_runtime_);
    g_script_memory_ = new ScriptableMemory(g_memory_);
    g_script_process_ = new ScriptableProcess(g_process_);

#ifdef HAVE_DBUS_LIBRARY
    g_machine_ = new Machine;
    g_power_ = new Power;
    g_user_ = new User;

    g_script_bios_ = new ScriptableBios(g_machine_);
    g_script_machine_ = new ScriptableMachine(g_machine_);
    g_script_power_ = new ScriptablePower(g_power_);
    g_script_processor_ = new ScriptableProcessor(g_machine_);
    g_script_user_ = new ScriptableUser(g_user_);

#ifdef HAVE_NETWORK_MANAGER
    g_network_ = new Network;
    g_script_network_ = new ScriptableNetwork(g_network_);
#endif
#endif
    return true;
  }

  void Finalize() {
    LOGI("Finalize linux_system_framework extension.");

    delete g_script_runtime_;
    delete g_script_memory_;
    delete g_script_process_;

    delete g_runtime_;
    delete g_memory_;
    delete g_process_;
    delete g_filesystem_;
    delete g_perfmon_;

#ifdef HAVE_DBUS_LIBRARY
    delete g_script_bios_;
    delete g_script_machine_;
    delete g_script_power_;
    delete g_script_processor_;
    delete g_script_user_;

    delete g_machine_;
    delete g_power_;
    delete g_user_;

#ifdef HAVE_NETWORK_MANAGER
    delete g_script_network_;
    delete g_network_;
#endif
#endif
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  GadgetInterface *gadget) {
    LOGI("Register linux_system_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();
    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    ResultVariant prop = framework->GetProperty("system");
    if (prop.v().type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable<UINT64_C(0xa5cc5f6479d1441f)>();
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

    const Permissions *perm = gadget->GetPermissions();
    if (perm && (perm->IsRequiredAndGranted(Permissions::FILE_READ) ||
                 perm->IsRequiredAndGranted(Permissions::FILE_WRITE))) {
      ScriptableFileSystem *script_filesystem =
          new ScriptableFileSystem(g_filesystem_, gadget);
      reg_system->RegisterVariantConstant("filesystem",
                                          Variant(script_filesystem));
    }

    // Check permissions.
    if (!perm || !perm->IsRequiredAndGranted(Permissions::DEVICE_STATUS)) {
      DLOG("No permission to access device status.");
      return true;
    }

    // FIXME: Should runtime be restricted by <devicestatus/> ?
    reg_framework->RegisterVariantConstant("runtime",
                                           Variant(g_script_runtime_));
    reg_system->RegisterVariantConstant("memory",
                                        Variant(g_script_memory_));
    reg_system->RegisterVariantConstant("process",
                                        Variant(g_script_process_));

    // ScriptablePerfmon is per gadget, so create a new instance here.
    ScriptablePerfmon *script_perfmon =
        new ScriptablePerfmon(g_perfmon_, gadget);

    reg_system->RegisterVariantConstant("perfmon", Variant(script_perfmon));

#ifdef HAVE_DBUS_LIBRARY
    reg_system->RegisterVariantConstant("bios",
                                        Variant(g_script_bios_));
    reg_system->RegisterVariantConstant("machine",
                                        Variant(g_script_machine_));
#ifdef HAVE_NETWORK_MANAGER
    reg_system->RegisterVariantConstant("network",
                                        Variant(g_script_network_));
#endif
    reg_system->RegisterVariantConstant("power",
                                        Variant(g_script_power_));
    reg_system->RegisterVariantConstant("processor",
                                        Variant(g_script_processor_));
    reg_system->RegisterVariantConstant("user",
                                        Variant(g_script_user_));
#endif
    return true;
  }
}
