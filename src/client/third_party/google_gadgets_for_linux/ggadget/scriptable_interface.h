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

#ifndef GGADGET_SCRIPTABLE_INTERFACE_H__
#define GGADGET_SCRIPTABLE_INTERFACE_H__

#include <limits.h>
#include <ggadget/common.h>
#include <ggadget/variant.h>
#include <ggadget/slot.h>
#include <ggadget/small_object.h>

namespace ggadget {

class Connection;
class RegisterableInterface;

/**
 * @defgroup ScriptableFoundation Scriptable Foundations
 * @ingroup CoreLibrary
 * Foundation classes to implement scriptable objects.
 * @{
 */

/**
 * @ingroup Interfaces
 * Object interface that can be called from script languages.
 * Normally an object need not to implement this interface directly, but
 * inherits from @c ScriptableHelper.
 *
 * Any interface or abstract class inheriting @c ScriptableInterface should
 * include @c CLASS_ID_DECL and @c CLASS_ID_IMPL to define its @c CLASS_ID and
 * @c IsInstanceOf() method.
 *
 * Any concrete implementation class should include @c DEFINE_CLASS_ID to
 * define its @c CLASS_ID and @c IsInstanceOf() method.
 */
class ScriptableInterface : public SmallObject<> {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~ScriptableInterface() { }

 public:

  /**
   * This ID uniquely identifies the class.  Each implementation should define
   * this field as a unique number.  You can simply use the first 3 parts of
   * the result of uuidgen.
   */
  static const uint64_t CLASS_ID = 0;

  /**
   * Gets the class id of this object. For debugging purpose only.
   */
  virtual uint64_t GetClassId() const = 0;

  /**
   * Adds a reference to this object.
   */
  virtual void Ref() const = 0;

  /**
   * Removes a reference from this object.
   * @param transient if @c true, the reference will be removed transiently,
   *     that is, the object will not be deleted even if reference count
   *     reaches zero (i.e. the object is floating). This is useful before
   *     returning an object from a function.
   */
  virtual void Unref(bool transient = false) const = 0;

  /**
   * Gets the current reference count.
   */
  virtual int GetRefCount() const = 0;

  /**
   * Judges if this instance is of a given class.
   */
  virtual bool IsInstanceOf(uint64_t class_id) const = 0;

  /**
   * Tests if this object is 'strict', that is, not allowing the script to
   * assign to a previously undefined property.
   */
  virtual bool IsStrict() const = 0;

  /**
   * Tests if this object can be enumerated by a for-in enumeration in script.
   */
  virtual bool IsEnumeratable() const = 0;

  /**
   * Connects a callback which will be called when @c Ref() or @c Unref() is
   * called.
   * @param slot the callback. The parameters of the slot are:
   *     - the reference count before change.
   *     - 1 or -1 indicating whether the reference count is about to be
   *       increased or decreased; or 0 if the object is about to be deleted(
   *       at this situation, reference count is meaningless).
   *
   *       It's possible that a native owned object is about to be deleted but
   *       the reference count has not reached 0, in which case all reference
   *       holders must release their references immediately in this callback.
   * @return the connected @c Connection.
   */
  virtual Connection *ConnectOnReferenceChange(
      Slot2<void, int, int> *slot) = 0;

  /** Types of named properties returned from @c GetPropertyInfo(). */
  enum PropertyType {
    /** The property doesn't exist. */
    PROPERTY_NOT_EXIST = -1,
    /** The property always exists. It's value may change during its life. */
    PROPERTY_NORMAL = 0,
    /** The property always exists. It's value doesn't change. */
    PROPERTY_CONSTANT,
    /**
     * The property is a dynamic property, which can be dynamically created
     * and deleted after the object is created.
     */
    PROPERTY_DYNAMIC,
    /** The property is a method which always exists. */
    PROPERTY_METHOD
  };

  /**
   * Gets the info of a named property by its name.
   *
   * If the property is a method, a prototype of type @c Variant::TYPE_SLOT
   * will be returned, then the caller can get the function details from
   * slot value of this prototype.
   *
   * A signal property also expects a script function as the value, and thus
   * also has a prototype of type @c Variant::TYPE_SLOT.
   *
   * @param name the name of the property.
   * @param[out,optional] prototype returns a prototype of the property value,
   *     from which the script engine can get detailed information; or returns
   *     the property value directly if the property is a constant.
   * @return the type of the property. If the property is known not existing,
   *     @c PROPERTY_NOT_EXIST should be returned.
   */
  virtual PropertyType GetPropertyInfo(const char *name,
                                       Variant *prototype) = 0;

