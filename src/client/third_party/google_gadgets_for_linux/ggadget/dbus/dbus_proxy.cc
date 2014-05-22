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

#include "dbus_proxy.h"
#include "dbus_utils.h"

#include <algorithm>
#include <vector>
#include <map>
#include <dbus/dbus.h>

#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/small_object.h>

#ifdef _DEBUG
// #define DBUS_VERBOSE_LOG
#endif

namespace ggadget {
namespace dbus {

class DBusProxy::Impl : public SmallObject<> {
  // Structure to hold information of an argument.
  struct ArgPrototype {
    std::string name;
    std::string signature;
  };
  typedef std::vector<ArgPrototype> ArgPrototypeVector;

  // Structure to hold information of a method or a signal.
  // in_args stores information of all arguments to be sent to remote dbus
  // object.
  // out_args stores information of all arguments to be read from remote dbus
  // object.
  //
  // For signal the in_args shall be empty.
  struct MethodSignalPrototype {
    ArgPrototypeVector in_args;
    ArgPrototypeVector out_args;
  };
  typedef LightMap<std::string, MethodSignalPrototype> MethodSignalPrototypeMap;

  struct PropertyPrototype {
    PropertyAccess access;
    std::string signature;
  };
  typedef LightMap<std::string, PropertyPrototype> PropertyPrototypeMap;

  typedef LightMap<int, DBusPendingCall *> PendingCallMap;

  // Class to hold owner<->names mapping information.
  class OwnerNamesCache {
   public:
    ~OwnerNamesCache() {
      Clear();
    }

    bool IsNameMonitored(const std::string &name) const {
      return names_info_.find(name) != names_info_.end();
    }
    Connection *MonitorName(const std::string &name, Slot0<void> *callback) {
      if (!name.empty()) {
        NameInfo *info = &names_info_[name];
        info->refcount++;
        if (callback) {
          if (!info->on_name_owner_changed) {
            info->on_name_owner_changed = new Signal0<void>();
          }
          return info->on_name_owner_changed->Connect(callback);
        }
      }
      return NULL;
    }
    void UnmonitorName(const std::string &name, Connection *connection) {
      NamesInfoMap::iterator it = names_info_.find(name);
      if (it != names_info_.end()) {
        -- it->second.refcount;
        if (it->second.refcount <= 0) {
          if (!it->second.owner.empty()) {
            RemoveOwnerName(it->second.owner, name);
          }
          delete it->second.on_name_owner_changed;
          names_info_.erase(it);
        } else if (connection) {
          ASSERT(it->second.on_name_owner_changed);
          it->second.on_name_owner_changed->Disconnect(connection);
        }
      }
    }
    void SetNameOwner(const std::string &name, const std::string &owner,
                      bool emit) {
      NamesInfoMap::iterator it = names_info_.find(name);
      if (it != names_info_.end()) {
        if (!it->second.owner.empty())
          RemoveOwnerName(it->second.owner, name);
        if (!owner.empty())
          AddOwnerName(owner, name);
        it->second.owner = owner;
        if (emit && it->second.on_name_owner_changed) {
          (*it->second.on_name_owner_changed)();
        }
      }
    }
    void GetOwnerNames(const std::string &owner,
                       StringVector *names) const {
      OwnerNamesMap::const_iterator it = owner_names_.find(owner);
      names->clear();
      if (it != owner_names_.end()) {
        names->assign(it->second.begin(), it->second.end());
      }
    }
    void Clear() {
      NamesInfoMap::iterator it = names_info_.begin();
      NamesInfoMap::iterator end = names_info_.end();
      for (; it != end; ++it) {
        delete it->second.on_name_owner_changed;
      }
      owner_names_.clear();
      names_info_.clear();
    }

   private:
    void AddOwnerName(const std::string &owner, const std::string &name) {
      OwnerNamesMap::iterator it = owner_names_.find(owner);
      if (it == owner_names_.end()) {
        owner_names_[owner].push_back(name);
      } else {
        StringVector::iterator nit = it->second.begin();
        for (; nit != it->second.end(); ++nit) {
          if (*nit == name) return;
        }
        it->second.push_back(name);
      }
    }
    void RemoveOwnerName(const std::string &owner, const std::string &name) {
      OwnerNamesMap::iterator it = owner_names_.find(owner);
      ASSERT(it != owner_names_.end());
      StringVector::iterator nit = it->second.begin();
      for (; nit != it->second.end(); ++nit) {
        if (*nit == name) {
          it->second.erase(nit);
          break;
        }
      }
      if (it->second.size() == 0)
        owner_names_.erase(it);
    }

    struct NameInfo {
      NameInfo() : refcount(0), on_name_owner_changed(NULL) { }
      int refcount;
      std::string owner;
      Signal0<void> *on_name_owner_changed;
    };

    // first: owner, second: names
    typedef LightMap<std::string, StringVector > OwnerNamesMap;
    OwnerNamesMap owner_names_;

    // first: name, second: name's information
    typedef LightMap<std::string, NameInfo> NamesInfoMap;
    NamesInfoMap names_info_;
  };

  // Class to manage a dbus connection.
  class Manager {
   public:
    explicit Manager(DBusBusType type)
      : type_(type),
        bus_(NULL),
        main_loop_closure_(NULL),
        bus_proxy_(NULL),
        destroying_(false) {
    }
    ~Manager() {
      ASSERT(proxies_.size() == 0);
      if (proxies_.size()) {
        LOGW("%zu DBusProxy objects are still available when destroying"
             " DBus Connection for bus %d",
             proxies_.size(), type_);
        // Detach existing proxies from this manager.
        ProxyMap::iterator it = proxies_.begin();
        for(; it != proxies_.end(); ++it)
          it->second->DetachFromManager();
      }
      Destroy();
    }
    DBusConnection *Get() {
      if (EnsureInitialized())
        return bus_;
      return NULL;
    }
    bool IsAsyncSupported() const {
      return main_loop_closure_ != NULL;
    }
    DBusBusType GetType() const {
      return type_;
    }
    // For debug purpose.
    const char *GetTypeName() const {
      return type_ == DBUS_BUS_SYSTEM ? "system" : "session";
    }

