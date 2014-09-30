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

#include <climits>
#include <set>
#include "memory_options.h"
#include "logger.h"
#include "scriptable_holder.h"
#include "string_utils.h"
#include "small_object.h"

namespace ggadget {

class OptionsItem {
 public:
  OptionsItem() {
  }

  explicit OptionsItem(const Variant &value) {
    SetValue(value);
  }

  void SetValue(const Variant &value) {
    value_ = value;
    if (value.type() == Variant::TYPE_SCRIPTABLE)
      holder_.Reset(VariantValue<ScriptableInterface *>()(value));
    else
      holder_.Reset(NULL);
  }

  Variant GetValue() const {
    return value_.type() == Variant::TYPE_SCRIPTABLE ?
           Variant(holder_.Get()) : value_;
  }

 private:
  Variant value_;
  ScriptableHolder<ScriptableInterface> holder_;
};

class MemoryOptions::Impl : public SmallObject<> {
 public:
  Impl(size_t size_limit)
      : size_limit_(size_limit), total_size_(0) {
  }

  void FireChangedEvent(const char *name, const Variant &value) {
    DLOG("option %s changed to %s", name, value.Print().c_str());
    onoptionchanged_signal_(name);
  }

  typedef LightMap<std::string, OptionsItem, GadgetStringComparator> OptionsMap;
  typedef LightSet<std::string, GadgetStringComparator> EncryptedSet;
  OptionsMap values_;
  OptionsMap defaults_;
  OptionsMap internal_values_;
  EncryptedSet encrypted_;
  Signal1<void, const char *> onoptionchanged_signal_;
  size_t size_limit_, total_size_;
};

MemoryOptions::MemoryOptions()
    // Though INT_MAX is much smaller than the maximum value of size_t on
    // some platforms, it is still big enough to be treated as "unlimited".
    : impl_(new Impl(INT_MAX)) {
}

MemoryOptions::MemoryOptions(size_t size_limit)
    : impl_(new Impl(size_limit)) {
}

MemoryOptions::~MemoryOptions() {
  delete impl_;
}

// Returns the approximate size of a variant.
static size_t GetVariantSize(const Variant& v) {
  switch (v.type()) {
    case Variant::TYPE_VOID:
      // It's important to return 0 for TYPE_VOID because sometimes
      // non-existance values are treated as void.
      return 0;
    case Variant::TYPE_STRING:
      return VariantValue<std::string>()(v).size();
    case Variant::TYPE_JSON:
      return VariantValue<JSONString>()(v).value.size();
    case Variant::TYPE_UTF16STRING:
      return VariantValue<UTF16String>()(v).size() * 2;
    default:
      // Value of other types only counted approximately.
      return sizeof(Variant);
  }
}

Connection *MemoryOptions::ConnectOnOptionChanged(
    Slot1<void, const char *> *handler) {
  return impl_->onoptionchanged_signal_.Connect(handler);
}

size_t MemoryOptions::GetCount() {
  return impl_->values_.size();
}

void MemoryOptions::Add(const char *name, const Variant &value) {
  std::string name_str(name); // Avoid multiple std::string() construction.
  if (impl_->values_.find(name_str) == impl_->values_.end()) {
    size_t new_total_size = impl_->total_size_ + name_str.size() +
                            GetVariantSize(value);
    if (new_total_size > impl_->size_limit_) {
      LOG("Options exceeds size limit %zu.", impl_->size_limit_);
    } else {
      impl_->total_size_ = new_total_size;
      impl_->values_[name_str].SetValue(value);
      impl_->FireChangedEvent(name, value);
    }
  }
}

bool MemoryOptions::Exists(const char *name) {
  return impl_->values_.find(name) != impl_->values_.end();
}

Variant MemoryOptions::GetDefaultValue(const char *name) {
  Impl::OptionsMap::const_iterator it = impl_->defaults_.find(name);
  return it == impl_->defaults_.end() ? Variant() : it->second.GetValue();
}

void MemoryOptions::PutDefaultValue(const char *name, const Variant &value) {
  impl_->defaults_[name].SetValue(value);
}

Variant MemoryOptions::GetValue(const char *name) {
  Impl::OptionsMap::const_iterator it = impl_->values_.find(name);
  return it == impl_->values_.end() ?
         GetDefaultValue(name) : it->second.GetValue();
}

void MemoryOptions::PutValue(const char *name, const Variant &value) {
  std::string name_str(name); // Avoid multiple std::string construction.
  Impl::OptionsMap::iterator it = impl_->values_.find(name_str);
  if (it == impl_->values_.end()) {
    Add(name, value);
  } else {
    Variant last_value = it->second.GetValue();
    if (last_value != value) {
      ASSERT(impl_->total_size_ >= GetVariantSize(last_value));
      size_t new_total_size = impl_->total_size_ + GetVariantSize(value) -
                              GetVariantSize(last_value);
      if (new_total_size > impl_->size_limit_) {
        LOG("Options exceeds size limit %zu.", impl_->size_limit_);
      } else {
        impl_->total_size_ = new_total_size;
        it->second.SetValue(value);
        impl_->FireChangedEvent(name, value);
      }
    }
    // Putting a value automatically removes the encrypted state.
    impl_->encrypted_.erase(name_str);
  }
}

void MemoryOptions::Remove(const char *name) {
  std::string name_str(name); // Avoid multiple std::string construction.
  Impl::OptionsMap::iterator it = impl_->values_.find(name_str);
  if (it != impl_->values_.end()) {
    size_t last_value_size = GetVariantSize(it->second.GetValue());
    ASSERT(impl_->total_size_ >= name_str.size() + last_value_size);
    impl_->total_size_ -= name_str.size() + last_value_size;
    impl_->values_.erase(it);
    impl_->encrypted_.erase(name_str);
    impl_->FireChangedEvent(name, Variant());
  }
}

void MemoryOptions::RemoveAll() {
  while (!impl_->values_.empty()) {
    Impl::OptionsMap::iterator it = impl_->values_.begin();
    std::string name(it->first);
    impl_->values_.erase(it);
    impl_->encrypted_.erase(name);
    impl_->FireChangedEvent(name.c_str(), Variant());
  }
  impl_->total_size_ = 0;
}

void MemoryOptions::EncryptValue(const char *name) {
  impl_->encrypted_.insert(name);
}

bool MemoryOptions::IsEncrypted(const char *name) {
  return impl_->encrypted_.find(name) != impl_->encrypted_.end();
}

Variant MemoryOptions::GetInternalValue(const char *name) {
  Impl::OptionsMap::const_iterator it = impl_->internal_values_.find(name);
  return it == impl_->internal_values_.end() ?
         Variant() : it->second.GetValue();
}

void MemoryOptions::PutInternalValue(const char *name, const Variant &value) {
  // Internal values are not counted in total_size_.
  impl_->internal_values_[name].SetValue(value);
}

bool MemoryOptions::Flush() {
  return true;
}

void MemoryOptions::DeleteStorage() {
  impl_->values_.clear();
  impl_->internal_values_.clear();
  impl_->encrypted_.clear();
  impl_->total_size_ = 0;
}

bool MemoryOptions::EnumerateItems(
    Slot3<bool, const char *, const Variant &, bool> *callback) {
  ASSERT(callback);
  for (Impl::OptionsMap::const_iterator it = impl_->values_.begin();
       it != impl_->values_.end(); ++it) {
    const char *name = it->first.c_str();
    if (!(*callback)(name, it->second.GetValue(), IsEncrypted(name))) {
      delete callback;
      return false;
    }
  }
  delete callback;
  return true;
}

bool MemoryOptions::EnumerateInternalItems(
    Slot2<bool, const char *, const Variant &> *callback) {
  ASSERT(callback);
  for (Impl::OptionsMap::const_iterator it = impl_->internal_values_.begin();
       it != impl_->internal_values_.end(); ++it) {
    if (!(*callback)(it->first.c_str(), it->second.GetValue())) {
      delete callback;
      return false;
    }
  }
  delete callback;
  return true;
}

} // namespace ggadget
