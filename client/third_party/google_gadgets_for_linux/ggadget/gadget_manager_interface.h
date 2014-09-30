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

#include <string>

#ifndef GGADGET_GADGET_MANAGER_INTERFACE_H__
#define GGADGET_GADGET_MANAGER_INTERFACE_H__

namespace ggadget {

class HostInterface;
class Permissions;

class Connection;
template <typename R, typename P1> class Slot1;

// Names of some built-in gadgets.
const char kRSSGadgetName[] = "rss";
const char kIGoogleGadgetName[] = "igoogle";

/**
 * @ingroup Interfaces
 * @ingroup Gadget
 * Manages instances of gadgets.
 */
class GadgetManagerInterface {
 protected:
  virtual ~GadgetManagerInterface() { }

 public:
  /**
   * Initialize the gadget manager.
   * Should be called after the main program has loaded all extension modules,
   * and before starting the main loop.
   */
  virtual void Init() = 0;

  /**
   * Creates an new instance of a gadget specified by the file path or the
   * name of a built-in gadget.
   * @param file location of a gadget file or name of a built-in gadget.
   *     The location can be a full path of a gadget file, or a location
   *     that can be recognized by the global file manager.
   * @return the gadget instance id (>=0) of the new instance, or -1 on error.
   */
  virtual int NewGadgetInstanceFromFile(const char *file) = 0;

  /**
   * Removes a gadget instance.
   * @param instance_id id of an active instance.
   * @return @c true if succeeded.
   */
  virtual bool RemoveGadgetInstance(int instance_id) = 0;

  /**
   * Returns the name to create the @c OptionsInterface instance for a gadget
   * instance.
   */
  virtual std::string GetGadgetInstanceOptionsName(int instance_id) = 0;

  /**
   * Enunerates all active gadget instances. The callback will receive an int
   * parameter which is the gadget instance id, and can return true if it
   * wants the enumeration to continue, or false to break the enumeration.
   */
  virtual bool EnumerateGadgetInstances(Slot1<bool, int> *callback) = 0;

  /**
   * Get the full path of the file for a gadget instance, either downloaded or
   * opened from local file system.
   * @param instance_id id of an active instance.
   * @return the full path of the file for a gadget instance.
   */
  virtual std::string GetGadgetInstancePath(int instance_id) = 0;

  /**
   * Shows the gadget browser dialog.
   */
  virtual void ShowGadgetBrowserDialog(HostInterface *host) = 0;

  /**
   * Gets the default permissions of a gadget instance.
   * @param instance_id id of an active instance.
   * @param[out] permissions The default permissions of the gadget instance,
   *             including all permissions required by the gadget instance,
   *             and all permissions granted/denied by default.
   * @return true if the permissions information is loaded successfully.
   */
  virtual bool GetGadgetDefaultPermissions(int instance_id,
                                           Permissions *permissions) = 0;

  /**
   * Gets information of a gadget instance.
   */
  virtual bool GetGadgetInstanceInfo(int instance_id, const char *locale,
                                     std::string *author,
                                     std::string *download_url,
                                     std::string *title,
                                     std::string *description) = 0;

  /**
   * Gets feedback url of a gadget instance, where users can submit feedback of
   * this gadget.
   * This method returns an empty string if the instance has no feedback
   * url.
   */
  virtual std::string GetGadgetInstanceFeedbackURL(int instance_id) = 0;

 public:
  /**
   * Connects to signals when a gadget instance is added, to be removed or
   * should be updated. The int parameter of the callback is the gadget
   * instance id. The callback of OnNewGadgetInstance can return @c false to
   * cancel the action.
   */
  virtual Connection *ConnectOnNewGadgetInstance(
      Slot1<bool, int> *callback) = 0;
  virtual Connection *ConnectOnRemoveGadgetInstance(
      Slot1<void, int> *callback) = 0;
  virtual Connection *ConnectOnUpdateGadgetInstance(
      Slot1<void, int> *callback) = 0;
};

/**
 * @ingroup Gadget
 * @relates GadgetManagerInterface
 * Sets the global GadgetManagerInterface instance.
 * A GadgetManager extension module can call this function in its
 * @c Initialize() function.
 */
bool SetGadgetManager(GadgetManagerInterface *gadget_parser);

/**
 * @ingroup Gadget
 * @relates GadgetManagerInterface
 * Gets the GadgetManagerInterface instance.
 *
 * The returned instance is a singleton provided by a GadgetManager
 * extension module, which is loaded into the global ExtensionManager in
 * advance.
 */
GadgetManagerInterface *GetGadgetManager();

} // namespace ggadget

#endif // GGADGET_GADGET_MANAGER_INTERFACE_H__