    // Creates a new Impl object.
    // The Impl object is shared among all DBusProxy objects with the same
    // name, path and interface.
    Impl *NewImpl(const std::string &name, const std::string &path,
                  const std::string &interface) {
      if (!ValidateBusName(name.c_str()) ||
          !ValidateObjectPath(path.c_str()) ||
          !ValidateInterface(interface.c_str())) {
        DLOG("Invalid DBus name, path or interface: %s, %s, %s",
             name.c_str(), path.c_str(), interface.c_str());
        return NULL;
      }

      if (EnsureInitialized()) {
        std::string tri_name = GetTriName(name, path, interface);
        ProxyMap::iterator it = proxies_.find(tri_name);
        if (it != proxies_.end()) {
          ASSERT(it->second);
          it->second->Ref();
          return it->second;
        } else {
          // Initial refcount is 1.
          Impl *impl = new Impl(this, name, path, interface);
          // Try to run Introspect() in sync mode first, it may fail if the
          // remote service is not ready yet. Then try to run Introspect() in
          // async mode again to make sure we can get the introspect data after
          // the service is ready.
          if (impl->Introspect(true) || impl->Introspect(false)) {
            proxies_[tri_name] = impl;
            MonitorImplName(impl);
            std::string match_rule = impl->GetMatchRule();
#ifdef DBUS_VERBOSE_LOG
            DLOG("Add Match to %s bus: %s", GetTypeName(), match_rule.c_str());
#endif
            dbus_bus_add_match(bus_, match_rule.c_str(), NULL);
            return impl;
          } else {
            delete impl;
          }
        }
      }
      return NULL;
    }
    // Deletes an existing Impl object.
    // Note, it doesn't care about the reference count of the Impl object.
    bool DeleteImpl(Impl *impl) {
      ASSERT(!destroying_);
      if (!destroying_) {
        std::string tri_name =
            GetTriName(impl->GetName(), impl->GetPath(), impl->GetInterface());
        ProxyMap::iterator it = proxies_.find(tri_name);
        if (it != proxies_.end()) {
          ASSERT(it->second == impl);
          if (bus_) {
            std::string match_rule = impl->GetMatchRule();
#ifdef DBUS_VERBOSE_LOG
            DLOG("Remove Match from %s bus: %s",
                 GetTypeName(), match_rule.c_str());
#endif
            dbus_bus_remove_match(bus_, match_rule.c_str(), NULL);
          }
          UnmonitorImplName(impl);
          delete impl;
          proxies_.erase(it);
          if (proxies_.size() == 0) {
            DLOG("No proxy left, destroy %s bus.", GetTypeName());
            // No more proxy, destroy the connection to save resource.
            Destroy();
          }
          return true;
        } else {
          DLOG("Unknown proxy: %s", tri_name.c_str());
        }
      }
      return false;
    }

    bool EnsureInitialized() {
      ASSERT(!destroying_);
      if (!bus_ && !destroying_) {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Initialize DBus %s bus.",
             type_ == DBUS_BUS_SYSTEM ? "system" : "session");
#endif
        DBusError error;
        dbus_error_init(&error);
        bus_ = dbus_bus_get_private(type_, &error);
        if (!bus_) {
          LOG("Failed to initialize DBus, type: %d, error: %s, %s",
              type_, error.name, error.message);
          return false;
        }
        dbus_error_free(&error);
        dbus_connection_set_exit_on_disconnect(bus_, FALSE);
        dbus_bus_add_match(bus_,
                           "type='signal',sender='" DBUS_SERVICE_DBUS
                           "',path='" DBUS_PATH_DBUS
                           "',interface='" DBUS_INTERFACE_DBUS
                           "',member='NameOwnerChanged'",
                           NULL);
        dbus_connection_add_filter(bus_, BusFilter, this, NULL);
        MainLoopInterface *main_loop = GetGlobalMainLoop();
        if (main_loop)
          main_loop_closure_ = new DBusMainLoopClosure(bus_, main_loop);
        else
          DLOG("DBus proxy may not work without main loop.");
        // Re-monitor names of existing proxies.
        if (proxies_.size()) {
          ProxyMap::iterator it = proxies_.begin();
          for(; it != proxies_.end(); ++it)
            MonitorImplName(it->second);
        }
      }
      return bus_ != NULL && !destroying_;
    }

    void Destroy() {
      ASSERT(!destroying_);
      if (bus_ && !destroying_) {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Destroy DBus %s bus.",
             type_ == DBUS_BUS_SYSTEM ? "system" : "session");
#endif
        destroying_ = true;
        // Remove filter first to avoid receiving signals anymore.
        dbus_connection_remove_filter(bus_, BusFilter, this);
        dbus_bus_remove_match(bus_,
                              "type='signal',sender='" DBUS_SERVICE_DBUS
                              "',path='" DBUS_PATH_DBUS
                              "',interface='" DBUS_INTERFACE_DBUS
                              "',member='NameOwnerChanged'",
                              NULL);

        ProxyMap::iterator it = proxies_.begin();
        for(; it != proxies_.end(); ++it) {
          // no need to disconnect it, as owner_names_ will be cleared.
          it->second->on_name_owner_changed_connection_ = NULL;
          // Cancel all pending calls.
          it->second->CancelAllPendingCalls();
        }

        // Clears all monitored names, without destroying existing proxies.
        // Existing proxies can be reused when dbus connection is
        // established again.
        owner_names_.Clear();
        delete main_loop_closure_;
        // Bus must be closed before unref.
        dbus_connection_close(bus_);
        dbus_connection_unref(bus_);
        delete bus_proxy_;
        main_loop_closure_ = NULL;
        bus_ = NULL;
        bus_proxy_ = NULL;
        destroying_ = false;
      }
    }

   private:
    Impl *GetBusProxy() {
      if (EnsureInitialized()) {
        if (!bus_proxy_) {
          bus_proxy_ = new Impl(this, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS,
                                DBUS_INTERFACE_DBUS);
          // bus_proxy_ must be introspected synchronously.
          if (!bus_proxy_->Introspect(true)) {
            DLOG("Failed to create bus proxy.");
            delete bus_proxy_;
            bus_proxy_ = NULL;
          }
        }
        return bus_proxy_;
      }
      return NULL;
    }

    // Callback to receive the result of "GetNameOwner" async call.
    bool GetNameOwnerCallback(int index, const Variant &result,
                              const std::string &name) {
      if (bus_ && !destroying_ && index == 0) {
        std::string owner;
        if (result.ConvertToString(&owner)) {
#ifdef DBUS_VERBOSE_LOG
          DLOG("The owner of name %s is %s", name.c_str(), owner.c_str());
#endif
          owner_names_.SetNameOwner(name, owner, false);
        }
      }
      // One owner is enough.
      return false;
    }

    void MonitorImplName(Impl *impl) {
      ASSERT(!impl->on_name_owner_changed_connection_);
      std::string name = impl->GetName();
      // Don't monitor owner names.
      if (name[0] == ':') return;
      Slot0<void> *callback = NewSlot(impl, &Impl::OnNameOwnerChanged);
      if (owner_names_.IsNameMonitored(name)) {
        // If the name is already monitored, just increate its refcount
        // by calling MonitorName() again.
        impl->on_name_owner_changed_connection_ =
            owner_names_.MonitorName(name, callback);
      } else {
        // Otherwise monitor the name and then try to fetch the name's owner.
        impl->on_name_owner_changed_connection_ =
            owner_names_.MonitorName(name, callback);
        Impl *proxy = GetBusProxy();
        if (proxy) {
          Arguments in_args;
          in_args.push_back(Argument(Variant(name)));
          ResultCallback *callback =
              NewSlot(this, &Manager::GetNameOwnerCallback, name);
          proxy->CallMethod("GetNameOwner", false, kDefaultDBusTimeout,
                            callback, &in_args);
        }
      }
    }

    void UnmonitorImplName(Impl *impl) {
      std::string name = impl->GetName();
      // Don't monitor owner names.
      if (name[0] == ':') return;
      owner_names_.UnmonitorName(name, impl->on_name_owner_changed_connection_);
    }

    void NameOwnerChanged(const char *name, const char *old_owner,
                          const char *new_owner) {
      GGL_UNUSED(old_owner);
      // Don't monitor owner names.
      if (name[0] == ':') return;
#ifdef DBUS_VERBOSE_LOG
      DLOG("NameOwnerChanged %s: %s -> %s", name, old_owner, new_owner);
#endif
      // Just set the name's owner to the new one.
      // If the name is not monitored, nothing will be done.
      owner_names_.SetNameOwner(name, new_owner, true);
    }

