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

#ifndef GGADGET_SIGNALS_H__
#define GGADGET_SIGNALS_H__

#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <ggadget/small_object.h>

namespace ggadget {

class Signal;

/**
 * @defgroup SignalSlot Signal and Slot
 * @ingroup ScriptableFoundation
 * @{
 */

/**
 * The connection object between a @c Signal and a @c Slot.
 * The caller can use the connection to temporarily block the slot.
 */
class Connection : public SmallObject<> {
 public:
  /**
   * Disconnect the connection. The connection itself will be deleted.
   */
  void Disconnect();

  /**
   * Reconnect the connection to another @c Slot.
   * The @a slot is then owned by this connection no matter @c Reconnect
   * succeeded or failed.
   * @param slot the new @c Slot to be connected.
   * @return @c true if succeeds.
   */
  bool Reconnect(Slot *slot);

  /** Get the target @c Slot */
  Slot *slot() const { return slot_; }

  /** Get the owner @c Signal */
  const Signal *signal() const { return signal_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Connection);

  friend class Signal;

  /**
   * Connection can be only constructed and destructed by @c Signal.
   * The connection owns the slot.
   */
  Connection(Signal *signal, Slot *slot);
  ~Connection();

  Signal *signal_;
  Slot *slot_;
};

/**
 * Signal caller that can connect and emit to 0 to many <code>Slot</code>s.
 */
class Signal : public SmallObject<> {
 public:
  virtual ~Signal();

  /**
   * Connect a general @c Slot (compile-time type not specialized from
   * templates).  It's useful to connect to <code>ScriptSlot</code>s
   * and <code>SignalSlot</code>s. Compatability is checked inside of
   * @c ConnectGeneral() at runtime.
   * @param slot the slot to connect. This signal takes the ownership of the
   *     slot pointer, no matter this call succeeded or failed, so don't share
   *     slots with other owners.
   *     If it's @c NULL, a unconnected @c Connection will be returned.
   * @return the connected @c Connection.  The pointer is owned by this signal.
   *     Return @c NULL on any error, such as argument incompatibility.
   */
  Connection *ConnectGeneral(Slot *slot);

  /**
   * Check if a @c Slot is compatible with this @c Signal.
   */
  bool CheckCompatibility(const Slot *slot) const;

  /**
   * Check if there is active (not blocked) connections.
   * @return true if there is at least one active connections.
   */
  bool HasActiveConnections() const;

  /**
   * Emit the signal in general format.
   * Normally C++ code should use @c operator() in the templated subclasses.
   * @param argc number of arguments.
   * @param argv argument array.  Can be @c NULL if <code>argc==0</code>.
   * @return the return value of the connected slots. Use @c ResultVariant
   *     instead @c Variant as the result type to ensure scriptable object
   *     to be deleted if the result won't be handled by the caller.
   *     If there is no active connections, a @c ResultVariant containing
   *     a @c Variant with the default value of @c GetReturnType() will be
   *     returned. If there are multiple active connections, the return value
   *     of the last slot will be returned.
   */
  ResultVariant Emit(int argc, const Variant argv[]) const;

  /**
   * Get metadata of the @c Signal.
   */
  virtual Variant::Type GetReturnType() const { return Variant::TYPE_VOID; }
  /**
   * Get metadata of the @c Signal.
   */
  virtual int GetArgCount() const { return 0; }
  /**
   * Get metadata of the @c Signal.
   */
  virtual const Variant::Type *GetArgTypes() const { return NULL; }

  /**
   * Disconnect a connection from this signal.
   */
  bool Disconnect(Connection *connection);

  /**
   * Gets/sets the single default slot to this signal. It's useful to manage
   * event handlers that doesn't have multiple instances.
   */
  Slot *GetDefaultSlot();
  Connection *SetDefaultSlot(Slot *slot);

  /**
   * Gets the count of connected connections, including blocked or unblocked.
   */
  size_t GetConnectionCount() const;

 protected:
  Signal();

  /**
   * Connect to a slot without runtime compatability checking, for use by
   * templated subclasses because their compatability are checked at compile
   * time.
   * @param slot same as @c ConnectGeneral().
   * @return same as @c ConnectGeneral().
   */
  Connection *Connect(Slot *slot);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Signal);
  class Impl;
  Impl *impl_;
};

/**
 * A @c ClassSignal implementation should contain a memmber pointer to
 * a @c Signal member of a class.
 */
class ClassSignal : public SmallObject<> {
 public:
  virtual ~ClassSignal() { }
  /** Binds the stored @c Signal member to a particular object. */
  virtual Signal *GetSignal(ScriptableInterface *object) = 0;
  /** Creates a @c Slot as the prototype of the contained signal. */
  virtual Slot *NewPrototypeSlot() = 0;
};

