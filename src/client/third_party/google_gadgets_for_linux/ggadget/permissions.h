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

#ifndef GGADGET_PERMISSIONS_H__
#define GGADGET_PERMISSIONS_H__
#include <string>
#include <ggadget/slot.h>

namespace ggadget {

/**
 * @ingroup Gadget
 *
 * A class to hold access control informations of a gadget.
 *
 * There are multiple permissions that can be applied to a gadget.
 * Each permission corresponds to one or more APIs:
 * - FILE_READ  name: <fileread/>
 *   Permission to read local files and directories.
 *   Following APIs require this permission:
 *   - framework.BrowseForFile(s)
 *   - framework.system.getFileIcon
 *   - framework.system.filesystem (APIs to read files and dirs)
 *   - basicElement.dropTarget
 * - FILE_WRITE name: <filewrite/>
 *   Permission to modify local files and directories.
 *   Following APIs require this permission:
 *   - framework.system.filesystem (APIs to modify files and dirs)
 * - DEVICE_STATUS name: <devicestatus/>
 *   Permission to query and change device status.
 *   Following APIs require this permission:
 *   - framework.system.bios
 *   - framework.system.cursor
 *   - framework.system.machine
 *   - framework.system.memory
 *   - framework.system.network
 *   - framework.system.power
 *   - framework.system.process
 *   - framework.system.screen
 *   - processInfo
 * - NETWORK name: <network/>
 *   Permission to access network.
 *   Following APIs require this permission:
 *   - framework.audio (if the audio file needs to be fetched from network)
 *   - contentitem.open_command (if being used to open an url)
 *   - framework.openUrl (if being used to open an url)
 *   - googleTalk object
 *   - XMLHttpRequest
 * - PERSONAL_DATA name: <personaldata/>
 *   Permission to access personalization data
 *   Following APIs require this permission:
 *   - framework.google
 *   - queryApi
 *   - eventApi
 *   - personalizationApi
 * - ALL_ACCESS name: <allaccess/>
 *   Permission that allows to do anything, such as execute native code or
 *   local binaries. It implies all above permissions.
 *   Following APIs require this permission:
 *   - contentitem.open_command (if being used to open a local file)
 *   - framework.openUrl (if being used to open a local file)
 *   - Load native modules and extensions.
 *
 * The required eermissions of a gadget shall be specified in gadget.manifest
 * file in <permissions> node, for example:
 * <permissions>
 *  <network/>
 *  <fileread/>
 * </permissions>
 *
 * An empty <permissions/> node indicates that there is no required permission.
 * For old gadgets without <permissions/> node, <allaccess/> will be assumed as
 * required.
 *
 * Relationship among different permissions:
 * - If <network/> is not required, then <fileread/> and <devicestatus/> will be
 *   granted automatically.
 * - If <allaccess/> is required, then all other permissions will be required
 *   automatically.
 * - If <allaccess/> is granted, then all other permissions will be granted
 *   automatically.
 */
class Permissions {
 public:
  enum Permission {
    FILE_READ = 0,
    FILE_WRITE,
    DEVICE_STATUS,
    NETWORK,
    PERSONAL_DATA,
    ALL_ACCESS
  };

  Permissions();
  Permissions(const Permissions &source);
  ~Permissions();

  /** Copy operator */
  Permissions& operator=(const Permissions &source);

  bool operator==(const Permissions &another) const;

  /**
   * Grants or denies a specified permission.
   *
   * If a superior permission, which implies the specified one, is granted, then
   * the specified one will always be granted, and denying it won't take effect,
   * unless the superior permission is denied.
   *
   * <fileread/> and <devicestatus/> will be granted automatically, if
   * <network/> is not granted.
   *
   * @param permission A permission to be granted or denied.
   * @param granted if it's true then grants the specified permission,
   *        otherwise denies it.
   */
  void SetGranted(int permission, bool granted);

  /**
   * Grants or denies a specified permission by its name.
   *
   * @sa SetGranted()
   *
   * @param permission The name of a permission to be granted or denied.
   * @param granted if it's true then grants the specified permission,
   *        otherwise denies it.
   */
  void SetGrantedByName(const char *permission, bool granted);

  /**
   * Grants or denies permissions according to another Permissions object.
   *
   * If granted is true, then all permissions explicitly granted in another
   * will be granted explicitly. Otherwise, all permissions explicitly denied
   * in another will be denied explicitly.
   */
  void SetGrantedByPermissions(const Permissions &another, bool granted);

  /** Checks if a specified permission is granted. */
  bool IsGranted(int permission) const;

  /**
   * Requires a specified permission.
   *
   * If a superior permission, which implies the specified one, is required,
   * then the specified one will always be required, and removing the
   * requirement of it won't take effect, unless the requirement of the
   * superior permission is removed.
   *
   * @param permission A permission to be required.
   * @param required if it's true then requires the specified permission,
   *        otherwise remove the requirement of it.
   */
  void SetRequired(int permission, bool required);

  /**
   * Requires a specified permission by its name.
   *
   * @sa SetRequired()
   *
   * @param permission The name of a permission to be required.
   * @param required if it's true then requires the specified permission,
   *        otherwise remove the requirement of it.
   */
  void SetRequiredByName(const char *permission, bool required);

  /**
   * Requires permissions according to another Permissions object.
   *
   * If required is true, then all permissions required in another will be
   * required. Otherwise, all permissions which are not required in another
   * will be removed.
   */
  void SetRequiredByPermissions(const Permissions &another, bool required);

  /** Checks if a specified permission is required. */
  bool IsRequired(int permission) const;

  /** Checks if a specified permission is required and granted. */
  bool IsRequiredAndGranted(int permission) const;

  /**
   * Checks if any required permissions are not granted yet.
   * @return true if one or more required permissions are not granted yet.
   */
  bool HasUngranted() const;

  /** Grants all permissions which are required explicitly. */
  void GrantAllRequired();

  /**
   * Removes all required permissions.
   *
   * Equals to SetRequiredByPermissions(Permissions(), false);
   */
  void RemoveAllRequired();

  /** Converts permissions information into a string. */
  std::string ToString() const;

  /**
   * Loads permissions information from a string.
   *
   * All old information will be erased before loading new information.
   */
  void FromString(const char *str);

  /**
   * Enumerates all granted permissions.
   *
   * @param slot The slot to be called against each granted permission.
   *        If the slot returns false then stop the enumeration.
   * @return true if there is one or more granted permissions and all of them
   *         are enumerated.
   */
  bool EnumerateAllGranted(Slot1<bool, int> *slot) const;

  /**
   * Enumerates all required permissions.
   *
   * @param slot The slot to be called against each required permission.
   *        If the slot returns false then stop the enumeration.
   * @return true if there is one or more required permissions and all of them
   *         are enumerated.
   */
  bool EnumerateAllRequired(Slot1<bool, int> *slot) const;

 public:
  /**
   * Gets the name of a specified permission.
   *
   * The returned name is only used in gadget.manifest, for human readable
   * purpose, use GetDescription() instead.
   */
  static std::string GetName(int permission);

  /**
   * Gets the corresponding permission of a specified name.
   *
   * @return the permission related to the specified name, returns -1 if the
   *         name is invalid.
   */
  static int GetByName(const char *name);

  /**
   * Gets the localized human readable description of a specified permission.
   */
  static std::string GetDescription(int permission);

  /**
   * Gets the localized human readable description of a specified permission
   * in a specified locale.
   */
  static std::string GetDescriptionForLocale(int permission,
                                             const char *locale);

 private:
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_PERMISSIONS_H__