    void EmitSignalMessage(DBusMessage *message) {
      const char *sender = dbus_message_get_sender(message);
      const char *path = dbus_message_get_path(message);
      const char *interface = dbus_message_get_interface(message);
#ifdef _DEBUG
      const char *member = dbus_message_get_member(message);
#endif
      // path, interface and member are mandatory according to dbus spec.
      ASSERT(path);
      ASSERT(interface);
      ASSERT(member);
      // sender is optional according to dbus spec.
      if (!sender) sender = "";

      StringVector names;
      // sender is always the unique bus name if it's available.
      owner_names_.GetOwnerNames(sender, &names);
      // Some proxies may bound to the unique name directly.
      if (sender[0])
        names.push_back(sender);

      for (StringVector::iterator it = names.begin(); it != names.end(); ++it) {
        std::string tri_name = GetTriName(*it, path, interface);
        ProxyMap::iterator proxy_it = proxies_.find(tri_name);
        if (proxy_it != proxies_.end()) {
#ifdef DBUS_VERBOSE_LOG
          DLOG("Emit signal %s on proxy %s", member, tri_name.c_str());
#endif
          proxy_it->second->EmitSignal(message);
        }
      }
    }

    static std::string GetTriName(const std::string &name,
                                  const std::string &path,
                                  const std::string &interface) {
      std::string tri_name;
      tri_name.append(name);
      tri_name.append("|");
      tri_name.append(path);
      tri_name.append("|");
      tri_name.append(interface);
      return tri_name;
    }

