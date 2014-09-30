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

#ifndef GGADGET_SLOT_H__
#define GGADGET_SLOT_H__

#include <cstring>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/small_object.h>
#include <ggadget/variant.h>

namespace ggadget {

/**
 * @ingroup SignalSlot
 * @{
 */

/**
 * A @c Slot is a calling target.
 * The real targets are implemented in subclasses.
 * The instances are immutable, because all methods are @c const.
 */
class Slot : public SmallObject<> {
 public:
  virtual ~Slot() { }

  /**
   * Call the <code>Slot</code>'s target.
   * The type of arguments and the return value must be compatible with the
   * actual calling target.
   * @see Variant
   * @param object the object associated with this invocation. It can be
   *     @c NULL if this invocation needs no object.
   * @param argc number of arguments.
   * @param argv argument array.  Can be @c NULL if <code>argc==0</code>.
   * @return the return value of the @c Slot target. Use @c ResultVariant
   *     instead @c Variant as the result type to ensure scriptable object
   *     to be deleted if the result won't be handled by the caller.
   */
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const = 0;

  /**
   * @return @c true if this @c Slot can provide metadata. Otherwise this Slot
   *     is supposed to accept any number of any type of parameters.
   */
  virtual bool HasMetadata() const { return true; }

  /**
   * Get return type of the @c Slot target.
   * This method is meaningless if HasMetadata() returns false.
   */
  virtual Variant::Type GetReturnType() const { return Variant::TYPE_VOID; }
  /**
   * Get the number of arguments of the @c Slot target.
   * This method is meaningless if HasMetadata() returns false.
   * Returns INT_MAX if this Slot can accept any number of parameters.
   * @see GetArgTypes() for more details.
   */
  virtual int GetArgCount() const { return 0; }
  /**
   * Get the type list of the arguments of the @c Slot target.
   * This method is meaningless if HasMetadata() returns false.
   * If GetArgCount() returns INT_MAX, then GetArgTypes() shall return
   * a an array terminated by TYPE_VOID, or just NULL if the arguments can have
   * any type.
   */
  virtual const Variant::Type *GetArgTypes() const { return NULL; }

  /**
   * Get the default values of arguments.
   * This method is meaningless if HasMetadata() returns false.
   * This method only has default implementation in all classed declaredd in
   * this header file. A costomized @c Slot class must be defined if you need
   * to provide default value policies.
   * Default arguments won't be automatically filled in @c Call(). The caller
   * must fill them before invoking @c Call().
   *
   * @return @c NULL if this slot has no default values for arguments.
   *     Otherwise returns an array of <code>Variant</code>s.  The count of
   *     items must be equal to the result of @c GetArgCount().
   *     If the type of an item is @c Variant::TYPE_VOID, the argument is
   *     required and has no default value; otherwise the item contains the
   *     default value of this argument if this argument is omitted.
   */
  virtual const Variant *GetDefaultArgs() const { return NULL; }

  /**
   * Equality tester, only for unit testing.
   * The slots to be tested must be of the same type, otherwise the program
   * may crash.
   */
  virtual bool operator==(const Slot &another) const = 0;

 protected:
  Slot() { }
};

/**
 * A @c Slot with no parameter.
 */
template <typename R>
class Slot0 : public Slot {
 public:
  R operator()() const {
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,
             ("Use Call() when the slot returns ScriptableInterface *"));
    return VariantValue<R>()(Call(NULL, 0, NULL).v());
  }
  virtual Variant::Type GetReturnType() const { return VariantType<R>::type; }
};

/**
 * Partial specialized @c Slot0 that returns @c void.
 */
template <>
class Slot0<void> : public Slot {
 public:
  void operator()() const { Call(NULL, 0, NULL); }
};

/**
 * A prototype slot is a slot used to represent a invocation prototype.
 */
template <typename R>
class PrototypeSlot0 : public Slot {
 public:
  typedef PrototypeSlot0<R> SelfType;
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(false);
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    return down_cast<const SelfType *>(&another);
  }
};

/**
 * A @c Slot that is targeted to a functor with no parameter.
 */
template <typename R, typename F>
class FunctorSlot0 : public Slot0<R> {
 public:
  typedef FunctorSlot0<R, F> SelfType;
  FunctorSlot0(F functor) : functor_(functor) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    return ResultVariant(Variant(functor_()));
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && functor_ == a->functor_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlot0);
  F functor_;
};

/**
 * Partial specialized @c FunctorSlot0 that returns @c void.
 */
template <typename F>
class FunctorSlot0<void, F> : public Slot0<void> {
 public:
  typedef FunctorSlot0<void, F> SelfType;
  FunctorSlot0(F functor) : functor_(functor) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    functor_();
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && functor_ == a->functor_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlot0);
  F functor_;
};

/**
 * A @c Slot that is targeted to a C++ non-static method of an object
 * with no parameter.
 */
template <typename R, typename T, typename M>
class MethodSlot0 : public Slot0<R> {
 public:
  typedef MethodSlot0<R, T, M> SelfType;
  MethodSlot0(T* object, M method) : object_(object), method_(method) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    // object parameter is ignored because object is bound when this object
    // is constructed.
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    return ResultVariant(Variant((object_->*method_)()));
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && object_ == a->object_ && method_ == a->method_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlot0);
  T *object_;
  M method_;
};

/**
 * Partial specialized @c MethodSlot0 that returns @c void.
 */
template <typename T, typename M>
class MethodSlot0<void, T, M> : public Slot0<void> {
 public:
  typedef MethodSlot0<void, T, M> SelfType;
  MethodSlot0(T* object, M method) : object_(object), method_(method) { }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    // object parameter is ignored because object is bound when this object
    // is constructed.
    GGL_UNUSED(object);
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    (object_->*method_)();
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && object_ == a->object_ && method_ == a->method_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlot0);
  T *object_;
  M method_;
};

/**
 * A @c Slot that is targeted to a C++ non-static method. Object is not bound
 * when this slot is constructed. An non-NULL object parameter must be given
 * when @c Call() is called.
 */
