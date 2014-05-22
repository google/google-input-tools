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

#ifndef GGADGET_SCRIPTABLE_ENUMERATOR_H__
#define GGADGET_SCRIPTABLE_ENUMERATOR_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

/**
 * @ingroup ScriptableObjects
 *
 * This class is used to reflect an enumerator to script.
 * @param E the native enumerator.
 * @param ClassId the class id of this scriptable enumerator.
 *
 * An enumerator must at least support the following operations:
 * <code>
 * class NativeEnumerator {
 *   void Destroy();
 *   bool AtEnd();
 *   ItemType *GetItem();
 *   void MoveFirst();
 *   void MoveNext();
 *   size_t GetCount();
 * };
 * </code>
 *
 * The ItemType must be a typedef to the real item type, and can be used as:
 * <code>typename E::ItemType</code>
 */
template <typename E, typename Wrapper, typename Param, uint64_t ClassId>
class ScriptableEnumerator : public SharedScriptable<ClassId> {
 public:
  ScriptableEnumerator(ScriptableInterface *owner,
                       E *enumerator,
                       Param param)
      : owner_(owner), enumerator_(enumerator), param_(param) {
    ASSERT(enumerator);
    ASSERT(owner);
    owner_->Ref();
  }

  virtual ~ScriptableEnumerator() {
    enumerator_->Destroy();
    owner_->Unref();
  }

  Wrapper *GetItem() {
    typename E::ItemType *item = enumerator_->GetItem();
    return item ? new Wrapper(item, param_) : NULL;
  }

 protected:
  virtual void DoClassRegister() {
    this->RegisterMethod(
        "atEnd",
        NewSlot(&E::AtEnd,
                &ScriptableEnumerator<E, Wrapper, Param, ClassId>
                ::enumerator_));
    this->RegisterMethod(
        "moveFirst",
        NewSlot(&E::MoveFirst,
                &ScriptableEnumerator<E, Wrapper, Param, ClassId>
                ::enumerator_));
    this->RegisterMethod(
        "moveNext",
        NewSlot(&E::MoveNext,
                &ScriptableEnumerator<E, Wrapper, Param, ClassId>
                ::enumerator_));
    this->RegisterMethod(
        "item",
        NewSlot(&ScriptableEnumerator<E, Wrapper, Param, ClassId>
                ::GetItem));
    this->RegisterProperty(
        "count",
        NewSlot(&E::GetCount,
                &ScriptableEnumerator<E, Wrapper, Param, ClassId>
                ::enumerator_),
        NULL);
  }

 private:
  ScriptableInterface *owner_;
  E *enumerator_;
  Param param_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_ENUMERATOR_H__