/**
 * Wrap a @c Signal as a @c Slot to enable complex signal emitting paths.
 */
class SignalSlot : public Slot {
 public:
  /**
   * The @c SignalSlot doesn't has ownership of the signal.
   * @param signal the @c Signal to wrap.
   */
  SignalSlot(const Signal *signal) : signal_(signal) { }

  const Signal *signal() const { return signal_; }

  virtual ResultVariant Call(ScriptableInterface *obj,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(obj);
    // A SignalSlot is always bound to an object, so obj is ignored.
    return signal_->Emit(argc, argv);
  }
  virtual Variant::Type GetReturnType() const {
    return signal_->GetReturnType();
  }
  virtual int GetArgCount() const {
    return signal_->GetArgCount();
  }
  virtual const Variant::Type *GetArgTypes() const {
    return signal_->GetArgTypes();
  }
  virtual bool operator==(const Slot &another) const {
    return signal_ == (down_cast<const SignalSlot *>(&another))->signal_;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(SignalSlot);

  const Signal *signal_;
};

/**
 * A @c Signal without parameter.
 */
template <typename R>
class Signal0 : public Signal {
 public:
  Signal0() { }
  Connection *Connect(Slot0<R> *slot) { return Signal::Connect(slot); }
  R operator()() const {
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,
             ("Use Emit() when the signal returns ScriptableInterface *"));
    return VariantValue<R>()(Emit(0, NULL).v());
  }
  virtual Variant::Type GetReturnType() const { return VariantType<R>::type; }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Signal0);
};

/**
 * Partial specialized @c Signal0 that returns @c void.
 */
template <>
class Signal0<void> : public Signal {
 public:
  Signal0() { }
  Connection *Connect(Slot0<void> *slot) { return Signal::Connect(slot); }
  void operator()() const { Emit(0, NULL); }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Signal0);
};

typedef Signal0<void> EventSignal;

template <typename R, typename T>
class ClassSignal0 : public ClassSignal {
 public:
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
  ClassSignal0(Signal0<R> T::*signal) : signal_(signal) { }
  virtual Signal *GetSignal(ScriptableInterface *object) {
    return &(down_cast<T *>(object)->*signal_);
  }
  virtual Slot *NewPrototypeSlot() {
    return new PrototypeSlot0<R>();
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ClassSignal0);
  Signal0<R> T::*signal_;
};

template <typename R, typename T>
inline ClassSignal *NewClassSignal(Signal0<R> T::*signal) {
  return new ClassSignal0<R, T>(signal);
}

template <typename R, typename T, typename DT, typename DelegateGetter>
class DelegatedClassSignal0 : public ClassSignal {
 public:
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),
                 T_must_be_ScriptableInterface_or_derived_from_it);
  DelegatedClassSignal0(Signal0<R> DT::*signal, DelegateGetter delegate_getter)
      : signal_(signal), delegate_getter_(delegate_getter) { }
  virtual Signal *GetSignal(ScriptableInterface *object) {
    return &(delegate_getter_(down_cast<T *>(object))->*signal_);
  }
  virtual Slot *NewPrototypeSlot() {
    return new PrototypeSlot0<R>();
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(DelegatedClassSignal0);
  Signal0<R> DT::*signal_;
  DelegateGetter delegate_getter_;
};

template <typename R, typename T, typename DT>
inline ClassSignal *NewClassSignal(Signal0<R> DT::*signal,
                                   DT *(*delegate_getter)(T *)) {
  return new DelegatedClassSignal0<R, T, DT, DT *(*)(T *)>(signal,
                                                           delegate_getter);
}
template <typename R, typename T, typename DT>
inline ClassSignal *NewClassSignal(Signal0<R> DT::*signal,
                                   DT *T::*delegate_field) {
  return new DelegatedClassSignal0<R, T, DT, FieldDelegateGetter<T, DT> >(
      signal, FieldDelegateGetter<T, DT>(delegate_field));
}

/**
 * Other <code>Signal</code>s are defined by this macro.
 */
#define DEFINE_SIGNAL(n, _arg_types, _arg_type_names, _args, _init_args)      \
template <typename R, _arg_types>                                             \
class Signal##n : public Signal {                                             \
 public:                                                                      \
  Signal##n() { }                                                             \
  Connection *Connect(Slot##n<R, _arg_type_names> *slot) {                    \
    return Signal::Connect(slot);                                             \
  }                                                                           \
  R operator()(_args) const {                                                 \
    ASSERT_M(GetReturnType() != Variant::TYPE_SCRIPTABLE,                     \
             ("Use Emit() when the signal returns ScriptableInterface *"));   \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    return VariantValue<R>()(Emit(n, vargs).v());                             \
  }                                                                           \
  virtual Variant::Type GetReturnType() const { return VariantType<R>::type; }\
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(Signal##n);                                      \
};                                                                            \
                                                                              \
