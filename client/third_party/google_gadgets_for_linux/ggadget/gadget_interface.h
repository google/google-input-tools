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

#ifndef GGADGET_GADGET_INTERFACE_H__
#define GGADGET_GADGET_INTERFACE_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/logger.h>

namespace ggadget {

template <typename R, typename P1, typename P2> class Slot2;

class Connection;
class DOMDocumentInterface;
class FileManagerInterface;
class HostInterface;
class MenuInterface;
class OptionsInterface;
class Permissions;
class View;
class XMLHttpRequestInterface;

/**
 * @defgroup Gadget Gadget
 * @ingroup CoreLibrary
 * Gadget related classes.
 */

/**
 * @ingroup Gadget
 * An interface class to be implemented by the Gadget class.
 */
class GadgetInterface {
 public:

  /**
   * This ID uniquely identifies the type of the gadget instance. Each
   * implementation should define this field as a unique number.
   */
  static const uint64_t TYPE_ID = 0;

  virtual ~GadgetInterface() {}

  /** Gets the type id of this gadget instance. */
  virtual uint64_t GetTypeId() const = 0;

  /** Checks if this gadget is an instance of a give type. */
  virtual bool IsInstanceOf(uint64_t type_id) const = 0;

  /**
   * Asks the Host to remove the Gadget instance.
   *
   * Unlike just delete the Gadget instance, this method will remove the Gadget
   * instance from GadgetManager, so that it won't be displayed anymore.
   *
   * @param save_data if save the options data of this Gadget instance.
   */
  virtual void RemoveMe(bool save_data) = 0;

  /** Checks if this gadget instance is safe to remove. */
  virtual bool IsSafeToRemove() const = 0;

  /** Returns the HostInterface instance used by this gadget. */
  virtual HostInterface *GetHost() const = 0;

  /**
   * Checks if the gadget is valid or not.
   */
  virtual bool IsValid() const = 0;

  /** Returns the instance id of this gadget instance. */
  virtual int GetInstanceID() const = 0;

  /**
   * Gets the FileManager instance used by this gadget.
   * Caller shall not destroy the returned FileManager instance.
   */
  virtual FileManagerInterface *GetFileManager() const = 0;

  /**
   * Gets the Options instance used by this gadget.
   * Caller shall not destroy the returned Options instance.
   */
  virtual OptionsInterface *GetOptions() = 0;

  /**
   * Get a value configured in the gadget manifest file.
   * @param key the value key like a simple XPath expression. See
   *     gadget_consts.h for available keys, and @c ParseXMLIntoXPathMap() in
   *     xml_utils.h for details of the XPath expression.
   * @return the configured value. @c NULL if not found.
   */
  virtual std::string GetManifestInfo(const char *key) const = 0;

  /**
   * Parse XML into DOM, using the entities defined in strings.xml.
   */
  virtual bool ParseLocalizedXML(const std::string &xml,
                                 const char *filename,
                                 DOMDocumentInterface *xmldoc) const = 0;

  /** Gets the main view of this gadget. */
  virtual View *GetMainView() const = 0;

  /** Shows main view of the gadget. */
  virtual bool ShowMainView() = 0;

  /** Closes main view of the gadget. */
  virtual void CloseMainView() = 0;

  /** Checks if about dialog can be shown. */
  virtual bool HasAboutDialog() const = 0;

  /** Shows this gadget's about dialog. */
  virtual void ShowAboutDialog() = 0;

  /** Checks whether this gadget has options dialog. */
  virtual bool HasOptionsDialog() const = 0;

  /**
   * Show the options dialog, either old @c GDDisplayWindowInterface style or
   * XML view style, depending on whether @c options.xml exists.
   * @return @c true if succeeded.
   */
  virtual bool ShowOptionsDialog() = 0;

  /**
   * Fires just before the gadget's menu is displayed. Handle this event to
   * customize the menu.
   */
  virtual void OnAddCustomMenuItems(MenuInterface *menu) = 0;

  /**
   * Creates a new @c XMLHttpRequestInterface instance.
   * All instances created by this gadget share the same set of cookies.
   */
  virtual XMLHttpRequestInterface *CreateXMLHttpRequest() = 0;

  /**
   * Sets whether the gadget is currently in user interaction.
   * The state should only applicable within one event loop.
   * @param in_user_interaction whether the current event loop is in user
   *     interaction.
   * @return the old in_user_interaction value.
   */
  virtual bool SetInUserInteraction(bool in_user_interaction) = 0;

  /**
   * Returns the current value of in_user_interaction.
   */
  virtual bool IsInUserInteraction() const = 0;

  /**
   * Opens the given URL in the user's default web brower.
   * Only HTTP, HTTPS URLs are supported.
   * Only called during user interaction is allowed.
   *
   * This method is just a delegation of HostInterface::OpenURL().
   */
  virtual bool OpenURL(const char *url) const = 0;

  /** Gets Permissions of this gadget. */
  virtual const Permissions* GetPermissions() const = 0;

  /**
   * Gets the default font size, which can be customized by the user.
   * @return the default font point size.
   */
  virtual int GetDefaultFontSize() const = 0;

  /**
   * Connect a log listener which will receive all logs for this gadget.
   */
  virtual Connection *ConnectLogListener(
      Slot2<void, LogLevel, const std::string &> *listener) = 0;

};

#define DEFINE_GADGET_TYPE_ID(tid, super)                            \
  static const uint64_t TYPE_ID = UINT64_C(tid);                     \
  virtual bool IsInstanceOf(uint64_t type_id) const {                \
    return type_id == TYPE_ID || super::IsInstanceOf(type_id);       \
  }                                                                  \
  virtual uint64_t GetTypeId() const { return UINT64_C(tid); }

inline bool GadgetInterface::IsInstanceOf(uint64_t type_id) const {
  return type_id == TYPE_ID;
}

} // namespace ggadget

#endif // GGADGET_GADGET_INTERFACE_H__