template <typename R, typename T, typename M>
class UnboundMethodSlot0 : public Slot0<R> {
 public:
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
  typedef UnboundMethodSlot0<R, T, M> SelfType;
  UnboundMethodSlot0(M method) : method_(method) { }
  using Slot0<R>::GetReturnType;
  R operator()(T *object) const {
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,
             ("Use Call() when the slot returns ScriptableInterface *"));
    return VariantValue<R>()(Call(object, 0, NULL).v());
  }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    ASSERT(object);
    return ResultVariant(Variant((down_cast<T *>(object)->*method_)()));
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && method_ == a->method_;
  }
 private:
  R operator()() const; // Hide this method.
  DISALLOW_EVIL_CONSTRUCTORS(UnboundMethodSlot0);
  M method_;
};

/**
 * Partial specialized @c UnboundMethodSlot0 that returns @c void.
 */
template <typename T, typename M>
class UnboundMethodSlot0<void, T, M> : public Slot0<void> {
 public:
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
  typedef UnboundMethodSlot0<void, T, M> SelfType;
  UnboundMethodSlot0(M method) : method_(method) { }
  void operator()(T *object) const { Call(object, 0, NULL); }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    ASSERT(object);
    (down_cast<T *>(object)->*method_)();
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && method_ == a->method_;
  }
 private:
  void operator()() const; // Hide this method.
  DISALLOW_EVIL_CONSTRUCTORS(UnboundMethodSlot0);
  M method_;
};

/**
 * Similar to @c UnboundMethodSlot0 except that the calls will be delegated
 * to another object provided by the @c DelegateGetter.
 */
template <typename R, typename T, typename M, typename DelegateGetter>
class DelegatedMethodSlot0 : public Slot0<R> {
 public:
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
  typedef DelegatedMethodSlot0<R, T, M, DelegateGetter> SelfType;
  DelegatedMethodSlot0(M method, DelegateGetter delegate_getter)
      : method_(method), delegate_getter_(delegate_getter) { }
  using Slot0<R>::GetReturnType;
  R operator()(T *object) const {
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,
             ("Use Call() when the slot returns ScriptableInterface *"));
    return VariantValue<R>()(Call(delegate_getter_(object), 0, NULL).v());
  }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    ASSERT(object && delegate_getter_(down_cast<T *>(object)));
    return ResultVariant(Variant(
        (delegate_getter_(down_cast<T *>(object))->*method_)()));
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && method_ == a->method_ &&
           delegate_getter_ == a->delegate_getter_;
  }
 private:
  R operator()() const; // Hide this method.
  DISALLOW_EVIL_CONSTRUCTORS(DelegatedMethodSlot0);
  M method_;
  DelegateGetter delegate_getter_;
};

/**
 * Partial specialized @c UnboundMethodSlot0 that returns @c void.
 */
template <typename T, typename M, typename DelegateGetter>
class DelegatedMethodSlot0<void, T, M, DelegateGetter> : public Slot0<void> {
 public:
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
  typedef DelegatedMethodSlot0<void, T, M, DelegateGetter> SelfType;
  DelegatedMethodSlot0(M method, DelegateGetter delegate_getter)
      : method_(method), delegate_getter_(delegate_getter) { }
  void operator()(T *object) const { Call(object, 0, NULL); }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    ASSERT(object && delegate_getter_(down_cast<T *>(object)));
    (delegate_getter_(down_cast<T *>(object))->*method_)();
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && method_ == a->method_ &&
           delegate_getter_ == a->delegate_getter_;
  }
 private:
  void operator()() const; // Hide this method.
  DISALLOW_EVIL_CONSTRUCTORS(DelegatedMethodSlot0);
  M method_;
  DelegateGetter delegate_getter_;
};

/**
 * A @c Slot that is targeted to another general typed slot with no parameter.
 * Useful to proxy not-typed script slot to typed slot.
 *
 * Note about the return type: the general slot may return value of any type
 * any type (for example, a script function), so there are two potential
 * problems:
 *   - If the target return type is void, but there is actual returned value,
 *     the value will be ignored. If the value contains allocated resources
 *     (such as Slot), the callee must take care of the resources to prevent
 *     leaks.
 *   - If the actual returned value is not of type the same as expected,
 *     runtime assetion failure will occurs. The caller/callee must avoid such
 *     situations.
 */
template <typename R>
class SlotProxy0 : public Slot0<R> {
 public:
  typedef SlotProxy0<R> SelfType;
  SlotProxy0(Slot* slot) : slot_(slot) { ASSERT(slot); }
  ~SlotProxy0() { delete slot_; slot_ = NULL; }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    ASSERT(argc == 0);
    return slot_->Call(object, argc, argv);
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && *slot_ == *a->slot_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(SlotProxy0);
  Slot *slot_;
};

/**
 * A special functor slot that converts a functor taking one argument into a
 * slot taking no argument.
 */
template <typename R, typename F, typename PA>
class FunctorSlotClosure0 : public Slot0<R> {
 public:
  typedef FunctorSlotClosure0<R, F, PA> SelfType;
  FunctorSlotClosure0(F functor, PA pa) : functor_(functor), pa_(pa) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    return ResultVariant(Variant(functor_(pa_)));
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && functor_ == a->functor_ && pa_ == a->pa_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlotClosure0);
  F functor_;
  PA pa_;
};

/**
 * Partial specialized @c FunctorSlotClosure0 that returns @c void.
 */
template <typename F, typename PA>
class FunctorSlotClosure0<void, F, PA> : public Slot0<void> {
 public:
  typedef FunctorSlotClosure0<void, F, PA> SelfType;
  FunctorSlotClosure0(F functor, PA pa) : functor_(functor), pa_(pa) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    functor_(pa_);
    return ResultVariant(Variant());
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && functor_ == a->functor_ && pa_ == a->pa_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlotClosure0);
  F functor_;
  PA pa_;
};

/**
 * A special method slot that converts an object method taking one argument
 * into a slot taking no argument.
 */
template <typename R, typename T, typename M, typename PA>
class MethodSlotClosure0 : public Slot0<R> {
 public:
  typedef MethodSlotClosure0<R, T, M, PA> SelfType;
  MethodSlotClosure0(T *obj, M method, PA pa)
    : obj_(obj), method_(method), pa_(pa) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    return ResultVariant(Variant((obj_->*method_)(pa_)));
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && obj_ == a->obj_ && method_ == a->method_ && pa_ == a->pa_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlotClosure0);
  T *obj_;
  M method_;
  PA pa_;
};

/**
 * Partial specialized @c MethodSlotClosure0 that returns @c void.
 */