    static DBusHandlerResult BusFilter(DBusConnection *bus,
                                       DBusMessage *message,
                                       void *user_data) {
      GGL_UNUSED(bus);
      if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

      Manager *manager = reinterpret_cast<Manager *>(user_data);
      ASSERT(bus == manager->bus_);
      ASSERT(!manager->destroying_);

#ifdef DBUS_VERBOSE_LOG
      DLOG("BusFilter(%s): sender:%s path:%s interface:%s member:%s",
           manager->GetTypeName(), dbus_message_get_sender(message),
           dbus_message_get_path(message),
           dbus_message_get_interface(message),
           dbus_message_get_member(message));
#endif

      if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL,
                                 "Disconnected")) {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Disconnected signal received for bus: %d", manager->type_);
#endif
        // bus is disconnected, destroy the manager.
        manager->Destroy();
      } else {
        // Handles NameOwnerChanged signal internally.
        if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS,
                                   "NameOwnerChanged")) {
          // Owner has been changed for a specified name, all proxies associated
          // to the name must be destroyed.
          const char *name;
          const char *prev_owner;
          const char *new_owner;
          DBusError error;
          dbus_error_init(&error);
          if (dbus_message_get_args(message, &error,
                                    DBUS_TYPE_STRING, &name,
                                    DBUS_TYPE_STRING, &prev_owner,
                                    DBUS_TYPE_STRING, &new_owner,
                                    DBUS_TYPE_INVALID)) {
            manager->NameOwnerChanged(name, prev_owner, new_owner);
          }
          dbus_error_free(&error);
        }

        // Forwards the signal to existing proxies.
        manager->EmitSignalMessage(message);
      }

      // Give others a chance to handle the signals.
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    typedef LightMap<std::string, Impl *> ProxyMap;
    ProxyMap proxies_;
    OwnerNamesCache owner_names_;

    DBusBusType type_;
    DBusConnection *bus_;
    DBusMainLoopClosure *main_loop_closure_;

    // Special proxy to retrieve a name's owner.
    Impl *bus_proxy_;
    bool destroying_;
  };

 public:
  Impl(Manager *manager, const std::string &name, const std::string &path,
       const std::string &interface)
    : manager_(manager),
      name_(name),
      path_(path),
      interface_(interface),
      on_name_owner_changed_connection_(NULL),
      refcount_(1),
      call_id_counter_(1) {
  }
  ~Impl() {
    CancelAllPendingCalls();
  }

  std::string GetName() const { return name_; }
  std::string GetPath() const { return path_; }
  std::string GetInterface() const { return interface_; }

  std::string GetMatchRule() const {
    return StringPrintf("type='signal',sender='%s',path='%s',interface='%s'",
                        name_.c_str(), path_.c_str(), interface_.c_str());
  }

  void Ref() {
    ++refcount_;
  }

  void Unref() {
    ASSERT(refcount_ > 0);
    --refcount_;
    if (refcount_ <= 0) {
      // If manager is still available, then let manager to delete this proxy,
      // otherwise just delete this to avoid memory leak.
      if (manager_)
        manager_->DeleteImpl(this);
      else
        delete this;
    }
  }

  void DetachFromManager() {
    // The manager has been destroyed.
    manager_ = NULL;
  }

  void OnNameOwnerChanged() {
    CancelAllPendingCalls();
    Introspect(false);
  }

  void CancelAllPendingCalls() {
    PendingCallMap::iterator it = pending_calls_.begin();
    for (; it != pending_calls_.end(); ++it) {
      dbus_pending_call_cancel(it->second);
      dbus_pending_call_unref(it->second);
    }
    pending_calls_.clear();
  }

  int CallMethod(const std::string &method, bool sync, int timeout,
                 ResultCallback *callback,
                 MessageType first_arg_type, va_list *args) {
    Arguments in_args;
    if (DBusMarshaller::ValistAdaptor(&in_args, first_arg_type, args))
      return CallMethod(method, sync, timeout, callback, &in_args);
    delete callback;
    return 0;
  }

  int CallMethod(const std::string &method, bool sync, int timeout,
                 ResultCallback *callback, Arguments *in_args) {
    Arguments out_args;
    MethodSignalPrototypeMap::iterator it = methods_.find(method);
    if (it != methods_.end()) {
      // Validate input arguments number and type.
      if (!ValidateArguments(it->second.in_args, in_args,
                             "method", method.c_str())) {
        CallAndFreeResultCallback(callback, out_args, false);
        return 0;
      }
    }

    DBusConnection *bus = GetBus();
    if (bus) {
      sync = (sync || !manager_->IsAsyncSupported());
      if (sync) {
        // Generate a new call id for sync all.
        int call_id = NewCallId();
        bool ret = CallMethodSync(bus, interface_.c_str(), method.c_str(),
                                  *in_args, &out_args, timeout);
        // Only validate return values when caller cares about them
        // (callback is not NULL).
        if (ret && it != methods_.end() && callback) {
          // Validate return values.
          ret = ValidateArguments(it->second.out_args, &out_args,
                                  "method", method.c_str());
        }
        if (!ret) {
          DLOG("Failed to call method %s of %s|%s|%s synchronously.",
               method.c_str(), name_.c_str(), path_.c_str(),
               interface_.c_str());
        }
        CallAndFreeResultCallback(callback, out_args, ret);
        return ret ? call_id : 0;
      } else {
        return CallMethodAsync(bus, interface_.c_str(), method.c_str(),
                               *in_args, callback, timeout);
      }
    }
    DLOG("Failed to call method %s of %s|%s|%s",
         method.c_str(), name_.c_str(), path_.c_str(), interface_.c_str());
    CallAndFreeResultCallback(callback, out_args, false);
    return 0;
  }

  bool CancelMethodCall(int index) {
    PendingCallMap::iterator it = pending_calls_.find(index);
    if (it != pending_calls_.end()) {
      dbus_pending_call_cancel(it->second);
      dbus_pending_call_unref(it->second);
      pending_calls_.erase(it);
      return true;
    }
    return false;
  }

  bool IsMethodCallPending(int index) {
    return pending_calls_.find(index) != pending_calls_.end();
  }

  bool GetMethodInfo(const std::string &method,
                     int *argc, Variant::Type **arg_types,
                     int *retc, Variant::Type **ret_types) {
    if (argc) *argc = 0;
    if (arg_types) *arg_types = NULL;
    if (retc) *retc = 0;
    if (ret_types) *ret_types = NULL;
    MethodSignalPrototypeMap::iterator it = methods_.find(method);
    if (it != methods_.end()) {
      // input arguments.
      if (it->second.in_args.size()) {
        if (argc) {
          *argc = static_cast<int>(it->second.in_args.size());
          if (arg_types && *argc > 0) {
            *arg_types = new Variant::Type[*argc];
            for (int i = 0; i < *argc; ++i) {
              (*arg_types)[i] =
                  GetVariantTypeFromSignature(it->second.in_args[i].signature);
            }
          }
        }
      }
      // output arguments.
      if (it->second.out_args.size()) {
        if (retc) {
          *retc = static_cast<int>(it->second.out_args.size());
          if (ret_types && *retc > 0) {
            *ret_types = new Variant::Type[*retc];
            for (int i = 0; i < *retc; ++i) {
              (*ret_types)[i] =
                  GetVariantTypeFromSignature(it->second.out_args[i].signature);
            }
          }
        }
      }
      return true;
    }
    return false;
  }

  bool EnumerateMethods(Slot1<bool, const std::string &> *callback) {
    ASSERT(callback);
    bool ret = true;
    MethodSignalPrototypeMap::iterator it = methods_.begin();
    for (; ret && it != methods_.end(); ++it) {
      ret = (*callback)(it->first);
    }
    delete callback;
    return ret;
  }

  ResultVariant GetProperty(const std::string &property) {
    Variant::Type expect_type = Variant::TYPE_VARIANT;
    PropertyPrototypeMap::iterator it = properties_.find(property);
    if (it != properties_.end()) {
      if ((it->second.access & PROP_READ) == 0) {
        DLOG("Property %s of %s|%s|%s is write only", property.c_str(),
             name_.c_str(), path_.c_str(), interface_.c_str());
        return ResultVariant();
      }
      expect_type = GetVariantTypeFromSignature(it->second.signature);
    }

    DBusConnection *bus = GetBus();
    if (bus) {
      Arguments in_args;
      // See http://dbus.freedesktop.org/doc/dbus-specification.html
      // org.freedesktop.DBus.Properties interface
      in_args.push_back(Argument(Variant(interface_)));
      in_args.push_back(Argument(Variant(property)));
      Arguments out_args;
      if (CallMethodSync(bus, DBUS_INTERFACE_PROPERTIES, "Get", in_args,
                         &out_args, kDefaultDBusTimeout)) {
        if (out_args.size() > 0) {
          if (expect_type != Variant::TYPE_VARIANT &&
              out_args[0].value.v().type() != expect_type) {
            DLOG("Type mismatch of property %s of %s|%s|%s, "
                 "expect:%d actual:%d", property.c_str(),
                 name_.c_str(), path_.c_str(), interface_.c_str(),
                 expect_type, out_args[0].value.v().type());
            return ResultVariant();
          }
          return out_args[0].value;
        }
      }
    }
    DLOG("Failed to get property %s of %s|%s|%s", property.c_str(),
         name_.c_str(), path_.c_str(), interface_.c_str());
    return ResultVariant();
  }

  bool SetProperty(const std::string &property, const Variant &value) {
    Arguments in_args;
    // See http://dbus.freedesktop.org/doc/dbus-specification.html
    // org.freedesktop.DBus.Properties interface
    in_args.push_back(Argument(Variant(interface_)));
    in_args.push_back(Argument(Variant(property)));
    in_args.push_back(Argument(value));
    PropertyPrototypeMap::iterator it = properties_.find(property);
    if (it != properties_.end()) {
      if ((it->second.access & PROP_WRITE) == 0) {
        DLOG("Property %s of %s|%s|%s is read only", property.c_str(),
             name_.c_str(), path_.c_str(), interface_.c_str());
        return false;
      }
      Variant::Type type = GetVariantTypeFromSignature(it->second.signature);
      if (type != Variant::TYPE_VARIANT && type != value.type()) {
        DLOG("Type mismatch of property %s of %s|%s|%s, expect:%d actual:%d",
             property.c_str(), name_.c_str(), path_.c_str(), interface_.c_str(),
             type, value.type());
        return false;
      }
      // Update value argument's signature.
      in_args[2].signature = it->second.signature;
    } else {
      DLOG("Unknown property %s of %s|%s|%s, set anyway.", property.c_str(),
           name_.c_str(), path_.c_str(), interface_.c_str());
    }
    DBusConnection *bus = GetBus();
    if (bus) {
      // No need to wait for reply.
      return SendMessage(bus, DBUS_INTERFACE_PROPERTIES, "Set",
                         in_args, NULL, -1);
    }
    DLOG("Failed to set property %s of %s|%s|%s", property.c_str(),
         name_.c_str(), path_.c_str(), interface_.c_str());
    return false;
  }

  PropertyAccess GetPropertyInfo(const std::string &property,
                                 Variant::Type *type) {
    if (type) *type = Variant::TYPE_VOID;
    PropertyPrototypeMap::iterator it = properties_.find(property);
    if (it != properties_.end()) {
      if (type)
        *type = GetVariantTypeFromSignature(it->second.signature);
      return it->second.access;
    }
    return PROP_UNKNOWN;
  }

  bool EnumerateProperties(Slot1<bool, const std::string &> *callback) {
    ASSERT(callback);
    bool ret = true;
    PropertyPrototypeMap::iterator it = properties_.begin();
    for (; ret && it != properties_.end(); ++it) {
      ret = (*callback)(it->first);
    }
    delete callback;
    return ret;
  }

  Connection *ConnectOnSignalEmit(
      Slot3<void, const std::string &, int, const Variant *> *callback) {
    return on_signal_emit_signal_.Connect(callback);
  }

  bool GetSignalInfo(const std::string &signal,
                     int *argc, Variant::Type **arg_types) {
    if (argc) *argc = 0;
    if (arg_types) *arg_types = NULL;
    MethodSignalPrototypeMap::iterator it = signals_.find(signal);
    if (it != signals_.end()) {
      if (it->second.out_args.size()) {
        if (argc) {
          *argc = static_cast<int>(it->second.out_args.size());
          if (arg_types && *argc > 0) {
            *arg_types = new Variant::Type[*argc];
            for (int i = 0; i < *argc; ++i) {
              (*arg_types)[i] =
                  GetVariantTypeFromSignature(it->second.out_args[i].signature);
            }
          }
        }
      }
      return true;
    }
    return false;
  }

  bool EnumerateSignals(Slot1<bool, const std::string &> *callback) {
    ASSERT(callback);
    bool ret = true;
    MethodSignalPrototypeMap::iterator it = signals_.begin();
    for (; ret && it != signals_.end(); ++it) {
      ret = (*callback)(it->first);
    }
    delete callback;
    return ret;
  }

  DBusProxy *NewChildProxy(const std::string &child,
                           const std::string &interface) {
    if (manager_ && child.length() && interface.length() && child[0] != '/') {
      std::string child_path = path_ + "/" + child;
      // It's not necessary to check if the interface is available or not,
      // because many dbus objects have no introspect data.
#ifdef DBUS_VERBOSE_LOG
      DLOG("New %s dbus proxy: %s|%s|%s", manager_->GetTypeName(),
           name_.c_str(), child_path.c_str(), interface.c_str());
#endif
      Impl *impl = manager_->NewImpl(name_, child_path, interface);
      if (impl) {
        DBusProxy *proxy = new DBusProxy();
        proxy->impl_ = impl;
        return proxy;
      }
    }
    DLOG("Failed to create dbus proxy: %s|%s/%s|%s",
         name_.c_str(), path_.c_str(), child.c_str(), interface.c_str());
    return NULL;
  }

  bool EnumerateChildren(Slot1<bool, const std::string &> *callback) {
    ASSERT(callback);
    bool ret = true;
    StringVector::iterator it = children_.begin();
    for (; ret && it != children_.end(); ++it) {
      ret = (*callback)(*it);
    }
    delete callback;
    return ret;
  }

  DBusProxy *NewInterfaceProxy(const std::string &interface) {
    if (manager_ && interface.length()) {
      // It's not necessary to check if the interface is available or not,
      // because many dbus objects have no introspect data.
#ifdef DBUS_VERBOSE_LOG
      DLOG("New %s dbus proxy: %s|%s|%s", manager_->GetTypeName(),
           name_.c_str(), path_.c_str(), interface.c_str());
#endif
      Impl *impl = manager_->NewImpl(name_, path_, interface);
      if (impl) {
        DBusProxy *proxy = new DBusProxy();
        proxy->impl_ = impl;
        return proxy;
      }
    }
    DLOG("Failed to create dbus proxy: %s|%s|%s",
         name_.c_str(), path_.c_str(), interface.c_str());
    return NULL;
  }

  bool EnumerateInterfaces(Slot1<bool, const std::string &> *callback) {
    ASSERT(callback);
    bool ret = true;
    StringVector::iterator it = interfaces_.begin();
    for (; ret && it != interfaces_.end(); ++it) {
      ret = (*callback)(*it);
    }
    delete callback;
    return ret;
  }

  void EmitSignal(DBusMessage *message) {
    const char *member = dbus_message_get_member(message);
    Arguments out_args;
    DBusDemarshaller demarshaller(message);
    if (demarshaller.GetArguments(&out_args)) {
      MethodSignalPrototypeMap::iterator it = signals_.find(member);
      if (it != signals_.end()) {
        // It's a registered signal, validate the argument types.
        if (!ValidateArguments(it->second.out_args, &out_args,
                               "signal", member)) {
          return;
        }
      } else {
        DLOG("Unknown signal received: %s, emit anyway", member);
      }
      Variant *vars = NULL;
      if (out_args.size()) {
        vars = new Variant[out_args.size()];
        for (size_t i = 0; i < out_args.size(); ++i)
          vars[i] = out_args[i].value.v();
      }
      on_signal_emit_signal_(member, static_cast<int>(out_args.size()), vars);
      delete[] vars;
    } else {
      DLOG("Failed to demarshal args of signal %s", member);
    }
  }

  Connection *ConnectOnReset(Slot0<void> *callback) {
    return on_reset_.Connect(callback);
  }

 public:
  static DBusProxy* NewSystemProxy(const std::string &name,
                                   const std::string &path,
                                   const std::string &interface) {
    if (name.length() && path.length() && interface.length()) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("New system dbus proxy: %s|%s|%s",
           name.c_str(), path.c_str(), interface.c_str());
#endif
      if (!system_bus_) {
        system_bus_ = new Manager(DBUS_BUS_SYSTEM);
      }

      Impl *impl = system_bus_->NewImpl(name, path, interface);
      if (impl) {
        DBusProxy *proxy = new DBusProxy();
        proxy->impl_ = impl;
        return proxy;
      }
    }
    DLOG("Failed to create system dbus proxy: %s|%s|%s",
         name.c_str(), path.c_str(), interface.c_str());
    return NULL;
  }

  static DBusProxy* NewSessionProxy(const std::string &name,
                                    const std::string &path,
                                    const std::string &interface) {
    if (name.length() && path.length() && interface.length()) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("New session dbus proxy: %s|%s|%s",
           name.c_str(), path.c_str(), interface.c_str());
#endif
      if (!session_bus_) {
        session_bus_ = new Manager(DBUS_BUS_SESSION);
      }

      Impl *impl = session_bus_->NewImpl(name, path, interface);
      if (impl) {
        DBusProxy *proxy = new DBusProxy();
        proxy->impl_ = impl;
        return proxy;
      }
    }
    DLOG("Failed to create session dbus proxy: %s|%s|%s",
         name.c_str(), path.c_str(), interface.c_str());
    return NULL;
  }

 private:
  // function_type and function_name are for debug purpose.
  bool ValidateArguments(const ArgPrototypeVector &expect_args,
                         Arguments *real_args,
                         const char *function_type,
                         const char *function_name) {
    if (expect_args.size() != real_args->size()) {
      DLOG("Arg number mismatch of %s %s of %s|%s|%s, "
           "expect:%zu actual:%zu", function_type, function_name,
           name_.c_str(), path_.c_str(), interface_.c_str(),
           expect_args.size(), real_args->size());
      return false;
    }
    for (size_t i = 0; i < expect_args.size(); ++i) {
      Variant::Type type =
          GetVariantTypeFromSignature(expect_args[i].signature);
      if (type != Variant::TYPE_VARIANT &&
          type != (*real_args)[i].value.v().type()) {
        DLOG("Type mismatch of arg %s of %s %s of %s|%s|%s, "
             "expect:%d actual:%d", expect_args[i].name.c_str(),
             function_type, function_name, name_.c_str(), path_.c_str(),
             interface_.c_str(), type, (*real_args)[i].value.v().type());
        return false;
      }
      // Update type signature of real argument, to meed expectation.
      (*real_args)[i].signature = expect_args[i].signature;
    }
    return true;
  }

  DBusConnection *GetBus() {
    if (manager_) {
      DBusConnection *bus = manager_->Get();
      if (!bus) {
        DLOG("Failed to get dbus for proxy %s|%s|%s",
             name_.c_str(), path_.c_str(), interface_.c_str());
      }
      return bus;
    } else {
      DLOG("Proxy %s|%s|%s has been detached from dbus.",
           name_.c_str(), path_.c_str(), interface_.c_str());
    }
    return NULL;
  }

  // Sends a message to remote object, if no reply is required, pass NULL to
  // pending_return parameter.
  bool SendMessage(DBusConnection *bus, const char *interface,
                   const char *method, const Arguments &in_args,
                   DBusPendingCall **pending_return, int timeout) {
    if (pending_return)
      *pending_return = NULL;

    DBusMessage *message =
        dbus_message_new_method_call(name_.c_str(), path_.c_str(),
                                     interface, method);
    if (!message) {
      DLOG("Failed to create message to %s|%s|%s|%s",
           name_.c_str(), path_.c_str(), interface, method);
      return false;
    }

    DBusMarshaller marshaller(message);
    if (!marshaller.AppendArguments(in_args)) {
      DLOG("Failed to marshal arguments for message to %s|%s|%s|%s",
           name_.c_str(), path_.c_str(), interface, method);
      dbus_message_unref(message);
      return false;
    }

    bool ret = false;
    if (pending_return) {
      ret = dbus_connection_send_with_reply(bus, message, pending_return,
                                            timeout);
      if (!ret && *pending_return) {
        dbus_pending_call_unref(*pending_return);
        *pending_return = NULL;
      } else if (*pending_return == NULL) {
        DLOG("DBus connection has been disconnected.");
        ret = false;
      }
    } else {
      ret = dbus_connection_send(bus, message, NULL);
    }
    if (!ret) {
      DLOG("Failed to send message to %s|%s|%s|%s",
           name_.c_str(), path_.c_str(), interface, method);
    } else {
      dbus_connection_flush(bus);
    }
    dbus_message_unref(message);
    return ret;
  }

  // Retrieves reply message from a pending call.
  // Returns true if a valid reply message is retrieved, the reply arguments
  // will be stored in out_args if it's not NULL.
  bool RetrieveReplyMessage(DBusPendingCall *pending_return,
                            Arguments *out_args) {
    bool ret = false;
    DBusError error;
    dbus_error_init(&error);
    dbus_pending_call_block(pending_return);
    DBusMessage *reply = dbus_pending_call_steal_reply(pending_return);
    if (reply) {
      if (!dbus_set_error_from_message(&error, reply)) {
        if (out_args) {
          DBusDemarshaller demarshaller(reply);
          ret = demarshaller.GetArguments(out_args);
        } else {
          ret = true;
        }
      }
      dbus_message_unref(reply);
    }
    if (!ret) {
      if (dbus_error_is_set(&error))
        DLOG("Failed to retrieve reply from %s|%s, error: %s, %s",
             name_.c_str(), path_.c_str(), error.name, error.message);
      else
        DLOG("Failed to retrieve reply from %s|%s",
             name_.c_str(), path_.c_str());
    }
    dbus_error_free(&error);
    return ret;
  }

  bool CallMethodSync(DBusConnection *bus, const char *interface,
                      const char *method, const Arguments &in_args,
                      Arguments *out_args, int timeout) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("Call method synchronously: %s|%s|%s|%s",
         name_.c_str(), path_.c_str(), interface, method);
