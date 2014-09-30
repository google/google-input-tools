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

#ifndef GGADGET_TESTS_MOCKED_GADGET_H__
#define GGADGET_TESTS_MOCKED_GADGET_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/gadget_base.h>
#include <ggadget/memory_options.h>
#include <ggadget/permissions.h>
#include <ggadget/string_utils.h>
#include <ggadget/view.h>
#include <ggadget/xml_parser_interface.h>

using namespace ggadget;

/**
 * @defgroup Gadget Gadget
 * @ingroup CoreLibrary
 * Gadget related classes.
 */

/**
 * @ingroup Gadget
 * A mock Gadget class for unit tests. It's now only for mocking a Permissions
 * object and an Options object.
 */
class MockedGadget : public GadgetBase {
 public:
  DEFINE_GADGET_TYPE_ID(0x3afa89d7b42c42d8, GadgetBase);

  explicit MockedGadget(HostInterface* host)
      : GadgetBase(host, 0) {
  }

  // Overridden from GadgetInterface:
  virtual void RemoveMe(bool save_data) {
    GGL_UNUSED(save_data);
  }

  virtual bool IsSafeToRemove() const {
    return true;
  }

  virtual bool IsValid() const {
    return true;
  }

  virtual FileManagerInterface* GetFileManager() const {
    return NULL;
  }

  virtual OptionsInterface* GetOptions() {
    return &options_;
  }

  virtual std::string GetManifestInfo(const char* key) const {
    GGL_UNUSED(key);
    return std::string();
  }

  virtual bool ParseLocalizedXML(const std::string &xml,
                                 const char* filename,
                                 DOMDocumentInterface* xmldoc) const {
    return GetXMLParser()->ParseContentIntoDOM(
        xml, NULL, xml.c_str(), NULL, NULL, NULL, xmldoc, NULL, NULL);
  }

  virtual View* GetMainView() const {
    return false;
  }

  virtual bool ShowMainView() {
    return false;
  }

  virtual void CloseMainView() {
  }

  virtual bool HasAboutDialog() const {
    return false;
  }

  virtual void ShowAboutDialog() {
  }

  virtual bool HasOptionsDialog() const {
    return false;
  }

  virtual bool ShowOptionsDialog() {
    return false;
  }

  virtual void OnAddCustomMenuItems(MenuInterface *menu) {
    GGL_UNUSED(menu);
  }

  virtual const Permissions* GetPermissions() const {
    return &permissions_;
  }

  Permissions* GetMutablePermissions() {
    return &permissions_;
  }

 private:
  MemoryOptions options_;
  Permissions permissions_;

  DISALLOW_EVIL_CONSTRUCTORS(MockedGadget);
};

#endif // GGADGET_TESTS_MOCKED_GADGET_H__
