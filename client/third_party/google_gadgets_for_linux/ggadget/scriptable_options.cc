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

#include "scriptable_options.h"
#include "options_interface.h"
#include "small_object.h"

namespace ggadget {

class ScriptableOptions::Impl : public SmallObject<> {
 public:
  Impl(OptionsInterface *options, bool raw_objects)
      : options_(options), raw_objects_(raw_objects) {
  }

  void Add(const char *name, const JSONString &value) {
    options_->Add(name, Variant(value));
  }

  // If an item doesn't exist, returns a blank string "" (JSON '""').
  JSONString GetDefaultValue(const char *name) {
    Variant value = options_->GetDefaultValue(name);
    return value.type() == Variant::TYPE_JSON ?
           VariantValue<JSONString>()(value) : JSONString("\"\"");
  }

  void PutDefaultValue(const char *name, const JSONString &value) {
    options_->PutDefaultValue(name, Variant(value));
  }

  // If an item doesn't exist, returns a blank string "" (JSON '""').
  JSONString GetValue(const char *name) {
    Variant value = options_->GetValue(name);
    return value.type() == Variant::TYPE_JSON ?
           VariantValue<JSONString>()(value) : JSONString("\"\"");
  }

  void PutValue(const char *name, const JSONString &value) {
    options_->PutValue(name, Variant(value));
  }

  // If an item doesn't exist, returns undefined (JSON "").
  JSONString OldGetDefaultValue(const char *name) {
    Variant value = options_->GetDefaultValue(name);
    return value.type() == Variant::TYPE_JSON ?
           VariantValue<JSONString>()(value) : JSONString("");
  }

  // If an item doesn't exist, returns undefined (JSON "").
  JSONString OldGetValue(const char *name) {
    Variant value = options_->GetValue(name);
    return value.type() == Variant::TYPE_JSON ?
           VariantValue<JSONString>()(value) : JSONString("");
  }

  OptionsInterface *options_;
  bool raw_objects_;
};

ScriptableOptions::ScriptableOptions(OptionsInterface *options,
                                     bool raw_objects)
    : impl_(new Impl(options, raw_objects)) {
}

void ScriptableOptions::DoRegister() {
  OptionsInterface *options = impl_->options_;
  RegisterProperty("count",
                   NewSlot(options, &OptionsInterface::GetCount), NULL);
  RegisterMethod("exists", NewSlot(options, &OptionsInterface::Exists));
  RegisterMethod("remove", NewSlot(options, &OptionsInterface::Remove));
  RegisterMethod("removeAll", NewSlot(options, &OptionsInterface::RemoveAll));
  RegisterMethod("encryptValue",
                 NewSlot(options, &OptionsInterface::EncryptValue));

  if (impl_->raw_objects_) {
    // Partly support the deprecated "item" property.
    RegisterMethod("item", NewSlot(options, &OptionsInterface::GetValue));
    // Partly support the deprecated "defaultValue" property.
    RegisterMethod("defaultValue",
                   NewSlot(options, &OptionsInterface::GetDefaultValue));
    // In raw objects mode, we don't support the deprecated properties.
    RegisterMethod("add", NewSlot(options, &OptionsInterface::Add));
    RegisterMethod("getDefaultValue",
                   NewSlot(options, &OptionsInterface::GetDefaultValue));
    RegisterMethod("getValue", NewSlot(options, &OptionsInterface::GetValue));
    RegisterMethod("putDefaultValue",
                   NewSlot(options, &OptionsInterface::PutDefaultValue));
    RegisterMethod("putValue", NewSlot(options, &OptionsInterface::PutValue));

    // Register the "default" method, allowing this object be called directly
    // as a function.
    RegisterMethod("", NewSlot(options, &OptionsInterface::GetValue));
  } else {
    // Partly support the deprecated "item" property.
    RegisterMethod("item", NewSlot(impl_, &Impl::OldGetValue));
    // Partly support the deprecated "defaultValue" property.
    RegisterMethod("defaultValue",
                   NewSlot(impl_, &Impl::OldGetDefaultValue));
    RegisterMethod("add", NewSlot(impl_, &Impl::Add));
    RegisterMethod("getDefaultValue",
                   NewSlot(impl_, &Impl::GetDefaultValue));
    RegisterMethod("getValue", NewSlot(impl_, &Impl::GetValue));
    RegisterMethod("putDefaultValue",
                   NewSlot(impl_, &Impl::PutDefaultValue));
    RegisterMethod("putValue", NewSlot(impl_, &Impl::PutValue));

    // Register the "default" method, allowing this object be called directly
    // as a function.
    RegisterMethod("", NewSlot(impl_, &Impl::OldGetValue));

    // Disable the following for now, because it's not in the public API.
    // SetDynamicPropertyHandler(NewSlot(impl_, &Impl::GetValue),
    //                           NewSlot(impl_, &Impl::PutValue));
  }
}

ScriptableOptions::~ScriptableOptions() {
  delete impl_;
}

const OptionsInterface *ScriptableOptions::GetOptions() const {
  return impl_->options_;
}

OptionsInterface *ScriptableOptions::GetOptions() {
  return impl_->options_;
}

} // namespace ggadget