#endif

    DBusPendingCall *pending_return = NULL;
    bool ret = SendMessage(bus, interface, method, in_args,
                           &pending_return, timeout);
    if (ret)
      ret = RetrieveReplyMessage(pending_return, out_args);

    if (pending_return)
      dbus_pending_call_unref(pending_return);

    return ret;
  }

  struct PendingCallClosure {
    Impl *impl;
    int call_id;
    std::string method;
    ResultCallback *callback;
  };

  static void PendingCallClosureFree(void *data) {
    PendingCallClosure *closure = reinterpret_cast<PendingCallClosure *>(data);
    ASSERT(closure);
    if (closure) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Free PendingCallClosure: %d, %s|%s", closure->call_id,
           closure->impl->name_.c_str(), closure->impl->path_.c_str());
#endif
      delete closure->callback;
      delete closure;
    }
  }

  static void PendingCallNotify(DBusPendingCall *pending, void *data) {
    PendingCallClosure *closure = reinterpret_cast<PendingCallClosure *>(data);
    ASSERT(closure);
    if (closure) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Pending call returned: %d, %s|%s", closure->call_id,
           closure->impl->name_.c_str(), closure->impl->path_.c_str());
#endif
      Impl *impl = closure->impl;
      if (closure->callback) {
        Arguments out_args;
        bool ret = impl->RetrieveReplyMessage(pending, &out_args);
        if (ret) {
          MethodSignalPrototypeMap::iterator it =
              impl->methods_.find(closure->method);
          if (it != closure->impl->methods_.end()) {
            // Validate return values.
            ret = impl->ValidateArguments(it->second.out_args, &out_args,
                                          "method", closure->method.c_str());
          }
        }
        CallResultCallback(closure->callback, out_args, ret);
      }
      // Remove this pending call from impl's pending call map.
      closure->impl->pending_calls_.erase(closure->call_id);
    }
    dbus_pending_call_unref(pending);
  }

  static void CallResultCallback(ResultCallback *callback,
                                 const Arguments &args,
                                 bool success) {
    if(args.size() && success) {
      Arguments::const_iterator it = args.begin();
      for (int count = 0; it != args.end(); ++it, ++count)
        if (!(*callback)(count, it->value.v()))
          break;
    } else {
      (*callback)(success ? 0 : -1, Variant());
    }
  }

  static void CallAndFreeResultCallback(ResultCallback *callback,
                                        const Arguments &args,
                                        bool success) {
    if (callback)
      CallResultCallback(callback, args, success);
    delete callback;
  }

  int NewCallId() {
    int call_id = call_id_counter_++;
    if (call_id_counter_ <= 0)
      call_id_counter_ = 1;
    return call_id;
  }

  // Returns call id, 0 means failed.
  int CallMethodAsync(DBusConnection *bus, const char *interface,
                      const char *method, const Arguments &in_args,
                      ResultCallback *callback, int timeout) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("Call method asynchronously: %s|%s|%s|%s",
         name_.c_str(), path_.c_str(), interface, method);
