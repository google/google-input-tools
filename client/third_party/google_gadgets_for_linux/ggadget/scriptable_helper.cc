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

#include <cstring>
#include <map>
#include <vector>
#include "scriptable_helper.h"
#include "logger.h"
#include "scriptable_holder.h"
#include "scriptable_interface.h"
#include "signals.h"
#include "slot.h"
#include "string_utils.h"

namespace ggadget {

// Define it to get verbose debug info about reference counting, especially
// useful when run with valgrind.
// #define VERBOSE_DEBUG_REF

namespace internal {

class ScriptableHelperImpl : public ScriptableHelperImplInterface {
 public:
  ScriptableHelperImpl(ScriptableHelperCallbackInterface *owner);
  virtual ~ScriptableHelperImpl();

  virtual void RegisterProperty(const char *name, Slot *getter, Slot *setter);
  virtual void RegisterStringEnumProperty(const char *name,
                                          Slot *getter, Slot *setter,
                                          const char **names, int count);
  virtual void RegisterMethod(const char *name, Slot *slot);
  virtual void RegisterSignal(const char *name, Signal *signal);
  virtual void RegisterVariantConstant(const char *name, const Variant &value);
  virtual void RegisterClassSignal(const char *name,
                                   ClassSignal *class_signal);
  virtual void SetInheritsFrom(ScriptableInterface *inherits_from);
  virtual void SetArrayHandler(Slot *getter, Slot *setter);
  virtual void SetDynamicPropertyHandler(Slot *getter, Slot *setter);

  // The following 3 methods declared in ScriptableInterface should never be
  // called.
  virtual uint64_t GetClassId() const { return 0; }
  virtual bool IsInstanceOf(uint64_t class_id) const {
    GGL_UNUSED(class_id);
    ASSERT(false); return false;
  }
  virtual bool IsStrict() const { ASSERT(false); return false; }
  virtual bool IsEnumeratable() const { ASSERT(false); return false; }

  virtual void Ref() const;
  virtual void Unref(bool transient) const;
  virtual int GetRefCount() const { return ref_count_; }

  virtual Connection *ConnectOnReferenceChange(Slot2<void, int, int> *slot);
  virtual PropertyType GetPropertyInfo(const char *name, Variant *prototype);
  virtual ResultVariant GetProperty(const char *name);
  virtual bool SetProperty(const char *name, const Variant &value);
  virtual ResultVariant GetPropertyByIndex(int index);
  virtual bool SetPropertyByIndex(int index, const Variant &value);

  virtual void SetPendingException(ScriptableInterface *exception);
  virtual ScriptableInterface *GetPendingException(bool clear);
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback);
  virtual bool EnumerateElements(EnumerateElementsCallback *callback);
  virtual bool RemoveProperty(const char *name);

  virtual RegisterableInterface *GetRegisterable() { return this; }

 private:
  void EnsureRegistered();
  class InheritedPropertiesCallback;
  class InheritedElementsCallback;
  void AddPropertyInfo(const char *name, PropertyType type,
                       const Variant &prototype,
                       Slot *getter, Slot *setter);

  struct PropertyInfo {
    PropertyInfo() : type(PROPERTY_NOT_EXIST) {
      memset(&u, 0, sizeof(u));
    }

    void OnRefChange(int ref_count, int change) {
      GGL_UNUSED(ref_count);
      // We have a similar mechanism in ScriptableHolder.
      // Please see the comments there.
      if (change == 0) {
        ASSERT(u.scriptable_info.ref_change_connection &&
               u.scriptable_info.scriptable);
        u.scriptable_info.ref_change_connection->Disconnect();
        u.scriptable_info.ref_change_connection = NULL;
        u.scriptable_info.scriptable->Unref(true);
        u.scriptable_info.scriptable = NULL;
        prototype = Variant(static_cast<ScriptableInterface *>(NULL));
      }
    }