  /**
   * Gets the value of a named property.
   * @param name the name of the property.
   * @return the property value, or a @c Variant of type @c Variant::TYPE_VOID
   *     if this property is not supported.
   */
  virtual ResultVariant GetProperty(const char *name) = 0;

  /**
   * Sets the value of a named property.
   * @param name the name of the property.
   * @param value the property value. The type must be compatible with the
   *     prototype returned from @c GetPropertyInfo().
   * @return @c true if the property is supported and succeeds.
   */
  virtual bool SetProperty(const char *name, const Variant &value) = 0;

  /**
   * Gets the value of a indexed property.
   * @param index the array index of the property.
   * @return the property value, or a @c Variant of type @c Variant::TYPE_VOID
   *     if this property is not supported.
   */
  virtual ResultVariant GetPropertyByIndex(int index) = 0;

  /**
   * Sets the value of a indexed property.
   * @param index the array index of the property.
   * @param value the property value.
   * @return @c true if the property is supported and succeeds.
   */
  virtual bool SetPropertyByIndex(int index, const Variant &value) = 0;

  /**
   * Gets and clears the current pending exception.
   * The script adapter will call this method after each call of
   * @c GetPropertyInfoById(), @c GetPropertyInfoByName(), @c GetProperty()
   * and @c SetProoperty().
   * @param clear if @c true, the pending exception will be cleared.
   * @return the pending exception.
   */
  virtual ScriptableInterface *GetPendingException(bool clear) = 0;

  typedef Slot3<bool, const char *, PropertyType, const Variant &>
      EnumeratePropertiesCallback;
  /**
   * Enumerates all known properties.
   * @param callback it will be called for each property. The parameters are
   *     const char *name, PropertyType type, const Variant &value.
   *     The callback should return @c false if it doesn't want to continue.
   * @return @c false if the callback returns @c false.
   */
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback) = 0;

  typedef Slot2<bool, int, const Variant &> EnumerateElementsCallback;
  /**
   * Enumerates all known elements (i.e. properties that can be accessed by
   * non-negative array indexes).
   * @param callback it will be called for each property. The parameters are
   *     int index, const Variant &value. The callback should return @c false
   *     if it doesn't want to continue.
   * @return @c false if the callback returns @c false.
   */
  virtual bool EnumerateElements(EnumerateElementsCallback *callback) = 0;

  /**
   * Returns the @c RegisterableInterface pointer if this object supports it,
   * otherwise returns @c NULL.
   */
  virtual RegisterableInterface *GetRegisterable() = 0;

};

/**
 * Used in the declaration section of an interface which inherits
 * @c ScriptableInterface to declare a class id.
 */
#define CLASS_ID_DECL(cls_id)                                                \
  static const uint64_t CLASS_ID = UINT64_C(cls_id);                         \
  virtual bool IsInstanceOf(uint64_t class_id) const = 0;

/**
 * Used after the declaration section of an interface which inherits
 * @c ScriptableInterface to define @c IsInstanceOf() method.
 */
#define CLASS_ID_IMPL(cls, super)                                            \
  inline bool cls::IsInstanceOf(uint64_t class_id) const {                   \
    return class_id == CLASS_ID || super::IsInstanceOf(class_id);            \
  }

/**
 * Used in the declaration section of a class which implements
 * @c ScriptableInterface or an interface inheriting @c ScriptableInterface.
 */
#define DEFINE_CLASS_ID(cls_id, super)                                       \
  static const uint64_t CLASS_ID = UINT64_C(cls_id);                         \
  virtual bool IsInstanceOf(uint64_t class_id) const {                       \
    return class_id == CLASS_ID || super::IsInstanceOf(class_id);            \
  }                                                                          \
  virtual uint64_t GetClassId() const { return UINT64_C(cls_id); }

inline bool ScriptableInterface::IsInstanceOf(uint64_t class_id) const {
  return class_id == CLASS_ID;
}

/** @} */

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_INTERFACE_H__
