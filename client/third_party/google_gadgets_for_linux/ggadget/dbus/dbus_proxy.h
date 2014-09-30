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

#ifndef GGADGET_DBUS_DBUS_PROXY_H__
#define GGADGET_DBUS_DBUS_PROXY_H__

#include <string>
#include <ggadget/variant.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/small_object.h>

namespace ggadget {

class RegisterableInterface;
DECLARE_VARIANT_PTR_TYPE(Variant);

namespace dbus {

/**
 * @defgroup DBusLibrary libggadget-dbus - the DBus Proxy
 * @ingroup SharedLibraries
 *
 * This shared library contains DBusProxy class, which is for accessing remote
 * DBus objects in C++ code.
 * @{
 */

static const int kDefaultDBusTimeout = 1000;  // 1 second.

/**
 * Message types corresponding to DBus types.
 */
enum MessageType {
  MESSAGE_TYPE_INVALID = 0,
  MESSAGE_TYPE_BYTE,
  MESSAGE_TYPE_BOOLEAN,
  MESSAGE_TYPE_INT16,
  MESSAGE_TYPE_UINT16,
  MESSAGE_TYPE_INT32,
  MESSAGE_TYPE_UINT32,
  MESSAGE_TYPE_INT64,
  MESSAGE_TYPE_UINT64,
  MESSAGE_TYPE_DOUBLE,
  MESSAGE_TYPE_STRING,
  MESSAGE_TYPE_OBJECT_PATH,
  MESSAGE_TYPE_SIGNATURE,
  MESSAGE_TYPE_ARRAY,
  MESSAGE_TYPE_STRUCT,
  MESSAGE_TYPE_VARIANT,
  MESSAGE_TYPE_DICT
};

/**
 * A class to wrap a remote DBus object.
 *
 * This class is used for C++ code to access remote DBus objects.
 * It supports accessing properties, methods and signals of remote DBus
 * objects.
 * Exporting local C++ object to DBus is not supported.
 */
class DBusProxy : public SmallObject<> {
 public:
  /**
   * Access typs of a property.
   * They can be used as bitmask.
   */
  enum PropertyAccess {
    PROP_UNKNOWN = 0,
    PROP_READ = 1,
    PROP_WRITE = 2,
    PROP_READ_WRITE = 3
  };

  ~DBusProxy();

  /** Gets the name of remote target bound to this proxy. */
  std::string GetName() const;

  /** Gets the path of remote object bound to this proxy. */
  std::string GetPath() const;

  /** Gets the interface name of remote object bound to this proxy. */
  std::string GetInterface() const;

  /**
   * Callback slot to receive values from the DBus server. The callback will
   * return a @c bool: @true if it want to keep receiving next argument and
   * @false otherwise. The first parameter of the callback indecate the index of
   * current argument, and the second parameter is the value of current argument.
   *
   * When error occurres, the callback will be called with -1 as the index and
   * an error message as the argument value.
   */
  typedef Slot2<bool, int, const Variant&> ResultCallback;

  /**
   * Calls a method of the remote DBus object.
   * All of the input arguments are specified first,
   * followed by @c MESSAGE_TYPE_INVALID.
   *
   * This method is for writing C/C++ code.
   *
   * @param method method name to call
   * @param sync @c true if the caller want to block this method and wait for
   *        reply and @c false otherwise.
   * @param timeout timeout in milisecond that the caller would
   *        cancel waiting reply. When @c timeout is set to -1,
   *        for sync case, sane default timeout value will be set by dbus,
   *        for async case, callback will always be there until a reply back.
   * @param callback callback to receive the reply returned from DBus server
   *        Note that the @c Call will own the callback and delete it after
   *        execute. If it is set to @c NULL, the method will not wait for the
   *        reply.
   * @param first_arg_type type of first input argument
   * @return a number greater than zero will be returned when succeeds,
   *    otherwise returns zero. For async call, the returned number can be used
   *    to cancel the call.
   */
  int CallMethod(const std::string &method, bool sync, int timeout,
                 ResultCallback *callback,
                 MessageType first_arg_type, ...);

  /**
   * Calls a method of the remote DBus object.
   *
   * This method is for script binding.
   *
   * @param method method name to call
   * @param sync @c true if the caller want to block this method and wait for
   *        reply and @c false otherwise.
   * @param timeout timeout in milisecond that the caller would
   *        cancel waiting reply. When @c timeout is set to -1,
   *        for sync case, sane default time out value will be set by dbus,
   *        and for async case, callback will always there until a reply back
   * @param callback callback to receive the arguments returned from DBus server
   *        Note that the @c Call will own the callback and delete it after
   *        execute. If it is set to @c NULL, the method will not wait for the
   *        reply.
   * @param argc number of arguments
   * @param argv array to hold arguments
   * @return a number greater than zero will be returned when succeeds,
   *    otherwise returns zero. For async call, the returned number can be used
   *    to cancel the call.
   */
  int CallMethod(const std::string &method, bool sync, int timeout,
                 ResultCallback *callback,
                 int argc, const Variant *argv);

  /**
   * Cancels an async method call.
   *
   * @param index the index returned by CallMethod().
   * @return @c true when succeeds, otherwsize returns @c false.
   */
  bool CancelMethodCall(int index);

  /** Checks if a specified method call is still pending. */
  bool IsMethodCallPending(int index) const;

