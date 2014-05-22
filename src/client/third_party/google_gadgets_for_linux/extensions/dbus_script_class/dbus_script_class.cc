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

#include <string>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/element_factory.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/variant.h>
#include <ggadget/slot.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/permissions.h>
#include "scriptable_dbus_object.h"

#define Initialize dbus_script_class_LTX_Initialize
#define Finalize dbus_script_class_LTX_Finalize
#define RegisterScriptExtension dbus_script_class_LTX_RegisterScriptExtension

using ggadget::dbus::ScriptableDBusObject;
using ggadget::dbus::DBusProxy;
using ggadget::Variant;
using ggadget::NewSlot;
using ggadget::ScriptContextInterface;
using ggadget::GadgetInterface;
using ggadget::Permissions;

static const char *kDBusSystemObjectName = "DBusSystemObject";
static const char *kDBusSessionObjectName = "DBusSessionObject";

static ScriptableDBusObject* NewSystemObject(const std::string &name,
                                             const std::string &path,
                                             const std::string &interface) {
  DBusProxy *proxy = DBusProxy::NewSystemProxy(name, path, interface);
  return proxy ? new ScriptableDBusObject(proxy) : NULL;
}

static ScriptableDBusObject* NewSessionObject(const std::string &name,
                                              const std::string &path,
                                              const std::string &interface) {
  DBusProxy *proxy = DBusProxy::NewSessionProxy(name, path, interface);
  return proxy ? new ScriptableDBusObject(proxy) : NULL;
}

extern "C" {
  bool Initialize() {
    LOGI("Initialize dbus_script_class extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize dbus_script_class extension.");
  }

  bool RegisterScriptExtension(ScriptContextInterface *context,
                               GadgetInterface *gadget) {
    LOGI("Register dbus_script_class extension.");
    // Only register D-Bus extension if <allaccess/> is granted.
    const Permissions *perm = gadget ? gadget->GetPermissions() : NULL;
    // Only calling inside unittest can have NULl gadget and permissions.
    if (!perm || !perm->IsRequiredAndGranted(Permissions::ALL_ACCESS)) {
      DLOG("No permission to access D-Bus.");
      return true;
    }
    if (context) {
      if (!context->RegisterClass(
          kDBusSystemObjectName, NewSlot(NewSystemObject))) {
        LOG("Failed to register %s class.", kDBusSystemObjectName);
        return false;
      }
      if (!context->RegisterClass(
          kDBusSessionObjectName, NewSlot(NewSessionObject))) {
        LOG("Failed to register %s class.", kDBusSessionObjectName);
        return false;
      }
      return true;
    }
    return false;
  }
}
