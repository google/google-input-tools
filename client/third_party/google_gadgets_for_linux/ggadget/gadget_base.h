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

#ifndef GGADGET_GADGET_BASE_H__
#define GGADGET_GADGET_BASE_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/logger.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/string_utils.h>

namespace ggadget {

class ScriptContextInterface;

/**
 * @defgroup Gadget Gadget
 * @ingroup CoreLibrary
 * Gadget related classes.
 */

/**
 * @ingroup Gadget
 * A base Gadget class that provides some common features.
 */
class GadgetBase : public GadgetInterface {
 public:
  GadgetBase(HostInterface *host, int instance_id);
  virtual ~GadgetBase();

  // Overridden from GadgetInterface:
  virtual HostInterface *GetHost() const;
  virtual int GetInstanceID() const;
  virtual XMLHttpRequestInterface *CreateXMLHttpRequest();
  virtual bool SetInUserInteraction(bool in_user_interaction);
  virtual bool IsInUserInteraction() const;
  virtual bool OpenURL(const char *url) const;
  virtual int GetDefaultFontSize() const;
  virtual Connection *ConnectLogListener(
      Slot2<void, LogLevel, const std::string &> *listener);

 protected:
  // Used by derived class to send log messages of a specified script context.
  std::string OnContextLog(LogLevel level,
                           const char *filename,
                           int line,
                           const std::string &message,
                           ScriptContextInterface *context);

  static FileManagerInterface *CreateFileManager(const char *manifest_filename,
                                                 const char *base_path,
                                                 const char *locale);

  static bool ExtractFileFromFileManager(FileManagerInterface *fm,
                                         const char *file,
                                         std::string *path);

  static bool ReadStringsAndManifest(FileManagerInterface *file_manager,
                                     const char *manifest_filename,
                                     const char *manifest_tag,
                                     StringMap *strings_map,
                                     StringMap *manifest_info_map);

  static bool GetManifestForLocale(const char *manifest_filename,
                                   const char *manifest_tag,
                                   const char *base_path,
                                   const char *locale,
                                   StringMap *data);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(GadgetBase);
};

} // namespace ggadget

#endif // GGADGET_GADGET_BASE_H__
