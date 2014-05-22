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

#ifndef GGADGET_HOST_UTILS_H__
#define GGADGET_HOST_UTILS_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/light_map.h>
#include <ggadget/variant.h>
#include <ggadget/slot.h>
#include <ggadget/view_interface.h>

namespace ggadget {

template <typename R> class Slot0;
class OptionsInterface;
class HostInterface;
class Gadget;

/**
 * @defgroup HostUtilities Host utilities
 * @ingroup Utilities
 * @{
 */

/**
 * Setup the global file manager.
 * @param profile_dir path name of the user profile directory.
 * @return @c true if succeeds.
 */
bool SetupGlobalFileManager(const char *profile_dir);

/**
 * Setup the logger.
 * @param log_level the minimum @c LogLevel.
 * @param long_log whether to output logs using long format.
 */
void SetupLogger(int log_level, bool long_log);

/**
 * Checks if the required extensions are properly loaded.
 * @param[out] message the error message that should be displayed to the user
 *     if any required extension is not property loaded.
 * @return @c true if all the required extensions are property loaded.
 */
bool CheckRequiredExtensions(std::string *error_message);

#if defined(OS_POSIX)  // TODO(zkfan): support XMLHttpRequest
/**
 * Initialize the default user agent for XMLHttpRequest class.
 * @param app_name the name of the main application.
 */
void InitXHRUserAgent(const char *app_name);
#endif

/**
 * Get popup position to show a (w1, h1) rectangle for an existing (x,
 * y, w, h) rectangle on a scene of sw width and sh height.
 */
void GetPopupPosition(int x, int y, int w, int h,
                      int w1, int h1, int sw, int sh,
                      int *x1, int *y1);

#if defined(OS_POSIX)  // These functions are not used on windows currently.
/**
 * Show a stand-alone (not belonging to any gadget) dialog which is defined
 * in XML.
 * @param host
 * @param location the location of the view XML definition file which can be
 *     loaded by the global file manager.
 * @param flags combination of ViewInterface::OptionsViewFlags.
 * @param params parameters to be set into the 'optionsViewData' variable
 *     in the view script context.
 * @return @c true if OK button is clicked, otherwise @c false.
 */
bool ShowDialogView(HostInterface *host, const char *location, int flags,
                    const LightMap<std::string, Variant> &params);

/**
 * Sets up a default handler to Gadget's GetFeedbackURL signal.
 */
void SetupGadgetGetFeedbackURLHandler(Gadget *gadget);

/**
 * Show the About dialog of the application.
 */
void ShowAboutDialog(HostInterface *host);

/**
 * Structure to hold information of a host command line argument.
 *
 * - id
 *   A numeric id to identify the argument. Must be >= 0.
 * - type
 *   The value type argument. Only supports BOOL, INT64, DOUBLE and STRING.
 * - short_name
 *   Short name of the argument, such as '-d'.
 * - long_name
 *   Long name of the argument, such as '--debug'.
 */
struct HostArgumentInfo {
  int id;
  Variant::Type type;
  const char *short_name;
  const char *long_name;
};

/**
 * Class to parse host arguments.
 *
 * The argument can be specified by something like "--debug 1" or "--debug=1"
 */
class HostArgumentParser {
 public:
  /**
   * Constructor.
   * @param args An array of known arguments' information, terminated by an
   *        entry with id == -1. It must be statically defined.
   */
  explicit HostArgumentParser(const HostArgumentInfo *args);

  ~HostArgumentParser();

  /**
   * Starts the parse process. All existing state will be erased.
   * @return true if success.
   */
  bool Start();

  /**
   * Appends one argument.
   * @return true if success.
   */
  bool AppendArgument(const char *arg);

  /**
   * Appends multiple arguments.
   * @return true if success.
   */
  bool AppendArguments(int argc, const char * const argv[]);

  /**
   * Finish the parse process, and validate if the arguments are valid.
   * @return true if all arguments are valid, otherwise returns false.
   */
  bool Finish();

  /**
   * Gets the value of a argument.
   *
   * @param id The id of the argument.
   * @param[out] value The value of the argument if it's specified. It can be
   *             NULL if the value is not important.
   * @return true if the argument is specified.
   */
  bool GetArgumentValue(int id, Variant *value) const;

  /**
   * Enumerates all recognized arguments.
   *
   * @param slot A callback to be called for each recognized arguments, in
   *        unified format, such as --debug=1.
   *        Returning false to stop enumeration.
   * @return true if all recognized arguments are enumerated.
   */
  bool EnumerateRecognizedArgs(Slot1<bool, const std::string &> *slot) const;

  /**
   * Enumerates remained arguments that are not in the predefined argument
   * list.
   *
   * @param slot A callback to be called for each remained arguments.
   *        Returning false to stop enumeration.
   * @return true if all remained arguments are enumerated.
   */
  bool EnumerateRemainedArgs(Slot1<bool, const std::string &> *slot) const;

 public:
  /**
   * Signature string used by hosts to identify the start and finish of a
   * RunOnce message group.
   */
  static const char kStartSignature[];
  static const char kFinishSignature[];

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(HostArgumentParser);
};
#endif
/** @} */

} // namespace ggadget

#endif // GGADGET_HOST_UTILS_H__
