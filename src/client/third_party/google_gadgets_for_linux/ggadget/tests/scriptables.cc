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

#include "ggadget/logger.h"
#include "scriptables.h"

using namespace ggadget;

// Store testing status to be checked in unit test code.
std::string g_buffer;
const char *enum_type_names[] = { "VALUE_0", "VALUE_1", "VALUE_2" };

void AppendBuffer(const char *format, ...) {
  char buffer[1024];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);
  g_buffer.append(buffer);
  printf("AppendBuffer: %s\n", buffer);
}

BaseScriptable::BaseScriptable(bool native_owned, bool register_class)
    : register_class_(register_class),
      native_owned_(native_owned),
      double_property_(0),
      int_property_(0),
      enum_property_(VALUE_0),
      variant_property_(0) {
  if (native_owned) Ref();
  g_buffer.clear();
  if (!register_class) {
    RegisterMethod("ClearBuffer",
                   NewSlot(this, &BaseScriptable::ClearBuffer));
    RegisterMethod("MethodDouble2",
                   NewSlot(this, &BaseScriptable::MethodDouble2));
    RegisterProperty("DoubleProperty",
                     NewSlot(this, &BaseScriptable::GetDoubleProperty),
                     NewSlot(this, &BaseScriptable::SetDoubleProperty));
    RegisterProperty("IntProperty",
                     NewSlot(this, &BaseScriptable::GetIntProperty),
                     NewSlot(this, &BaseScriptable::SetIntProperty));
    RegisterProperty("BufferReadOnly",
                     NewSlot(this, &BaseScriptable::GetBuffer), NULL);
    RegisterProperty("Buffer",
                     NewSlot(this, &BaseScriptable::GetBuffer),
                     NewSlot(this, &BaseScriptable::SetBuffer));
    RegisterProperty("JSON",
                     NewSlot(this, &BaseScriptable::GetJSON),
                     NewSlot(this, &BaseScriptable::SetJSON));
    // This signal is only for test, no relation to ConnectOnReferenceChange.
    RegisterSignal("my_ondelete", &my_ondelete_signal_);
    RegisterSimpleProperty("EnumSimple", &enum_property_);
    RegisterStringEnumProperty("EnumString",
                               NewSimpleGetterSlot(&enum_property_),
                               NewSimpleSetterSlot(&enum_property_),
                               enum_type_names, arraysize(enum_type_names));
    RegisterSimpleProperty("VariantProperty", &variant_property_);
    RegisterConstants(arraysize(enum_type_names), enum_type_names, NULL);
    DoRegisterConstants();
  }
}

void BaseScriptable::DoClassRegister() {
  if (register_class_) {
    RegisterMethod("ClearBuffer", NewSlot(&BaseScriptable::ClearBuffer));
    RegisterMethod("MethodDouble2",
                   NewSlot(&BaseScriptable::MethodDouble2));
    RegisterProperty("DoubleProperty",
                     NewSlot(&BaseScriptable::GetDoubleProperty),
                     NewSlot(&BaseScriptable::SetDoubleProperty));
    RegisterProperty("IntProperty",
                     NewSlot(&BaseScriptable::GetIntProperty),
                     NewSlot(&BaseScriptable::SetIntProperty));
    RegisterProperty("BufferReadOnly",
                     NewSlot(&BaseScriptable::GetBuffer), NULL);
    RegisterProperty("Buffer",
                     NewSlot(&BaseScriptable::GetBuffer),
                     NewSlot(&BaseScriptable::SetBuffer));
    RegisterProperty("JSON",
                     NewSlot(&BaseScriptable::GetJSON),
                     NewSlot(&BaseScriptable::SetJSON));
    // This signal is only for test, no relation to ConnectOnReferenceChange.
    RegisterClassSignal("my_ondelete", &BaseScriptable::my_ondelete_signal_);
    RegisterProperty("EnumSimple",
                     NewSlot(&BaseScriptable::GetEnumProperty),
                     NewSlot(&BaseScriptable::SetEnumProperty));
    RegisterStringEnumProperty("EnumString",
                               NewSlot(&BaseScriptable::GetEnumProperty),
                               NewSlot(&BaseScriptable::SetEnumProperty),
                               enum_type_names, arraysize(enum_type_names));
    RegisterProperty("VariantProperty",
                     NewSlot(&BaseScriptable::GetVariantProperty),
                     NewSlot(&BaseScriptable::SetVariantProperty));
    DoRegisterConstants();
  }
}