template <_arg_types>                                                         \
class Signal##n<void, _arg_type_names> : public Signal {                      \
 public:                                                                      \
  Signal##n() { }                                                             \
  Connection *Connect(Slot##n<void, _arg_type_names> *slot) {                 \
    return Signal::Connect(slot);                                             \
  }                                                                           \
  void operator()(_args) const {                                              \
    Variant vargs[n];                                                         \
    _init_args;                                                               \
    Emit(n, vargs);                                                           \
  }                                                                           \
  virtual int GetArgCount() const { return n; }                               \
  virtual const Variant::Type *GetArgTypes() const {                          \
    return ArgTypesHelper<_arg_type_names>();                                 \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(Signal##n);                                      \
};                                                                            \
                                                                              \
template <typename R, _arg_types, typename T>                                 \
class ClassSignal##n : public ClassSignal {                                   \
 public:                                                                      \
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),                  \
                 T_must_be_ScriptableInterface_or_derived_from_it);           \
  ClassSignal##n(Signal##n<R, _arg_type_names> T::*signal)                    \
      : signal_(signal) { }                                                   \
  virtual Signal *GetSignal(ScriptableInterface *object) {                    \
    return &(down_cast<T *>(object)->*signal_);                               \
  }                                                                           \
  virtual Slot *NewPrototypeSlot() {                                          \
    return new PrototypeSlot##n<R, _arg_type_names>();                        \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(ClassSignal##n);                                 \
  Signal##n<R, _arg_type_names> T::*signal_;                                  \
};                                                                            \
template <typename R, _arg_types, typename T>                                 \
inline ClassSignal *NewClassSignal(Signal##n<R, _arg_type_names> T::*signal) {\
  return new ClassSignal##n<R, _arg_type_names, T>(signal);                   \
}                                                                             \
                                                                              \
template <typename R, _arg_types, typename T, typename DT,                    \
          typename DelegateGetter>                                            \
class DelegatedClassSignal##n : public ClassSignal {                          \
 public:                                                                      \
  COMPILE_ASSERT((IsDerived<ScriptableInterface, T>::value),                  \
                 T_must_be_ScriptableInterface_or_derived_from_it);           \
  DelegatedClassSignal##n(Signal##n<R, _arg_type_names> DT::*signal,          \
                          DelegateGetter delegate_getter)                     \
      : signal_(signal), delegate_getter_(delegate_getter) { }                \
  virtual Signal *GetSignal(ScriptableInterface *object) {                    \
    return &(delegate_getter_(down_cast<T *>(object))->*signal_);             \
  }                                                                           \
  virtual Slot *NewPrototypeSlot() {                                          \
    return new PrototypeSlot##n<R, _arg_type_names>();                        \
  }                                                                           \
 private:                                                                     \
  DISALLOW_EVIL_CONSTRUCTORS(DelegatedClassSignal##n);                        \
  Signal##n<R, _arg_type_names> DT::*signal_;                                 \
  DelegateGetter delegate_getter_;                                            \
};                                                                            \
template <typename R, _arg_types, typename T, typename DT>                    \
inline ClassSignal *NewClassSignal(                                           \
    Signal##n<R, _arg_type_names> DT::*signal, DT *(*delegate_getter)(T *)) { \
  return new DelegatedClassSignal##n<R, _arg_type_names, T, DT, DT *(*)(T *)>(\
      signal, delegate_getter);                                               \
}                                                                             \
template <typename R, _arg_types, typename T, typename DT>                    \
inline ClassSignal *NewClassSignal(                                           \
    Signal##n<R, _arg_type_names> DT::*signal, DT *T::*delegate_field) {      \
  return new DelegatedClassSignal##n<R, _arg_type_names, T, DT,               \
                                     FieldDelegateGetter<T, DT> >(            \
      signal, FieldDelegateGetter<T, DT>(delegate_field));                    \
}                                                                             \

#define INIT_ARG(n)      vargs[n-1] = Variant(p##n)

#define ARG_TYPES1      typename P1
#define ARG_TYPE_NAMES1 P1
#define ARGS1           P1 p1
#define INIT_ARGS1      INIT_ARG(1)
DEFINE_SIGNAL(1, ARG_TYPES1, ARG_TYPE_NAMES1, ARGS1, INIT_ARGS1)

