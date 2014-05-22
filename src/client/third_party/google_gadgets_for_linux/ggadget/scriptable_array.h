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

#ifndef GGADGET_SCRIPTABLE_ARRAY_H__
#define GGADGET_SCRIPTABLE_ARRAY_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/variant.h>

namespace ggadget {

/**
 * @defgroup ScriptableObjects Scriptable objects
 * @ingroup CoreLibrary
 *
 * Objects that can be accessed from JavaScript.
 */

/**
 * @ingroup ScriptableObjects
 *
 * This class is used to reflect a const native array to script.
 * The script can access this object by getting "count" property and "item"
 * method, or with an Enumerator.
 */
class ScriptableArray : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x65cf1406985145a9, ScriptableInterface);

  /** Creates an empty ScriptableArray object. */
  ScriptableArray();

  /** Appends an item to the array. */
  void Append(const Variant &item);

 public:
  /**
   * Creates a @c ScriptableArray with begin and end iterators.
   * All items between [begin, end) will be stored in the newly created array.
   *
   * @param begin the begin iterator.
   * @param end the end iterator.
   */
  template <typename I>
  static ScriptableArray *Create(I begin, I end) {
    ScriptableArray *array = new ScriptableArray();
    for (I it = begin; it != end; ++it)
      array->Append(Variant(*it));
    return array;
  }

  /**
   * Same as above, but accepts a @c NULL terminated array of pointers.
   * A copy of the input array will be made.
   */
  template <typename T>
  static ScriptableArray *Create(T *const *ptr) {
    ScriptableArray *array = NULL;
    if (ptr) {
      array = new ScriptableArray();
      for (; *ptr; ++ptr)
        array->Append(Variant(*ptr));
    }
    return array;
  }

  /**
   * These methods are override to make this object act like normal
   * JavaScript arrays for C++ users.
   */
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback);
  virtual bool EnumerateElements(EnumerateElementsCallback *callback);

  size_t GetCount() const;
  Variant GetItem(size_t index) const;

 protected:
  virtual void DoClassRegister();
  virtual ~ScriptableArray();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableArray);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_ARRAY_H__