template <typename T, typename M, typename PA>
class MethodSlotClosure0<void, T, M, PA> : public Slot0<void> {
 public:
  typedef MethodSlotClosure0<void, T, M, PA> SelfType;
  MethodSlotClosure0(T *obj, M method, PA pa)
    : obj_(obj), method_(method), pa_(pa) { }
  virtual ResultVariant Call(ScriptableInterface *,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    (obj_->*method_)(pa_);
    return ResultVariant(Variant());
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && obj_ == a->obj_ && method_ == a->method_ && pa_ == a->pa_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlotClosure0);
  T *obj_;
  M method_;
  PA pa_;
};

/**
 * A special slot proxy that converts a slot taking one argument into a slot
 * taking no argument.
 */
template <typename R, typename PA>
class SlotProxyClosure0 : public Slot0<R> {
 public:
  typedef SlotProxyClosure0<R, PA> SelfType;
  SlotProxyClosure0(Slot* slot, PA pa) : slot_(slot), pa_(pa) { }
  ~SlotProxyClosure0() { delete slot_; slot_ = NULL; }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    ASSERT(argc == 0);
    Variant vargs[1];
    vargs[1] = Variant(pa_);
    return slot_->Call(object, 1, vargs);
  }
  virtual bool operator==(const Slot &another) const {
    const SelfType *a = down_cast<const SelfType *>(&another);
    return a && *slot_ == *a->slot_ && pa_ == a->pa_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(SlotProxyClosure0);
  Slot *slot_;
  PA pa_;
};

/**
 * Helper functor to create a @c FunctorSlot0 instance with a C/C++ function
 * or a static method.
 * The caller should delete the instance after use.
 */
template <typename R>
inline Slot0<R> *NewSlot(R (*functor)()) {
  return new FunctorSlot0<R, R (*)()>(functor);
}

template <typename R, typename PA>
inline Slot0<R> *NewSlot(R (*functor)(PA), PA pa) {
  return new FunctorSlotClosure0<R, R (*)(PA), PA>(functor, pa);
}

template <typename R, typename PA>
inline Slot0<R> *NewSlot(R (*functor)(const PA&), const PA &pa) {
  return new FunctorSlotClosure0<R, R (*)(const PA&), PA>(functor, pa);
}

/**
 * Helper functor to create a @c MethodSlot0 instance.
 * The type of method must be of
 * <code>R (T::*)()</code> or <code>R (T::*)() const</code>.
 * The caller should delete the instance after use.
 */
template <typename R, typename T>
inline Slot0<R> *NewSlot(T *object, R (T::*method)()) {
  return new MethodSlot0<R, T, R (T::*)()>(object, method);
}
template <typename R, typename T>
inline Slot0<R> *NewSlot(const T *object, R (T::*method)() const) {
  return new MethodSlot0<R, const T, R (T::*)() const>(object, method);
}

template <typename R, typename T>
inline Slot0<R> *NewSlot(R (T::*method)()) {
  return new UnboundMethodSlot0<R, T, R (T::*)()>(method);
}
template <typename R, typename T>
inline Slot0<R> *NewSlot(R (T::*method)() const) {
  return new UnboundMethodSlot0<R, const T, R (T::*)() const>(method);
}

template <typename R, typename T, typename DT>
inline Slot0<R> *NewSlot(R (DT::*method)(), DT *(*delegate_getter)(T *)) {
  return new DelegatedMethodSlot0<R, T, R (DT::*)(), DT *(*)(T *)>(
      method, delegate_getter);
}
template <typename R, typename T, typename DT>
inline Slot0<R> *NewSlot(R (DT::*method)() const,
                         const DT *(*delegate_getter)(T *)) {
  return new DelegatedMethodSlot0<R, T, R (DT::*)() const, const DT *(*)(T *)>(
      method, delegate_getter);
}

template <typename T, typename DT>
struct FieldDelegateGetter {
  FieldDelegateGetter(DT *T::*field) : field_(field) { }
  DT *operator()(T *object) const { return object->*field_; }
  bool operator==(const FieldDelegateGetter &another) const {
    return another.field_ == field_;
  }
  DT *T::*field_;
};
template <typename R, typename T, typename DT>
inline Slot0<R> *NewSlot(R (DT::*method)(), DT *T::*delegate_field) {
  return new DelegatedMethodSlot0<R, T, R (DT::*)(),
                                  FieldDelegateGetter<T, DT> >(
      method, FieldDelegateGetter<T, DT>(delegate_field));
}
template <typename R, typename T, typename DT>
inline Slot0<R> *NewSlot(R (DT::*method)() const,
                         DT *T::*delegate_field) {
  return new DelegatedMethodSlot0<R, T, R (DT::*)() const,
                                  FieldDelegateGetter<T, DT> >(
      method, FieldDelegateGetter<T, DT>(delegate_field));
}
template <typename R, typename T, typename DT>
inline Slot0<R> *NewSlot(R (DT::*method)() const,
                         const DT *T::*delegate_field) {
  return new DelegatedMethodSlot0<R, T, R (DT::*)() const,
                                  FieldDelegateGetter<T, const DT> >(
      method, FieldDelegateGetter<T, const DT>(delegate_field));
}

template <typename R, typename T, typename PA>
inline Slot0<R> *NewSlot(T *object, R (T::*method)(PA), PA pa) {
  return new MethodSlotClosure0<R, T, R (T::*)(PA), PA>(object, method, pa);
}
template <typename R, typename T, typename PA>
inline Slot0<R> *NewSlot(const T *object, R (T::*method)(PA) const, PA pa) {
  return new MethodSlotClosure0<R, const T,
                                R (T::*)(PA) const, PA>(object, method, pa);
}

template <typename R, typename T, typename PA>
inline Slot0<R> *NewSlot(T *object, R (T::*method)(const PA&), const PA &pa) {
  return new MethodSlotClosure0<R, T, R (T::*)(const PA&), PA>(
      object, method, pa);
}
template <typename R, typename T, typename PA>
inline Slot0<R> *NewSlot(
    const T *object, R (T::*method)(const PA&) const, const PA &pa) {
  return new MethodSlotClosure0<R, const T, R (T::*)(const PA&) const, PA>(
      object, method, pa);
}

/**
 * Helper functor to create a @c FunctorSlot0 instance with a functor object.
 * The caller should delete the instance after use.
 * Place <code>typename F</code> at the end of the template parameter to
 * let the compiler populate the default type.
 *
 * Usage: <code>NewFunctorSlot<int>(AFunctor());</code>
 */
