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

#ifndef GGADGET_DBUS_DBUS_RESULT_RECEIVER_H__
#define GGADGET_DBUS_DBUS_RESULT_RECEIVER_H__

#include <map>
#include <string>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/variant.h>
#include <ggadget/slot.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/dbus/dbus_proxy.h>

namespace ggadget {
namespace dbus {
/**
 * Template class to receive single result from DBusProxy::Call().
 *
 * Usage:
 *
 * DBusProxy *proxy;
 *
 * ...
 *
 * DBusSingleResultReceiver<std::string> receiver;
 * proxy->Call(method, true, -1, receiver.NewSlot(), ...);
 *
 * return receiver.GetValue();
 */
template<typename T>
class DBusSingleResultReceiver {
 public:
  typedef typename VariantValue<T>::value_type RType;

  DBusSingleResultReceiver()
    : result_(Variant(VariantType<T>::type)) {
  }

  DBusSingleResultReceiver(T def_value)
    : result_(Variant(def_value)) {
  }

  DBusProxy::ResultCallback *NewSlot() {
    return ggadget::NewSlot(this, &DBusSingleResultReceiver<T>::Callback);
  }

  RType GetValue() const {
    return VariantValue<T>()(result_.v());
  }

  bool Callback(int id, const Variant& result) {
    if (id == 0 && result.type() == VariantType<T>::type) {
      result_ = ResultVariant(result);
      return true;
    }
    return false;
  }

  void Reset() {
    result_ = ResultVariant(Variant(VariantType<T>::type));
  }

  void Reset(T def_value) {
    result_ = ResultVariant(Variant(def_value));
  }

 private:
  ResultVariant result_;
};

typedef DBusSingleResultReceiver<bool> DBusBooleanReceiver;
typedef DBusSingleResultReceiver<int64_t> DBusIntReceiver;
typedef DBusSingleResultReceiver<std::string> DBusStringReceiver;
typedef DBusSingleResultReceiver<double> DBusDoubleReceiver;
typedef DBusSingleResultReceiver<ScriptableInterface *> DBusScriptableReceiver;


/**
 * Class to receive an array of single type from DBusProxy::Call()
 *
 * Usage:
 *
 * DBusProxy *proxy;
 *
 * ...
 *
 * StringVector result
 * DBusArrayResultReceiver<StringVector> receiver(&result);
 *
 * proxy->Call("method, true, -1, receiver.NewSlot(), ...);
 *
 * return result;
 */
template<typename T>
class DBusArrayResultReceiver {
 public:
  typedef typename T::value_type ValueType;

  DBusArrayResultReceiver(T *result)
    : result_(result) {
    ASSERT(result_);
    result_->clear();
  }

  DBusProxy::ResultCallback *NewSlot() {
    return ggadget::NewSlot(this, &DBusArrayResultReceiver<T>::Callback);
  }

  bool Callback(int id, const Variant& result) {
    if (id == 0 && result.type() == Variant::TYPE_SCRIPTABLE) {
      result_->clear();
      ScriptableInterface *array = VariantValue<ScriptableInterface*>()(result);
      return array->EnumerateElements(
          ggadget::NewSlot(this, &DBusArrayResultReceiver::Enumerator));
    }
    return false;
  }

 private:
  bool Enumerator(int id, const Variant &value) {
    if (VariantType<ValueType>::type != value.type()) {
      DLOG("Type mismatch of the no. %d element in the array,"
           " expect %d, actual %d", id, VariantType<T>::type, value.type());
      return false;
    }
    result_->push_back(VariantValue<ValueType>()(value));
    return true;
  }

  T *result_;
};

typedef DBusArrayResultReceiver<std::vector<bool> > DBusBooleanArrayReceiver;
typedef DBusArrayResultReceiver<std::vector<int64_t> > DBusIntArrayReceiver;
typedef DBusArrayResultReceiver<StringVector> DBusStringArrayReceiver;
typedef DBusArrayResultReceiver<std::vector<double> > DBusDoubleArrayReceiver;

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_DBUS_RECEIVER_H__
