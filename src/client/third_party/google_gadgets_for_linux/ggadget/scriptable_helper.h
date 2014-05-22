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

#ifndef GGADGET_SCRIPTABLE_HELPER_H__
#define GGADGET_SCRIPTABLE_HELPER_H__

#include <ggadget/common.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>

namespace ggadget {

namespace internal {

class ScriptableHelperImplInterface : public ScriptableInterface,
                                      public RegisterableInterface {
 public:
  virtual ~ScriptableHelperImplInterface() { }
  virtual void RegisterClassSignal(const char *name,
                                   ClassSignal *class_signal) = 0;
  virtual void SetInheritsFrom(ScriptableInterface *inherits_from) = 0;
  virtual void SetArrayHandler(Slot *getter, Slot *setter) = 0;
  virtual void SetDynamicPropertyHandler(Slot *getter, Slot *setter) = 0;
  virtual void SetPendingException(ScriptableInterface *exception) = 0;
  virtual bool RemoveProperty(const char *name) = 0;
};

/**
 * The callback interface used by the @c ScriptableHelperImplInterface
 * implementation to callback to its owner (@c ScriptableHelper object).
 */
class ScriptableHelperCallbackInterface {
 public:
  virtual ~ScriptableHelperCallbackInterface() { }
  virtual void DoRegister() = 0;
  virtual void DoClassRegister() = 0;
  virtual ScriptableInterface *GetScriptable() = 0;
};

ScriptableHelperImplInterface *NewScriptableHelperImpl(
    ScriptableHelperCallbackInterface *owner);

} // namespace internal

/**
 * @ingroup ScriptableFoundation
 * @{
 */

/**
 * A @c ScriptableInterface implementation helper.
 */
template <typename I>
class ScriptableHelper : public I,
                         public RegisterableInterface,
                         public internal::ScriptableHelperCallbackInterface {
 private:
  // Checks at compile time if the argument I is ScriptableInterface or
  // derived from it.
  COMPILE_ASSERT((IsDerived<ScriptableInterface, I>::value),
                 I_must_be_ScriptableInterface_or_derived_from_it);

 public:
  ScriptableHelper()
      : impl_(internal::NewScriptableHelperImpl(this)) {
  }

  virtual ~ScriptableHelper() {
    delete impl_;
  }

  /** @see RegisterableInterface::RegisterProperty() */
  virtual void RegisterProperty(const char *name, Slot *getter, Slot *setter) {
    impl_->RegisterProperty(name, getter, setter);
  }

  /**
   * Register a simple scriptable property that maps to a variable.
   * @param name property name.  It must point to static allocated memory.
   * @param valuep point to a value.
   */
  template <typename T>
  void RegisterSimpleProperty(const char *name, T *valuep) {
    impl_->RegisterProperty(name,
                            NewSimpleGetterSlot<T>(valuep),
                            NewSimpleSetterSlot<T>(valuep));
  }

  /**
   * Register a simple readonly scriptable property that maps to a variable.
   * @param name property name.  It must point to static allocated memory.
   * @param valuep point to a value.
   */
  template <typename T>
  void RegisterReadonlySimpleProperty(const char *name, const T *valuep) {
    impl_->RegisterProperty(name, NewSimpleGetterSlot<T>(valuep), NULL);
  }

  /** @see RegisterableInterface::RegisterStringEnumProperty() */
  virtual void RegisterStringEnumProperty(const char *name,
                                          Slot *getter, Slot *setter,
                                          const char **names, int count) {
    impl_->RegisterStringEnumProperty(name, getter, setter, names, count);
  }

  /** @see RegisterableInterface::RegisterMethod() */
  virtual void RegisterMethod(const char *name, Slot *slot) {
    impl_->RegisterMethod(name, slot);
  }

  /** @see RegisterableInterface::RegisterSignal() */
  virtual void RegisterSignal(const char *name, Signal *signal) {
    impl_->RegisterSignal(name, signal);
  }

  /** @see RegisterableInterface::RegisterVariantConstant() */
  virtual void RegisterVariantConstant(const char *name,
                                       const Variant &value) {
    impl_->RegisterVariantConstant(name, value);
  }

  /**
   * Register a set of constants.
   * @param count number of constants to register.
   * @param names array of names of the constants.  The pointers must point to
   *     static allocated memory.
   * @param values array of constant values.  If it is @c NULL, the values
   *     will be automatically assigned from @c 0 to count-1, which is useful
   *     to define enum values.
   */
  void RegisterConstants(size_t count,
                         const char *const names[],
                         const Variant values[]) {
    ASSERT(names);
    for (size_t i = 0; i < count; i++)
      impl_->RegisterVariantConstant(
          names[i],
          values ? values[i] : Variant(static_cast<int64_t>(i)));
  }

  /**
   * Register a constant.
   * @param name the constant name. The pointers must point to static
   *     allocated memory.
   * @param value the constant value.
   */
  template <typename T>
  void RegisterConstant(const char *name, T value) {
    impl_->RegisterVariantConstant(name, Variant(value));
  }

  /**
   * Register a single class signal.
   * @param name property name.  It must point to static allocated memory.
   * @param signal the signal to register.
   */
  template <typename T, typename S>
  void RegisterClassSignal(const char *name, S T::*signal) {
    COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                   T_must_be_ScriptableInterface_or_derived_from_it);
    COMPILE_ASSERT((IsDerived<Signal, S>::value),
                   S_must_be_derived_from_Signal);
    impl_->RegisterClassSignal(name, NewClassSignal(signal));
  }

