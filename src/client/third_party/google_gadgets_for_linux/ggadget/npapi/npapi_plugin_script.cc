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

#include "npapi_plugin_script.h"

#include <ggadget/variant.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/scriptable_function.h>

#include "npapi_utils.h"

namespace ggadget {
namespace npapi {

Variant ConvertNPToNative(const NPVariant *np_var) {
  switch (np_var->type) {
    case NPVariantType_Null:
      return Variant(Variant::TYPE_STRING);
    case NPVariantType_Bool:
      return Variant(np_var->value.boolValue);
    case NPVariantType_Int32:
      return Variant(np_var->value.intValue);
    case NPVariantType_Double:
      return Variant(np_var->value.doubleValue);
    case NPVariantType_String:
      return Variant(std::string(np_var->value.stringValue.utf8characters,
                                 np_var->value.stringValue.utf8length));
    case NPVariantType_Object:
      return Variant(new ScriptableNPObject(np_var->value.objectValue));
    default:
      return Variant();
  }
}

void ConvertNativeToNP(const Variant &var, NPVariant *np_var) {
  switch (var.type()) {
    case Variant::TYPE_VOID:
      VOID_TO_NPVARIANT(*np_var);
      break;
    case Variant::TYPE_BOOL:
      BOOLEAN_TO_NPVARIANT(VariantValue<bool>()(var), *np_var);
      break;
    case Variant::TYPE_INT64:
      INT32_TO_NPVARIANT(VariantValue<uint32_t>()(var), *np_var);
      break;
    case Variant::TYPE_DOUBLE:
      DOUBLE_TO_NPVARIANT(VariantValue<double>()(var), *np_var);
      break;
    case Variant::TYPE_STRING:
      NewNPVariantString(VariantValue<std::string>()(var), np_var);
      break;
    case Variant::TYPE_SCRIPTABLE: {
      ScriptableNPObject *obj = VariantValue<ScriptableNPObject *>()(var);
      if (obj) {
        // The object is a scriptable wrapper for a NPObject, returns
        // the NPObject.
        NPObject *np_obj = obj->UnWrap();
        OBJECT_TO_NPVARIANT(RetainNPObject(np_obj), *np_var);
      } else {
        LOG("Can't pass native objects to NP plugin");
        VOID_TO_NPVARIANT(*np_var);
      }
      break;
    }
    case Variant::TYPE_VARIANT:
    case Variant::TYPE_JSON:
    case Variant::TYPE_UTF16STRING:
    case Variant::TYPE_SLOT:
    case Variant::TYPE_DATE:
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
    default:
      LOG("Data type is not supported when pass native value to NP plugin: %s",
          var.Print().c_str());
      VOID_TO_NPVARIANT(*np_var);
      break;
  }
}

class ScriptableNPObject::Impl {
 public:
  class NPSlot : public Slot {
   public:
    NPSlot(Impl *owner, NPIdentifier id) : owner_(owner), id_(id) {
      owner_->owner_->Ref();
    }
    virtual ~NPSlot() {
      owner_->owner_->Unref();
    }

    // We don't know how many arguments the plugin function can receive.
    // Rely on the plugin to report error if any exists.
    virtual bool HasMetadata() const {
      return false;
    }

    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      GGL_UNUSED(object);
      NPObject *np_obj = owner_->np_obj_;

      scoped_array<NPVariant> args(new NPVariant[argc]);
      for (int i = 0; i < argc; ++i)
        ConvertNativeToNP(argv[i], &args[i]);

      NPVariant result;
      bool ok = np_obj->_class->invoke &&
                np_obj->_class->invoke(np_obj, id_, args.get(), argc, &result);
      for (int i = 0; i < argc; ++i)
        ReleaseNPVariant(&args[i]);
      if (ok) {
        ResultVariant ret(ConvertNPToNative(&result));
        ReleaseNPVariant(&result);
        return ret;
      }
      return ResultVariant();
    }

    virtual bool operator==(const Slot &another) const {
      const NPSlot *a = down_cast<const NPSlot *>(&another);
      return a && owner_ == a->owner_ && id_ == a->id_;
    }

   private:
    DISALLOW_EVIL_CONSTRUCTORS(NPSlot);
    Impl *owner_;
    NPIdentifier id_;
  };

  Impl(ScriptableNPObject *owner, NPObject *np_obj)
      : owner_(owner), np_obj_(np_obj) {
    RetainNPObject(np_obj_);
    owner->SetDynamicPropertyHandler(NewSlot(this, &Impl::GetDynamicProperty),
                                     NewSlot(this, &Impl::SetDynamicProperty));
    owner->SetArrayHandler(NewSlot(this, &Impl::GetArrayProperty),
                           NewSlot(this, &Impl::SetArrayProperty));
  }

  ~Impl() {
    ReleaseNPObject(np_obj_);
  }

  Variant GetDynamicProperty(const char *name) {
    if (!np_obj_ )
      return Variant();
    return GetProperty(GetStringIdentifier(name));
  }

  bool SetDynamicProperty(const char *name, const Variant &value) {
    if (!np_obj_ )
      return false;
    return SetProperty(GetStringIdentifier(name), value);
  }

  Variant GetArrayProperty(int index) {
    if (!np_obj_)
      return Variant();
    return GetProperty(GetIntIdentifier(static_cast<int32_t>(index)));
  }

  bool SetArrayProperty(int index, const Variant &value) {
    if (!np_obj_)
      return false;
    return SetProperty(GetIntIdentifier(static_cast<int32_t>(index)), value);
  }

  Variant GetProperty(NPIdentifier id) {
    NPVariant result;
    if (np_obj_->_class->hasMethod &&
        np_obj_->_class->hasMethod(np_obj_, id)) {
      NPSlot *slot = new NPSlot(this, id);
      return Variant(new ScriptableFunction(slot));
    }
    if (np_obj_->_class->getProperty &&
        np_obj_->_class->getProperty(np_obj_, id, &result)) {
      Variant ret = ConvertNPToNative(&result);
      ReleaseNPVariant(&result);
      return ret;
    }
    return Variant();
  }

  bool SetProperty(NPIdentifier id, const Variant &value) {
    NPVariant np_value;
    ConvertNativeToNP(value, &np_value);
    bool result = np_obj_->_class->setProperty(np_obj_, id, &np_value);
    ReleaseNPVariant(&np_value);
    return result;
  }

  ScriptableNPObject *owner_;
  NPObject *np_obj_;
};

ScriptableNPObject::ScriptableNPObject(NPObject *np_obj)
    : impl_(new Impl(this, np_obj)) {
}

ScriptableNPObject::~ScriptableNPObject() {
  delete impl_;
  impl_ = NULL;
}

NPObject *ScriptableNPObject::UnWrap() {
  return impl_->np_obj_;
}

} // namespace ggadget
} // namespace npapi
