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

#ifndef GGADGET_DBUS_SCRIPTABLE_DBUS_OBJECT_H__
#define GGADGET_DBUS_SCRIPTABLE_DBUS_OBJECT_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/dbus/dbus_proxy.h>

namespace ggadget {
namespace dbus {

class ScriptableDBusObject : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0xe45aa627937b466b, ScriptableInterface);
  ScriptableDBusObject(DBusProxy *proxy);
  virtual ~ScriptableDBusObject();

 protected:
  virtual void DoRegister();
  virtual void DoClassRegister();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableDBusObject);
};

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_SCRIPTABLE_DBUS_OBJECT_H__