#endif

    DBusPendingCall *pending = NULL;
    bool ret = SendMessage(bus, interface, method, in_args,
                           &pending, timeout);
    if (ret && pending) {
      PendingCallClosure *closure = new PendingCallClosure;
      closure->impl = this;
      closure->call_id = NewCallId();
      closure->method = method;
      closure->callback = callback;
      dbus_pending_call_set_notify(pending, PendingCallNotify,
                                   closure, PendingCallClosureFree);
      pending_calls_[closure->call_id] = pending;
#ifdef DBUS_VERBOSE_LOG
      DLOG("Succeeded: pending call id: %d", closure->call_id);
#endif
      return closure->call_id;
    }

    delete callback;
    if (pending)
      dbus_pending_call_unref(pending);

    DLOG("Asynchronous call Failed: %s|%s|%s|%s",
         name_.c_str(), path_.c_str(), interface, method);
    return 0;
  }

  bool PingPeer(DBusConnection *bus) {
    // DBus service itself doesn't support PEER interface.
    if (name_ == DBUS_SERVICE_DBUS)
      return true;
    Arguments in_args;
    return CallMethodSync(bus, DBUS_INTERFACE_PEER, "Ping",
                          in_args, NULL, kDefaultDBusTimeout);
  }

  void ClearIntrospectData() {
    methods_.clear();
    signals_.clear();
    properties_.clear();
    interfaces_.clear();
    children_.clear();
  }

  // See:
  // http://dbus.freedesktop.org/doc/dbus-specification.html
  // Section: Introspection Data Format
  bool Introspect(bool sync) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("Introspect dbus object %s: %s|%s", (sync ? "sync" : "async"),
         name_.c_str(), path_.c_str());
#endif
    DBusConnection* bus = GetBus();
    ASSERT(bus);

    if (!bus)
      return false;

    Arguments in_args;
    if (sync) {
      Arguments out_args;
      if (!CallMethodSync(bus, DBUS_INTERFACE_INTROSPECTABLE, "Introspect",
                          in_args, &out_args, kDefaultDBusTimeout)) {
        DLOG("Failed to get introspect xml from %s|%s",
             name_.c_str(), path_.c_str());
        return false;
      }
      std::string xml;
      if (out_args.size() < 1 || !out_args[0].value.v().ConvertToString(&xml)) {
        DLOG("Invalid introspect xml data got from %s|%s",
             name_.c_str(), path_.c_str());
        return false;
      }
      if (!ParseIntrospectResult(xml)) {
        ClearIntrospectData();
        return false;
      }
      return true;
    } else {
      int call_id =
          CallMethodAsync(bus, DBUS_INTERFACE_INTROSPECTABLE,
                          "Introspect", in_args,
                          NewSlot(this, &Impl::IntrospectResultReceiver),
                          -1);
      if (call_id == 0) {
        ClearIntrospectData();
        // IntrospectResultReceiver() won't be called if CallMethodAsync()
        // failed, so we need emit on_reset_ signal here.
        on_reset_();
        return false;
      }
      return true;
    }
  }

  bool IntrospectResultReceiver(int index, const Variant& result) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("Introspect data received: %s|%s", name_.c_str(), path_.c_str());
