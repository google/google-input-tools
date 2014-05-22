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

#include "common.h"
#include "slot.h"

namespace ggadget {

namespace {

class SlotWithDefaultArgs : public Slot {
 public:
  SlotWithDefaultArgs(Slot *slot, const Variant *default_args)
      : slot_(slot), default_args_(default_args) {
#ifdef _DEBUG
    ASSERT(slot);
    if (default_args) {
      const Variant::Type *arg_types = slot->GetArgTypes();
      int arg_count = slot->GetArgCount();
      for (int i = 0; i < arg_count; i++) {
        Variant::Type type = default_args[i].type();
        ASSERT(type == Variant::TYPE_VOID ||
               type == arg_types[i] ||
               arg_types[i] == Variant::TYPE_VARIANT);
      }
    }
#endif
  }
  virtual ~SlotWithDefaultArgs() {
    delete slot_;
    slot_ = NULL;
  }
  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const {
    return slot_->Call(object, argc, argv);
  }
  virtual bool HasMetadata() const {
    return slot_->HasMetadata();
  }
  virtual Variant::Type GetReturnType() const {
    return slot_->GetReturnType();
  }
  virtual int GetArgCount() const {
    return slot_->GetArgCount();
  }
  virtual const Variant::Type *GetArgTypes() const {
    return slot_->GetArgTypes();
  }
  virtual const Variant *GetDefaultArgs() const {
    return default_args_;
  }
  virtual bool operator==(const Slot &another) const {
    return *slot_ == *down_cast<const SlotWithDefaultArgs *>(&another)->slot_ &&
           default_args_ ==
               down_cast<const SlotWithDefaultArgs *>(&another)->default_args_;
  }
 private:
  Slot *slot_;
  const Variant *default_args_;
};

} // anonymous namespace

Slot *NewSlotWithDefaultArgs(Slot *slot, const Variant *default_args) {
  return new SlotWithDefaultArgs(slot, default_args);
}

} // namespace ggadget