  /**
   * Register a single class signal which will be delegated from T to DT,
   * using a delegate getter function.
   */
  template <typename T, typename DT, typename S>
  void RegisterClassSignal(const char *name, S DT::*signal,
                           DT *(*delegate_getter)(T *)) {
    COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                   T_must_be_ScriptableInterface_or_derived_from_it);
    COMPILE_ASSERT((IsDerived<Signal, S>::value),
                   S_must_be_derived_from_Signal);
    impl_->RegisterClassSignal(name, NewClassSignal(signal, delegate_getter));
  }

  /**
   * Register a single class signal which will be delegated from T to DT,
   * using the delegated field pointer.
   */
  template <typename T, typename DT, typename S>
  void RegisterClassSignal(const char *name, S DT::*signal,
                           DT *T::*delegate_field) {
    COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                   T_must_be_ScriptableInterface_or_derived_from_it);
    COMPILE_ASSERT((IsDerived<Signal, S>::value),
                   S_must_be_derived_from_Signal);
    impl_->RegisterClassSignal(name, NewClassSignal(signal, delegate_field));
  }

  /**
   * Set a object from which this object inherits common properties (including
   * methods and signals).
   * Any operations to properties not registered in current
   * @c ScriptableHelper object are delegated to @a inherits_from.
   * One @a inherits_from object can be shared among multiple
   * <code>ScriptableHelper</code>s.
   */
  void SetInheritsFrom(ScriptableInterface *inherits_from) {
    impl_->SetInheritsFrom(inherits_from);
  }

  /**
   * Set the array handler which will handle array accesses.
   * @param getter handles 'get' accesses.  It accepts an int parameter as the
   *     array index and return the result of any type that can be contained
   *     in a @c Variant.  It should return a @c Variant of type
   *     @c Variant::TYPE_VOID if it doesn't support the property.
   * @param setter handles 'set' accesses.  It accepts an int parameter as the
   *     array index and a bool value indicating if property setting is
   *     successful.
   */
  void SetArrayHandler(Slot *getter, Slot *setter) {
    impl_->SetArrayHandler(getter, setter);
  }

  /**
   * Set the dynamic property handler which will handle property accesses not
   * registered statically.
   *
   * @param getter handles 'get' accesses.  It accepts a property name
   *     parameter (<code>const char *</code>) and return the result of any
   *     type that can be contained in a @c Variant.
   * @param setter handles 'set' accesses.  It accepts a property name
   *     parameter (<code>const char *</code>) and a bool value indicating
   *     if property setting is successful.
   */
  void SetDynamicPropertyHandler(Slot *getter, Slot *setter) {
    impl_->SetDynamicPropertyHandler(getter, setter);
  }

  /**
   * Sets the exception to be raised to the script engine.
   * There must be no pending exception when this method is called to prevent
   * memory leaks.
   * The specified exception object must not be strict, otherwise some script
   * runtimes may not be able to handle it (such as webkit-script-runtime).
   */
  void SetPendingException(ScriptableInterface *exception) {
    impl_->SetPendingException(exception);
  }

  /**
   * Removes a property previously registered to the scriptable object. Any
   * kinds of object properties can be removed, such as, properties, signals,
   * methods, constants, etc. But class properties cannot be removed.
   * @param name The name of the property to be removed.
   * @return true if a property is removed.
   */
  bool RemoveProperty(const char *name) {
    return impl_->RemoveProperty(name);
  }

  /**
   * Used to register a setter that does nothing. There is no DummyGetter()
   * because a NULL getter acts like it.
   */
  static void DummySetter(const Variant &) { }

  /**
   * @see ScriptableInterface::Ref()
   * Normally this method is not allowed to be overriden.
   */
  virtual void Ref() const {
    impl_->Ref();
  }

  /**
   * @see ScriptableInterface::Unref()
   * Normally this method is not allowed to be overriden.
   */
  virtual void Unref(bool transient = false) const {
    impl_->Unref(transient);
    if (!transient && impl_->GetRefCount() == 0) delete this;
  }

  /**
   * @see ScriptableInterface::GetRefCount()
   * Normally this method is not allowed to be overriden.
   */
  virtual int GetRefCount() const { return impl_->GetRefCount(); }

  /**
   * Default strict policy.
   * @see ScriptableInterface::IsStrict()
   */
  virtual bool IsStrict() const { return true; }

  /**
   * By default, scriptable object can not be enumerated by script.
   * @see ScriptableInterface::IsEnumeratable()
   */
  virtual bool IsEnumeratable() const { return false; }

  /** @see ScriptableInterface::ConnectionOnReferenceChange() */
  virtual Connection *ConnectOnReferenceChange(Slot2<void, int, int> *slot) {
    return impl_->ConnectOnReferenceChange(slot);
  }

  /** @see ScriptableInterface::GetPropertyInfo() */
  virtual ScriptableInterface::PropertyType GetPropertyInfo(
      const char *name, Variant *prototype) {
    return impl_->GetPropertyInfo(name, prototype);
  }

  /** @see ScriptableInterface::GetProperty() */
  virtual ResultVariant GetProperty(const char *name) {
    return impl_->GetProperty(name);
  }

  /** @see ScriptableInterface::SetProperty() */
  virtual bool SetProperty(const char *name, const Variant &value) {
    return impl_->SetProperty(name, value);
  }

  /** @see ScriptableInterface::GetPropertyByIndex() */
  virtual ResultVariant GetPropertyByIndex(int index) {
    return impl_->GetPropertyByIndex(index);
  }

  /** @see ScriptableInterface::SetPropertyByIndex() */
  virtual bool SetPropertyByIndex(int index, const Variant &value) {
    return impl_->SetPropertyByIndex(index, value);
  }

  /** @see ScriptableInterface::GetPendingException() */
  virtual ScriptableInterface *GetPendingException(bool clear) {
    return impl_->GetPendingException(clear);
  }

  /** @see ScriptableInterface::EnumerateProperties() */
  virtual bool EnumerateProperties(
      ScriptableInterface::EnumeratePropertiesCallback *callback) {
    return impl_->EnumerateProperties(callback);
  }

  /** @see ScriptableInterface::EnumerateElements() */
  virtual bool EnumerateElements(
      ScriptableInterface::EnumerateElementsCallback *callback) {
    return impl_->EnumerateElements(callback);
  }

  /** @see ScriptableInterface::GetRegisterable() */
  virtual RegisterableInterface *GetRegisterable() { return impl_; }

  // Declare it here to make it accessible from this template.
  virtual uint64_t GetClassId() const = 0;

 protected:
  /**
   * The subclass overrides this method to register its scriptable properties
   * (including methods and signals).
   * A subclass can select from two methods to register scriptable properties:
   *   - Registering properties in its constructor;
   *   - Registering properties in this method.
   * If an instance of a class is not used in script immediately after creation,
   * the class should use the latter method to reduce overhead on creation.
   */
  virtual void DoRegister() { }

  /**
   * The subclass overrides this method to register class-wide scriptable
   * properties.
   */
  virtual void DoClassRegister() { }

 private:
  virtual ScriptableInterface *GetScriptable() { return this; }
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableHelper);
  internal::ScriptableHelperImplInterface *impl_;
};