#define ARG_TYPES2      ARG_TYPES1, typename P2
#define ARG_TYPE_NAMES2 ARG_TYPE_NAMES1, P2
#define ARGS2           ARGS1, P2 p2
#define INIT_ARGS2      INIT_ARGS1; INIT_ARG(2)
DEFINE_SIGNAL(2, ARG_TYPES2, ARG_TYPE_NAMES2, ARGS2, INIT_ARGS2)

#define ARG_TYPES3      ARG_TYPES2, typename P3
#define ARG_TYPE_NAMES3 ARG_TYPE_NAMES2, P3
#define ARGS3           ARGS2, P3 p3
#define INIT_ARGS3      INIT_ARGS2; INIT_ARG(3)
DEFINE_SIGNAL(3, ARG_TYPES3, ARG_TYPE_NAMES3, ARGS3, INIT_ARGS3)

#define ARG_TYPES4      ARG_TYPES3, typename P4
#define ARG_TYPE_NAMES4 ARG_TYPE_NAMES3, P4
#define ARGS4           ARGS3, P4 p4
#define INIT_ARGS4      INIT_ARGS3; INIT_ARG(4)
DEFINE_SIGNAL(4, ARG_TYPES4, ARG_TYPE_NAMES4, ARGS4, INIT_ARGS4)

#define ARG_TYPES5      ARG_TYPES4, typename P5
#define ARG_TYPE_NAMES5 ARG_TYPE_NAMES4, P5
#define ARGS5           ARGS4, P5 p5
#define INIT_ARGS5      INIT_ARGS4; INIT_ARG(5)
DEFINE_SIGNAL(5, ARG_TYPES5, ARG_TYPE_NAMES5, ARGS5, INIT_ARGS5)

#define ARG_TYPES6      ARG_TYPES5, typename P6
#define ARG_TYPE_NAMES6 ARG_TYPE_NAMES5, P6
#define ARGS6           ARGS5, P6 p6
#define INIT_ARGS6      INIT_ARGS5; INIT_ARG(6)
DEFINE_SIGNAL(6, ARG_TYPES6, ARG_TYPE_NAMES6, ARGS6, INIT_ARGS6)

#define ARG_TYPES7      ARG_TYPES6, typename P7
#define ARG_TYPE_NAMES7 ARG_TYPE_NAMES6, P7
#define ARGS7           ARGS6, P7 p7
#define INIT_ARGS7      INIT_ARGS6; INIT_ARG(7)
DEFINE_SIGNAL(7, ARG_TYPES7, ARG_TYPE_NAMES7, ARGS7, INIT_ARGS7)

#define ARG_TYPES8      ARG_TYPES7, typename P8
#define ARG_TYPE_NAMES8 ARG_TYPE_NAMES7, P8
#define ARGS8           ARGS7, P8 p8
#define INIT_ARGS8      INIT_ARGS7; INIT_ARG(8)
DEFINE_SIGNAL(8, ARG_TYPES8, ARG_TYPE_NAMES8, ARGS8, INIT_ARGS8)

#define ARG_TYPES9      ARG_TYPES8, typename P9
#define ARG_TYPE_NAMES9 ARG_TYPE_NAMES8, P9
#define ARGS9           ARGS8, P9 p9
#define INIT_ARGS9      INIT_ARGS8; INIT_ARG(9)
DEFINE_SIGNAL(9, ARG_TYPES9, ARG_TYPE_NAMES9, ARGS9, INIT_ARGS9)

// Undefine macros to avoid name polution.
#undef DEFINE_SIGNAL
#undef INIT_ARG

#undef ARG_TYPES1
#undef ARG_TYPE_NAMES1
#undef INIT_ARGS1
#undef ARGS1
#undef ARG_TYPES2
#undef ARG_TYPE_NAMES2
#undef INIT_ARGS2
#undef ARGS2
#undef ARG_TYPES3
#undef ARG_TYPE_NAMES3
#undef INIT_ARGS3
#undef ARGS3
#undef ARG_TYPES4
#undef ARG_TYPE_NAMES4
#undef INIT_ARGS4
#undef ARGS4
#undef ARG_TYPES5
#undef ARG_TYPE_NAMES5
#undef INIT_ARGS5
#undef ARGS5
#undef ARG_TYPES6
#undef ARG_TYPE_NAMES6
#undef INIT_ARGS6
#undef ARGS6
#undef ARG_TYPES7
#undef ARG_TYPE_NAMES7
#undef INIT_ARGS7
#undef ARGS7
#undef ARG_TYPES8
#undef ARG_TYPE_NAMES8
#undef INIT_ARGS8
#undef ARGS8
#undef ARG_TYPES9
#undef ARG_TYPE_NAMES9
#undef INIT_ARGS9
#undef ARGS9

/** @} */

} // namespace ggadget

#endif // GGADGET_SIGNALS_H__
