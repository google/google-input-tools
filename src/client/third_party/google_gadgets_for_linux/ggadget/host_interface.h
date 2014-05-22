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

#ifndef GGADGET_HOST_INTERFACE_H__
#define GGADGET_HOST_INTERFACE_H__

#include <vector>
#include <string>
#include <ggadget/view_host_interface.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;
class GadgetInterface;
class MainLoopInterface;
class OptionsInterface;
class ScriptableInterface;
class Signal;

/**
 * @ingroup Interfaces
 * Interface for providing host services to the gadgets.
 * All gadgets may share one HostInterface instance.
 * The @c HostInterface implementation should depend on the host.
 */
class HostInterface {
 public:
  virtual ~HostInterface() { }

  /**
   * Creates a new ViewHost instance.
   * The newly created ViewHost instance must be deleted by the caller
   * afterwards.
   *
   * @param gadget The Gadget instance which will own this ViewHost instance.
   * @param type Type of the new ViewHost instance.
   *
   * @return a new Viewhost instance.
   */
  virtual ViewHostInterface *NewViewHost(GadgetInterface *gadget,
                                         ViewHostInterface::Type type) = 0;

  /**
   * Loads a gadget instance from a specified path.
   *
   * @param path Path to the gadget file or directory.
   * @param options_name Options name for the gadget instance.
   * @param instance_id Unique instance id.
   * @param show_debug_console If it's true then shows debug console of this
   *        gadget instance as early as possible.
   *
   * @return the newly created gadget instance, or NULL if failed.
   */
  virtual GadgetInterface *LoadGadget(const char *path,
                                      const char *options_name,
                                      int instance_id,
                                      bool show_debug_console) = 0;

  /**
   * Requests that the gadget be removed from the container (e.g. sidebar).
   * The specified gadget shall be removed in the next main loop cycle,
   * otherwise the behavior is undefined.
   *
   * @param instance_id the id of the gadget instance to be removed.
   * @param save_data if @c true, the gadget's state is saved before the gadget
   *     is removed.
   */
  virtual void RemoveGadget(GadgetInterface *gadget, bool save_data) = 0;

  /** Temporarily install a given font on the system. */
  virtual bool LoadFont(const char *filename) = 0;

  /**
   * Shows a debug console that will display all logs for the gadget.
   */
  virtual void ShowGadgetDebugConsole(GadgetInterface *gadget) = 0;

  /**
   * Gets the default font size, which can be customized by the user.
   * @return the default font point size.
   */
  virtual int GetDefaultFontSize() = 0;

  /**
   * Opens the given URL in the user's default web brower.
   *
   * @param gadget The gadget which wants to open the url, the permissions of
   *        this gadget will be checked to see if opening the url is allowed.
   *        If it's NULL, then only urls with http:// or https:// prefixes can
   *        be opened.
   * @param url The url to be opened.
   */
  virtual bool OpenURL(const GadgetInterface *gadget, const char *url) = 0;
};

} // namespace ggadget

#endif // GGADGET_HOST_INTERFACE_H__
