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

#include <vector>
#include "scriptable_array.h"
#include "small_object.h"

namespace ggadget {

class ScriptableArray::Impl : public SmallObject<> {
 public:
  Impl(ScriptableArray *owner) : owner_(owner) {
  }

  ScriptableArray *ToArray() { return owner_; }

  // Use ResultVariant to add reference to scriptable items, to prevent them
  // from being destroyed by others.
  // It's not necessary to handle the situation that an scriptable item is
  // deleted by others. They shall not be deleted explicitly.
  std::vector<ResultVariant> array_;
  ScriptableArray *owner_;
};

ScriptableArray::ScriptableArray()
    : impl_(new Impl(this)) {
  // Simulates JavaScript array.
  SetArrayHandler(NewSlot(this, &ScriptableArray::GetItem), NULL);
}

void ScriptableArray::DoClassRegister() {
  RegisterProperty("count", NewSlot(&ScriptableArray::GetCount), NULL);
  RegisterMethod("item", NewSlot(&ScriptableArray::GetItem));
  // Simulates JavaScript array.
  RegisterProperty("length", NewSlot(&ScriptableArray::GetCount), NULL);
  // Simulates VBArray.
  RegisterMethod("toArray", NewSlot(&Impl::ToArray, &ScriptableArray::impl_));
}

ScriptableArray::~ScriptableArray() {
  delete impl_;
  impl_ = NULL;
}

bool ScriptableArray::EnumerateProperties(
    EnumeratePropertiesCallback *callback) {
  // This object enumerates no properties, just like a normal JavaScript array.
  delete callback;
  return true;
}

bool ScriptableArray::EnumerateElements(EnumerateElementsCallback *callback) {
  ASSERT(callback);
  size_t count = impl_->array_.size();
  for (size_t i = 0; i < count; i++) {
    if (!(*callback)(static_cast<int>(i), impl_->array_[i].v())) {
      delete callback;
      return false;
    }
  }
  delete callback;
  return true;
}

size_t ScriptableArray::GetCount() const {
  return impl_->array_.size();
}

Variant ScriptableArray::GetItem(size_t index) const {
  return index < impl_->array_.size() ? impl_->array_[index].v() : Variant();
}

void ScriptableArray::Append(const Variant &item) {
  impl_->array_.push_back(ResultVariant(item));
}

} // namespace ggadget
