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

#ifndef GGADGET_REGISTERABLE_INTERFACE_H__
#define GGADGET_REGISTERABLE_INTERFACE_H__

namespace ggadget {

class Signal;
class Slot;
class Variant;

/**
 * @ingroup ScriptableFoundation
 * @ingroup Interfaces
 * Represents registerable capabilities of an object.
 */
class RegisterableInterface {
 protected:
  virtual ~RegisterableInterface() { }

 public:
  /**
   * Register a scriptable property.
   * This @c ScriptableHelper @a name owns the pointers of the
   * @a getter and the @a setter.
   * @param name property name.  It must point to static allocated memory.
   * @param getter the getter slot of the property.
   * @param setter the setter slot of the property.
   */
  virtual void RegisterProperty(const char *name,
                                Slot *getter, Slot *setter) = 0;

  /**
   * Register a scriptable property having enumerated values that should
   * mapped to strings.
   * @param name property name.  It must point to static allocated memory.
   * @param getter a getter slot returning an enum value.
   * @param setter a setter slot accepting an enum value. @c NULL if the
   *     property is readonly.
   * @param names a table containing string values of every enum values.
   * @param count number of entries in the @a names table.
   */
  virtual void RegisterStringEnumProperty(const char *name,
                                          Slot *getter, Slot *setter,
                                          const char **names, int count) = 0;

  /**
   * Register a scriptable method.
   * This @c ScriptableHelper owns the pointer of @c slot.
   * @param name method name.  It must point to static allocated memory.
   * @param slot the method slot.
   */
  virtual void RegisterMethod(const char *name, Slot *slot) = 0;

  /**
   * Register a @c Signal that can connect to various @c Slot callbacks.
   * After this call, a same named property will be automatically registered
   * that can be used to get/set the @c Slot callback.
   * @param name the name of the @a signal.  It must point to static
   *     allocated memory.
   * @param signal the @c Signal to be registered.
   */
  virtual void RegisterSignal(const char *name, Signal *signal) = 0;

  /**
   * Register a @c Variant constant.
   * @param name the constant name.
   * @param value the constant value.
   */
  virtual void RegisterVariantConstant(const char *name,
                                       const Variant &value) = 0;

};

} // namespace ggadget

#endif // GGADGET_REGISTERABLE_INTERFACE_H__