template <typename R, typename F>
inline Slot0<R> *NewFunctorSlot(F functor) {
  return new FunctorSlot0<R, F>(functor);
}

template <typename R, typename F, typename PA>
inline Slot0<R> *NewFunctorSlot(F functor, PA pa) {
  return new FunctorSlotClosure0<R, F, PA>(functor, pa);
}

/**
 * <code>Slot</code>s with 1 or more parameters are defined by this macro.
 */
#define DEFINE_SLOT(n, _arg_types, _arg_type_names, _args, _init_args,        \
                    _init_arg_types, _call_args)                              \
template <_arg_types>                                                         \
inline const Variant::Type *ArgTypesHelper() {                                \
  static Variant::Type arg_types[] = { _init_arg_types };                     \
  return arg_types;                                                           \
}                                                                             \
                                                                              \
template <typename R, _arg_types>                                             \
class Slot##n : public Slot {                                                 \
 public:                                                                      \
  R operator()(_args) const {                                                 \
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,                     \
             ("Use Call() when the slot returns ScriptableInterface *"));     \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    return VariantValue<R>()(Call(NULL, n, vargs).v());                       \
  }                                                                           \
  virtual Variant::Type GetReturnType() const { return VariantType<R>::type; }\
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
};                                                                            \
                                                                              \
template <_arg_types>                                                         \
class Slot##n<void, _arg_type_names> : public Slot {                          \
 public:                                                                      \
  void operator()(_args) const {                                              \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    Call(NULL, n, vargs);                                                     \
  }                                                                           \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
class PrototypeSlot##n : public Slot##n<R, _arg_type_names> {                 \
 public:                                                                      \
  typedef PrototypeSlot##n<R, _arg_type_names> SelfType;                      \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(false); return ResultVariant();                                    \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    return down_cast<const SelfType *>(&another);                             \
  }                                                                           \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename F>                                 \
class FunctorSlot##n : public Slot##n<R, _arg_type_names> {                   \
 public:                                                                      \
  typedef FunctorSlot##n<R, _arg_type_names, F> SelfType;                     \
  FunctorSlot##n(F functor) : functor_(functor) { }                           \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    return ResultVariant(Variant(functor_(_call_args)));                      \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && functor_ == a->functor_;                                      \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlot##n);                                 \
  F functor_;                                                                 \
};                                                                            \
                                                                              \
template <_arg_types, typename F>                                             \
class FunctorSlot##n<void, _arg_type_names, F> :                              \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef FunctorSlot##n<void, _arg_type_names, F> SelfType;                  \
  FunctorSlot##n(F functor) : functor_(functor) { }                           \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    functor_(_call_args);                                                     \
    return ResultVariant(Variant());                                          \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && functor_ == a->functor_;                                      \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlot##n);                                 \
  F functor_;                                                                 \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename T, typename M>                     \
class MethodSlot##n : public Slot##n<R, _arg_type_names> {                    \
 public:                                                                      \
  typedef MethodSlot##n<R, _arg_type_names, T, M> SelfType;                   \
  MethodSlot##n(T *obj, M method) : obj_(obj), method_(method) { }            \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    return ResultVariant(Variant((obj_->*method_)(_call_args)));              \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && obj_ == a->obj_ && method_ == a->method_;                     \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlot##n);                                  \
  T *obj_;                                                                    \
  M method_;                                                                  \
};                                                                            \
                                                                              \
template <_arg_types, typename T, typename M>                                 \
class MethodSlot##n<void, _arg_type_names, T, M> :                            \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef MethodSlot##n<void, _arg_type_names, T, M> SelfType;                \
  MethodSlot##n(T *obj, M method) : obj_(obj), method_(method) { }            \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    (obj_->*method_)(_call_args);                                             \
    return ResultVariant(Variant());                                          \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && obj_ == a->obj_ && method_ == a->method_;                     \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlot##n);                                  \
  T *obj_;                                                                    \
  M method_;                                                                  \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename T, typename M>                     \
class UnboundMethodSlot##n : public Slot##n<R, _arg_type_names> {             \
 public:                                                                      \
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),                  \
                 T_must_be_ScriptableInterface_or_derived_from_it);           \
  typedef UnboundMethodSlot##n<R, _arg_type_names, T, M> SelfType;            \
  UnboundMethodSlot##n(M method) : method_(method) { }                        \
  using Slot##n<R, _arg_type_names>::GetReturnType;                           \
  R operator()(T *obj, _args) const {                                         \
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,                     \
             ("Use Call() when the slot returns ScriptableInterface *"));     \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    return VariantValue<R>()(Call(obj, n, vargs).v());                        \
  }                                                                           \
  virtual ResultVariant Call(ScriptableInterface *obj,                        \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    ASSERT(obj);                                                              \
    return ResultVariant(Variant(                                             \
        (down_cast<T *>(obj)->*method_)(_call_args)));                        \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && method_ == a->method_;                                        \
  }                                                                           \
 private:                                                                     \
  R operator()(_args) const;                                                  \
  DISALLOW_EVIL_CONSTRUCTORS(UnboundMethodSlot##n);                           \
  M method_;                                                                  \
};                                                                            \
                                                                              \