    PropertyType type;
    Variant prototype;
    union {
      // For normal properties.
      struct {
        Slot *getter, *setter;
      } slots;
      // For ScriptableInterface * constants. Not using ScriptableHolder to
      // make it possible to use union to save memory.
      struct {
        // This is a dup of the scriptable pointer stored in prototype, to
        // avoid virtual method call during destruction of the scriptable
        // object.
        ScriptableInterface *scriptable;
        Connection *ref_change_connection;
      } scriptable_info;
    } u;
  };

  // Because PropertyInfo is a copy-able struct, we must deallocate the
  // resource outside of the struct instead of in ~PropertyInfo().
  static void DestroyPropertyInfo(PropertyInfo *info);
  const PropertyInfo *GetPropertyInfoInternal(const char *name);

  ScriptableHelperCallbackInterface *owner_;
  mutable int ref_count_;
  bool registering_class_;

  typedef LightMap<const char *, PropertyInfo,
                   GadgetCharPtrComparator> PropertyInfoMap;

  class ClassInfoMap : public LightMap<uint64_t, PropertyInfoMap> {
   public:
    ~ClassInfoMap() {
      ClassInfoMap::iterator it = this->begin();
      ClassInfoMap::iterator end = this->end();
      for (; it != end; ++it) {
        PropertyInfoMap::iterator prop_it = it->second.begin();
        PropertyInfoMap::iterator prop_end = it->second.end();
        for (; prop_it != prop_end; ++prop_it)
          ScriptableHelperImpl::DestroyPropertyInfo(&prop_it->second);
      }
    }
  };

  // Stores information of all properties of this object.
  PropertyInfoMap property_info_;
  // Stores class-based property information for all classes.
  static ClassInfoMap *all_class_info_;
  // If a class has no class-based property_info, let class_property_info_
  // point to this map to save duplicated blank maps.
  static PropertyInfoMap *blank_property_info_;
  PropertyInfoMap *class_property_info_;

#ifdef _DEBUG
  struct ClassStatInfo {
    ClassStatInfo()
        : class_property_count(0), obj_property_count(0), total_created(0) { }
    int class_property_count;
    int obj_property_count;
    int total_created;
  };
  typedef LightMap<uint64_t, ClassStatInfo> ClassStatMap;
  struct ClassStat {
    ~ClassStat() {
      // Don't use LOG because the logger may be unavailable now.
      printf("ScriptableHelper class stat: classes: %" PRIuS "\n", map.size());
      int properties_reg_in_ctor = 0;
      int total_properties = 0;
      int properties_if_obj_reg = 0;
      for (ClassStatMap::iterator it = map.begin(); it != map.end(); ++it) {
        printf("%" PRIx64 ": class properties: %d object properties:"
               " %d objects: %d\n",
               it->first, it->second.class_property_count,
               it->second.obj_property_count, it->second.total_created);
        if (it->second.total_created == 0)
          properties_reg_in_ctor += it->second.obj_property_count;
        properties_if_obj_reg += it->second.obj_property_count +
            it->second.class_property_count * it->second.total_created;
        total_properties += it->second.obj_property_count +
                            it->second.class_property_count;
      }
      printf("properties registered in constructors: %d\n"
             "total properties: %d (if all obj reg: %d, saved %f%%)\n",
             properties_reg_in_ctor, total_properties, properties_if_obj_reg,
             100.0 - 100.0 * total_properties / properties_if_obj_reg);
    }
    ClassStatMap map;
  };
  static ClassStat class_stat_;
#endif

