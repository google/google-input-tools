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

#ifndef GGADGET_GADGET_H__
#define GGADGET_GADGET_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/gadget_base.h>
#include <ggadget/string_utils.h>

namespace ggadget {

template <typename R> class Slot0;
template <typename R, typename P1> class Slot1;
class DetailsViewData;
class ScriptableInterface;

/**
 * @defgroup Gadget Gadget
 * @ingroup CoreLibrary
 * Gadget related classes.
 */

/**
 * @ingroup Gadget
 * A class to hold a Google Desktop Gadget instance.
 */
class Gadget : public GadgetBase {
 public:
  DEFINE_GADGET_TYPE_ID(0xde6ec21196ed4215, GadgetBase);

  /** special commands that can be executed by gadget. */
  enum Command {
    /** Show About dialog. */
    CMD_ABOUT_DIALOG = 1,
    /** User clicked the 'back' button. */
    CMD_TOOLBAR_BACK = 2,
    /** User clicked the 'forward' button. */
    CMD_TOOLBAR_FORWARD = 3
  };

  /** Display states of the gadget's main view. */
  enum DisplayState {
    /** Tile is not visible. */
    TILE_DISPLAY_STATE_HIDDEN = 0,
    /** Tile is restored from being minimized or popped out states. */
    TILE_DISPLAY_STATE_RESTORED = 1,
    /** Tile is minimized and only the title bar is visible. */
    TILE_DISPLAY_STATE_MINIMIZED = 2,
    /** Tile is 'popped-out' of the sidebar in a separate window. */
    TILE_DISPLAY_STATE_POPPED_OUT = 3,
    /** Tile is resized. */
    TILE_DISPLAY_STATE_RESIZED = 4
  };

  /** Display targets of the gadget's main view. */
  enum DisplayTarget {
    /** Item is being displayed/drawn in the Sidebar. */
    TARGET_SIDEBAR = 0,
    /** Item is being displayed/drawn in the notification window. */
    TARGET_NOTIFIER = 1,
    /** Item is being displayed in its own window floating on the desktop */
    TARGET_FLOATING_VIEW = 2,
    TARGET_INVALID
  };

  /**
   * Special flags that can be changed by gadget.
   * These flags shall be handled by VewHost or ViewDecorator to determine how
   * to show the decorator of the gadget's main view.
   */
  enum PluginFlags {
    PLUGIN_FLAG_NONE = 0,
    /** Adds a "back" button in the plugin toolbar. */
    PLUGIN_FLAG_TOOLBAR_BACK = 1,
    /** Adds a "forward" button in the plugin toolbar. */
    PLUGIN_FLAG_TOOLBAR_FORWARD = 2
  };

  enum DebugConsoleConfig {
    /** Disable debug console. */
    DEBUG_CONSOLE_DISABLED = 0,
    /** Display a "Debug Console" menu item. */
    DEBUG_CONSOLE_ON_DEMMAND = 1,
    /** Show the debug console during gadget initialization. */
    DEBUG_CONSOLE_INITIAL = 2
  };

 public:
  /**
   * Constructor.
   *
   * The gadget will be loaded and initialized, if failed then IsValid() method
   * will return false.
   *
   * The gadget's initial granted and denied permissions must be stored in its
   * options database as an internal value with key "permissions". If there is
   * no initial permissions when creating the gadget instance, then the gadget
   * instance will NOT get any permissions granted by default.
   *
   * @param host the host of this gadget.
   * @param base_path the base path of this gadget. It can be a directory,
   *     path to a .gg file, or path to a gadget.gmanifest file.
   * @param options_name the name of the options instance for this instance.
   * @param instance_id An unique id to identify this Gadget instance. It can
   *        be used to remove this Gadget instance by calling
   *        HostInteface::RemoveGadget() method.
   * @param global_permissions The global granted and denied permissions.
   *        The granted permissions for the gadget instance is the intersection
   *        of global permissions and the gadget's initial permissions.
   * @param debug_console_config
   */
  Gadget(HostInterface *host,
         const char *base_path,
         const char *options_name,
         int instance_id,
         const Permissions &global_permissions,
         DebugConsoleConfig debug_console_config);

  /** Destructor */
  virtual ~Gadget();

  // Overridden from GadgetInterface:
  virtual void RemoveMe(bool save_data);
  virtual bool IsSafeToRemove() const;
  virtual bool IsValid() const;
  virtual FileManagerInterface *GetFileManager() const;
  virtual OptionsInterface *GetOptions();
  virtual std::string GetManifestInfo(const char *key) const;
  virtual bool ParseLocalizedXML(const std::string &xml,
                                 const char *filename,
                                 DOMDocumentInterface *xmldoc) const;
  virtual View *GetMainView() const;
  virtual bool ShowMainView();
  virtual void CloseMainView();
  virtual bool HasAboutDialog() const;
  virtual void ShowAboutDialog();
  virtual bool HasOptionsDialog() const;
  virtual bool ShowOptionsDialog();
  virtual void OnAddCustomMenuItems(MenuInterface *menu);
  virtual const Permissions* GetPermissions() const;

  // Additional methods for Google Desktop Gadgets.

  /** Returns current plugin flags of the gadget. */
  int GetPluginFlags() const;

  DisplayTarget GetDisplayTarget() const;

  void SetDisplayTarget(DisplayTarget target);

