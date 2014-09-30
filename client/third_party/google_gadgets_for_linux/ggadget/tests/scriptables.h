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

#ifndef GGADGET_TESTS_SCRIPTABLES_H__
#define GGADGET_TESTS_SCRIPTABLES_H__

// This file is to be included by unittest files

#include <string>
#include <map>
#include <stdarg.h>
#include <stdio.h>
#include "ggadget/scriptable_array.h"
#include "ggadget/scriptable_interface.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/scriptable_holder.h"
#include "ggadget/signals.h"
#include "ggadget/slot.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
extern std::string g_buffer;

void AppendBuffer(const char *format, ...);

// A normal scriptable class.
class BaseScriptable : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0xdb06ba021f1b4c05, ScriptableInterface);

  BaseScriptable(bool native_owned, bool register_class);
  virtual ~BaseScriptable();

  void ClearBuffer() {
    g_buffer.clear();
  }
  double MethodDouble2(bool p1, long p2) {
    AppendBuffer("MethodDouble2(%d, %ld)\n", p1, p2);
    return static_cast<double>(p1 ? p2 : -p2);
  }
  void SetDoubleProperty(double double_property) {
    double_property_ = double_property;
    AppendBuffer("SetDoubleProperty(%.3lf)\n", double_property_);
  }
  double GetDoubleProperty() const {
    AppendBuffer("GetDoubleProperty()=%.3lf\n", double_property_);
    return double_property_;
  }
  void SetIntProperty(int64_t int_property) {
    int_property_ = int_property;
    AppendBuffer("SetIntProperty(%jd)\n", int_property_);
  }
  int64_t GetIntProperty() const {
    AppendBuffer("GetIntProperty()=%jd\n", int_property_);
    return int_property_;
  }

  std::string GetBuffer() const {
    return g_buffer;
  }
  void SetBuffer(const std::string& buffer) {
    g_buffer = "Buffer:" + buffer;
  }

  JSONString GetJSON() const {
    return JSONString(g_buffer);
  }
  void SetJSON(const JSONString json) {
    g_buffer = json.value;
  }

  bool IsNativeOwned() { return native_owned_; }

  // This signal is only for test, no relation to ConnectOnReferenceChange.
  // Place this signal declaration here for testing.
  Signal0<void> my_ondelete_signal_;

  enum EnumType { VALUE_0, VALUE_1, VALUE_2 };

  EnumType GetEnumProperty() const { return enum_property_; }
  void SetEnumProperty(EnumType e) { enum_property_ = e; }

  Variant GetVariantProperty() const { return variant_property_; }
  void SetVariantProperty(const Variant &v) { variant_property_ = v; }

 protected:
  void DoRegisterConstants();
  virtual void DoClassRegister();
  // If !register_class_, register is done in the constructor.
  bool register_class_;

 private:
  bool native_owned_;
  double double_property_;
  int64_t int_property_;
  EnumType enum_property_;
  Variant variant_property_;
};

class Prototype : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xbb7f8eddc2e94353, ScriptableInterface);
  static Prototype *GetInstance() {
    return instance_ ? instance_ : (instance_ = new Prototype());
  }

  // Place this signal declaration here for testing.
  // In production code, it should be palced in private section.
  Signal0<void> ontest_signal_;

  ScriptableInterface *Method(const ScriptableInterface *s) {
    return const_cast<ScriptableInterface *>(s);
  }
  Prototype *GetSelf() { return this; }

 private:
  Prototype();

  static Prototype *instance_;
};

// A scriptable class with some dynamic properties, supporting array indexes,
// and some property/methods with arguments or return types of Scriptable.
class ExtScriptable : public BaseScriptable {
 public:
  DEFINE_CLASS_ID(0xa88ea50b8b884e, BaseScriptable);
  ExtScriptable(bool native_owned, bool strict, bool register_class);
  virtual ~ExtScriptable();

  virtual bool IsStrict() const { return strict_; }

  static const int kArraySize = 20;

  Variant GetArray(int index) const {
    if (index >= 0 && index < kArraySize)
      return Variant(array_[index]);
    return Variant();  // void
  }