typedef ScriptableHelper<ScriptableInterface> ScriptableHelperDefault;

/**
 * For objects that are owned by the native side. The native side can neglect
 * the reference counting things and just use the objects as normal C++
 * objects. For example, define the objects as local variables or data members,
 * as well as pointers.
 */
template <typename I>
class ScriptableHelperNativeOwned : public ScriptableHelper<I> {
 public:
  ScriptableHelperNativeOwned() {
    ScriptableHelper<I>::Ref();
  }
  virtual ~ScriptableHelperNativeOwned() {
    ScriptableHelper<I>::Unref(true);
  }
};

typedef ScriptableHelperNativeOwned<ScriptableInterface>
    ScriptableHelperNativeOwnedDefault;

/**
 * A handy class that can be used as simple native owned scriptable object.
 */
template <uint64_t ClassId>
class NativeOwnedScriptable : public ScriptableHelperNativeOwnedDefault {
 public:
  static const uint64_t CLASS_ID = ClassId;
  virtual bool IsInstanceOf(uint64_t class_id) const {
    return class_id == ClassId || ScriptableInterface::IsInstanceOf(class_id);
  }
  virtual uint64_t GetClassId() const { return ClassId; }
};

/**
 * A handy class that can be used as simple shared scriptable object.
 */
template <uint64_t ClassId>
class SharedScriptable : public ScriptableHelperDefault {
 public:
  static const uint64_t CLASS_ID = ClassId;
  virtual bool IsInstanceOf(uint64_t class_id) const {
    return class_id == ClassId || ScriptableInterface::IsInstanceOf(class_id);
  }
  virtual uint64_t GetClassId() const { return ClassId; }
};

/** @} */

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_HELPER_H__