  /**
   * Gets the information of a known method by its name.
   *
   * @param method the name of the method.
   * @param[out] argc number of arguments
   * @param[out] arg_types an array containing the types of arguments,
   *    caller must free it.
   * @param[out] retc number of return values.
   * @param[out] ret_types an array containing the types of return values,
   *    caller must free it.
   * @return @c true if the method is available.
   */
  bool GetMethodInfo(const std::string &method,
                     int *argc, Variant::Type **arg_types,
                     int *retc, Variant::Type **ret_types);

  /**
   * Enumerates all known properties.
   * @param callback it will be called for each property with property's name.
   *        The callback should return @c false if it doesn't want to continue.
   *        It will be deleted by this method after finishing enumeration.
   * @return @c false if the callback returns @c false.
   */
  bool EnumerateMethods(Slot1<bool, const std::string &> *callback);

  /**
   * Gets the value of a named property of the DBusProxy object.
   *
   * @param property the name of the property.
   * @return the property value, or a @c Variant of type @c Variant::TYPE_VOID
   *     if this property is not supported.
   */
  ResultVariant GetProperty(const std::string &property);

  /**
   * Sets the value of a named property.
   * @param property the name of the property.
   * @param value the property value. The type must be compatible with the
   *     prototype returned from @c GetPropertyInfo().
   * @return @c true if the property is supported and succeeds.
   */
  bool SetProperty(const std::string &property, const Variant &value);

  /**
   * Gets the information of a known property by its name.
   *
   * @param property the name of the property.
   * @param[out,optional] type returns the value type of the property.
   * @return The access type of the property. If the property is unknown,
   *         PROP_UNKNOWN will be returned.
   */
  PropertyAccess GetPropertyInfo(const std::string &property,
                                 Variant::Type *type);

  /**
   * Enumerates all known properties.
   * @param callback it will be called for each property with property's name.
   *        The callback should return @c false if it doesn't want to continue.
   *        It will be deleted by this method after finishing enumeration.
   * @return @c false if the callback returns @c false.
   */
  bool EnumerateProperties(Slot1<bool, const std::string &> *callback);

  /**
   * Connects a slot to SignalEmit signal.
   *
   * The SignalEmit signal will be emitted whenever a signal message of the
   * DBusProxy object is received. The slot connected to this signal will be
   * called with following parameters:
   *  - name of the remote signal
   *  - number of arguments
   *  - an Variant array containing values of all arguments
   * The slot returns nothing.
   *
   * @param callback the slot to be connected to this signal.
   * @return the connected @c Connection, or @c NULL on any error. The caller
   *    shall always save the connection and disconnect it when destroying,
   *    because dbus proxies with the same name, path and interface shares
   *    the same OnSignalEmit signal.
   */
  Connection* ConnectOnSignalEmit(
      Slot3<void, const std::string &, int, const Variant *> *callback);

  /**
   * Gets the information of a known signal by its name.
   *
   * @param signal the name of the signal.
   * @param[out] argc number of arguments
   * @param[out] arg_types an array containing the types of arguments,
   *    caller must free it.
   * @return @c true if the signal is available.
   */
  bool GetSignalInfo(const std::string &signal,
                     int *argc, Variant::Type **arg_types);

  /**
   * Enumerates all known signals.
   * @param callback it will be called for each signal with signal's name.
   *        The callback should return @c false if it doesn't want to continue.
   *        It will be deleted by this method after finishing enumeration.
   * @return @c false if the callback returns @c false.
   */
  bool EnumerateSignals(Slot1<bool, const std::string &> *callback);

  /**
   * Creates a new proxy for a child node of this proxy.
   *
   * @param path the relative path of the child node.
   * @param interface the interface to be used.
   * @return a new DBusProxy object for the specified child node, or @c NULL
   *    if the specified child node is not available.
   */
  DBusProxy *NewChildProxy(const std::string &path,
                           const std::string &interface);

  /**
   * Enumerates all known children of this proxy.
   * @param callback it will be called for each child with child's name.
   *        The callback should return @c false if it doesn't want to continue.
   *        It will be deleted by this method after finishing enumeration.
   * @return @c false if the callback returns @c false.
   */
  bool EnumerateChildren(Slot1<bool, const std::string &> *callback);

  /**
   * Creates a new proxy for a specified interface of this proxy.
   *
   * @param interface name of the interface.
   * @return a new DBusProxy object for the specified interface, or @c NULL
   *    if the specified interface is not supported by this proxy.
   */
  DBusProxy *NewInterfaceProxy(const std::string &interface);

  /**
   * Enumerates all known interfaces of this proxy.
   * @param callback it will be called for names of each interface.
   *        The callback should return @c false if it doesn't want to continue.
   *        It will be deleted by this method after finishing enumeration.
   * @return @c false if the callback returns @c false.
   */
  bool EnumerateInterfaces(Slot1<bool, const std::string &> *callback);

  /**
   * A signal which will be emitted when the proxy has been reset,
   * because remote object owner has been changed.
   */
  Connection *ConnectOnReset(Slot0<void> *callback);

 public:
  /**
   * Creates a proxy for an object on system bus.
   * @param name destination name on the message bus
   * @param path path to the object instance
   * @param interface name of the interface of the object
   */
  static DBusProxy* NewSystemProxy(const std::string &name,
                                   const std::string &path,
                                   const std::string &interface);

  /**
   * Creates a proxy for an object on session bus.
   * @param name destination name on the message bus
   * @param path path to the object instance
   * @param interface name of the interface of the object
   */
  static DBusProxy* NewSessionProxy(const std::string &name,
                                    const std::string &path,
                                    const std::string &interface);

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating DBusProxy object directly.
   */
  DBusProxy();

  DISALLOW_EVIL_CONSTRUCTORS(DBusProxy);
};

/** @} */

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_DBUS_PROXY_H__