  /**
   * Shows an XML view as a modal options dialog.
   * @param flags combination of @c ViewInterface::OptionsViewFlags values to
   *     indicate which dialog buttons should be displayed.
   * @param xml_file the file name within the current gadget.
   * @param param the param to be registered as a variable named
   *     "optionsViewData" in the options view script context. It can be used
   *     to pass parameters and return values to and from the dialog.
   * @return @c true if OK button is clicked, otherwise @c false.
   */
  bool ShowXMLOptionsDialog(int flags, const char *xml_file,
                            ScriptableInterface *param);

  /**
   * Displays a details view containing the specified details control and the
   * specified title.  If there is already details view opened, it will be
   * closed first.
   * @param details_view
   * @param title the title of the details view.
   * @param flags combination of @c ViewInterface::DetailsViewFlags.
   * @param feedback_handler called when user clicks on feedback buttons. The
   *     handler has one parameter, which specifies @c DetailsViewFlags.
   */
  bool ShowDetailsView(DetailsViewData *details_view_data,
                       const char *title, int flags,
                       Slot1<bool, int> *feedback_handler);

  /**
   * Close the details view if it is opened.
   */
  void CloseDetailsView();


  /** Execute a gadget specific command. */
  void OnCommand(Command command);

  /**
   * Connects a slot to the DisplayStateChanged signal. The specified slot will
   * be called when the gadget's main view's display state is changed.
   *
   * The slot accepts one int parameter which is the new display state value.
   */
  Connection *ConnectOnDisplayStateChanged(Slot1<void, int> *handler);

  /**
   * Connects a slot to the DisplayTargetChanged signal. The specified slot will
   * be called when the gadget's display target is changed.
   *
   * The slot accepts one int parameter which is the new display target value.
   */
  Connection *ConnectOnDisplayTargetChanged(Slot1<void, int> *handler);

  /**
   * Connects a slot to the PluginFlagsChanged signal. The specified slot will
   * be called when the gadget's plugin flags is changed.
   *
   * Either ViewHost or ViewDecorator of the gadget's main view shall connect
   * to this signal to monitor the changes of plugin flags and update the view
   * decorator UI accordingly.
   *
   * The slot accepts one int parameter which is the new plugin flags value.
   */
  Connection *ConnectOnPluginFlagsChanged(Slot1<void, int> *handler);

  /**
   * Connects a slot to the GetFeedbackURL signal. The specified slot will be
   * called when user request to open the feedback url of this gadget instance.
   *
   * This slot shall return the feedback url. If this gadget instance doesn't
   * have feedback url, an empty string shall be returned.
   *
   * Usually, Host shall connect to this signal and decide which feedback url
   * should be opened.
   */
  Connection *ConnectOnGetFeedbackURL(Slot0<std::string> *handler);

 public:
  /**
   * A utility to get the manifest infomation of a gadget without
   * constructing a Gadget object.
   * @param base_path see document for Gadget constructor.
   * @param[out] data receive the manifest data.
   * @return @c true if succeeds.
   */
  static bool GetGadgetManifest(const char *base_path, StringMap *data);

  /**
   * Locale version of GetGadgetManifest. You can specify locale other than
   * using system locale.
   */
  static bool GetGadgetManifestForLocale(const char *base_path,
                                         const char *locale,
                                         StringMap *data);
  /**
   * A utility to get an FileManagerInterface of a gadget without constructing a
   * Gadget object.
   * @param base_path see document for Gadget constructor.
   * @param locale Locale to be used, system locale will be used if it's NULL
   *               or is "".
   * @return the file manager of the gadget
   */
  static FileManagerInterface *GetGadgetFileManagerForLocale(
      const char *base_path,
      const char *locale);

  /**
   * A utility to get required permissions of a gadget from its manifest
   * information.
   *
   * @param manifest Manifest information loaded by GetGadgetManifest().
   * @param[out] required Stores the required permissions, the old required
   *             permissions information will be erased, but the granted/denied
   *             information will remain untouched.
   * @return true if permissions information is available.
   */
  static bool GetGadgetRequiredPermissions(const StringMap *manifest,
                                           Permissions *required);

  /**
   * Saves the initial permissions of a gadget instance.
   *
   * The gadget's initial permissions is stored in its options database as an
   * internal value with key "permissions".
   *
   * @param options_path Path to the options database of this gadget instance.
   * @param permissions The initial permissions to be saved. Only
   *        granted/denied permissions will be saved, required permissions will
   *        be discarded.
   * @return true if the initial permissions is saved successfully. If the
   *        initial permissions was already saved before, or anything wrong
   *        happens, false will be returned.
   */
  static bool SaveGadgetInitialPermissions(const char *options_path,
                                           const Permissions &permissions);

  /**
   * Loads the initial permissions of a gadget instance.
   *
   * The gadget's initial permissions is stored in its options database as an
   * internal value with key "permissions".
   *
   * @param options_path Path to the options database of this gadget instance.
   * @param permissions The initial permissions to be loaded. Only
   *        granted/denied permissions will be loaded, required permissions will
   *        not be changed.
   * @return true if the initial permissions is loaded successfully. If the
   *        initial permissions was not available, or anything wrong
   *        happens, false will be returned.
   */
  static bool LoadGadgetInitialPermissions(const char *options_path,
                                           Permissions *permissions);
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Gadget);
};

} // namespace ggadget

#endif // GGADGET_GADGET_H__