template <_arg_types, typename T, typename M>                                 \
class UnboundMethodSlot##n<void, _arg_type_names, T, M> :                     \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),                  \
                 T_must_be_ScriptableInterface_or_derived_from_it);           \
  typedef UnboundMethodSlot##n<void, _arg_type_names, T, M> SelfType;         \
  UnboundMethodSlot##n(M method) : method_(method) { }                        \
  void operator()(T *obj, _args) const {                                      \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    Call(obj, n, vargs);                                                      \
  }                                                                           \
  virtual ResultVariant Call(ScriptableInterface *obj,                        \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    ASSERT(obj);                                                              \
    (down_cast<T *>(obj)->*method_)(_call_args);                              \
    return ResultVariant(Variant());                                          \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && method_ == a->method_;                                        \
  }                                                                           \
 private:                                                                     \
  void operator()(_args) const;                                               \
  DISALLOW_EVIL_CONSTRUCTORS(UnboundMethodSlot##n);                           \
  M method_;                                                                  \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename T, typename M,                     \
          typename DelegateGetter>                                            \
class DelegatedMethodSlot##n : public Slot##n<R, _arg_type_names> {           \
 public:                                                                      \
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),                  \
                 T_must_be_ScriptableInterface_or_derived_from_it);           \
  typedef DelegatedMethodSlot##n<R, _arg_type_names, T, M, DelegateGetter>    \
      SelfType;                                                               \
  DelegatedMethodSlot##n(M method, DelegateGetter delegate_getter)            \
      : method_(method), delegate_getter_(delegate_getter) { }                \
  using Slot##n<R, _arg_type_names>::GetReturnType;                           \
  R operator()(T *obj, _args) const {                                         \
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,                     \
             ("Use Call() when the slot returns ScriptableInterface *"));     \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    return VariantValue<R>()(Call(obj, n, vargs).v());                        \
  }                                                                           \
  virtual ResultVariant Call(ScriptableInterface *obj,                        \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    ASSERT(obj && delegate_getter_(down_cast<T *>(obj)));                     \
    return ResultVariant(Variant(                                             \
        (delegate_getter_(down_cast<T *>(obj))->*method_)(_call_args)));      \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && method_ == a->method_ &&                                      \
           delegate_getter_ == a->delegate_getter_;                           \
  }                                                                           \
 private:                                                                     \
  R operator()() const;                                                       \
  DISALLOW_EVIL_CONSTRUCTORS(DelegatedMethodSlot##n);                         \
  M method_;                                                                  \
  DelegateGetter delegate_getter_;                                            \
};                                                                            \
                                                                              \
