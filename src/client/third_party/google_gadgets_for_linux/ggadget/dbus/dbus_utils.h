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

#ifndef GGADGET_DBUS_DBUS_UTILS_H__
#define GGADGET_DBUS_DBUS_UTILS_H__

#include <map>
#include <vector>
#include <string>

#include <ggadget/common.h>
#include <ggadget/string_utils.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_holder.h>

class DBusConnection;
class DBusMessage;

namespace ggadget {

class MainLoopInterface;

namespace dbus {

/**
 * Container object to hold value transfered between DBusProxy and the caller.
 */
class ScriptableDBusContainer : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x7829c86eb35a4168, ScriptableInterface);

  /** Allows script to enumerate this object. */
  virtual bool IsEnumeratable() const { return true; }

  /**
   * Don't use @c RegisterConstant() directly, since we want to register
   * constant properties by dynamic char pointer. This case is not applicable
   * for the map used in scriptable_helper object, which use const char*
   * not std::string as the key.
   */
  void AddProperty(const std::string &name, const Variant &value) {
    if (name.empty()) return;
    keys_.push_back(name);
    RegisterConstant((keys_.end() - 1)->c_str(), value);
  }

 private:
  StringVector keys_;
};

typedef ScriptableHolder<ScriptableDBusContainer> ScriptableDBusContainerHolder;

std::string GetVariantSignature(const Variant &value);
Variant::Type GetVariantTypeFromSignature(const std::string &signature);
bool ValidateObjectPath(const char *path);
bool ValidateInterface(const char *interface);
bool ValidateBusName(const char *name);

struct Argument {
  Argument() {}
  explicit Argument(const Variant& v) : value(v) {}
  explicit Argument(const ResultVariant& v) : value(v) {}
  explicit Argument(const std::string &sig) : signature(sig) {}
  Argument(const std::string &sig, const Variant &v)
    : signature(sig), value(v) {
  }
  Argument(const std::string &sig, const ResultVariant &v)
    : signature(sig), value(v){
  }

  std::string signature;
  ResultVariant value;
};
typedef std::vector<Argument> Arguments;

/**
 * Marshaller for DBusMessage. Not a external API, user should not use it
 * directly.
 */
class DBusMarshaller {
 public:
  explicit DBusMarshaller(DBusMessage *message);
  ~DBusMarshaller();

  bool AppendArguments(const Arguments &args);
  bool AppendArgument(const Argument &arg);

  static bool ValistAdaptor(Arguments *in_args,
                            MessageType first_arg_type, va_list *va_args);
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusMarshaller);
};

/**
 * Marshaller for DBusMessage. Not a external API, user should not use it
 * directly.
 */
class DBusDemarshaller {
 public:
  explicit DBusDemarshaller(DBusMessage *message);
  ~DBusDemarshaller();

  bool GetArguments(Arguments *args);
  bool GetArgument(Argument *arg);

  static bool ValistAdaptor(const Arguments &out_args,
                            MessageType first_arg_type, va_list *va_args);
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusDemarshaller);
};

/**
 * a wrapper of DBusConnection for working with our MainLoopInterface.
 */
class DBusMainLoopClosure {
 public:
  DBusMainLoopClosure(DBusConnection* connection,
                      MainLoopInterface* main_loop);
  ~DBusMainLoopClosure();
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusMainLoopClosure);
};

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_DBUS_UTILS_H__