  Signal2<void, int, int> on_reference_change_signal_;
  ScriptableInterface *inherits_from_;
  Slot *array_getter_;
  Slot *array_setter_;
  Slot *dynamic_property_getter_;
  Slot *dynamic_property_setter_;
  ScriptableInterface *pending_exception_;
};

// Class information shall be truely static and shouldn't be destroyed when
// exiting.
ScriptableHelperImpl::ClassInfoMap *ScriptableHelperImpl::all_class_info_ =
  new ScriptableHelperImpl::ClassInfoMap;
ScriptableHelperImpl::PropertyInfoMap
  *ScriptableHelperImpl::blank_property_info_ =
    new ScriptableHelperImpl::PropertyInfoMap;

#ifdef _DEBUG
ScriptableHelperImpl::ClassStat ScriptableHelperImpl::class_stat_;
#endif

ScriptableHelperImplInterface *NewScriptableHelperImpl(
    ScriptableHelperCallbackInterface *owner) {
  return new ScriptableHelperImpl(owner);
}

ScriptableHelperImpl::ScriptableHelperImpl(
    ScriptableHelperCallbackInterface *owner)
    : owner_(owner),
      ref_count_(0),
      registering_class_(false),
      class_property_info_(NULL),
      inherits_from_(NULL),
      array_getter_(NULL),
      array_setter_(NULL),
      dynamic_property_getter_(NULL),
      dynamic_property_setter_(NULL),
      pending_exception_(NULL) {
}

ScriptableHelperImpl::~ScriptableHelperImpl() {
  // Emit the ondelete signal, as early as possible.
  on_reference_change_signal_(0, 0);
  ASSERT(ref_count_ == 0);

  // Free all owned slots.
  for (PropertyInfoMap::iterator it = property_info_.begin();
       it != property_info_.end(); ++it) {
    DestroyPropertyInfo(&it->second);
  }

  delete array_getter_;
  delete array_setter_;
  delete dynamic_property_getter_;
  delete dynamic_property_setter_;
}

void ScriptableHelperImpl::EnsureRegistered() {
  if (!class_property_info_) {
    uint64_t class_id = owner_->GetScriptable()->GetClassId();
    ClassInfoMap::iterator it = all_class_info_->find(class_id);
    if (it == all_class_info_->end()) {
      registering_class_ = true;
      owner_->DoClassRegister();
      registering_class_ = false;
      it = all_class_info_->find(class_id);
      if (it == all_class_info_->end()) {
        // This class's DoClassRegister() did nothing.
        class_property_info_ = blank_property_info_;
      } else {
        class_property_info_ = &it->second;
      }
    } else {
      class_property_info_ = &it->second;
    }
    owner_->DoRegister();
#ifdef _DEBUG
    class_stat_.map[class_id].total_created++;
#endif
  }
}

void ScriptableHelperImpl::DestroyPropertyInfo(PropertyInfo *info) {
  if (info->prototype.type() == Variant::TYPE_SLOT)
    delete VariantValue<Slot *>()(info->prototype);

  if (info->type == PROPERTY_NORMAL) {
    delete info->u.slots.getter;
    delete info->u.slots.setter;
  } else if (info->type == PROPERTY_CONSTANT &&
             info->prototype.type() == Variant::TYPE_SCRIPTABLE) {
    if (info->u.scriptable_info.scriptable) {
      ASSERT(info->u.scriptable_info.ref_change_connection);
      info->u.scriptable_info.ref_change_connection->Disconnect();
      info->u.scriptable_info.ref_change_connection = NULL;
      info->u.scriptable_info.scriptable->Unref();
      info->u.scriptable_info.scriptable = NULL;
      info->prototype = Variant(static_cast<ScriptableInterface *>(NULL));
    }
  }
}

void ScriptableHelperImpl::AddPropertyInfo(const char *name, PropertyType type,
                                           const Variant &prototype,
                                           Slot *getter, Slot *setter) {
  uint64_t class_id = owner_->GetScriptable()->GetClassId();
  PropertyInfo *info =
      registering_class_ ? &(*all_class_info_)[class_id][name] :
      &property_info_[name];
  if (info->type != PROPERTY_NOT_EXIST) {
    // A previously registered property is overriden.
    DestroyPropertyInfo(info);
  }
  info->type = type;
  info->prototype = prototype;

  if (type == PROPERTY_CONSTANT &&
      prototype.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(prototype);
    if (scriptable) {
      info->u.scriptable_info.ref_change_connection =
          scriptable->ConnectOnReferenceChange(
              NewSlot(info, &PropertyInfo::OnRefChange));
      info->u.scriptable_info.scriptable = scriptable;
      scriptable->Ref();
    }
  } else {
    info->u.slots.getter = getter;
    info->u.slots.setter = setter;
  }

#ifdef _DEBUG
  if (registering_class_)
    class_stat_.map[class_id].class_property_count++;
  else
    class_stat_.map[class_id].obj_property_count++;
#endif
}

static Variant DummyGetter() {
  return Variant();
}

void ScriptableHelperImpl::RegisterProperty(const char *name,
                                            Slot *getter, Slot *setter) {
  ASSERT(name);
  Variant prototype;
  ASSERT(!setter || setter->GetArgCount() == 1);
  if (getter) {
    ASSERT(getter->GetArgCount() == 0);
    prototype = Variant(getter->GetReturnType());
    ASSERT(!setter || prototype.type() == setter->GetArgTypes()[0]);
  } else {
    getter = NewSlot(DummyGetter);
    if (setter)
      prototype = Variant(setter->GetArgTypes()[0]);

    if (prototype.type() == Variant::TYPE_SLOT) {
      DLOG("Warning: property '%s' is of type Slot, please make sure the return"
           " type of this Slot parameter is void or Variant, or use"
           " RegisterSignal instead.", name);
    }
  }

  AddPropertyInfo(name, PROPERTY_NORMAL, prototype, getter, setter);
}

class StringEnumGetter : public Slot0<const char *> {
 public:
  StringEnumGetter(Slot *slot, const char **names, int count)
      : slot_(slot), names_(names), count_(count) { }
  virtual ~StringEnumGetter() {
    delete slot_;
  }
  virtual ResultVariant Call(ScriptableInterface *obj,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    int index = VariantValue<int>()(slot_->Call(obj, 0, NULL).v());
    return ResultVariant(index >= 0 && index < count_ ?
                         Variant(names_[index]) : Variant(""));
  }
  virtual bool operator==(const Slot &another) const {
    GGL_UNUSED(another);
    return false; // Not used.
  }
  Slot *slot_;
  const char **names_;
  int count_;
};

class StringEnumSetter : public Slot1<void, const char *> {
 public:
  StringEnumSetter(Slot *slot, const char **names, int count)
      : slot_(slot), names_(names), count_(count) { }
  virtual ~StringEnumSetter() {
    delete slot_;
  }
  virtual ResultVariant Call(ScriptableInterface *obj,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    const char *name = VariantValue<const char *>()(argv[0]);
    if (!name)
      return ResultVariant();
    for (int i = 0; i < count_; i++) {
      if (strcmp(name, names_[i]) == 0) {
        Variant param(i);
        slot_->Call(obj, 1, &param);
        return ResultVariant();
      }
    }
    LOG("Invalid enumerated name: %s", name);
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    GGL_UNUSED(another);
    return false; // Not used.
  }
  Slot *slot_;
  const char **names_;
  int count_;
};

void ScriptableHelperImpl::RegisterStringEnumProperty(
    const char *name, Slot *getter, Slot *setter,
    const char **names, int count) {
  ASSERT(getter);
  RegisterProperty(name, new StringEnumGetter(getter, names, count),
                   setter ? new StringEnumSetter(setter, names, count) : NULL);
}

void ScriptableHelperImpl::RegisterMethod(const char *name, Slot *slot) {
  ASSERT(name);
  ASSERT(slot);
  AddPropertyInfo(name, PROPERTY_METHOD, Variant(slot), NULL, NULL);
}

void ScriptableHelperImpl::RegisterSignal(const char *name, Signal *signal) {
  ASSERT(!registering_class_);
  ASSERT(name);
  ASSERT(signal);

  // Create a SignalSlot as the value of the prototype to let others know
  // the calling convention.  It is owned by property_info_.
  Variant prototype = Variant(new SignalSlot(signal));
  Slot *getter = NewSlot(signal, &Signal::GetDefaultSlot);
  Slot *setter = NewSlot(signal, &Signal::SetDefaultSlot);
  AddPropertyInfo(name, PROPERTY_NORMAL, prototype, getter, setter);
}

class ClassSignalGetter : public Slot0<Slot *> {
 public:
  ClassSignalGetter(ClassSignal *class_signal)
      : class_signal_(class_signal) {
  }
  virtual ResultVariant Call(ScriptableInterface *obj,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    GGL_UNUSED(argv);
    Slot *slot = class_signal_->GetSignal(obj)->GetDefaultSlot();
    return ResultVariant(Variant(slot));
  }
  virtual bool operator==(const Slot &another) const {
    GGL_UNUSED(another);
    return false; // Not used.
  }
 private:
  ClassSignal *class_signal_;
};

class ClassSignalSetter : public Slot1<void, Slot *> {
 public:
  ClassSignalSetter(ClassSignal *class_signal)
      : class_signal_(class_signal) {
  }
  virtual ~ClassSignalSetter() {
    // class_signal_ is shared between ClassSignalGetter and ClassSignalSetter.
    // Let this class care for the deletion.
    delete class_signal_;
  }
  virtual ResultVariant Call(ScriptableInterface *obj,
                             int argc, const Variant argv[]) const {
    GGL_UNUSED(argc);
    ASSERT(argc == 1);
    Signal *signal = class_signal_->GetSignal(obj);
    Slot *slot = VariantValue<Slot *>()(argv[0]);
    signal->SetDefaultSlot(slot);
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    GGL_UNUSED(another);
    return false; // Not used.
  }
 private:
  ClassSignal *class_signal_;
};

void ScriptableHelperImpl::RegisterClassSignal(const char *name,
                                               ClassSignal *class_signal) {
  ASSERT(registering_class_);
  ASSERT(name);
  ASSERT(class_signal);
  Variant prototype = Variant(class_signal->NewPrototypeSlot());
  AddPropertyInfo(name, PROPERTY_NORMAL, prototype,
                  new ClassSignalGetter(class_signal),
                  new ClassSignalSetter(class_signal));
}

void ScriptableHelperImpl::RegisterVariantConstant(const char *name,
                                                   const Variant &value) {
  ASSERT(name);
  ASSERT_M(value.type() != Variant::TYPE_SLOT,
           ("Don't register Slot constant. Use RegisterMethod instead."));
  AddPropertyInfo(name, PROPERTY_CONSTANT, value, NULL, NULL);
}

void ScriptableHelperImpl::SetInheritsFrom(
    ScriptableInterface *inherits_from) {
  ASSERT(!registering_class_);
  inherits_from_ = inherits_from;
}

void ScriptableHelperImpl::SetArrayHandler(Slot *getter, Slot *setter) {
  ASSERT(!registering_class_);
  ASSERT(getter && getter->GetArgCount() == 1 &&
         getter->GetArgTypes()[0] == Variant::TYPE_INT64);
  ASSERT(!setter || (setter->GetArgCount() == 2 &&
                     setter->GetArgTypes()[0] == Variant::TYPE_INT64 &&
                     setter->GetReturnType() == Variant::TYPE_BOOL));
  delete array_getter_;
  delete array_setter_;
  array_getter_ = getter;
  array_setter_ = setter;
}

void ScriptableHelperImpl::SetDynamicPropertyHandler(
    Slot *getter, Slot *setter) {
  ASSERT(!registering_class_);
  ASSERT(getter &&
         ((getter->GetArgCount() == 1 &&
           getter->GetArgTypes()[0] == Variant::TYPE_STRING) ||
          (getter->GetArgCount() == 2 &&
           getter->GetArgTypes()[0] == Variant::TYPE_STRING &&
           getter->GetArgTypes()[1] == Variant::TYPE_BOOL)));
  ASSERT(!setter || (setter->GetArgCount() == 2 &&
                     setter->GetArgTypes()[0] == Variant::TYPE_STRING &&
                     setter->GetReturnType() == Variant::TYPE_BOOL));
  delete dynamic_property_getter_;
  delete dynamic_property_setter_;
  dynamic_property_getter_ = getter;
  dynamic_property_setter_ = setter;
}

void ScriptableHelperImpl::Ref() const {
#ifdef VERBOSE_DEBUG_REF
  DLOG("Ref ref_count_ = %d", ref_count_);
#endif
  ASSERT(ref_count_ >= 0);
  on_reference_change_signal_(ref_count_, 1);
  ref_count_++;
}

void ScriptableHelperImpl::Unref(bool transient) const {
  GGL_UNUSED(transient);
  // The parameter traisnent is ignored here. Let the ScriptableHelper
  // template deal with it.
#ifdef VERBOSE_DEBUG_REF
  DLOG("Unref ref_count_ = %d", ref_count_);
#endif
  ASSERT(ref_count_ > 0);
  on_reference_change_signal_(ref_count_, -1);
  ref_count_--;
}

Connection *ScriptableHelperImpl::ConnectOnReferenceChange(
    Slot2<void, int, int> *slot) {
  return on_reference_change_signal_.Connect(slot);
}

const ScriptableHelperImpl::PropertyInfo *
ScriptableHelperImpl::GetPropertyInfoInternal(const char *name) {
  EnsureRegistered();
  ASSERT(class_property_info_);
  PropertyInfoMap::const_iterator it = property_info_.find(name);
  bool found = it != property_info_.end();
  if (!found) {
    it = class_property_info_->find(name);
    found = it != class_property_info_->end();
  }
  return found ? &it->second : NULL;
}

ScriptableInterface::PropertyType ScriptableHelperImpl::GetPropertyInfo(
    const char *name, Variant *prototype) {
  const PropertyInfo *info = GetPropertyInfoInternal(name);
  if (info) {
    if (prototype)
      *prototype = info->prototype;
    return info->type;
  }

  // Try dynamic properties.
  if (dynamic_property_getter_) {
    // The second parameter means get property's info.
    Variant params[] = { Variant(name), Variant(true) };
    int argc = dynamic_property_getter_->GetArgCount();
    Variant dynamic_value = dynamic_property_getter_->Call(
        owner_->GetScriptable(), argc, params).v();
    if (dynamic_value.type() != Variant::TYPE_VOID) {
      if (prototype) {
        // Returns the slot as prototype, in case this dynamic property is
        // a signal.
        if (dynamic_value.type() == Variant::TYPE_SLOT)
          *prototype = dynamic_value;
        else
          *prototype = Variant(dynamic_value.type());
      }
      return PROPERTY_DYNAMIC;
    }
  }

  // Try inherited properties.
  if (inherits_from_)
    return inherits_from_->GetPropertyInfo(name, prototype);
  return PROPERTY_NOT_EXIST;
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
ResultVariant ScriptableHelperImpl::GetProperty(const char *name) {
  const PropertyInfo *info = GetPropertyInfoInternal(name);
  if (info) {
    switch (info->type) {
      case PROPERTY_NORMAL:
        ASSERT(info->u.slots.getter);
        return info->u.slots.getter->Call(owner_->GetScriptable(), 0, NULL);
      case PROPERTY_CONSTANT:
      case PROPERTY_METHOD:
        return ResultVariant(info->prototype);
      default:
        ASSERT(false);
        break;
    }
  } else {
    if (dynamic_property_getter_) {
      // The second parameter means get property's value.
      Variant params[] = { Variant(name), Variant(false) };
      int argc = dynamic_property_getter_->GetArgCount();
      ResultVariant result =
          dynamic_property_getter_->Call(owner_->GetScriptable(), argc, params);
      if (result.v().type() != Variant::TYPE_VOID)
        return result;
    }
    if (inherits_from_)
      return inherits_from_->GetProperty(name);
  }
  return ResultVariant();
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
bool ScriptableHelperImpl::SetProperty(const char *name,
                                       const Variant &value) {
  const PropertyInfo *info = GetPropertyInfoInternal(name);
  if (info) {
    switch (info->type) {
      case PROPERTY_NORMAL:
        if (info->u.slots.setter) {
          info->u.slots.setter->Call(owner_->GetScriptable(), 1, &value);
          return true;
        }
        return false;
      case PROPERTY_CONSTANT:
      case PROPERTY_METHOD:
        return false;
      default:
        ASSERT(false);
        break;
    }
  } else {
    if (dynamic_property_setter_) {
      Variant params[] = { Variant(name), value };
      Variant result = dynamic_property_setter_->Call(
          owner_->GetScriptable(), 2, params).v();
      ASSERT(result.type() == Variant::TYPE_BOOL);
      if (VariantValue<bool>()(result))
        return true;
    }
    if (inherits_from_ && inherits_from_->SetProperty(name, value))
      return true;
  }
  return false;
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
ResultVariant ScriptableHelperImpl::GetPropertyByIndex(int index) {
  EnsureRegistered();
  if (array_getter_) {
    Variant param(index);
    return array_getter_->Call(owner_->GetScriptable(), 1, &param);
  }
  return ResultVariant();
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
bool ScriptableHelperImpl::SetPropertyByIndex(int index,
                                              const Variant &value) {
  EnsureRegistered();
  if (array_setter_) {
    Variant params[] = { Variant(index), value };
    Variant result = array_setter_->Call(owner_->GetScriptable(),
                                         2, params).v();
    ASSERT(result.type() == Variant::TYPE_BOOL);
    return VariantValue<bool>()(result);
  }
  return false;
}

void ScriptableHelperImpl::SetPendingException(ScriptableInterface *exception) {
  ASSERT(pending_exception_ == NULL);
  pending_exception_ = exception;
}

ScriptableInterface *ScriptableHelperImpl::GetPendingException(bool clear) {
  ScriptableInterface *result = pending_exception_;
  if (clear)
    pending_exception_ = NULL;
  return result;
}

class ScriptableHelperImpl::InheritedPropertiesCallback {
 public:
  InheritedPropertiesCallback(ScriptableHelperImpl *owner,
                              EnumeratePropertiesCallback *callback)
      : owner_(owner), callback_(callback) {
  }

  bool Callback(const char *name, PropertyType type, const Variant &value) {
    if (owner_->property_info_.find(name) ==
            owner_->property_info_.end() &&
        owner_->class_property_info_->find(name) ==
            owner_->class_property_info_->end()) {
      // Only emunerate inherited properties which are not overriden by this
      // scriptable object.
      return (*callback_)(name, type, value);
    }
    return true;
  }

  ScriptableHelperImpl *owner_;
  EnumeratePropertiesCallback *callback_;
};

bool ScriptableHelperImpl::EnumerateProperties(
    EnumeratePropertiesCallback *callback) {
  ASSERT(callback);
  EnsureRegistered();

  // The algorithm below is not optimal, but is fairly clear and short.
  // This is not a big problem because this method impl is only used in
  // unittests.
  if (inherits_from_) {
    InheritedPropertiesCallback inherited_callback(this, callback);
    if (!inherits_from_->EnumerateProperties(
        NewSlot(&inherited_callback, &InheritedPropertiesCallback::Callback))) {
      delete callback;
      return false;
    }
  }
  for (PropertyInfoMap::const_iterator it = class_property_info_->begin();
       it != class_property_info_->end(); ++it) {
    if (property_info_.find(it->first) == property_info_.end()) {
      ResultVariant value = GetProperty(it->first);
      if (!(*callback)(it->first, it->second.type, value.v())) {
        delete callback;
        return false;
      }
    }
  }
  for (PropertyInfoMap::const_iterator it = property_info_.begin();
       it != property_info_.end(); ++it) {
    ResultVariant value = GetProperty(it->first);
    if (!(*callback)(it->first, it->second.type, value.v())) {
      delete callback;
      return false;
    }
  }
  delete callback;
  return true;
}

bool ScriptableHelperImpl::EnumerateElements(
    EnumerateElementsCallback *callback) {
  // This helper does nothing.
  delete callback;
  return true;
}

bool ScriptableHelperImpl::RemoveProperty(const char *name) {
  ASSERT(name);
  ASSERT(!registering_class_);

  EnsureRegistered();
  ASSERT(class_property_info_);

  PropertyInfoMap::iterator it = property_info_.find(name);
  if (it == property_info_.end())
    return false;
  DestroyPropertyInfo(&it->second);
  property_info_.erase(it);
  return true;
}

} // namespace internal

} // namespace ggadget