template <_arg_types, typename T, typename M, typename DelegateGetter>        \
class DelegatedMethodSlot##n<void, _arg_type_names, T, M, DelegateGetter>     \
    : public Slot##n<void, _arg_type_names> {                                 \
 public:                                                                      \
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),                  \
                 T_must_be_ScriptableInterface_or_derived_from_it);           \
  typedef DelegatedMethodSlot##n<void, _arg_type_names, T, M,                 \
                                 DelegateGetter> SelfType;                    \
  DelegatedMethodSlot##n(M method, DelegateGetter delegate_getter)            \
      : method_(method), delegate_getter_(delegate_getter) { }                \
  using Slot##n<void, _arg_type_names>::GetReturnType;                        \
  void operator()(T *obj, _args) const {                                      \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    Call(obj, n, vargs);                                                      \
  }                                                                           \
  virtual ResultVariant Call(ScriptableInterface *obj,                        \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    ASSERT(obj && delegate_getter_(down_cast<T *>(obj)));                     \
    (delegate_getter_(down_cast<T *>(obj))->*method_)(_call_args);            \
    return ResultVariant(Variant());                                          \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && method_ == a->method_ &&                                      \
           delegate_getter_ == a->delegate_getter_;                           \
  }                                                                           \
 private:                                                                     \
  void operator()() const;                                                    \
  DISALLOW_EVIL_CONSTRUCTORS(DelegatedMethodSlot##n);                         \
  M method_;                                                                  \
  DelegateGetter delegate_getter_;                                            \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
class SlotProxy##n : public Slot##n<R, _arg_type_names> {                     \
 public:                                                                      \
  typedef SlotProxy##n<R, _arg_type_names> SelfType;                          \
  SlotProxy##n(Slot* slot) : slot_(slot) { }                                  \
  ~SlotProxy##n() { delete slot_; slot_ = NULL; }                             \
  virtual ResultVariant Call(ScriptableInterface *obj,                        \
                             int argc, const Variant argv[]) const {          \
    ASSERT(argc == n);                                                        \
    return slot_->Call(obj, argc, argv);                                      \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && *slot_ == *a->slot_;                                          \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(SlotProxy##n);                                   \
  Slot *slot_;                                                                \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename F, typename PA>                    \
class FunctorSlotClosure##n : public Slot##n<R, _arg_type_names> {            \
 public:                                                                      \
  typedef FunctorSlotClosure##n<R, _arg_type_names, F, PA> SelfType;          \
  FunctorSlotClosure##n(F functor, PA pa) : functor_(functor), pa_(pa) { }    \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    return ResultVariant(Variant(functor_(_call_args, pa_)));                 \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && functor_ == a->functor_ && pa_ == a->pa_;                     \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlotClosure##n);                          \
  F functor_;                                                                 \
  PA pa_;                                                                     \
};                                                                            \
                                                                              \
template <_arg_types, typename F, typename PA>                                \
class FunctorSlotClosure##n<void, _arg_type_names, F, PA> :                   \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef FunctorSlotClosure##n<void, _arg_type_names, F, PA> SelfType;       \
  FunctorSlotClosure##n(F functor, PA pa) : functor_(functor), pa_(pa) { }    \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    functor_(_call_args, pa_);                                                \
    return ResultVariant(Variant());                                          \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && functor_ == a->functor_ && pa_ == a->pa_;                     \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(FunctorSlotClosure##n);                          \
  F functor_;                                                                 \
  PA pa_;                                                                     \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename T, typename M, typename PA>        \
class MethodSlotClosure##n : public Slot##n<R, _arg_type_names> {             \
 public:                                                                      \
  typedef MethodSlotClosure##n<R, _arg_type_names, T, M, PA> SelfType;        \
  MethodSlotClosure##n(T *obj, M method, PA pa)                               \
    : obj_(obj), method_(method), pa_(pa) { }                                 \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    return ResultVariant(Variant((obj_->*method_)(_call_args, pa_)));         \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && obj_ == a->obj_ && method_ == a->method_ && pa_ == a->pa_;    \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlotClosure##n);                           \
  T *obj_;                                                                    \
  M method_;                                                                  \
  PA pa_;                                                                     \
};                                                                            \
                                                                              \
template <_arg_types, typename T, typename M, typename PA>                    \
class MethodSlotClosure##n<void, _arg_type_names, T, M, PA> :                 \
    public Slot##n<void, _arg_type_names> {                                   \
 public:                                                                      \
  typedef MethodSlotClosure##n<void, _arg_type_names, T, M, PA> SelfType;     \
  MethodSlotClosure##n(T *obj, M method, PA pa)                               \
    : obj_(obj), method_(method), pa_(pa) { }                                 \
  virtual ResultVariant Call(ScriptableInterface *,                           \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    GGL_UNUSED(argv);                                                         \
    ASSERT(argc == n);                                                        \
    (obj_->*method_)(_call_args, pa_);                                        \
    return ResultVariant(Variant());                                          \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && obj_ == a->obj_ && method_ == a->method_ && pa_ == a->pa_;    \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(MethodSlotClosure##n);                           \
  T *obj_;                                                                    \
  M method_;                                                                  \
  PA pa_;                                                                     \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename PA>                                \
class SlotProxyClosure##n : public Slot##n<R, _arg_type_names> {              \
 public:                                                                      \
  typedef SlotProxyClosure##n<R, _arg_type_names, PA> SelfType;               \
  SlotProxyClosure##n(Slot* slot, PA pa) : slot_(slot), pa_(pa) { }           \
  ~SlotProxyClosure##n() { delete slot_; slot_ = NULL; }                      \
  virtual ResultVariant Call(ScriptableInterface *obj,                        \
                             int argc, const Variant argv[]) const {          \
    GGL_UNUSED(argc);                                                         \
    ASSERT(argc == n);                                                        \
    Variant vargs[n + 1];                                                     \
    for(size_t i = 0; i < n; ++i) vargs[i] = argv[i];                         \
    vargs[n] = Variant(pa_);                                                  \
    return slot_->Call(obj, n + 1, vargs);                                    \
  }                                                                           \
  virtual bool operator==(const Slot &another) const {                        \
    const SelfType *a = down_cast<const SelfType *>(&another);                \
    return a && *slot_ == *a->slot_ && pa_ == a->pa_;                         \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(SlotProxyClosure##n);                            \
  Slot *slot_;                                                                \
  PA pa_;                                                                     \
};                                                                            \
                                                                              \
template <typename R, _arg_types>                                             \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (*f)(_arg_type_names)) {                                            \
  return new FunctorSlot##n<R, _arg_type_names, R (*)(_arg_type_names)>(f);   \
}                                                                             \
template <typename R, _arg_types, typename T>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(T *obj, R (T::*method)(_arg_type_names)) {                            \
  return new MethodSlot##n<R, _arg_type_names, T,                             \
                           R (T::*)(_arg_type_names)>(obj, method);           \
}                                                                             \
template <typename R, _arg_types, typename T>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(const T *obj, R (T::*method)(_arg_type_names) const) {                \
  return new MethodSlot##n<R, _arg_type_names, const T,                       \
                           R (T::*)(_arg_type_names) const>(obj, method);     \
}                                                                             \
template <typename R, _arg_types, typename T>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (T::*method)(_arg_type_names)) {                                    \
  return new UnboundMethodSlot##n<R, _arg_type_names, T,                      \
                                  R (T::*)(_arg_type_names)>(method);         \
}                                                                             \
template <typename R, _arg_types, typename T>                                 \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (T::*method)(_arg_type_names) const) {                              \
  return new UnboundMethodSlot##n<R, _arg_type_names, const T,                \
                                  R (T::*)(_arg_type_names) const>(method);   \
}                                                                             \
template <typename R, _arg_types, typename T, typename DT>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (DT::*method)(_arg_type_names), DT *(*delegate_getter)(T *)) {      \
  return new DelegatedMethodSlot##n<R, _arg_type_names, T,                    \
                                    R (DT::*)(_arg_type_names),               \
                                    DT *(*)(T *)>(                            \
      method, delegate_getter);                                               \
}                                                                             \
template <typename R, _arg_types, typename T, typename DT>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (DT::*method)(_arg_type_names) const,                               \
        const DT *(*delegate_getter)(T *)) {                                  \
  return new DelegatedMethodSlot##n<R, _arg_type_names, T,                    \
                                    R (DT::*)(_arg_type_names) const,         \
                                    const DT *(*)(T *)>(                      \
      method, delegate_getter);                                               \
}                                                                             \
template <typename R, _arg_types, typename T, typename DT>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (DT::*method)(_arg_type_names), DT *T::*delegate_field) {           \
  return new DelegatedMethodSlot##n<R, _arg_type_names, T,                    \
                                    R (DT::*)(_arg_type_names),               \
                                    FieldDelegateGetter<T, DT> >(             \
      method, FieldDelegateGetter<T, DT>(delegate_field));                    \
}                                                                             \
template <typename R, _arg_types, typename T, typename DT>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (DT::*method)(_arg_type_names) const,                               \
        DT *T::*delegate_field) {                                             \
  return new DelegatedMethodSlot##n<R, _arg_type_names, T,                    \
                                    R (DT::*)(_arg_type_names) const,         \
                                    FieldDelegateGetter<T, DT> >(             \
      method, FieldDelegateGetter<T, DT>(delegate_field));                    \
}                                                                             \
template <typename R, _arg_types, typename T, typename DT>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (DT::*method)(_arg_type_names) const,                               \
        const DT *T::*delegate_field) {                                       \
  return new DelegatedMethodSlot##n<R, _arg_type_names, T,                    \
                                    R (DT::*)(_arg_type_names) const,         \
                                    FieldDelegateGetter<T, const DT> >(       \
      method, FieldDelegateGetter<T, const DT>(delegate_field));              \
}                                                                             \
template <typename R, _arg_types, typename F>                                 \
inline Slot##n<R, _arg_type_names> * NewFunctorSlot(F f) {                    \
  return new FunctorSlot##n<R, _arg_type_names, F>(f);                        \
}                                                                             \
                                                                              \
template <typename R, _arg_types, typename PA>                                \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (*f)(_arg_type_names, PA), PA pa) {                                 \
  return new FunctorSlotClosure##n<R, _arg_type_names,                        \
                                   R (*)(_arg_type_names, PA), PA>(f, pa);    \
}                                                                             \
template <typename R, _arg_types, typename PA>                                \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(R (*f)(_arg_type_names, const PA&), const PA &pa) {                   \
  return new FunctorSlotClosure##n<R, _arg_type_names,                        \
                   R (*)(_arg_type_names, const PA&), PA>(f, pa);             \
}                                                                             \
template <typename R, _arg_types, typename T, typename PA>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(T *obj, R (T::*method)(_arg_type_names, PA), PA pa) {                 \
  return new MethodSlotClosure##n<R, _arg_type_names, T,                      \
                         R (T::*)(_arg_type_names, PA), PA>(obj, method, pa); \
}                                                                             \
template <typename R, _arg_types, typename T, typename PA>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(const T *obj, R (T::*method)(_arg_type_names, PA) const, PA pa) {     \
  return new MethodSlotClosure##n<R, _arg_type_names, const T,                \
                   R (T::*)(_arg_type_names, PA) const, PA>(obj, method, pa); \
}                                                                             \
template <typename R, _arg_types, typename T, typename PA>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(T *obj, R (T::*method)(_arg_type_names, const PA&), const PA &pa) {   \
  return new MethodSlotClosure##n<R, _arg_type_names, T,                      \
                 R (T::*)(_arg_type_names, const PA&), PA>(obj, method, pa);  \
}                                                                             \
template <typename R, _arg_types, typename T, typename PA>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewSlot(const T *obj, R (T::*method)(_arg_type_names, const PA&) const,       \
        const PA &pa) {                                                       \
  return new MethodSlotClosure##n<R, _arg_type_names, const T,                \
            R (T::*)(_arg_type_names, const PA&) const, PA>(obj, method, pa); \
}                                                                             \
template <typename R, _arg_types, typename F, typename PA>                    \
inline Slot##n<R, _arg_type_names> *                                          \
NewFunctorSlot(F f, PA pa) {                                                  \
  return new FunctorSlotClosure##n<R, _arg_type_names, F, PA>(f, pa);         \
}