#endif
    ClearIntrospectData();
    std::string xml;
    if (index == 0 && result.ConvertToString(&xml)) {
      if (!ParseIntrospectResult(xml))
        ClearIntrospectData();
    }
    on_reset_();
    return false;
  }

  bool ParseIntrospectResult(const std::string& xml) {
    XMLParserInterface *xml_parser = GetXMLParser();
    ASSERT(xml_parser);

    DOMDocumentInterface *domdoc = xml_parser->CreateDOMDocument();
    domdoc->Ref();
    std::string filename =
        StringPrintf("%s|%s/Introspect.xml", name_.c_str(), path_.c_str());
    if (!xml_parser->ParseContentIntoDOM(xml, NULL, filename.c_str(), NULL,
                                         NULL, kEncodingFallback, domdoc,
                                         NULL, NULL)) {
      DLOG("Failed to parse introspect xml content of %s|%s:\n%s",
           name_.c_str(), path_.c_str(), xml.c_str());
      ASSERT(domdoc->GetRefCount() == 1);
      domdoc->Unref();
      return false;
    }

    DOMElementInterface *root_node = domdoc->GetDocumentElement();
    if (!root_node) {
      DLOG("Failed to get root node, %s|%s:\n%s",
           name_.c_str(), path_.c_str(), xml.c_str());
      ASSERT(domdoc->GetRefCount() == 1);
      domdoc->Unref();
      return false;
    }
    std::string tag_name = root_node->GetTagName();
    std::string name_attr = root_node->GetAttribute("name");
    if (tag_name != "node" || !(name_attr.empty() || name_attr == path_)) {
      DLOG("Invalid root node, %s|%s:\n%s",
           name_.c_str(), path_.c_str(), xml.c_str());
      ASSERT(domdoc->GetRefCount() == 1);
      domdoc->Unref();
      return false;
    }

    bool result = true;
    DOMNodeInterface *node = root_node->GetFirstChild();
    for (; node && result; node = node->GetNextSibling()) {
      tag_name = node->GetNodeName();
      if (node->GetNodeType() != DOMNodeInterface::ELEMENT_NODE) {
        DLOG("Invalid root sub node: %s", tag_name.c_str());
        continue;
      }
      DOMElementInterface *elm = down_cast<DOMElementInterface *>(node);
      if (tag_name == "interface") {
        result = ParseInterfaceNode(elm);
      } else if (tag_name == "node") {
        result = ParseChildNode(elm);
      }
    }

    ASSERT(domdoc->GetRefCount() == 1);
    domdoc->Unref();
    if (!result)
      DLOG("Failed to introspect %s|%s", name_.c_str(), path_.c_str());
#ifdef DBUS_VERBOSE_LOG
    else
      DLOG("Introspect result:\n%s", PrintProxyInfo().c_str());
#endif
    return result;
  }

  bool ParseInterfaceNode(DOMElementInterface *interface_node) {
    std::string name_attr = interface_node->GetAttribute("name");
    if (std::find(interfaces_.begin(), interfaces_.end(), name_attr) ==
        interfaces_.end()) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Found interface for %s|%s: %s",
           name_.c_str(), path_.c_str(), name_attr.c_str());
#endif
      interfaces_.push_back(name_attr);
    }

    // If it's not self interface, just return.
    if (name_attr != interface_)
      return true;

    bool result = true;
    DOMNodeInterface *node = interface_node->GetFirstChild();
    for (; node && result; node = node->GetNextSibling()) {
      std::string tag_name = node->GetNodeName();
      if (node->GetNodeType() != DOMNodeInterface::ELEMENT_NODE) {
        DLOG("Invalid interface sub node: %s", tag_name.c_str());
        continue;
      }
      DOMElementInterface *elm = down_cast<DOMElementInterface *>(node);
      if (tag_name == "method") {
        result = ParseMethodSignalNode(elm, true);
      } else if (tag_name == "signal") {
        result = ParseMethodSignalNode(elm, false);
      } else if (tag_name == "property") {
        result = ParsePropertyNode(elm);
      }
    }
    return result;
  }

  bool ParseChildNode(DOMElementInterface *node) {
    std::string name_attr = node->GetAttribute("name");
    // Child node can't have absolute path.
    if (name_attr.length() && name_attr[0] =='/')
      return false;
    if (name_attr.empty())
      name_attr = StringPrintf("child_%zu", children_.size());
    children_.push_back(name_attr);
    return true;
  }

  bool ParseMethodSignalNode(DOMElementInterface *node, bool is_method) {
    std::string name_attr = node->GetAttribute("name");
    if (name_attr.empty()) {
      DLOG("Ignore anonymous %s node.", is_method ? "method" : "signal");
      return true;
    }

    MethodSignalPrototype proto;
    DOMNodeInterface *sub_node = node->GetFirstChild();
    for (; sub_node; sub_node = sub_node->GetNextSibling()) {
      std::string tag_name = sub_node->GetNodeName();
      if (sub_node->GetNodeType() != DOMNodeInterface::ELEMENT_NODE) {
        DLOG("Invalid %s sub node: %s",
             is_method ? "method" : "signal", tag_name.c_str());
        continue;
      }
      DOMElementInterface *elm = down_cast<DOMElementInterface *>(sub_node);
      if (tag_name == "arg") {
        ArgPrototype arg_proto;
        bool is_in;
        if (ParseArgNode(elm, &arg_proto, &is_in)) {
          if (is_method && is_in)
            proto.in_args.push_back(arg_proto);
          else
            proto.out_args.push_back(arg_proto);
        }
      }
    }

    if (is_method)
      methods_[name_attr] = proto;
    else
      signals_[name_attr] = proto;

    return true;
  }

  bool ParsePropertyNode(DOMElementInterface *node) {
    std::string name_attr = node->GetAttribute("name");
    std::string type_attr = node->GetAttribute("type");
    std::string access_attr = node->GetAttribute("access");
    if (name_attr.length() && type_attr.length() && access_attr.length()) {
      PropertyPrototype proto;
      if (access_attr == "read")
        proto.access = PROP_READ;
      else if (access_attr == "write")
        proto.access = PROP_WRITE;
      else if (access_attr == "readwrite")
        proto.access = PROP_READ_WRITE;
      else
        proto.access = PROP_UNKNOWN;

      if (proto.access != PROP_UNKNOWN) {
        proto.signature = type_attr;
        properties_[name_attr] = proto;
      }
    }
    return true;
  }

  bool ParseArgNode(DOMElementInterface *arg_node, ArgPrototype *proto,
                    bool *is_in) {
    std::string name_attr = arg_node->GetAttribute("name");
    std::string type_attr = arg_node->GetAttribute("type");
    std::string dir_attr = arg_node->GetAttribute("direction");
    proto->name = name_attr;
    proto->signature = type_attr;
    *is_in = (dir_attr.empty() || dir_attr == "in");
    return type_attr.length() && (*is_in || dir_attr == "out");
  }