  bool SetArray(int index, int value) {
    if (index >= 0 && index < kArraySize) {
      // Distinguish from JavaScript builtin logic.
      array_[index] = value + 10000;
      return true;
    }
    return false;
  }

  Variant GetDynamicProperty(const char *name, bool get_info) {
    if (name[0] == 'd')
      return Variant(dynamic_properties_[name]);
    if (name[0] == 's') {
      if (get_info)
        return Variant(&dynamic_signal_prototype_);
      return Variant(dynamic_signal_.GetDefaultSlot());
    }
    return Variant();
  }

  bool SetDynamicProperty(const char *name, const Variant& value) {
    if (name[0] == 'd') {
      // Distinguish from JavaScript builtin logic.
      dynamic_properties_[name] =
          std::string("Value:") + VariantValue<const std::string&>()(value);
      return true;
    }
    if (name[0] == 's') {
      dynamic_signal_.SetDefaultSlot(VariantValue<Slot *>()(value));
      return true;
    }
    return false;
  }

  void FireDynamicSignal() {
    dynamic_signal_();
  }

  ExtScriptable *GetSelf() { return this; }
  ExtScriptable *ObjectMethod(const ExtScriptable *t) {
    return const_cast<ExtScriptable *>(t);
  }
  ExtScriptable *NewObject(bool native_owned, bool strict,
                           bool register_class) {
    ExtScriptable *result = new ExtScriptable(native_owned, strict,
                                              register_class);
    DLOG("NewObject: %p", result);
    return result;
  }
  void ReleaseObject(ExtScriptable *obj) {
    if (obj) {
      if (obj->IsNativeOwned())
        delete obj;
      else
        obj->Unref();
    }
    DLOG("ReleaseObject: %p", obj);
  }
  ScriptableArray *ConcatArray(ScriptableInterface *input1,
                               ScriptableInterface *input2);

  void SetCallback(Slot *callback);
  std::string CallCallback(int x);

  // Place signal declarations here for testing.
  // In production code, they should be palced in private section.
  typedef Signal1<std::string, const std::string &> OnLunchSignal;
  typedef Signal2<std::string, const std::string &,
                  ExtScriptable *> OnSupperSignal;
  typedef Signal2<ScriptableInterface *, const char *, int> ComplexSignal;

  class Inner {
   public:
    Inner(ExtScriptable *owner) : owner_(owner) { }
    OnLunchSignal onlunch_signal_;
    OnSupperSignal onsupper_signal_;
    ComplexSignal complex_signal_;

    std::string GetTime() const { return time_; }
    void SetTime(const std::string &time) {
      time_ = time;
      if (time == "lunch")
        signal_result_ = onlunch_signal_(std::string("Have lunch"));
      else if (time == "supper")
        signal_result_= onsupper_signal_(std::string("Have supper"), owner_);
    }

    void FireComplexSignal(const char *s, int i) {
      // Signals returning ScriptableInterface * call only be called with Emit().
      Variant params[] = { Variant(s), Variant(i) };
      ResultVariant signal_result = complex_signal_.Emit(2, params);
      complex_signal_data_.Reset(
          VariantValue<ScriptableInterface *>()(signal_result.v()));
    }

    ScriptableInterface *GetComplexSignalData() {
      return complex_signal_data_.Get();
    }

    ExtScriptable *owner_;
    std::string time_;
    ScriptableHolder<ScriptableInterface> complex_signal_data_;
    std::string signal_result_;
  };

  static Inner *GetInner(ExtScriptable *s) { return &s->inner_; }
  static const Inner *GetInnerConst(const ExtScriptable *s) {
    return &s->inner_;
  }

 protected:
  virtual void DoClassRegister();
  virtual void DoRegister();

 private:
  int array_[kArraySize];
  bool strict_;
  std::map<std::string, std::string> dynamic_properties_;
  Slot *callback_;
  Inner inner_;
  Signal0<void> dynamic_signal_;
  SignalSlot dynamic_signal_prototype_;
};

extern const Variant kNewObjectDefaultArgs[];
extern const Variant kReleaseObjectDefaultArgs[];

#endif // GGADGET_TESTS_SCRIPTABLES_H__