#define INIT_ARG_TYPE(n) VariantType<P##n>::type
#define GET_ARG(n)       VariantValue<P##n>()(argv[n-1])
#define INIT_ARG(n)      vargs[n-1] = Variant(p##n)

#define ARG_TYPES1      typename P1
#define ARG_TYPE_NAMES1 P1
#define ARGS1           P1 p1
#define INIT_ARGS1      INIT_ARG(1)
#define INIT_ARG_TYPES1 INIT_ARG_TYPE(1)
#define CALL_ARGS1      GET_ARG(1)
DEFINE_SLOT(1, ARG_TYPES1, ARG_TYPE_NAMES1, ARGS1, INIT_ARGS1,
            INIT_ARG_TYPES1, CALL_ARGS1)

#define ARG_TYPES2      ARG_TYPES1, typename P2
#define ARG_TYPE_NAMES2 ARG_TYPE_NAMES1, P2
#define ARGS2           ARGS1, P2 p2
#define INIT_ARGS2      INIT_ARGS1; INIT_ARG(2)
#define INIT_ARG_TYPES2 INIT_ARG_TYPES1, INIT_ARG_TYPE(2)
#define CALL_ARGS2      CALL_ARGS1, GET_ARG(2)
DEFINE_SLOT(2, ARG_TYPES2, ARG_TYPE_NAMES2, ARGS2, INIT_ARGS2,
            INIT_ARG_TYPES2, CALL_ARGS2)

#define ARG_TYPES3      ARG_TYPES2, typename P3
#define ARG_TYPE_NAMES3 ARG_TYPE_NAMES2, P3
#define ARGS3           ARGS2, P3 p3
#define INIT_ARGS3      INIT_ARGS2; INIT_ARG(3)
#define INIT_ARG_TYPES3 INIT_ARG_TYPES2, INIT_ARG_TYPE(3)
#define CALL_ARGS3      CALL_ARGS2, GET_ARG(3)
DEFINE_SLOT(3, ARG_TYPES3, ARG_TYPE_NAMES3, ARGS3, INIT_ARGS3,
            INIT_ARG_TYPES3, CALL_ARGS3)

#define ARG_TYPES4      ARG_TYPES3, typename P4
#define ARG_TYPE_NAMES4 ARG_TYPE_NAMES3, P4
#define ARGS4           ARGS3, P4 p4
#define INIT_ARGS4      INIT_ARGS3; INIT_ARG(4)
#define INIT_ARG_TYPES4 INIT_ARG_TYPES3, INIT_ARG_TYPE(4)
#define CALL_ARGS4      CALL_ARGS3, GET_ARG(4)
DEFINE_SLOT(4, ARG_TYPES4, ARG_TYPE_NAMES4, ARGS4, INIT_ARGS4,
            INIT_ARG_TYPES4, CALL_ARGS4)

#define ARG_TYPES5      ARG_TYPES4, typename P5
#define ARG_TYPE_NAMES5 ARG_TYPE_NAMES4, P5
#define ARGS5           ARGS4, P5 p5
#define INIT_ARGS5      INIT_ARGS4; INIT_ARG(5)
#define INIT_ARG_TYPES5 INIT_ARG_TYPES4, INIT_ARG_TYPE(5)
#define CALL_ARGS5      CALL_ARGS4, GET_ARG(5)
DEFINE_SLOT(5, ARG_TYPES5, ARG_TYPE_NAMES5, ARGS5, INIT_ARGS5,
            INIT_ARG_TYPES5, CALL_ARGS5)

#define ARG_TYPES6      ARG_TYPES5, typename P6
#define ARG_TYPE_NAMES6 ARG_TYPE_NAMES5, P6
#define ARGS6           ARGS5, P6 p6
#define INIT_ARGS6      INIT_ARGS5; INIT_ARG(6)
#define INIT_ARG_TYPES6 INIT_ARG_TYPES5, INIT_ARG_TYPE(6)
#define CALL_ARGS6      CALL_ARGS5, GET_ARG(6)
DEFINE_SLOT(6, ARG_TYPES6, ARG_TYPE_NAMES6, ARGS6, INIT_ARGS6,
            INIT_ARG_TYPES6, CALL_ARGS6)

#define ARG_TYPES7      ARG_TYPES6, typename P7
#define ARG_TYPE_NAMES7 ARG_TYPE_NAMES6, P7
#define ARGS7           ARGS6, P7 p7
#define INIT_ARGS7      INIT_ARGS6; INIT_ARG(7)
#define INIT_ARG_TYPES7 INIT_ARG_TYPES6, INIT_ARG_TYPE(7)
#define CALL_ARGS7      CALL_ARGS6, GET_ARG(7)
DEFINE_SLOT(7, ARG_TYPES7, ARG_TYPE_NAMES7, ARGS7, INIT_ARGS7,
            INIT_ARG_TYPES7, CALL_ARGS7)

#define ARG_TYPES8      ARG_TYPES7, typename P8
#define ARG_TYPE_NAMES8 ARG_TYPE_NAMES7, P8
#define ARGS8           ARGS7, P8 p8
#define INIT_ARGS8      INIT_ARGS7; INIT_ARG(8)
#define INIT_ARG_TYPES8 INIT_ARG_TYPES7, INIT_ARG_TYPE(8)
#define CALL_ARGS8      CALL_ARGS7, GET_ARG(8)
DEFINE_SLOT(8, ARG_TYPES8, ARG_TYPE_NAMES8, ARGS8, INIT_ARGS8,
            INIT_ARG_TYPES8, CALL_ARGS8)