void BaseScriptable::DoRegisterConstants() {
  RegisterConstant("Fixed", 123456789);
  RegisterConstants(arraysize(enum_type_names), enum_type_names, NULL);

  // Register 10 integer constants.
  static char names_arr[20][20]; // Ugly...
  char *names[10];
  for (int i = 0; i < 10; i++) {
    names[i] = names_arr[i];
    sprintf(names[i], "ICONSTANT%d", i);
  }
  RegisterConstants(10, names, NULL);

  // Register 10 string constants.
  Variant const_values[10];
  for (int i = 0; i < 10; i++) {
    names[i] = names_arr[i + 10];
    sprintf(names[i], "SCONSTANT%d", i);
    const_values[i] = Variant(names[i]);
  }
  RegisterConstants(10, names, const_values);
}

BaseScriptable::~BaseScriptable() {
  LOG("BaseScriptable Destruct: this=%p", this);
  my_ondelete_signal_();
  AppendBuffer("Destruct\n");
  LOG("BaseScriptable Destruct End: this=%p", this);
  if (native_owned_) Unref(true);
  // Then ScriptableHelper::~ScriptableHelper will be called, and in turn
  // the "official" ondelete signal will be emitted.
}

Prototype *Prototype::instance_ = NULL;

const Variant kNewObjectDefaultArgs[] =
    { Variant(true), Variant(true), Variant(true) };
const Variant kReleaseObjectDefaultArgs[] =
    { Variant(static_cast<ScriptableInterface *>(NULL)) };

Prototype::Prototype() {
  RegisterMethod("PrototypeMethod", NewSlot(this, &Prototype::Method));
  RegisterProperty("PrototypeSelf", NewSlot(this, &Prototype::GetSelf),
                   NULL);
  RegisterSignal("ontest", &ontest_signal_);
  RegisterConstant("Const", 987654321);
  RegisterProperty("OverrideSelf",
                   NewSlot(this, &Prototype::GetSelf), NULL);
}

ExtScriptable::ExtScriptable(bool native_owned, bool strict,
                             bool register_class)
    : BaseScriptable(native_owned, register_class),
      strict_(strict), callback_(NULL),
      inner_(this),
      dynamic_signal_prototype_(&dynamic_signal_) {
}

void ExtScriptable::DoRegister() {
  BaseScriptable::DoRegister();
  if (!register_class_) {
    RegisterMethod("ObjectMethod", NewSlot(this, &ExtScriptable::ObjectMethod));
    RegisterSignal("onlunch", &inner_.onlunch_signal_);
    RegisterSignal("onsupper", &inner_.onsupper_signal_);
    RegisterProperty("time",
        NewSimpleGetterSlot(&inner_.time_), NewSlot(&inner_, &Inner::SetTime));
    RegisterProperty("OverrideSelf",
                     NewSlot(this, &ExtScriptable::GetSelf), NULL);
    RegisterConstant("length", kArraySize);

    RegisterMethod("NewObject",
        NewSlotWithDefaultArgs(NewSlot(this, &ExtScriptable::NewObject),
                               kNewObjectDefaultArgs));
    RegisterMethod("ReleaseObject",
        NewSlotWithDefaultArgs(NewSlot(this, &ExtScriptable::ReleaseObject),
                               kReleaseObjectDefaultArgs));
    RegisterProperty("NativeOwned",
                     NewSlot(implicit_cast<BaseScriptable *>(this),
                             &BaseScriptable::IsNativeOwned), NULL);
    RegisterMethod("ConcatArray", NewSlot(this, &ExtScriptable::ConcatArray));
    RegisterMethod("SetCallback", NewSlot(this, &ExtScriptable::SetCallback));
    RegisterMethod("CallCallback", NewSlot(this, &ExtScriptable::CallCallback));
    RegisterSignal("oncomplex", &inner_.complex_signal_);
    RegisterProperty("ComplexSignalData",
                     NewSlot(&inner_, &Inner::GetComplexSignalData), NULL);
    RegisterMethod("FireComplexSignal",
                   NewSlot(&inner_, &Inner::FireComplexSignal));
    RegisterMethod("", NewSlot(this, &ExtScriptable::ObjectMethod));
  }

  // The following are always object-based.
  RegisterReadonlySimpleProperty("SignalResult", &inner_.signal_result_);
  SetInheritsFrom(Prototype::GetInstance());
  SetArrayHandler(NewSlot(this, &ExtScriptable::GetArray),
                  NewSlot(this, &ExtScriptable::SetArray));
  SetDynamicPropertyHandler(
      NewSlot(this, &ExtScriptable::GetDynamicProperty),
      NewSlot(this, &ExtScriptable::SetDynamicProperty));
}

