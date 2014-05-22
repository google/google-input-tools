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

#ifndef GGADGET_OPTIONS_INTERFACE_H__
#define GGADGET_OPTIONS_INTERFACE_H__

#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>

namespace ggadget {

class Connection;
template <typename R, typename P1> class Slot1;

/**
 * @defgroup Options Options
 * @ingroup CoreLibrary
 * Options related classes.
 * @{
 */

/**
 * @ingroup Interfaces
 *
 * Interface class for storing options.
 */
class OptionsInterface {
 public:
  virtual ~OptionsInterface() { }

  /**
   * Connects a handler which will be called when any option changed.
   * The name of the changed option will be sent as the parameter.
   */
  virtual Connection *ConnectOnOptionChanged(
      Slot1<void, const char *> *handler) = 0;

  /**
   * @return the number of items in the options.
   */
  virtual size_t GetCount() = 0;

  /**
   * Adds an item to the options if it doesn't already exist.
   */
  virtual void Add(const char *name, const Variant &value) = 0;

  /**
   * @return @c true if the value of @a name has been set.
   */
  virtual bool Exists(const char *name) = 0;

  /**
   * @return the default value of the @a name.
   */
  virtual Variant GetDefaultValue(const char *name) = 0;

  /**
   * Sets the default value of the name.
   */
  virtual void PutDefaultValue(const char *name, const Variant &value) = 0;

  /**
   * @return the value associated with the name.
   */
  virtual Variant GetValue(const char *name) = 0;

  /**
   * Sets the value associated with the name, creating the entry if one
   * doesn't already exist for the name.
   */
  virtual void PutValue(const char *name, const Variant &value) = 0;

  /**
   * Removes the value from the options.
   */
  virtual void Remove(const char *name) = 0;

  /**
   * Removes all values from the options.
   */
  virtual void RemoveAll() = 0;

  /**
   * Encrypt an item's value. Once @c encryptValue() is called for an item,
   * the value of the item is stored in encrypted form. The value is
   * automatically decrypted when it is retrieved.
   * Note: If you remove an item and then re-add it, the item is no longer
   * secure.
   */
  virtual void EncryptValue(const char *name) = 0;

  /**
   * Returns whether the specified item is encrypted.
   */
  virtual bool IsEncrypted(const char *name) = 0;

  /**
   * This method is only for C++ code. Internal option values are not
   * accessible from the gadget script.
   * @return the internal options value associated with the name.
   */
  virtual Variant GetInternalValue(const char *name) = 0;

  /**
   * This method is only for C++ code. Internal option values are not
   * accessible from the gadget script.
   * Sets the internal value associated with the name, creating the entry if one
   * doesn't already exist for the name.
   */
  virtual void PutInternalValue(const char *name, const Variant &value) = 0;

  /**
   * Flush the options data into permanent storage if the impl supports.
   */
  virtual bool Flush() = 0;

  /**
   * Deletes the permanent storage for this OptionsInterface instance if the
   * impl supports.
   */
  virtual void DeleteStorage() = 0;

  /**
   * Enumerate all items.
   * @param callback called for each item, with the following parameters:
   *     - option item name;
   *     - option item value;
   *     - whether the item is encrypted.
   *     The callback can break the enumeration by returning @c false.
   * @return @c true if the enumeration is not broken by the callback.
   */
  virtual bool EnumerateItems(
      Slot3<bool, const char *, const Variant &, bool> *callback) = 0;

  /**
   * Enumerate all internal items.
   * @see EnumerateItems()
   */
  virtual bool EnumerateInternalItems(
      Slot2<bool, const char *, const Variant &> *callback) = 0;
};

/**
 * @relates OptionsInterface
 * The function to create an @c OptionsInterface instance.
 * @param name distinct name of the instance.
 * @return the created @c OptionsInterface instance, or @c NULL on failure.
 *     The caller then owns the returned pointer.
 */
typedef OptionsInterface *(*OptionsFactory)(const char *name);

/**
 * @relates OptionsInterface
 * Sets an OptionsFactory as the global options factory. An Options extension
 * module can call this function in its @c Initailize() function.
 */
bool SetOptionsFactory(OptionsFactory options_factory);

/**
 * @relates OptionsInterface
 * Creates an @c OptionsInterface instance. Invocations will be delegated to
 * the global @c OptionsFactory.
 */
OptionsInterface *CreateOptions(const char *name);

/**
 * @relates OptionsInterface
 * Set the global options instance which is used to store global config data.
 * Normally not the Options extension modoule but the host should call
 * this function.
 */
bool SetGlobalOptions(OptionsInterface *global_options);

/**
 * @relates OptionsInterface
 * Gets the global options instance previously set by @c SetGlobalOptions().
 */
OptionsInterface *GetGlobalOptions();

/** @} */

} // namespace ggadget

#endif  // GGADGET_OPTIONS_INTERFACE_H__