#ifdef _DEBUG
  std::string PrintProxyInfo() {
    std::string info;
    info = StringPrintf("%s|%s|%s:\n",
                        name_.c_str(), path_.c_str(), interface_.c_str());
    StringAppendPrintf(&info, "Methods:\n");
    MethodSignalPrototypeMap::iterator it = methods_.begin();
    for (; it != methods_.end(); ++it) {
      StringAppendPrintf(&info, "  %s:\n", it->first.c_str());
      StringAppendPrintf(&info, "     in :");
      ArgPrototypeVector::iterator ait = it->second.in_args.begin();
      ArgPrototypeVector::iterator aend = it->second.in_args.end();
      for (; ait != aend; ++ait) {
        StringAppendPrintf(&info, " %s:%s",
                           ait->name.c_str(), ait->signature.c_str());
      }
      StringAppendPrintf(&info, "\n    out :");
      ait = it->second.out_args.begin();
      aend = it->second.out_args.end();
      for (; ait != aend; ++ait) {
        StringAppendPrintf(&info, " %s:%s",
                           ait->name.c_str(), ait->signature.c_str());
      }
      info.append("\n");
    }
    StringAppendPrintf(&info, "Signals:\n");
    it = signals_.begin();
    for (; it != signals_.end(); ++it) {
      StringAppendPrintf(&info, "  %s:", it->first.c_str());
      ArgPrototypeVector::iterator ait = it->second.out_args.begin();
      ArgPrototypeVector::iterator aend = it->second.out_args.end();
      for (; ait != aend; ++ait) {
        StringAppendPrintf(&info, " %s:%s",
                           ait->name.c_str(), ait->signature.c_str());
      }
      info.append("\n");
    }
    StringAppendPrintf(&info, "Properties:\n");
    PropertyPrototypeMap::iterator pit = properties_.begin();
    for (; pit != properties_.end(); ++pit) {
      StringAppendPrintf(&info, "  %s: type:%s dir:%d\n",
                         pit->first.c_str(), pit->second.signature.c_str(),
                         pit->second.access);
    }
    StringAppendPrintf(&info, "Interfaces:\n");
    StringVector::iterator sit = interfaces_.begin();
    for(; sit != interfaces_.end(); ++sit) {
      StringAppendPrintf(&info, "  %s\n", sit->c_str());
    }
    StringAppendPrintf(&info, "Children:\n");
    sit = children_.begin();
    for(; sit != children_.end(); ++sit) {
      StringAppendPrintf(&info, "  %s\n", sit->c_str());
    }
    return info;
  }
#endif

 private:
  Manager *manager_;

  std::string name_;
  std::string path_;
  std::string interface_;
  Connection *on_name_owner_changed_connection_;

  int refcount_;
  int call_id_counter_;
  PendingCallMap pending_calls_;

  MethodSignalPrototypeMap methods_;
  MethodSignalPrototypeMap signals_;
  PropertyPrototypeMap properties_;
  StringVector interfaces_;
  StringVector children_;

  Signal3<void, const std::string &, int, const Variant *>
      on_signal_emit_signal_;

  Signal0<void> on_reset_;

 private:
  static Manager *system_bus_;
  static Manager *session_bus_;
};

DBusProxy::Impl::Manager *DBusProxy::Impl::system_bus_ = NULL;
DBusProxy::Impl::Manager *DBusProxy::Impl::session_bus_ = NULL;

DBusProxy::DBusProxy() : impl_(NULL) {
}
DBusProxy::~DBusProxy() {
  impl_->Unref();
}
std::string DBusProxy::GetName() const {
  return impl_->GetName();
}
std::string DBusProxy::GetPath() const {
  return impl_->GetPath();
}
std::string DBusProxy::GetInterface() const {
  return impl_->GetInterface();
}
int DBusProxy::CallMethod(const std::string &method, bool sync, int timeout,
                          ResultCallback *callback,
                          MessageType first_arg_type, ...) {
  va_list args;
  va_start(args, first_arg_type);
  int ret = impl_->CallMethod(method, sync, timeout, callback,
                              first_arg_type, &args);
  va_end(args);
  return ret;
}
int DBusProxy::CallMethod(const std::string &method, bool sync, int timeout,
                          ResultCallback *callback,
                          int argc, const Variant *argv) {
  ASSERT(argc == 0 || argv);
  Arguments in_args;
  if (argc > 0 && argv) {
    for (int i = 0; i < argc; ++i)
      in_args.push_back(Argument(argv[i]));
  }
  return impl_->CallMethod(method, sync, timeout, callback, &in_args);
}
bool DBusProxy::CancelMethodCall(int index) {
  return impl_->CancelMethodCall(index);
}
bool DBusProxy::IsMethodCallPending(int index) const {
  return impl_->IsMethodCallPending(index);
}
bool DBusProxy::GetMethodInfo(const std::string &method,
                              int *argc, Variant::Type **arg_types,
                              int *retc, Variant::Type **ret_types) {
  return impl_->GetMethodInfo(method, argc, arg_types, retc, ret_types);
}
bool DBusProxy::EnumerateMethods(Slot1<bool, const std::string &> *callback) {
  return impl_->EnumerateMethods(callback);
}
ResultVariant DBusProxy::GetProperty(const std::string &property) {
  return impl_->GetProperty(property);
}
bool DBusProxy::SetProperty(const std::string &property, const Variant &value) {
  return impl_->SetProperty(property, value);
}
DBusProxy::PropertyAccess DBusProxy::GetPropertyInfo(
    const std::string &property, Variant::Type *type) {
  return impl_->GetPropertyInfo(property, type);
}
bool DBusProxy::EnumerateProperties(
    Slot1<bool, const std::string &> *callback) {
  return impl_->EnumerateProperties(callback);
}
Connection* DBusProxy::ConnectOnSignalEmit(
    Slot3<void, const std::string &, int, const Variant *> *callback) {
  return impl_->ConnectOnSignalEmit(callback);
}
bool DBusProxy::GetSignalInfo(const std::string &signal, int *argc,
                              Variant::Type **arg_types) {
  return impl_->GetSignalInfo(signal, argc, arg_types);
}
bool DBusProxy::EnumerateSignals(Slot1<bool, const std::string &> *callback) {
  return impl_->EnumerateSignals(callback);
}
DBusProxy* DBusProxy::NewChildProxy(const std::string &path,
                                    const std::string &interface) {
  return impl_->NewChildProxy(path, interface);
}
bool DBusProxy::EnumerateChildren(Slot1<bool, const std::string &> *callback) {
  return impl_->EnumerateChildren(callback);
}
DBusProxy* DBusProxy::NewInterfaceProxy(const std::string &interface) {
  return impl_->NewInterfaceProxy(interface);
}
bool DBusProxy::EnumerateInterfaces(
    Slot1<bool, const std::string &> *callback) {
  return impl_->EnumerateInterfaces(callback);
}
Connection* DBusProxy::ConnectOnReset(Slot0<void> *callback) {
  return impl_->ConnectOnReset(callback);
}

DBusProxy* DBusProxy::NewSystemProxy(const std::string &name,
                                     const std::string &path,
                                     const std::string &interface) {
  return DBusProxy::Impl::NewSystemProxy(name, path, interface);
}
DBusProxy* DBusProxy::NewSessionProxy(const std::string &name,
                                      const std::string &path,
                                      const std::string &interface) {
  return DBusProxy::Impl::NewSessionProxy(name, path, interface);
}

}  // namespace dbus
}  // namespace ggadget
