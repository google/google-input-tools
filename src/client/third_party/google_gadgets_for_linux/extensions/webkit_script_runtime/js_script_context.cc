/*
  Copyright 2009 Google Inc.

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

#include "js_script_context.h"

#include <set>
#include <cstdlib>
#include <sys/types.h>
#include <signal.h>
#include <ggadget/logger.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/slot.h>
#include <ggadget/small_object.h>
#include <ggadget/string_utils.h>
#include <ggadget/scriptable_function.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/light_map.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/js/jscript_massager.h>
#include "js_script_runtime.h"
#include "converter.h"
#include "json.h"

#ifdef _DEBUG
// Uncomment the following to get verbose debug messages.
//#define DEBUG_JS_VERBOSE
//#define DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
//#define DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
//#define DEBUG_JS_VERBOSE_JS_FUNCTION_SLOT
//#define DEBUG_FORCE_GC
#endif


// TODO:
// Current JavaScriptCore API doesn't support these features.
// 1. GetCurrentFileAndLine()
// 2. ScriptBlockedFeedback()

namespace ggadget {
namespace webkit {

class JSScriptContext::Impl : public SmallObject<> {
  // A class to hold necessary information of a Scriptable to JSObject wrapper.
  // Its instance will be attached to wrapper JSObject as private data.
  // scriptable js wrapper class is in charge of creating and destroying it.
  class ScriptableJSWrapper : public SmallObject<> {
   public:
    ScriptableJSWrapper()
      : impl(NULL), scriptable(NULL), on_reference_change_connection(NULL) {
    }

    Impl *impl;
    ScriptableInterface *scriptable;
    Connection *on_reference_change_connection;
  };

  // A class to hold necessary information to call a method of a Scriptable.
  class ScriptableMethodCaller : public SmallObject<> {
   public:
    ScriptableMethodCaller(Impl *a_impl, const char *name, Slot *slot)
      : impl(a_impl), method_name(name), function_slot(slot) { }
    Impl *impl;
    std::string method_name;
    Slot *function_slot;
  };

  // Forward declaration.
  class JSScriptableWrapper;
  class JSFunctionSlot;

  // A class to hold necessary information of a JSObject to native wrapper.
  // A JSObject can be wrapped into a Scriptable object or a native Slot.
  // This object will be stored as private data of a JSObjectTrackerClass
  // JavaScript object, which is hooked to the JSObject being wrapped to track
  // its lifetime.
  class JSObjectTracker : public SmallObject<> {
   public:
    JSObjectTracker() : scriptable_wrapper(NULL) { }
    // On JSObject can only have one JSScriptableWrapper object,
    // but a function JSObject may be wrapped into multiple native slots.
    JSScriptableWrapper *scriptable_wrapper;
    Signal0<void> slot_detach_signal;
  };

  // A class to hold information of a native class constructor.
  class ClassConstructorData : public SmallObject<> {
   public:
    ClassConstructorData()
      : impl(NULL), class_name(NULL), constructor(NULL) { }
    ~ClassConstructorData() { delete constructor; }
    Impl *impl;
    const char *class_name;
    Slot *constructor;
  };

  // A class to cache JSStringRef to UTF8 conversion result.
  class JSStringUTF8Accessor {
   public:
    JSStringUTF8Accessor(JSStringRef js_str) : result_(NULL) {
      size_t max_size = JSStringGetMaximumUTF8CStringSize(js_str);
      if (max_size <= kCacheSize) {
        JSStringGetUTF8CString(js_str, fixed_cache_, kCacheSize);
        result_ = fixed_cache_;
      } else {
        dynamic_cache_ = ConvertJSStringToUTF8(js_str);
        result_ = dynamic_cache_.c_str();
        DLOG("JSStringUTF8Accessor: Too long: %s", result_);
      }
    }

    const char *Get() const { return result_; }

   private:
    // 128 bytes should be enough for most cases.
    static const size_t kCacheSize = 128;
    char fixed_cache_[kCacheSize];
    std::string dynamic_cache_;
    const char *result_;
  };

  // A class to wrap a JSObject into a native Scriptable object.
  // It implements ScriptableInterface directly to save memory space.
  // Normally, it'll always be destroyed along with wrapped JSObject.
  class JSScriptableWrapper : public ScriptableInterface {
   public:
    DEFINE_CLASS_ID(0xde065d3e3f9e4f37, ScriptableInterface);

    JSScriptableWrapper(Impl *impl, JSObjectRef object)
      : impl_(impl),
        object_(object),
        call_self_slot_(NULL),
        ref_count_(0) {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      ScopedLogContext log_context(impl_->owner_);
      DLOG("JSScriptableWrapper(impl=%p, ctx=%p, this=%p, jsobj=%p)",
           impl_, impl_->GetContext(), this, object_);
#endif
      ASSERT(object_);
      // Count the current JavaScript reference.
      Ref();
      ASSERT(GetRefCount() == 1);
      if (JSObjectIsFunction(context(), object_)) {
        // This object can be called as a function.
        call_self_slot_ = new JSFunctionSlot(impl_, NULL, NULL, this, object_);
      }

      // Record this wrapper, so that it can be detached when the context is
      // being destroyed.
      impl_->js_scriptable_wrappers_.insert(this);
    }

    // It'll be called when JSObject is garbage collected.
    // See WrapJSObject() and TrackerFinalizeCallback() below.
    void DetachJS() {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      ScopedLogContext log_context(impl_->owner_);
      DLOG("JSScriptableWrapper::DetachJS(impl=%p, ctx=%p, this=%p, jsobj=%p)",
           impl_, impl_->GetContext(), this, object_);
#endif
      if (object_) {
        // Clear it first to prevent DetachJS() from being called recursively.
        object_ = NULL;
        impl_->js_scriptable_wrappers_.erase(this);
        delete call_self_slot_;
        call_self_slot_ = NULL;
        RemoveAllSlots();
        // Remove JavaScript reference.
        Unref(false);
      }
    }

   protected:
    virtual ~JSScriptableWrapper() {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      ScopedLogContext log_context(impl_->owner_);
      DLOG("~JSScriptableWrapper(impl=%p, ctx=%p, this=%p, jsobj=%p)",
           impl_, impl_->GetContext(), this, object_);
#endif
      // Emit the ondelete signal, as early as possible.
      on_reference_change_signal_(0, 0);
      ASSERT(ref_count_ == 0);
      // DetachJS() must already be called.
      ASSERT(call_self_slot_ == NULL);
      ASSERT(method_slots_.size() == 0);
    }

   public:
    virtual void Ref() const {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      ScopedLogContext log_context(impl_->owner_);
      DLOG("JSScriptableWrapper::Ref(impl=%p, ctx=%p, this=%p, jsobj=%p, "
           "ref=%d)", impl_, impl_->GetContext(), this, object_, ref_count_);
#endif
      ASSERT(ref_count_ >= 0);
      on_reference_change_signal_(ref_count_, 1);
      ref_count_++;

      if (object_ && ref_count_ == 2) {
        // There must be a new native reference, JavaScript object
        // must be protected to avoid being garbage collected.
        JSValueProtect(context(), object_);
      }
    }

    virtual void Unref(bool transient) const {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      ScopedLogContext log_context(impl_->owner_);
      DLOG("JSScriptableWrapper::Unref(impl=%p, ctx=%p, this=%p, jsobj=%p, "
           "ref=%d, trans=%d)", impl_, impl_->GetContext(), this, object_,
           ref_count_, transient);
#endif
      on_reference_change_signal_(ref_count_, -1);
      ref_count_--;
      ASSERT(ref_count_ >= 0);

      if (object_ && ref_count_ == 1) {
        // The last native reference has been released, unprotect
        // JavaScript object to allow it being garbage collected.
        JSValueUnprotect(context(), object_);
#ifdef DEBUG_FORCE_GC
        // It might cause Unref() being called recursively.
        impl_->CollectGarbage();
#endif
      } else if (!transient && ref_count_ == 0) {
        delete this;
      }
    }

    virtual int GetRefCount() const {
      return ref_count_;
    }

    virtual bool IsStrict() const {
      return false;
    }

    virtual bool IsEnumeratable() const {
      return true;
    }

    virtual Connection *ConnectOnReferenceChange(
        Slot2<void, int, int> *slot) {
      return on_reference_change_signal_.Connect(slot);
    }

    virtual PropertyType GetPropertyInfo(const char *name, Variant *prototype) {
      ScopedLogContext log_context(impl_->owner_);
      PropertyType result = ScriptableInterface::PROPERTY_NOT_EXIST;
      if (object_) {
        if (name && *name) {
          JSStringRef js_name = JSStringCreateWithUTF8CString(name);
          if (JSObjectHasProperty(context(), object_, js_name)) {
            // Always return PROPERTY_DYNAMIC for existing properties,
            // because it's impossible to know the data type without getting
            // the property.
            result = ScriptableInterface::PROPERTY_DYNAMIC;
            if (prototype)
              *prototype = Variant(Variant::TYPE_VARIANT);
          }
          JSStringRelease(js_name);
        } else if (call_self_slot_) {
          if (prototype)
            *prototype = Variant(call_self_slot_);
          result = ScriptableInterface::PROPERTY_METHOD;
        }
      }
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::GetPropertyInfo(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p, prop=%s, result=%d)", impl_, impl_->GetContext(), this,
           object_, name, result);
#endif
      return result;
    }

    virtual ResultVariant GetProperty(const char *name) {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::GetProperty(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p, prop=%s)", impl_, impl_->GetContext(), this, object_,
           name);
#endif
      Variant result;
      if (object_) {
        if (name && *name) {
          JSValueRef js_val = NULL;
          // function object will be returned as slot.
          // See ConvertPropertyToNative() below.
          if (impl_->GetJSObjectProperty(object_, name, &js_val) &&
              !ConvertPropertyToNative(js_val, &result)) {
            DLOG("Failed to convert JS property %s value(%s) to native.",
                 name, PrintJSValue(impl_->owner_, js_val).c_str());
          }
        } else if (call_self_slot_) {
          result = Variant(call_self_slot_);
        }
      }
      return ResultVariant(result);
    }

    virtual bool SetProperty(const char *name, const Variant &value) {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::SetProperty(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p, prop=%s, value=%s)", impl_, impl_->GetContext(), this,
           object_, name, value.Print().c_str());
#endif
      if (!object_ || !name || !*name)
        return false;

      // Can't set a slot back to javascript side.
      ASSERT(value.type() != Variant::TYPE_SLOT);
      JSValueRef js_val = NULL;
      if (!ConvertNativeToJS(impl_->owner_, value, &js_val)) {
        DLOG("Failed to convert native property %s value(%s) to jsval.",
             name, value.Print().c_str());
        return false;
      }

      JSStringRef js_name = JSStringCreateWithUTF8CString(name);
      JSValueRef exception = NULL;
      JSObjectSetProperty(context(), object_, js_name, js_val, 0, &exception);
      JSStringRelease(js_name);
      return impl_->CheckJSException(exception);
    }

    virtual ResultVariant GetPropertyByIndex(int index) {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::GetPropertyByIndex(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p, idx=%d)", impl_, impl_->GetContext(), this, object_,
           index);
#endif
      if (!object_)
        return ResultVariant(Variant());

      if (index < 0)
        return GetProperty(StringPrintf("%d", index).c_str());

      unsigned uidx = static_cast<unsigned>(index);
      JSValueRef exception = NULL;
      JSValueRef js_val =
          JSObjectGetPropertyAtIndex(context(), object_, uidx, &exception);

      Variant result;
      if (impl_->CheckJSException(exception) &&
          !ConvertPropertyToNative(js_val, &result)) {
        DLOG("Failed to convert JS property %d value(%s) to native.",
             index, PrintJSValue(impl_->owner_, js_val).c_str());
      }
      return ResultVariant(result);
    }

    virtual bool SetPropertyByIndex(int index, const Variant &value) {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::SetPropertyByIndex(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p, idx=%d, value=%s)", impl_, impl_->GetContext(), this,
           object_, index, value.Print().c_str());
#endif
      if (!object_)
        return false;

      if (index < 0)
        return SetProperty(StringPrintf("%d", index).c_str(), value);

      // Can't set a slot back to javascript side.
      ASSERT(value.type() != Variant::TYPE_SLOT);
      JSValueRef js_val = NULL;
      if (!ConvertNativeToJS(impl_->owner_, value, &js_val)) {
        DLOG("Failed to convert native property %d value(%s) to jsval.",
             index, value.Print().c_str());
        return false;
      }

      unsigned uidx = static_cast<unsigned>(index);
      JSValueRef exception = NULL;
      JSObjectSetPropertyAtIndex(context(), object_, uidx, js_val, &exception);
      return impl_->CheckJSException(exception);
    }

    virtual ScriptableInterface *GetPendingException(bool clear) {
      // FIXME: Unimplemented. Is it necessary?
      return NULL;
    }

    virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback) {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::EnumerateProperties(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p)", impl_, impl_->GetContext(), this, object_);
#endif
      ASSERT(callback);
      bool result = true;
      if (object_) {
        JSPropertyNameArrayRef name_array =
            JSObjectCopyPropertyNames(context(), object_);
        size_t count = JSPropertyNameArrayGetCount(name_array);
        for (size_t i = 0; i < count; ++i) {
          JSStringRef name = JSPropertyNameArrayGetNameAtIndex(name_array, i);
          JSStringUTF8Accessor utf8_name(name);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
          DLOG("  Enumerate: %s", utf8_name.Get());
#endif
          if (!(*callback)(utf8_name.Get(),
                           ScriptableInterface::PROPERTY_DYNAMIC,
                           GetProperty(utf8_name.Get()).v())) {
            result = false;
            break;
          }
        }
        JSPropertyNameArrayRelease(name_array);
      }
      delete callback;
      return result;
    }

    virtual bool EnumerateElements(EnumerateElementsCallback *callback) {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::EnumerateElements(impl=%p, ctx=%p, this=%p, "
           "jsobj=%p)", impl_, impl_->GetContext(), this, object_);
#endif
      ASSERT(callback);
      bool result = true;
      if (object_) {
        JSPropertyNameArrayRef name_array =
            JSObjectCopyPropertyNames(context(), object_);
        size_t count = JSPropertyNameArrayGetCount(name_array);
        for (size_t i = 0; i < count; ++i) {
          JSStringRef name = JSPropertyNameArrayGetNameAtIndex(name_array, i);
          JSStringUTF8Accessor utf8_name(name);
          int index;
          // Only enumerate properties with positive numeric index.
          if (!IsIndexProperty(utf8_name.Get(), &index))
              continue;
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
          DLOG("  Enumerate: %s", utf8_name.Get());
#endif
          if (!(*callback)(index, GetPropertyByIndex(index).v())) {
            result = false;
            break;
          }
        }
        JSPropertyNameArrayRelease(name_array);
      }
      delete callback;
      return result;
    }

    virtual RegisterableInterface *GetRegisterable() {
      return NULL;
    }

   public:
    JSObjectRef object() const { return object_; }
    JSContextRef context() const { return impl_->context_; }
    Impl *impl() const { return impl_; }

    void MethodRemoved(JSFunctionSlot *slot, JSObjectRef js_function) {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::MethodRemoved(impl=%p, ctx=%p, this=%p, "
           "slot=%p, jsfunc=%p)", impl_, impl_->GetContext(), this, slot,
           js_function);
#endif
      MethodSlotMap::iterator it = method_slots_.find(js_function);
      ASSERT(it != method_slots_.end());
      ASSERT(it->second == static_cast<void *>(slot));
      method_slots_.erase(it);
      delete slot;
    }

   private:
    bool ConvertPropertyToNative(JSValueRef js_val, Variant *native_val) {
      if (JSValueIsObject(context(), js_val)) {
        JSObjectRef js_obj = JSValueToObject(context(), js_val, NULL);
        ScriptableInterface *scriptable = impl_->UnwrapScriptable(js_obj);
        if (scriptable) {
          *native_val = Variant(scriptable);
          return true;
        } else if (JSObjectIsFunction(context(), js_obj)) {
          Slot *slot = NULL;
          MethodSlotMap::iterator it = method_slots_.find(js_obj);
          if (it != method_slots_.end()) {
            slot = static_cast<Slot *>(it->second);
          } else {
            slot =  new JSFunctionSlot(impl_, NULL, NULL, this, js_obj);
            method_slots_[js_obj] = slot;
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
            DLOG("JSScriptableWrapper::GetSlotProperty"
                 "(this=%p, slot=%p, jsfunc=%p)", this, slot, js_obj);
#endif
          }
          *native_val = Variant(slot);
          return true;
        }
      }
      return ConvertJSToNativeVariant(impl_->owner_, js_val, native_val);
    }

    void RemoveAllSlots() {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
      DLOG("JSScriptableWrapper::RemoveAllSlots(this=%p)", this);
#endif
      MethodSlotMap::iterator it = method_slots_.begin();
      for (; it != method_slots_.end(); ++it) {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
        DLOG("  DeleteMethodSlot(this=%p, slot=%p)",
             this, it->second);
#endif
        delete static_cast<Slot *>(it->second);
      }
      method_slots_.clear();
    }

   private:
    DISALLOW_EVIL_CONSTRUCTORS(JSScriptableWrapper);

    Signal2<void, int, int> on_reference_change_signal_;

    // first = function object property (JSObjectRef)
    // second = native slot wrapping the function object (JSFunctionSlot*)
    typedef LightMap<void *, void *> MethodSlotMap;
    MethodSlotMap method_slots_;

    Impl *impl_;
    JSObjectRef object_;
    Slot *call_self_slot_;
    mutable int ref_count_;
  };

  // A slot implementation to call a JavaScript function object.
  // It'll only be deleted from native side.
  // owner is the js object that this js function object shall be attached to.
  // parent is the JSScriptableWrapper object that owns this JSFunctionSlot
  // object.
  class JSFunctionSlot : public Slot {
   public:
    JSFunctionSlot(Impl *impl, const Slot *prototype, JSObjectRef owner,
                   JSScriptableWrapper *parent, JSObjectRef function)
      : impl_(impl), prototype_(prototype), owner_(owner), parent_(parent),
        function_(function), slot_detach_connection_(NULL),
        death_flag_ptr_(NULL) {
      ASSERT(function_);
      // A JS function object may be wrapped into multiple JSFunctionSlot
      // objects, which are owned by native code. When the JS function object
      // is being garbage collected, associated JSFunctionSlot objects will be
      // informed through slot_detach_signal, so that the connection between
      // wrappers and JS function object can be removed properly.
      JSObjectTracker *tracker = impl_->GetJSObjectTracker(function_);
      slot_detach_connection_ = tracker->slot_detach_signal.Connect(
          NewSlot(this, &JSFunctionSlot::DetachJS, true));

      // If it's a method of a JSScriptableWrapper, then no need protecting.
      if (!parent) {
        if (owner_) {
          // Attach the JS function object to its owner object, so that it will
          // be garbage collected along with its owner.
          JSPropertyAttributes attributes =
              kJSPropertyAttributeDontEnum  | kJSPropertyAttributeDontDelete;
          impl_->SetJSObjectProperty(owner_, GetHookName().c_str(),
                                     function_, attributes);
        } else {
          JSValueProtect(context(), function_);
        }
      }
      // Record this slot, so that it can be detached when the context is
      // being destroyed.
      // And when returning this slot from native to javascript, it can be
      // used to unwrap this slot.
      impl_->js_function_slots_.insert(this);
#ifdef DEBUG_JS_VERBOSE_JS_FUNCTION_SLOT
      ScopedLogContext log_context(impl_->owner_);
      DLOG("JSFunctionSlot(impl=%p, ctx=%p, this=%p, func=%p, owner=%p, "
           "parent=%p)", impl_, impl_->GetContext(), this, function_, owner_,
           parent_);
#endif
    }

    virtual ~JSFunctionSlot() {
#ifdef DEBUG_JS_VERBOSE_JS_FUNCTION_SLOT
      ScopedLogContext log_context(impl_->owner_);
      DLOG("~JSFunctionSlot(impl=%p, ctx=%p, this=%p, func=%p, owner=%p, "
           "parent=%p)", impl_, impl_->GetContext(), this, function_, owner_,
           parent_);
#endif
      // Set *death_flag_ to true to let Call() know this slot is to be deleted.
      if (death_flag_ptr_)
        *death_flag_ptr_ = true;

      DetachJS(false);
    }

    std::string GetHookName() const {
      return StringPrintf("[[[JSFS:%p]]]", this);
    }

    void DetachJS(bool js_function_object_destroyed) {
#ifdef DEBUG_JS_VERBOSE_JS_FUNCTION_SLOT
      ScopedLogContext log_context(impl_->owner_);
      DLOG("JSFunctionSlot::DetachJS(impl=%p, ctx=%p, this=%p, func=%p, "
           "owner=%p, %d)", impl_, impl_->GetContext(), this, function_,
           owner_, js_function_object_destroyed);
#endif
      if (function_) {
        JSObjectRef function = function_;
        // Clear it first to prevent DetachJS() from being called recursively.
        function_ = NULL;
        if (slot_detach_connection_) {
          slot_detach_connection_->Disconnect();
          slot_detach_connection_ = NULL;
        }

        impl_->js_function_slots_.erase(this);
        if (!parent_) {
          // JavaScript resource shall be cleaned up if the slot is deleted
          // from native side.
          if (!js_function_object_destroyed) {
            if (owner_)
              impl_->DeleteJSObjectProperty(owner_, GetHookName().c_str());
            else
              JSValueUnprotect(context(), function);
#ifdef DEBUG_FORCE_GC
            impl_->CollectGarbage();
#endif
          }
        } else if (js_function_object_destroyed) {
          // If js function is destroyed(GCed) before this JSFunctionSlot is
          // destroyed by its parent, then inform its parent to delete this
          // slot.
          parent_->MethodRemoved(this, function);
        }

        prototype_ = NULL;
        owner_ = NULL;
      }
    }

    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      ScopedLogContext log_context(impl_->owner_);
#ifdef DEBUG_JS_VERBOSE_JS_FUNCTION_SLOT
      DLOG("JSFunctionSlot::Call(impl=%p, ctx=%p, this=%p, scriptable=%p, "
           "func=%p)", impl_, impl_->GetContext(), this, object, function_);
#endif
      Variant return_value(GetReturnType());
      if (!function_) {
        DLOG("Finalized JavaScript function is still be called.");
        return ResultVariant(return_value);
      }

      scoped_array<JSValueRef> js_args;
      if (argc > 0) {
        js_args.reset(new JSValueRef[argc]);
        for (int i = 0; i < argc; ++i) {
          if (!ConvertNativeToJS(impl_->owner_, argv[i], &js_args[i])) {
            DLOG("Failed to convert argument %d(%s) to jsval.",
                 i, argv[i].Print().c_str());
            return ResultVariant(return_value);
          }
        }
      }

      bool death_flag = false;
      bool *death_flag_ptr = &death_flag;
      if (!death_flag_ptr_) {
        // Let the destructor inform us when this object is to be deleted.
        death_flag_ptr_ = death_flag_ptr;
      } else {
        // There must be some upper stack frame containing Call() call of the same
        // object. We just use the outer most death_flag_.
        death_flag_ptr = death_flag_ptr_;
      }

      JSObjectRef this_object = NULL;
      if (object && object->IsInstanceOf(JSScriptableWrapper::CLASS_ID))
        this_object = down_cast<JSScriptableWrapper *>(object)->object();

      JSValueRef exception = NULL;
      JSValueRef result = JSObjectCallAsFunction(context(), function_,
                                                 this_object, argc,
                                                 js_args.get(), &exception);
      if (!*death_flag_ptr) {
        if (death_flag_ptr == &death_flag)
          death_flag_ptr_ = NULL;

        if (result) {
          if (!ConvertJSToNative(impl_->owner_, this_object, return_value,
                                 result, &return_value)) {
            DLOG("Failed to convert JS function return value(%s) to native",
                 PrintJSValue(impl_->owner_, result).c_str());
          } else {
            // Must first hold return_value in a ResultValue, to prevent the
            // result from being deleted during GC.
            ResultVariant result(return_value);
            // Normal GC triggering doesn't work well if only little JS code is
            // executed but many native objects are referenced by dead JS
            // objects. Call MaybeGC to ensure GC is not starved.
            impl_->MaybeGC();
            return result;
          }
        } else {
          impl_->CheckJSException(exception);
        }
      }

#ifdef DEBUG_JS_VERBOSE_JS_FUNCTION_SLOT
      DLOG("JSFunctionSlot::Call(impl=%p, ctx=%p, this=%p, this_obj=%p) "
           "result=%s", impl_, impl_->GetContext(), this, this_object,
           return_value.Print().c_str());
#endif
      return ResultVariant(return_value);
    }

    virtual bool HasMetadata() const {
      return prototype_ != NULL;
    }

    virtual Variant::Type GetReturnType() const {
      return prototype_ ? prototype_->GetReturnType() : Variant::TYPE_VARIANT;
    }

    virtual int GetArgCount() const {
      return prototype_ ? prototype_->GetArgCount() : 0;
    }

    virtual const Variant::Type *GetArgTypes() const {
      return prototype_ ? prototype_->GetArgTypes() : NULL;
    }

    virtual bool operator==(const Slot &another) const {
      return function_ ==
             down_cast<const JSFunctionSlot *>(&another)->function_;
    }

   public:
    JSContextRef context() const { return impl_->context_; }
    JSObjectRef function() const { return function_; }
    JSScriptableWrapper *parent() const { return parent_; }

   private:
    Impl *impl_;
    const Slot *prototype_;
    JSObjectRef owner_;
    JSScriptableWrapper *parent_;
    JSObjectRef function_;
    Connection *slot_detach_connection_;

    // This slot object may be deleted during Call(). In Call(), This pointer
    // should point to a local bool variable, and once *death_flag_ptr_ becomes
    // true, Call() should return immediately.
    mutable bool *death_flag_ptr_;
  };

  // A class to track JavaScript context's stack.
  class JSContextScope {
   public:
    JSContextScope(Impl *impl, JSContextRef context)
      : impl_(impl),
        saved_context_(impl->context_),
        log_context_(impl->owner_) {
      impl_->context_ = context;
    }

    ~JSContextScope() {
      impl_->context_ = saved_context_;
    }

   private:
    Impl *impl_;
    JSContextRef saved_context_;
    ScopedLogContext log_context_;
  };

 public:
  Impl(JSScriptContext *owner, JSScriptRuntime *runtime,
       JSContextRef js_context)
    : owner_(owner),
      runtime_(runtime),
      context_(js_context),
      scriptable_js_wrapper_class_(NULL),
      scriptable_method_caller_class_(NULL),
      js_object_tracker_class_(NULL),
      class_constructor_class_(NULL),
      js_object_tracker_reference_name_(NULL),
      is_nan_func_(NULL),
      is_finite_func_(NULL),
      last_gc_time_(0) {
    ScopedLogContext log_context(owner_);
    ASSERT(runtime_);

    // These class refs will be released along with runtime.
    scriptable_js_wrapper_class_ =
        runtime_->GetClassRef(&kScriptableJSWrapperClassDefinition);
    ASSERT(scriptable_js_wrapper_class_);
    scriptable_method_caller_class_ =
        runtime_->GetClassRef(&kScriptableMethodCallerClassDefinition);
    ASSERT(scriptable_method_caller_class_);
    js_object_tracker_class_ =
        runtime_->GetClassRef(&kJSObjectTrackerClassDefinition);
    ASSERT(js_object_tracker_class_);
    class_constructor_class_ =
        runtime_->GetClassRef(&kClassConstructorClassDefinition);

    js_object_tracker_reference_name_ =
        JSStringCreateWithUTF8CString(kJSObjectTrackerReferenceName);
    ASSERT(js_object_tracker_reference_name_);

    if (!context_)
      context_ = JSGlobalContextCreate(NULL);
    else
      JSGlobalContextRetain(const_cast<JSGlobalContextRef>(context_));

    // Native global object will be attached to the default javascript global
    // oblect as its prototype, so no need to use special class here.
    // See SetGlobalObject() below for details.
    //context_ = JSGlobalContextCreate(scriptable_js_wrapper_class_);
    ASSERT(context_);

    is_nan_func_ = GetFunctionOrConstructorObject(NULL, "isNaN");
    ASSERT(is_nan_func_);
    is_finite_func_ = GetFunctionOrConstructorObject(NULL, "isFinite");
    ASSERT(is_finite_func_);
    date_class_obj_ = GetFunctionOrConstructorObject(NULL, "Date");
    ASSERT(date_class_obj_);
    array_class_obj_ = GetFunctionOrConstructorObject(NULL, "Array");
    ASSERT(array_class_obj_);

    DLOG("Create JSScriptContext: impl=%p, ctx=%p", this, context_);
  }

#ifdef _DEBUG
  void PrintRemainedObjectsInfo() {
    DLOG("Remained:\n"
         "  ScriptableJSWrapper : %zu\n"
         "  JSFunctionSlot : %zu\n"
         "  JSScriptableWrapper : %zu",
         scriptable_js_wrappers_.size(),
         js_function_slots_.size(),
         js_scriptable_wrappers_.size());
#ifdef DEBUG_JS_VERBOSE
    for (ScriptableJSWrapperMap::iterator it = scriptable_js_wrappers_.begin();
         it != scriptable_js_wrappers_.end(); ++it) {
      DLOG("  ScriptableJSWrapper: ClassId: %jx, Ref: %d, %p",
           static_cast<ScriptableInterface *>(it->first)->GetClassId(),
           static_cast<ScriptableInterface *>(it->first)->GetRefCount(),
           it->first);
    }
    for (JSFunctionSlotMap::iterator it = js_function_slots_.begin();
         it != js_function_slots_.end() ; ++it) {
      DLOG("  JSFunctionSlot: Parent: %p, %p",
           static_cast<JSFunctionSlot *>(*it)->parent(), *it);
    }
    for (JSScriptableWrapperMap::iterator it = js_scriptable_wrappers_.begin();
         it != js_scriptable_wrappers_.end(); ++it) {
      DLOG("  JSScriptableWrapper: Ref: %d, %p",
           static_cast<ScriptableInterface *>(*it)->GetRefCount(), *it);
    }
#endif
  }
#endif

  ~Impl() {
    ScopedLogContext log_context(owner_);
    DLOG("Destroy JSScriptContext: impl=%p, ctx=%p", this, context_);
    CollectGarbage();
#ifdef _DEBUG
    PrintRemainedObjectsInfo();
#endif
    // Detach JSFunctionSlot first, because they may still attached to
    // js objects which are wrapped by ScriptableJSWrapper objects.
    while(!js_function_slots_.empty()) {
      JSFunctionSlotMap::iterator it = js_function_slots_.begin();
      static_cast<JSFunctionSlot *>(*it)->DetachJS(false);
    }
    while(!scriptable_js_wrappers_.empty()) {
      ScriptableJSWrapperMap::iterator it = scriptable_js_wrappers_.begin();
      DetachScriptable(static_cast<JSObjectRef>(it->second), false);
    }
    while(!js_scriptable_wrappers_.empty()) {
      JSScriptableWrapperMap::iterator it = js_scriptable_wrappers_.begin();
      static_cast<JSScriptableWrapper *>(*it)->DetachJS();
    }
    JSGlobalContextRelease(const_cast<JSGlobalContextRef>(context_));
    JSStringRelease(js_object_tracker_reference_name_);
  }

  void Execute(const char *script, const char *filename, int lineno) {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::Execute(impl=%p, ctx=%p, script=%s, file=%s, "
         "line=%d)", this, context_, script, filename, lineno);
#endif
    ASSERT(script && *script);
    std::string massaged_script =
        ggadget::js::MassageJScript(script, false, filename, lineno);
    JSStringRef js_script =
        JSStringCreateWithUTF8CString(massaged_script.c_str());
    JSStringRef source_url = NULL;
    if (filename && *filename)
      source_url = JSStringCreateWithUTF8CString(filename);
    JSValueRef exception = NULL;
    JSEvaluateScript(context_, js_script,
                     JSContextGetGlobalObject(context_),
                     source_url, lineno, &exception);
    JSStringRelease(js_script);
    if (source_url)
      JSStringRelease(source_url);
    CheckJSException(exception);
  }

  Slot *Compile(const char *script, const char *filename, int lineno) {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::Compile(impl=%p, ctx=%p, script=%s, file=%s, "
         "line=%d)", this, context_, script, filename, lineno);
#endif
    if (script && *script) {
      JSValueRef exception = NULL;
      JSObjectRef js_function =
          CompileFunction(owner_, script, filename, lineno, &exception);
      if (js_function && CheckJSException(exception))
        return WrapJSObjectIntoSlot(NULL, NULL, js_function);
    }
    return NULL;
  }

  bool SetGlobalObject(ScriptableInterface *global) {
    ScopedLogContext log_context(owner_);
    ASSERT(global);
    JSObjectRef global_object = JSContextGetGlobalObject(context_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::SetGlobalObject(impl=%p, ctx=%p, global=%p, "
         "js_global=%p)", this, context_, global, global_object);
#endif
    // Add some adapters for JScript.
    // JScript programs call Date.getVarDate() to convert a JavaScript Date to
    // a COM VARDATE. We just use Date's where VARDATE's are expected by JScript
    // programs.
    JSObjectRef date_prototype = JSValueToObject(
        context_, JSObjectGetPrototype(context_, date_class_obj_), NULL);
    ASSERT(date_prototype);
    RegisterObjectMethod(date_prototype, "getVarDate", ReturnSelfFunc);

    // For Windows compatibility.
    RegisterObjectMethod(NULL, "CollectGarbage", CollectGarbageFunc);

#ifdef _DEBUG
    // For debug purpose.
    RegisterObjectMethod(NULL, "Interrupt", InterruptFunc);
#endif

    // There is still no way to support "name overriding", so binding native
    // global object directly to javascript object will break some gadgets that
    // depends on "name overriding", such as GMail gadget.
    // As a ugly workaround, attaching native global object to javascript global
    // object as its prototype can emulate a strong "name overriding" behavior.
    // Then all properties of native global object becomes "read only" and will
    // be overridden by either variable declaration statement or assignment
    // statement which uses the same variable name as the property
    // (case sensitive).
    // Fortunately, this workaround works for most gadgets.
#if 0
    ASSERT(global_object);
    AttachScriptable(global_object, global);
#endif
    JSValueRef global_prototype;
    if (!ConvertNativeToJS(owner_, Variant(global), &global_prototype)) {
      DLOG("Failed to convert global object into javascript object.");
      return false;
    }
    JSObjectSetPrototype(context_, global_object, global_prototype);

    return true;
  }

  bool RegisterClass(const char *name, Slot *constructor) {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::RegisterClass(impl=%p, ctx=%p, name=%s, ctor=%p)",
         this, context_, name, constructor);
#endif
    ASSERT(name && *name);
    ASSERT(constructor);
    JSObjectRef global_object = JSContextGetGlobalObject(context_);
    ASSERT(global_object);

    ClassConstructorData *data = new ClassConstructorData();
    data->impl = this;
    data->class_name = name;
    data->constructor = constructor;
    JSObjectRef class_object =
        JSObjectMake(context_, class_constructor_class_, data);
    ASSERT(class_object);

    return SetJSObjectProperty(global_object, name, class_object, 0);
  }

  bool AssignFromContext(ScriptableInterface *dest_object,
                         const char *dest_object_expr,
                         const char *dest_property,
                         ScriptContextInterface *src_context,
                         ScriptableInterface *src_object,
                         const char *src_expr) {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::AssignFromContext(impl=%p, ctx=%p, dest_obj=%p,"
         " dest_expr=%s, dest_prop=%s, src_c_ctx=%p, src_obj=%p, src_expr=%s)",
         this, context_, dest_object, dest_object_expr, dest_property,
         src_context, src_object, src_expr);
#endif
    ASSERT(src_context);
    ASSERT(dest_property);

    JSValueRef dest_js_val = NULL;
    if (!EvaluateToJSValue(dest_object, dest_object_expr, &dest_js_val) ||
        !dest_js_val || !JSValueIsObject(context_, dest_js_val) ||
        JSValueIsNull(context_, dest_js_val)) {
      DLOG("Expression %s doesn't evaluate to a non-null object",
           dest_object_expr);
      return false;
    }

    JSValueRef exception = NULL;
    JSObjectRef dest_js_obj =
        JSValueToObject(context_, dest_js_val, &exception);
    if (dest_js_obj && CheckJSException(exception)) {
      JSValueRef src_js_val = NULL;
      JSScriptContext *src_js_context =
          down_cast<JSScriptContext *>(src_context);
      if (src_js_context->impl_->EvaluateToJSValue(src_object, src_expr,
                                                   &src_js_val)) {
        Variant native_val;
        // Current stable JavaScriptCore doesn't support sharing a js value
        // among multiple js context. So it's necessary to wrap the value
        // twice.
        if (ConvertJSToNativeVariant(src_js_context, src_js_val, &native_val) &&
            ConvertNativeToJS(owner_, native_val, &dest_js_val)) {
          return SetJSObjectProperty(dest_js_obj, dest_property,
                                     dest_js_val, 0);
        }
      }
    }
    return false;
  }

  bool AssignFromNative(ScriptableInterface *object,
                        const char *object_expr,
                        const char *property,
                        const Variant &value) {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::AssignFromNative(impl=%p, ctx=%p, obj=%p, expr=%s, "
         "prop=%s, value=%s)", this, context_, object, object_expr, property,
         value.Print().c_str());
#endif
    ASSERT(property);
    JSValueRef js_val = NULL;
    if (!EvaluateToJSValue(object, object_expr, &js_val) || !js_val ||
        !JSValueIsObject(context_, js_val) || JSValueIsNull(context_, js_val)) {
      DLOG("Expression %s doesn't evaluate to a non-null object", object_expr);
      return false;
    }
    JSValueRef exception = NULL;
    JSObjectRef js_obj = JSValueToObject(context_, js_val, &exception);
    if (js_obj && CheckJSException(exception)) {
      JSValueRef prop_val = NULL;
      if (ConvertNativeToJS(owner_, value, &prop_val) && prop_val)
        return SetJSObjectProperty(js_obj, property, prop_val, 0);
    }
    return false;
  }

  Variant Evaluate(ScriptableInterface *object, const char *expr) {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::Evaluate(impl=%p, ctx=%p, obj=%p, expr=%s)",
         this, context_, object, expr);
#endif
    Variant result;
    JSValueRef js_val = NULL;
    if (EvaluateToJSValue(object, expr, &js_val) && js_val) {
      ConvertJSToNativeVariant(owner_, js_val, &result);
      // Just left result in void state on any error.
    }
    return result;
  }

  void CollectGarbage() {
    ScopedLogContext log_context(owner_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::CollectGarbage(impl=%p, ctx=%p) start", this, context_);
#endif
    JSGarbageCollect(context_);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::CollectGarbage(impl=%p, ctx=%p) end", this, context_);
#endif
  }

  void MaybeGC() {
    MainLoopInterface *main_loop = GetGlobalMainLoop();
    uint64_t now = main_loop ? main_loop->GetCurrentTime() : 0;
    if (now - last_gc_time_ > kMaxGCInterval) {
#ifdef DEBUG_JS_VERBOSE
      DLOG("JSScriptContext::MaybeGC(impl=%p, ctx=%p): GC Triggered: %ju",
           this, context_, now);
#endif
      JSGarbageCollect(context_);
      last_gc_time_ = now;
#ifdef DEBUG_JS_VERBOSE
      DLOG("JSScriptContext::MaybeGC(impl=%p, ctx=%p): GC Finished: %ju",
           this, context_, now);
#endif
    }
  }

  void GetCurrentFileAndLine(std::string *filename, int *lineno) {
    // FIXME
    if (filename)
      *filename = "unknown";
    if (lineno)
      *lineno = -1;
  }

  // Don't unwrap a JSScriptableWrapper, because sharing a JSObjectRef
  // across contexts is not supported.
  JSObjectRef WrapScriptable(ScriptableInterface *scriptable) {
    if (!scriptable) return NULL;
    JSObjectRef object = GetJSWrapperFromScriptable(scriptable);
    if (!object) {
      object = JSObjectMake(context_, scriptable_js_wrapper_class_, NULL);
      AttachScriptable(object, scriptable);
    }
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::WrapScriptable(impl=%p, ctx=%p, scriptable=%p, "
         "object=%p)", this, context_, scriptable, object);
#endif
    return object;
  }

  ScriptableInterface *WrapJSObject(JSObjectRef object) {
    if (!object) return NULL;
    if (IsWrapperOfScriptable(object))
      return GetScriptable(object);

    JSObjectTracker *tracker = GetJSObjectTracker(object);
    if (!tracker->scriptable_wrapper)
      tracker->scriptable_wrapper = new JSScriptableWrapper(this, object);
#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::WrapJSObject(impl=%p, ctx=%p, js_obj=%p, "
         "scriptable=%p)", this, context_, object, tracker->scriptable_wrapper);
#endif
    return tracker->scriptable_wrapper;
  }

  JSFunctionSlot *WrapJSObjectIntoSlot(const Slot *prototype,
                                       JSObjectRef owner,
                                       JSObjectRef object) {
    if (JSObjectIsFunction(context_, object))
      return new JSFunctionSlot(this, prototype, owner, NULL, object);
    return NULL;
  }

  ScriptableInterface *UnwrapScriptable(JSObjectRef object) {
    if (IsWrapperOfScriptable(object))
      return GetScriptable(object);
    return NULL;
  }

  JSObjectRef UnwrapJSObject(ScriptableInterface *scriptable) {
    if (IsWrapperOfJSObject(scriptable))
      return down_cast<JSScriptableWrapper *>(scriptable)->object();
    return NULL;
  }

  bool UnwrapJSFunctionSlot(Slot *slot, JSValueRef *js_func) {
    if (slot && js_function_slots_.count(slot)) {
      *js_func = down_cast<JSFunctionSlot *>(slot)->function();
      if (!*js_func)
        *js_func = JSValueMakeNull(context_);
      return true;
    }
    return false;
  }

  bool IsWrapperOfScriptable(JSObjectRef object) {
    return object && JSValueIsObjectOfClass(
        context_, object, scriptable_js_wrapper_class_);
  }

  bool IsWrapperOfJSObject(ScriptableInterface *scriptable) {
    return scriptable &&
        scriptable->IsInstanceOf(JSScriptableWrapper::CLASS_ID) &&
        down_cast<JSScriptableWrapper *>(scriptable)->impl() == this;
  }

  bool CheckJSException(JSValueRef exception) {
    if (exception) {
      std::string msg;
      JSONEncode(owner_, exception, &msg);
      LOGE("JSException: %s", msg.c_str());
      return false;
    }
    return true;
  }

  bool CheckScriptableException(ScriptableInterface *scriptable,
                                JSValueRef *exception) {
    ScriptableInterface *scriptable_exception = NULL;
    if (scriptable) {
      scriptable_exception = scriptable->GetPendingException(true);
    }
    if (scriptable_exception) {
      scriptable_exception->Ref();
      Variant to_string_proto;
      std::string msg_str("unknown");
      if (scriptable_exception->GetPropertyInfo("toString", &to_string_proto) ==
          ScriptableInterface::PROPERTY_METHOD) {
        Slot *to_string_slot = VariantValue<Slot *>()(to_string_proto);
        if (to_string_slot) {
          ResultVariant msg =
              to_string_slot->Call(scriptable_exception, 0, NULL);
          msg.v().ConvertToString(&msg_str);
        }
      }
      LOGE("NativeException: [obj:%p ID:%jx exception:%p]: %s",
           scriptable, scriptable->GetClassId(),
           scriptable_exception, msg_str.c_str());
      if (exception) {
        *exception = WrapScriptable(scriptable_exception);
      }
      scriptable_exception->Unref();
      return false;
    }
    return true;
  }

  JSScriptRuntime *GetRuntime() {
    return runtime_;
  }

  JSContextRef GetContext() {
    return context_;
  }

  Connection *ConnectScriptBlockedFeedback(ScriptBlockedFeedback *feedback) {
    return script_blocked_signal_.Connect(feedback);
  }

  bool IsNaN(JSValueRef value) {
    if (!value || JSValueIsUndefined(context_, value) ||
        JSValueIsNull(context_, value))
      return true;

    if (is_nan_func_) {
      JSValueRef result = JSObjectCallAsFunction(context_, is_nan_func_,
                                                 NULL, 1, &value, NULL);
      return JSValueToBoolean(context_, result);
    }
    return false;
  }

  bool IsFinite(JSValueRef value) {
    if (!value || JSValueIsUndefined(context_, value) ||
        JSValueIsNull(context_, value))
      return false;

    if (is_finite_func_) {
      JSValueRef result = JSObjectCallAsFunction(context_, is_finite_func_,
                                                 NULL, 1, &value, NULL);
      return JSValueToBoolean(context_, result);
    }
    return true;
  }

  bool IsDate(JSValueRef value) {
    JSValueRef exception = NULL;
    return JSValueIsInstanceOfConstructor(
        context_, value, date_class_obj_, &exception) &&
        CheckJSException(exception);
  }

  bool IsArray(JSValueRef value) {
    JSValueRef exception = NULL;
    return JSValueIsInstanceOfConstructor(
        context_, value, array_class_obj_, &exception) &&
        CheckJSException(exception);
  }

  unsigned int GetArrayLength(JSObjectRef array) {
    static JSStringRef length_name =
        JSStringCreateWithUTF8CString("length");

    JSValueRef exception = NULL;
    JSValueRef js_length =
        JSObjectGetProperty(context_, array, length_name, &exception);
    if (CheckJSException(exception) && JSValueIsNumber(context_, js_length)) {
      double length = JSValueToNumber(context_, js_length, NULL);
      return length >= 0 ? static_cast<unsigned int>(length) : 0;
    }
    return 0;
  }

  void RegisterObjectMethod(JSObjectRef object, const char *name,
                            JSObjectCallAsFunctionCallback callback) {
    ASSERT(name && *name);
    ASSERT(callback);
    if (!object)
      object = JSContextGetGlobalObject(context_);

    JSClassDefinition function_class_def = kJSClassDefinitionEmpty;
    function_class_def.callAsFunction = callback;
    function_class_def.attributes = kJSClassAttributeNoAutomaticPrototype;
    JSClassRef function_class = JSClassCreate(&function_class_def);
    JSObjectRef js_function =
        JSObjectMake(context_, function_class, owner_);
    SetJSObjectProperty(object, name, js_function, 0);
    JSClassRelease(function_class);
  }

 private:
  // Attachs a given Scriptable object to a JSObjectRef object.
  // The JSObjectRef object must be an instance of ScriptableJSWrapper class.
  void AttachScriptable(JSObjectRef object, ScriptableInterface *scriptable) {
    ScopedLogContext log_context(owner_);
    ASSERT(IsWrapperOfScriptable(object));
    ASSERT(scriptable);

    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    ASSERT(wrapper);
    ASSERT(wrapper->impl == NULL || wrapper->impl == this);

    if (wrapper->scriptable) {
      if (wrapper->scriptable != scriptable) {
        DetachScriptable(object, false);
      } else {
#ifdef DEBUG_JS_VERBOSE
        DLOG("Scriptable already attached: impl=%p, ctx=%p, jsobj=%p, "
             "scriptable=%p", this, context_, object, scriptable);
#endif
        return;
      }
    }

#ifdef DEBUG_JS_VERBOSE
    DLOG("JSScriptContext::AttachScriptable(impl=%p, ctx=%p, jsobj=%p, "
         "scriptable=%p)", this, context_, object, scriptable);
#endif
    // It's harmless and simple to reset wrapper->impl every time.
    // As we don't support accessing an JS object in multiple contexts.
    wrapper->impl = this;
    wrapper->scriptable = scriptable;

    if (scriptable->GetRefCount() > 0) {
      // There must be at least one native reference,
      // JavaScript wrapper shall be protected.
#ifdef DEBUG_JS_VERBOSE
      DLOG("Protect Object: impl=%p, ctx=%p, jsobj=%p, scriptable=%s",
           this, context_, object, GetJSWrapperObjectName(object).c_str());
#endif
      JSValueProtect(context_, object);
    }

    // Lets the wrapper hold a reference.
    scriptable->Ref();

    // Watches the reference change signal after adding wrapper's reference,
    // to prevent the callback from being triggered uncessarily.
    wrapper->on_reference_change_connection =
        scriptable->ConnectOnReferenceChange(
            NewSlot(this, &Impl::OnScriptableReferenceChange,
                    static_cast<void *>(object)));

    // Record Scriptable to JSWrapper mapping.
    // A Scriptable object might be mapped to multiple ScriptContexts,
    // so this mapping is per ScriptContext.
    scriptable_js_wrappers_[scriptable] = object;
#ifdef DEBUG_FORCE_GC
    CollectGarbage();
#endif
  }

  // Detachs the Scriptable object attached to a given JSObjectRef object.
  void DetachScriptable(JSObjectRef object, bool caused_by_native) {
    ScopedLogContext log_context(owner_);
    ASSERT(IsWrapperOfScriptable(object));
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    ASSERT(wrapper);
    ASSERT(wrapper->impl == NULL || wrapper->impl == this);

    if (wrapper->scriptable) {
      ScriptableInterface *scriptable = wrapper->scriptable;

      // Clear it to prevent this function being called recursively.
      wrapper->scriptable = NULL;
#ifdef DEBUG_JS_VERBOSE
      DLOG("JSScriptContext::DetachScriptable(impl=%p, ctx=%p, jsobj=%p, "
           "scriptable=%p)", this, context_, object, scriptable);
#endif
      scriptable_js_wrappers_.erase(scriptable);

      wrapper->on_reference_change_connection->Disconnect();
      wrapper->on_reference_change_connection = NULL;

      if (scriptable->GetRefCount() > 1) {
        // There must be at least one native reference,
        // so the JavaScript wrapper must still be protected, and needs
        // unprotecting first.
#ifdef DEBUG_JS_VERBOSE
        DLOG("Unprotect Object: impl=%p, ctx=%p, jsobj=%p, scriptable=%s",
             this, context_, object, GetJSWrapperObjectName(object).c_str());
#endif
        JSValueUnprotect(context_, object);
#ifdef DEBUG_FORCE_GC
        CollectGarbage();
#endif
      }
      scriptable->Unref(caused_by_native);
    }
  }

  JSObjectRef GetJSWrapperFromScriptable(ScriptableInterface *scriptable) {
    ScriptableJSWrapperMap::iterator it =
        scriptable_js_wrappers_.find(scriptable);
    return static_cast<JSObjectRef>(
        it != scriptable_js_wrappers_.end() ? it->second : NULL);
  }

  void OnScriptableReferenceChange(int ref_count, int change, void *data) {
    JSObjectRef object = static_cast<JSObjectRef>(data);
    if (change == 0) {
      // The Scriptable object is being destroyed, it must be detached from the
      // wrapper.
      DetachScriptable(object, true);
    } else {
      ScopedLogContext log_context(owner_);
      ASSERT(change == 1 || change == -1);
      if (change == 1 && ref_count == 1) {
        // The Scriptable object is not floating anymore, so the wrapper object
        // needs protecting.
#ifdef DEBUG_JS_VERBOSE
        DLOG("Protect Object: impl=%p, ctx=%p, jsobj=%p, scriptable=%s",
             this, context_, object, GetJSWrapperObjectName(object).c_str());
#endif
        JSValueProtect(context_, object);
      } else if (change == -1 && ref_count == 2) {
        // The Scriptable object is about to be floating again, so unprotect
        // the wrapper object to make it garbage collectable.
#ifdef DEBUG_JS_VERBOSE
        ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
        ASSERT(wrapper);
        DLOG("Unprotect Object: impl=%p, ctx=%p, jsobj=%p, scriptable=%p",
             this, context_, object, wrapper->scriptable);
#endif
        JSValueUnprotect(context_, object);
#ifdef DEBUG_FORCE_GC
        CollectGarbage();
#endif
      }
    }
  }

  bool GetJSObjectProperty(JSObjectRef object, const char *name,
                           JSValueRef *result) {
    JSStringRef js_name = JSStringCreateWithUTF8CString(name);
    JSValueRef exception = NULL;
    *result = JSObjectGetProperty(context_, object, js_name, &exception);
    JSStringRelease(js_name);
    return CheckJSException(exception);
  }

  // Sets a given value to a specified property of a specified object,
  // with specified attributes.
  bool SetJSObjectProperty(JSObjectRef object, const char *name,
                           JSValueRef value, JSPropertyAttributes attributes) {
    JSStringRef js_name = JSStringCreateWithUTF8CString(name);
    JSValueRef exception = NULL;
    JSObjectSetProperty(context_, object, js_name, value,
                        attributes, &exception);
    JSStringRelease(js_name);
    return CheckJSException(exception);
  }

  bool DeleteJSObjectProperty(JSObjectRef object, const char *name) {
    JSStringRef js_name = JSStringCreateWithUTF8CString(name);
    JSValueRef exception = NULL;
    bool result =
        JSObjectDeleteProperty(context_, object, js_name, &exception) &&
        CheckJSException(exception);
    JSStringRelease(js_name);
    return result;
  }

  JSObjectTracker *GetJSObjectTracker(JSObjectRef object) {
    ASSERT(object);
    JSValueRef exception = NULL;
    JSObjectRef tracker_object = NULL;
    if (JSObjectHasProperty(context_, object,
                            js_object_tracker_reference_name_)) {
      JSValueRef tracker_prop = JSObjectGetProperty(
          context_, object, js_object_tracker_reference_name_, &exception);
      tracker_object = JSValueToObject(context_, tracker_prop, &exception);
    }

    if (!tracker_object || !JSValueIsObjectOfClass(
        context_, tracker_object, js_object_tracker_class_)) {
      tracker_object = JSObjectMake(context_, js_object_tracker_class_, NULL);
      JSPropertyAttributes attributes =
          kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete;
      JSObjectSetProperty(context_, object, js_object_tracker_reference_name_,
                          tracker_object, attributes, &exception);
    }

    JSObjectTracker *tracker = NULL;
    if (tracker_object && CheckJSException(exception)) {
      tracker = static_cast<JSObjectTracker *>(
          JSObjectGetPrivate(tracker_object));
    }
    // This method shall not fail.
    ASSERT(tracker);
    return tracker;
  }

  // Gets a function property of a specified object.
  JSObjectRef GetFunctionOrConstructorObject(JSObjectRef this_obj,
                                             const char *name) {
    if (!this_obj)
      this_obj = JSContextGetGlobalObject(context_);
    JSValueRef prop = NULL;
    if (GetJSObjectProperty(this_obj, name, &prop) &&
        JSValueIsObject(context_, prop)) {
      JSValueRef exception = NULL;
      JSObjectRef prop_obj = JSValueToObject(context_, prop, &exception);
      if (CheckJSException(exception) &&
          (JSObjectIsFunction(context_, prop_obj) ||
           JSObjectIsConstructor(context_, prop_obj)))
        return prop_obj;
    }
    return NULL;
  }

  bool EvaluateToJSValue(ScriptableInterface *object, const char *expr,
                         JSValueRef *result) {
    JSObjectRef js_object = NULL;

    if (object) {
      js_object = GetJSWrapperFromScriptable(object);
      if (!js_object) {
        DLOG("Scriptable %p doesn't have a JavaScript wrapper.", object);
        return false;
      }
    } else {
      js_object = JSContextGetGlobalObject(context_);
    }

    if (expr && *expr) {
      std::string massaged_script =
          ggadget::js::MassageJScript(expr, false, NULL, 0);
      JSStringRef js_script =
          JSStringCreateWithUTF8CString(massaged_script.c_str());
      JSValueRef exception = NULL;
      *result = JSEvaluateScript(context_, js_script, js_object,
                                 NULL, 0, &exception);
      JSStringRelease(js_script);
      if (!(*result && CheckJSException(exception))) {
        DLOG("Failed to evaluate script %s against JSObject %p",
             expr, js_object);
        return false;
      }
    } else {
      *result = js_object;
    }
    return true;
  }

  JSObjectRef CreateScriptableMethodCaller(const char *name, Slot *slot) {
    ScriptableMethodCaller *data = new ScriptableMethodCaller(this, name, slot);
    JSObjectRef caller = JSObjectMake(
        context_, scriptable_method_caller_class_, data);
    return caller;
  }

  JSValueRef CallNativeSlot(const char *name, JSObjectRef this_obj,
                            ScriptableInterface *scriptable, Slot *slot,
                            size_t argc, const JSValueRef argv[],
                            JSValueRef *exception) {
    Variant *params = NULL;
    size_t expected_argc = argc;

    if (!ConvertJSArgsToNative(owner_, this_obj, name, slot, argc, argv,
                               &params, &expected_argc, exception)) {
      return NULL;
    }

    ResultVariant result =
        slot->Call(scriptable, static_cast<int>(expected_argc), params);
    delete [] params;
    params = NULL;

    if (!CheckScriptableException(scriptable, exception))
      return NULL;

    JSValueRef return_val = NULL;
    if (!ConvertNativeToJS(owner_, result.v(), &return_val)) {
      RaiseJSException(owner_, exception,
                       "Failed to convert native function result(%s) to jsval",
                       result.v().Print().c_str());
      return NULL;
    }
    return return_val;
  }

 private:
  static ScriptableJSWrapper *GetScriptableJSWrapper(JSObjectRef object) {
    ASSERT(object);
    return static_cast<ScriptableJSWrapper *>(JSObjectGetPrivate(object));
  }

  static ScriptableInterface *GetScriptable(JSObjectRef object) {
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    return wrapper ? wrapper->scriptable : NULL;
  }

  static std::string GetJSWrapperObjectName(JSObjectRef object) {
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    if (wrapper && wrapper->scriptable) {
      return StringPrintf("[object %p CLASS_ID=%jx]", wrapper->scriptable,
                          wrapper->scriptable->GetClassId());
    }
    return "[null object]";
  }

  static bool IsSpecialProperty(const char *prop_name) {
    return prop_name[0] == '[' && prop_name[1] == '[' && prop_name[2] == '[';
  }

  static bool IsIndexProperty(const char *prop_name, int *index) {
    char *endptr = NULL;
    *index = static_cast<int>(strtol(prop_name, &endptr, 10));
    return *index >= 0 && (*endptr == '\0');
  }

 private:
  static void InitializeCallback(JSContextRef ctx, JSObjectRef object) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("InitializeCallback(ctx=%p, obj=%p)", ctx, object);
#endif
    ASSERT(JSObjectGetPrivate(object) == NULL);
    JSObjectSetPrivate(object, new ScriptableJSWrapper());
  }

  static void FinalizeCallback(JSObjectRef object) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("FinalizeCallback(obj=%p)", object);
#endif
    ScriptableJSWrapper *wrapper =
        static_cast<ScriptableJSWrapper *>(JSObjectGetPrivate(object));

    if (wrapper && wrapper->impl)
      wrapper->impl->DetachScriptable(object, false);

    delete wrapper;
  }

  static bool HasPropertyCallback(JSContextRef ctx,
                                  JSObjectRef object,
                                  JSStringRef property_name) {
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    ASSERT(wrapper);
    if (!wrapper->impl) return false;

    Impl *impl = wrapper->impl;
    ScriptableInterface *scriptable = wrapper->scriptable;
    JSStringUTF8Accessor utf8_name(property_name);

    // Save current context for other functions.
    JSContextScope(impl, ctx);

    if (!scriptable) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("HasPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) detached", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      return false;
    }

    if (IsSpecialProperty(utf8_name.Get())) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("HasPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) special", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      return false;
    }

    Variant prototype;
    int index = 0;
    bool result = false;
    if (IsIndexProperty(utf8_name.Get(), &index)) {
      prototype = scriptable->GetPropertyByIndex(index).v();
      result = (prototype.type() != Variant::TYPE_VOID);
    }
    if (!result) {
      ScriptableInterface::PropertyType prop_type =
          scriptable->GetPropertyInfo(utf8_name.Get(), &prototype);
      result = (prop_type != ScriptableInterface::PROPERTY_NOT_EXIST);
    }

    if (!impl->CheckScriptableException(scriptable, NULL))
      result = false;

#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("HasPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
         "prop=%s): %d", impl, ctx, object, scriptable, utf8_name.Get(),
         result);
#endif
    return result;
  }

  static JSValueRef GetPropertyCallback(JSContextRef ctx,
                                        JSObjectRef object,
                                        JSStringRef property_name,
                                        JSValueRef* exception) {
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    ASSERT(wrapper);
    if (!wrapper->impl) return NULL;

    Impl *impl = wrapper->impl;
    ScriptableInterface *scriptable = wrapper->scriptable;
    JSStringUTF8Accessor utf8_name(property_name);

    // Save current context for other functions.
    JSContextScope(impl, ctx);

    if (!scriptable) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("GetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) detached", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      RaiseJSException(impl->owner_, exception,
                       "Failed to get property %s, scriptable detached.",
                       utf8_name.Get());
      return NULL;
    }

    if (IsSpecialProperty(utf8_name.Get())) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("GetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) special", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      return NULL;
    }

    ResultVariant prop;
    int index = 0;
    if (IsIndexProperty(utf8_name.Get(), &index))
      prop = scriptable->GetPropertyByIndex(index);
    if (prop.v().type() == Variant::TYPE_VOID)
      prop = scriptable->GetProperty(utf8_name.Get());

    if (!impl->CheckScriptableException(scriptable, exception))
      return NULL;

    // Handles slot property specially.
    if (prop.v().type() == Variant::TYPE_SLOT) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      bool method = false;
#endif
      Slot *slot = VariantValue<Slot *>()(prop.v());
      JSValueRef js_func = NULL;
      if (!slot) {
        js_func = JSValueMakeNull(impl->context_);
      } else if (!impl->UnwrapJSFunctionSlot(slot, &js_func)) {
        // It's a method, returns a special object for calling this method.
        js_func = impl->CreateScriptableMethodCaller(utf8_name.Get(), slot);
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
        method = true;
#endif
      }
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("GetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s): %s (slot=%p, jsfunc=%p)", impl, ctx, object, scriptable,
             utf8_name.Get(), method ? "method" : "slot", slot, js_func);
#endif
      return js_func;
    }

    JSValueRef result = NULL;
    if (!ConvertNativeToJS(impl->owner_, prop.v(), &result)) {
      RaiseJSException(impl->owner_, exception,
                       "Failed to convert native property %s value(%s) to "
                       "JSValue.", utf8_name.Get(), prop.v().Print().c_str());
    }
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("GetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
         "prop=%s): %s", impl, ctx, object, scriptable, utf8_name.Get(),
         prop.v().Print().c_str());
#endif
    return result;
  }

  static bool SetPropertyCallback(JSContextRef ctx,
                                  JSObjectRef object,
                                  JSStringRef property_name,
                                  JSValueRef value,
                                  JSValueRef* exception) {
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    ASSERT(wrapper);
    if (!wrapper->impl) return false;

    Impl *impl = wrapper->impl;
    ScriptableInterface *scriptable = wrapper->scriptable;
    JSStringUTF8Accessor utf8_name(property_name);

    // Save current context for other functions.
    JSContextScope(impl, ctx);

    if (!scriptable) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("SetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) detached", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      RaiseJSException(impl->owner_, exception,
                       "Failed to set property %s, scriptable detached.",
                       utf8_name.Get());
      return false;
    }

    // Let JavaScript engine to handle special properties.
    if (IsSpecialProperty(utf8_name.Get())) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("SetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) special", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      return false;
    }

    int index = 0;
    bool is_index = false;
    Variant prototype;
    ScriptableInterface::PropertyType prop_type =
        ScriptableInterface::PROPERTY_NOT_EXIST;

    // Try index property first.
    if (IsIndexProperty(utf8_name.Get(), &index)) {
      is_index = true;
      prop_type = ScriptableInterface::PROPERTY_DYNAMIC;
      prototype = scriptable->GetPropertyByIndex(index).v();
    }

    if (!impl->CheckScriptableException(scriptable, exception)) {
      // bypass default javascript logic.
      return true;
    }

    // Then normal property.
    if (prototype.type() == Variant::TYPE_VOID) {
      is_index = false;
      prop_type = scriptable->GetPropertyInfo(utf8_name.Get(), &prototype);
    }

    if (!impl->CheckScriptableException(scriptable, exception)) {
      // bypass default javascript logic.
      return true;
    }

    if (prop_type == ScriptableInterface::PROPERTY_METHOD ||
        prop_type == ScriptableInterface::PROPERTY_CONSTANT) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("SetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) readonly", impl, ctx, object, scriptable, utf8_name.Get());
#endif
      return true;
    }

    if (prop_type == ScriptableInterface::PROPERTY_NOT_EXIST && !is_index) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("SetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "prop=%s) not exist", impl, ctx, object, scriptable,
           utf8_name.Get());
#endif
      // JSScriptableWrapper can handle property set even if the property is not
      // exist.
      if (scriptable->IsInstanceOf(JSScriptableWrapper::CLASS_ID)) {
        prototype = Variant(Variant::TYPE_VARIANT);
      } else if (scriptable->IsStrict()) {
        RaiseJSException(
            impl->owner_, exception,
            "The native object doesn't support setting property %s.",
            utf8_name.Get());
        return true;
      } else {
        return false;
      }
    }

    Variant native_value;
    if (!ConvertJSToNative(impl->owner_, object, prototype,
                           value, &native_value)) {
      RaiseJSException(impl->owner_, exception,
                       "Failed to convert JS property %s value(%s) to native.",
                       utf8_name.Get(),
                       PrintJSValue(impl->owner_, value).c_str());
      // bypass default javascript logic.
      return true;
    }

    bool result = false;
    if (is_index) {
      result = scriptable->SetPropertyByIndex(index, native_value);
      if (!impl->CheckScriptableException(scriptable, exception))
        result = false;
    }

    if (!result) {
      result = scriptable->SetProperty(utf8_name.Get(), native_value);
      if (!impl->CheckScriptableException(scriptable, exception))
        result = false;
    }

    if (!result) {
      if (scriptable->IsStrict()) {
        RaiseJSException(impl->owner_, exception,
                         "Failed to set native property %s (may be readonly).",
                         utf8_name.Get());
      }
      FreeNativeValue(native_value);
    }

#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("SetPropertyCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
         "prop=%s) %d, %s", impl, ctx, object, scriptable, utf8_name.Get(),
         result, native_value.Print().c_str());
#endif

    return result || scriptable->IsStrict();
  }

  struct EnumerateScriptablePropertiesData {
    Impl *impl;
    ScriptableInterface *scriptable;
    JSPropertyNameAccumulatorRef property_names;
  };

  static bool AddPropertyNameSlot(const char *name,
                                  ScriptableInterface::PropertyType prop_type,
                                  const Variant &value,
                                  void *data) {
    EnumerateScriptablePropertiesData *enum_data =
        static_cast<EnumerateScriptablePropertiesData *>(data);
    JSStringRef js_name = JSStringCreateWithUTF8CString(name);
    JSPropertyNameAccumulatorAddName(enum_data->property_names, js_name);
    JSStringRelease(js_name);
    return enum_data->impl->CheckScriptableException(enum_data->scriptable,
                                                     NULL);
  }

  static void GetPropertyNamesCallback(
      JSContextRef ctx, JSObjectRef object,
      JSPropertyNameAccumulatorRef property_names) {
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(object);
    ASSERT(wrapper);
    if (!wrapper->impl) return;

    // Save current context for other functions.
    JSContextScope(wrapper->impl, ctx);

    EnumerateScriptablePropertiesData data;
    data.impl = wrapper->impl;
    data.scriptable = wrapper->scriptable;
    data.property_names = property_names;

    wrapper->scriptable->EnumerateProperties(
        NewSlot(AddPropertyNameSlot, static_cast<void *>(&data)));
  }

  static JSValueRef CallAsFunctionCallback(JSContextRef ctx,
                                           JSObjectRef function,
                                           JSObjectRef this_object,
                                           size_t argument_count,
                                           const JSValueRef arguments[],
                                           JSValueRef* exception) {
    // In this case, function is atually this object, rather than this_object.
    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(function);
    ASSERT(wrapper);
    if (!wrapper->impl) return NULL;

    Impl *impl = wrapper->impl;
    ScriptableInterface *scriptable = wrapper->scriptable;

    // Save current context for other functions.
    JSContextScope(impl, ctx);

    if (!scriptable) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("CallAsFunctionCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p "
           "this=%p) detached", impl, ctx, function, scriptable, this_object);
#endif
      return NULL;
    }

    Variant prototype;
    // Get the default method for this object.
    if (scriptable->GetPropertyInfo("", &prototype) !=
        ScriptableInterface::PROPERTY_METHOD) {
      RaiseJSException(impl->owner_, exception,
                       "Object can't be called as a function");
      return NULL;
    }

    JSValueRef result = impl->CallNativeSlot(
        "SELF", function, scriptable, VariantValue<Slot *>()(prototype),
        argument_count, arguments, exception);

#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("CallAsFunctionCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p "
         "this=%p): %s", impl, ctx, function, scriptable, this_object,
         PrintJSValue(impl->owner_, result).c_str());
#endif
    return result;
  }

  // Callbacks for class constructors.
  static void ClassConstructorFinalizeCallback(JSObjectRef object) {
    delete static_cast<ClassConstructorData *>(JSObjectGetPrivate(object));
  }

  static JSObjectRef ClassConstructorCallAsConstructorCallback(
      JSContextRef ctx, JSObjectRef constructor, size_t argument_count,
      const JSValueRef arguments[], JSValueRef* exception) {
    ClassConstructorData *data =
        static_cast<ClassConstructorData *>(JSObjectGetPrivate(constructor));
    ASSERT(data && data->constructor);

    // Save current context for other functions.
    JSContextScope(data->impl, ctx);

    JSValueRef result = data->impl->CallNativeSlot(
        data->class_name, NULL, NULL, data->constructor, argument_count,
        arguments, exception);

    if (!*exception && (!result || JSValueIsNull(ctx, result) ||
                        JSValueIsUndefined(ctx, result))) {
      RaiseJSException(data->impl->owner_, exception,
                       "Native constructor returns null.");
      return NULL;
    }

    return JSValueToObject(ctx, result, exception);
  }

  // Callbacks for ScriptableMethodCaller.
  static void ScriptableMethodCallerFinalizeCallback(JSObjectRef object) {
    delete static_cast<ScriptableMethodCaller *>(JSObjectGetPrivate(object));
  }

  static JSValueRef ScriptableMethodCallerCallAsFunctionCallback(
      JSContextRef ctx, JSObjectRef function, JSObjectRef this_obj,
      size_t argc, const JSValueRef argv[], JSValueRef* exception) {
    ScriptableMethodCaller *data =
        static_cast<ScriptableMethodCaller *>(JSObjectGetPrivate(function));
    ASSERT(data);

    ScriptableJSWrapper *wrapper = GetScriptableJSWrapper(this_obj);

    // The scriptable might be this object's prototype. Only global object has
    // this problem.
    if (!wrapper) {
      ASSERT(JSContextGetGlobalObject(ctx) == this_obj);
      this_obj = JSValueToObject(ctx, JSObjectGetPrototype(ctx, this_obj),
                                 NULL);
      wrapper = GetScriptableJSWrapper(this_obj);
    }

    Impl *impl = data->impl;
    ScriptableInterface *scriptable = NULL;

    // Allows this_obj to be null..
    if (wrapper) {
      impl = wrapper->impl;
      scriptable = wrapper->scriptable;
      ASSERT(impl == data->impl);
    }

    // Save current context for other functions.
    JSContextScope(impl, ctx);

#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
    DLOG("CallMethodCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
         "method=%s)", impl, ctx, this_obj, scriptable,
         data->method_name.c_str());
#endif

    if (wrapper && !scriptable) {
#ifdef DEBUG_JS_VERBOSE_SCRIPTABLE_JS_WRAPPER
      DLOG("CallMethodCallback(impl=%p, ctx=%p, jsobj=%p, scriptable=%p, "
           "method=%s) detached", impl, ctx, this_obj, scriptable,
           data->method_name.c_str());
#endif
      RaiseJSException(impl->owner_, exception,
                       "Cannot call method %s of a detached scriptable.",
                       data->method_name.c_str());
      return NULL;
    }

    // If function slot is not retrieved yet, try get it first.
    if (!data->function_slot) {
      Variant prototype;
      if (scriptable &&
          scriptable->GetPropertyInfo(data->method_name.c_str(), &prototype) ==
          ScriptableInterface::PROPERTY_METHOD) {
        data->function_slot = VariantValue<Slot *>()(prototype);
      }
      if (!data->function_slot) {
        RaiseJSException(impl->owner_, exception, "Invalid method %s called.",
                         data->method_name.c_str());
        return NULL;
      }
    }
    return impl->CallNativeSlot(data->method_name.c_str(), this_obj, scriptable,
                                data->function_slot, argc, argv, exception);
  }

  // Callbacks for JSObjectTracker.
  static void TrackerInitializeCallback(JSContextRef ctx, JSObjectRef object) {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
    DLOG("TrackerInitializeCallback(ctx=%p, obj=%p)", ctx, object);
#endif
    ASSERT(JSObjectGetPrivate(object) == NULL);
    JSObjectSetPrivate(object, new JSObjectTracker());
  }

  static void TrackerFinalizeCallback(JSObjectRef object) {
#ifdef DEBUG_JS_VERBOSE_JS_SCRIPTABLE_WRAPPER
    DLOG("TrackerFinalizeCallback(obj=%p)", object);
#endif
    JSObjectTracker *tracker =
        static_cast<JSObjectTracker *>(JSObjectGetPrivate(object));

    if (tracker) {
      if (tracker->scriptable_wrapper)
        tracker->scriptable_wrapper->DetachJS();
      tracker->slot_detach_signal();
    }
    delete tracker;
  }

  static JSValueRef ReturnSelfFunc(JSContextRef ctx, JSObjectRef function,
                                   JSObjectRef this_object,
                                   size_t argument_count,
                                   const JSValueRef arguments[],
                                   JSValueRef* exception) {
    return this_object;
  }

  static JSValueRef CollectGarbageFunc(JSContextRef ctx, JSObjectRef function,
                                       JSObjectRef this_object,
                                       size_t argument_count,
                                       const JSValueRef arguments[],
                                       JSValueRef* exception) {
    JSScriptContext *context =
        static_cast<JSScriptContext *>(JSObjectGetPrivate(function));
    ASSERT(context);
    context->CollectGarbage();
    return JSValueMakeUndefined(ctx);
  }

#ifdef _DEBUG
  static JSValueRef InterruptFunc(JSContextRef ctx, JSObjectRef function,
                                  JSObjectRef this_object,
                                  size_t argument_count,
                                  const JSValueRef arguments[],
                                  JSValueRef* exception) {
    kill(0, SIGINT);
    return JSValueMakeUndefined(ctx);
  }
#endif

 private:
  JSScriptContext *owner_;
  JSScriptRuntime *runtime_;
  JSContextRef context_;
  JSClassRef scriptable_js_wrapper_class_;
  JSClassRef scriptable_method_caller_class_;
  JSClassRef js_object_tracker_class_;
  JSClassRef class_constructor_class_;
  JSStringRef js_object_tracker_reference_name_;
  JSObjectRef is_nan_func_;
  JSObjectRef is_finite_func_;
  JSObjectRef date_class_obj_;
  JSObjectRef array_class_obj_;

  // first: scriptable, second: js object (wrapper)
  typedef LightMap<void *, void *> ScriptableJSWrapperMap;
  ScriptableJSWrapperMap scriptable_js_wrappers_;

  typedef LightSet<void *> JSFunctionSlotMap;
  JSFunctionSlotMap js_function_slots_;

  typedef LightSet<void *> JSScriptableWrapperMap;
  JSScriptableWrapperMap js_scriptable_wrappers_;

  Signal2<bool, const char *, int> script_blocked_signal_;

  uint64_t last_gc_time_;

 private:
  static const uint64_t kMaxGCInterval = 10000; // 10 seconds.
  static const char kJSObjectTrackerReferenceName[];
  static const JSClassDefinition kScriptableJSWrapperClassDefinition;
  static const JSClassDefinition kScriptableMethodCallerClassDefinition;
  static const JSClassDefinition kJSObjectTrackerClassDefinition;
  static const JSClassDefinition kClassConstructorClassDefinition;
};

const char JSScriptContext::Impl::kJSObjectTrackerReferenceName[] =
  "[[[_JSObjectTracker_]]]";

const JSClassDefinition
JSScriptContext::Impl::kScriptableJSWrapperClassDefinition = {
  0,                            // version, shall be 0
  kJSClassAttributeNone,        // attributes
  "ScriptableJSWrapper",        // className, utf-8 encoded
  NULL,                         // parentClass
  NULL,                         // staticValues
  NULL,                         // staticFunctions
  JSScriptContext::Impl::InitializeCallback,
  JSScriptContext::Impl::FinalizeCallback,
  JSScriptContext::Impl::HasPropertyCallback,
  JSScriptContext::Impl::GetPropertyCallback,
  JSScriptContext::Impl::SetPropertyCallback,
  NULL,                         // deleteProperty
  JSScriptContext::Impl::GetPropertyNamesCallback,
  JSScriptContext::Impl::CallAsFunctionCallback,
  NULL,                         // callAsConstructor
  NULL,                         // hasInstance,
  NULL,                         // convertToType
};

const JSClassDefinition
JSScriptContext::Impl::kScriptableMethodCallerClassDefinition = {
  0,                            // version, shall be 0
  kJSClassAttributeNoAutomaticPrototype,
  "ScriptableMethodCaller",     // className, utf-8 encoded
  NULL,                         // parentClass
  NULL,                         // staticValues
  NULL,                         // staticFunctions
  NULL,                         // initialize
  JSScriptContext::Impl::ScriptableMethodCallerFinalizeCallback,
  NULL,                         // hasInstance
  NULL,                         // getProperty
  NULL,                         // setProperty
  NULL,                         // deleteProperty
  NULL,                         // getPropertyNames
  JSScriptContext::Impl::ScriptableMethodCallerCallAsFunctionCallback,
  NULL,                         // callAsConstructor,
  NULL,                         // hasInstance,
  NULL,                         // convertToType
};

const JSClassDefinition
JSScriptContext::Impl::kJSObjectTrackerClassDefinition = {
  0,                            // version, shall be 0
  kJSClassAttributeNoAutomaticPrototype,
  "JSObjectTracker",            // className, utf-8 encoded
  NULL,                         // parentClass
  NULL,                         // staticValues
  NULL,                         // staticFunctions
  JSScriptContext::Impl::TrackerInitializeCallback,
  JSScriptContext::Impl::TrackerFinalizeCallback,
  NULL,                         // hasInstance
  NULL,                         // getProperty
  NULL,                         // setProperty
  NULL,                         // deleteProperty
  NULL,                         // getPropertyNames
  NULL,                         // callAsFunction
  NULL,                         // callAsConstructor,
  NULL,                         // hasInstance,
  NULL,                         // convertToType
};

const JSClassDefinition
JSScriptContext::Impl::kClassConstructorClassDefinition = {
  0,                            // version, shall be 0
  kJSClassAttributeNone,
  "ClassConstructor",           // className, utf-8 encoded
  NULL,                         // parentClass
  NULL,                         // staticValues
  NULL,                         // staticFunctions
  NULL,                         // initialize
  JSScriptContext::Impl::ClassConstructorFinalizeCallback,
  NULL,                         // hasInstance
  NULL,                         // getProperty
  NULL,                         // setProperty
  NULL,                         // deleteProperty
  NULL,                         // getPropertyNames
  NULL,                         // callAsFunction
  JSScriptContext::Impl::ClassConstructorCallAsConstructorCallback,
  NULL,                         // hasInstance,
  NULL,                         // convertToType
};

JSScriptContext::JSScriptContext(JSScriptRuntime *runtime, JSContextRef js_context)
  : impl_(new Impl(this, runtime, js_context)) {
}

JSScriptContext::~JSScriptContext() {
  delete impl_;
}

void JSScriptContext::Destroy() {
  delete this;
}

void JSScriptContext::Execute(const char *script,
                              const char *filename,
                              int lineno) {
  impl_->Execute(script, filename, lineno);
}

Slot *JSScriptContext::Compile(const char *script,
                               const char *filename,
                               int lineno) {
  return impl_->Compile(script, filename, lineno);
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global) {
  return impl_->SetGlobalObject(global);
}

bool JSScriptContext::RegisterClass(const char *name, Slot *constructor) {
  return impl_->RegisterClass(name, constructor);
}

bool JSScriptContext::AssignFromContext(ScriptableInterface *dest_object,
                                        const char *dest_object_expr,
                                        const char *dest_property,
                                        ScriptContextInterface *src_context,
                                        ScriptableInterface *src_object,
                                        const char *src_expr) {
  return impl_->AssignFromContext(dest_object, dest_object_expr,
                                  dest_property, src_context, src_object,
                                  src_expr);
}

bool JSScriptContext::AssignFromNative(ScriptableInterface *object,
                                       const char *object_expr,
                                       const char *property,
                                       const Variant &value) {
  return impl_->AssignFromNative(object, object_expr, property, value);
}

Variant JSScriptContext::Evaluate(ScriptableInterface *object,
                                  const char *expr) {
  return impl_->Evaluate(object, expr);
}

Connection *JSScriptContext::ConnectScriptBlockedFeedback(
    ScriptBlockedFeedback *feedback) {
  return impl_->ConnectScriptBlockedFeedback(feedback);
}

void JSScriptContext::CollectGarbage() {
  impl_->CollectGarbage();
}

void JSScriptContext::GetCurrentFileAndLine(std::string *filename,
                                            int *lineno) {
  impl_->GetCurrentFileAndLine(filename, lineno);
}

JSScriptRuntime *JSScriptContext::GetRuntime() const {
  return impl_->GetRuntime();
}

JSContextRef JSScriptContext::GetContext() const {
  return impl_->GetContext();
}

JSObjectRef JSScriptContext::WrapScriptable(ScriptableInterface *scriptable) {
  return impl_->WrapScriptable(scriptable);
}

ScriptableInterface *JSScriptContext::UnwrapScriptable(JSObjectRef object) {
  return impl_->UnwrapScriptable(object);
}

ScriptableInterface *JSScriptContext::WrapJSObject(JSObjectRef object) {
  return impl_->WrapJSObject(object);
}

Slot *JSScriptContext::WrapJSObjectIntoSlot(const Slot *prototype,
                                            JSObjectRef owner,
                                            JSObjectRef object) {
  return impl_->WrapJSObjectIntoSlot(prototype, owner, object);
}

JSObjectRef JSScriptContext::UnwrapJSObject(ScriptableInterface *scriptable) {
  return impl_->UnwrapJSObject(scriptable);
}

bool JSScriptContext::UnwrapJSFunctionSlot(Slot *slot, JSValueRef *js_func) {
  return impl_->UnwrapJSFunctionSlot(slot, js_func);
}

bool JSScriptContext::IsWrapperOfScriptable(JSObjectRef object) const {
  return impl_->IsWrapperOfScriptable(object);
}

bool JSScriptContext::IsWrapperOfJSObject(
    ScriptableInterface *scriptable) const {
  return impl_->IsWrapperOfJSObject(scriptable);
}

bool JSScriptContext::CheckJSException(JSValueRef exception) {
  return impl_->CheckJSException(exception);
}

bool JSScriptContext::CheckScriptableException(ScriptableInterface *scriptable,
                                               JSValueRef *exception) {
  return impl_->CheckScriptableException(scriptable, exception);
}

bool JSScriptContext::IsNaN(JSValueRef value) const {
  return impl_->IsNaN(value);
}

bool JSScriptContext::IsFinite(JSValueRef value) const {
  return impl_->IsFinite(value);
}

bool JSScriptContext::IsDate(JSValueRef value) const {
  return impl_->IsDate(value);
}

bool JSScriptContext::IsArray(JSValueRef value) const {
  return impl_->IsArray(value);
}

unsigned int JSScriptContext::GetArrayLength(JSObjectRef array) const {
  return impl_->GetArrayLength(array);
}

void JSScriptContext::RegisterGlobalFunction(
    const char *name, JSObjectCallAsFunctionCallback callback) {
  impl_->RegisterObjectMethod(NULL, name, callback);
}

} // namespace webkit
} // namespace ggadget