#define ARG_TYPES9      ARG_TYPES8, typename P9
#define ARG_TYPE_NAMES9 ARG_TYPE_NAMES8, P9
#define ARGS9           ARGS8, P9 p9
#define INIT_ARGS9      INIT_ARGS8; INIT_ARG(9)
#define INIT_ARG_TYPES9 INIT_ARG_TYPES8, INIT_ARG_TYPE(9)
#define CALL_ARGS9      CALL_ARGS8, GET_ARG(9)
DEFINE_SLOT(9, ARG_TYPES9, ARG_TYPE_NAMES9, ARGS9, INIT_ARGS9,
            INIT_ARG_TYPES9, CALL_ARGS9)

// Undefine macros to avoid name polution.
#undef DEFINE_SLOT
#undef INIT_ARG_TYPE
#undef GET_ARG

#undef ARG_TYPES1
#undef ARG_TYPE_NAMES1
#undef ARGS1
#undef INIT_ARGS1
#undef INIT_ARG_TYPES1
#undef CALL_ARGS1
#undef ARG_TYPES2
#undef ARG_TYPE_NAMES2
#undef ARGS2
#undef INIT_ARGS2
#undef INIT_ARG_TYPES2
#undef CALL_ARGS2
#undef ARG_TYPES3
#undef ARG_TYPE_NAMES3
#undef ARGS3
#undef INIT_ARGS3
#undef INIT_ARG_TYPES3
#undef CALL_ARGS3
#undef ARG_TYPES4
#undef ARG_TYPE_NAMES4
#undef ARGS4
#undef INIT_ARGS4
#undef INIT_ARG_TYPES4
#undef CALL_ARGS4
#undef ARG_TYPES5
#undef ARG_TYPE_NAMES5
#undef ARGS5
#undef INIT_ARGS5
#undef INIT_ARG_TYPES5
#undef CALL_ARGS5
#undef ARG_TYPES6
#undef ARG_TYPE_NAMES6
#undef ARGS6
#undef INIT_ARGS6
#undef INIT_ARG_TYPES6
#undef CALL_ARGS6
#undef ARG_TYPES7
#undef ARG_TYPE_NAMES7
#undef ARGS7
#undef INIT_ARGS7
#undef INIT_ARG_TYPES7
#undef CALL_ARGS7
#undef ARG_TYPES8
#undef ARG_TYPE_NAMES8
#undef ARGS8
#undef INIT_ARGS8
#undef INIT_ARG_TYPES8
#undef CALL_ARGS8
#undef ARG_TYPES9
#undef ARG_TYPE_NAMES9
#undef ARGS9
#undef INIT_ARGS9
#undef INIT_ARG_TYPES9
#undef CALL_ARGS9

template <typename T>
class FixedGetter {
 public:
  FixedGetter(T value) : value_(value) { }
  T operator()() const { return value_; }
  bool operator==(FixedGetter<T> another) const {
    return value_ == another.value_;
  }
 private:
  T value_;
};

template <typename T>
class SimpleGetter {
 public:
  SimpleGetter(const T *value_ptr) : value_ptr_(value_ptr) { }
  T operator()() const { return *value_ptr_; }
  bool operator==(SimpleGetter<T> another) const {
    return value_ptr_ == another.value_ptr_;
  }
 private:
  const T *value_ptr_;
};

template <typename T>
class SimpleSetter {
 public:
  SimpleSetter(T *value_ptr) : value_ptr_(value_ptr) { }
  void operator()(T value) const { *value_ptr_ = value; }
  bool operator==(SimpleSetter<T> another) const {
    return value_ptr_ == another.value_ptr_;
  }
 private:
  T *value_ptr_;
};

/**
 * Helper function to new a @c Slot that always return a fixed @a value.
 * @param value the fixed value.
 * @return the getter @c Slot.
 */
template <typename T>
inline Slot0<T> *NewFixedGetterSlot(T value) {
  return NewFunctorSlot<T>(FixedGetter<T>(value));
}

/**
 * Helper function to new a @c Slot that can get a value from @a value_ptr.
 * @param value_ptr pointer to a value.
 * @return the getter @c Slot.
 */
template <typename T>
inline Slot0<T> *NewSimpleGetterSlot(const T *value_ptr) {
  return NewFunctorSlot<T>(SimpleGetter<T>(value_ptr));
}

/**
 * Helper function to new a @c Slot that can set a value to @a value_ptr.
 * @param value_ptr pointer to a value.
 * @return the getter @c Slot.
 */
template <typename T>
inline Slot1<void, T> *NewSimpleSetterSlot(T *value_ptr) {
  return NewFunctorSlot<void, T>(SimpleSetter<T>(value_ptr));
}

/**
 * Helper macro for defining static delegate getters. Both const version and
 * non-const version will be defined.
 * @param getter name of the getter.
 * @param expr the expression to get the object to delegate to.
 * @param src_type the source object type.
 * @param dest_type the destination object type.
 */
#define DEFINE_DELEGATE_GETTER_NO_CONST(getter, expr, src_type, dest_type) \
  static dest_type *getter(src_type *src) { return expr; }
#define DEFINE_DELEGATE_GETTER_CONST(getter, expr, src_type, dest_type)    \
  static const dest_type *getter(src_type *src) { return expr; }
#define DEFINE_DELEGATE_GETTER(getter, expr, src_type, dest_type)          \
  DEFINE_DELEGATE_GETTER_NO_CONST(getter, expr, src_type, dest_type)       \
  DEFINE_DELEGATE_GETTER_CONST(getter##Const, expr, src_type, dest_type)

/**
 * Create a slot with default arguments.
 * @param slot all slot operations except @c GetDefaultArgs() will be deligate
 *     to this slot.
 * @param default_args an array of default argument values. The pointer must be
 *     statically allocated, or automatically allocated but life longer than
 *     that of the returned slot.
 * @return a new slot with default arguments.
 * @see Variant::GetDefaultArgs()
 */
Slot *NewSlotWithDefaultArgs(Slot *slot, const Variant *default_args);

/** @} */

} // namespace ggadget

#endif // GGADGET_SLOT_H__