void ExtScriptable::DoClassRegister() {
  BaseScriptable::DoClassRegister();
  if (register_class_) {
    RegisterMethod("ObjectMethod", NewSlot(&ExtScriptable::ObjectMethod));
    RegisterClassSignal("onlunch", &Inner::onlunch_signal_, GetInner);
    RegisterClassSignal("onsupper", &Inner::onsupper_signal_, GetInner);
    RegisterProperty("time",
                     NewSlot(&Inner::GetTime, GetInnerConst),
                     NewSlot(&Inner::SetTime, GetInner));
    RegisterProperty("OverrideSelf",
                     NewSlot(&ExtScriptable::GetSelf), NULL);
    RegisterConstant("length", kArraySize);

    RegisterMethod("NewObject",
        NewSlotWithDefaultArgs(NewSlot(&ExtScriptable::NewObject),
                               kNewObjectDefaultArgs));
    RegisterMethod("ReleaseObject",
        NewSlotWithDefaultArgs(NewSlot(&ExtScriptable::ReleaseObject),
                               kReleaseObjectDefaultArgs));
    RegisterProperty("NativeOwned",
                     NewSlot(&BaseScriptable::IsNativeOwned), NULL);
    RegisterMethod("ConcatArray", NewSlot(&ExtScriptable::ConcatArray));
    RegisterMethod("SetCallback", NewSlot(&ExtScriptable::SetCallback));
    RegisterMethod("CallCallback", NewSlot(&ExtScriptable::CallCallback));
    RegisterClassSignal("oncomplex", &Inner::complex_signal_, GetInner);
    RegisterProperty("ComplexSignalData",
                     NewSlot(&Inner::GetComplexSignalData, GetInner), NULL);
    RegisterMethod("FireComplexSignal",
                   NewSlot(&Inner::FireComplexSignal, GetInner));
    RegisterMethod("", NewSlot(&ExtScriptable::ObjectMethod));
    RegisterMethod("FireDynamicSignal",
                   NewSlot(&ExtScriptable::FireDynamicSignal));
  }
}

ExtScriptable::~ExtScriptable() {
  delete callback_;
  LOG("ExtScriptable Destruct: this=%p", this);
}

ScriptableArray *ExtScriptable::ConcatArray(ScriptableInterface *array1,
                                            ScriptableInterface *array2) {
  if (array1 == NULL || array2 == NULL)
    return NULL;
  int count1 = VariantValue<int>()(array1->GetProperty("length").v());
  int count2 = VariantValue<int>()(array2->GetProperty("length").v());

  ScriptableArray *array = new ScriptableArray();
  for (int i = 0; i < count1; i++)
    array->Append(array1->GetPropertyByIndex(i).v());
  for (int i = 0; i < count2; i++)
    array->Append(array2->GetPropertyByIndex(i).v());
  return array;
}

void ExtScriptable::SetCallback(Slot *callback) {
  delete callback_;
  callback_ = callback;
}

std::string ExtScriptable::CallCallback(int x) {
  if (callback_) {
    Variant vx(x);
    return callback_->Call(NULL, 1, &vx).v().Print();
  }
  return "NO CALLBACK";
}
