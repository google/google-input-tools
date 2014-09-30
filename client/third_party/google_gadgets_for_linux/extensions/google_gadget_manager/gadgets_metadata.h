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

#ifndef GGADGET_GOOGLE_GADGETS_METADATA_H__
#define GGADGET_GOOGLE_GADGETS_METADATA_H__

#include <time.h>
#include <map>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>

namespace ggadget {

class FileManagerInterface;
class XMLHttpRequestInterface;
template <typename R, typename P1, typename P2> class Slot2;

namespace google {

const char kPluginsXMLRequestPrefix[] =
    "https://clients2.google.com/desktop/plugins.xml?platform="
    GGL_PLATFORM_SHORT "&cv=" GGL_API_VERSION;

const char kBuiltinGadgetsXMLLocation[] = "resource://builtin_gadgets.xml";
const char kPluginsXMLLocation[] = "profile://plugins.xml";

enum GadgetSource {
  SOURCE_LOCAL_FILE,
  SOURCE_BUILTIN,
  SOURCE_PLUGINS_XML
};

/** This structure contains the metadata for a single gadget. */
struct GadgetInfo {
  GadgetInfo()
      : source(SOURCE_PLUGINS_XML), updated_date(0), accessed_date(0) {
  }

  /**
   * This id is used throughout this system to uniquely identifies a gadget.
   * For now we use the guid attribute in plugins.xml for sidebar gadgets,
   * the download_url attribute for igoogle gadgets, or the local path for
   * pre-installed and local file gadgets.
   */
  std::string id;

  GadgetSource source;

  /**
   * Maps from names to values for all attributes defined with the <plugin>
   * element in plugins.xml for this gadget.
   */
  StringMap attributes;

  /**
   * Maps from locale names to localized titles defined with the <title>
   * subelement of the <plugin> element in plugins.xml for this gadget.
   * This field is only applicable for gadgets from plugins.xml.
   * The locale names are in lower case.
   */
  StringMap titles;

  /**
   * Maps from locale names to localized description defined with the
   * <description> subelement of the <plugin> element in plugins.xml
   * for this gadget. This field is only applicable for gadgets from
   * plugins.xml. The locale names are in lower case.
   */
  StringMap descriptions;

  /**
   * The last updated time of this gadget. Its value is parsed from the
   * updated_date attribute if exists or created_date attribute.
   * The value is number of milliseconds since EPOCH.
   */
  uint64_t updated_date;

  /**
   * The last accessed time, i.e. when the gadget was last added. This value
   * is filled by GadgetManager. The value is number of milliseconds since
   * EPOCH.
   */
  uint64_t accessed_date;
};

typedef LightMap<std::string, GadgetInfo> GadgetInfoMap;

class GadgetsMetadata {
 public:
  /**
   * Constructs a @c GadgetMetaData instance. The cached plugins.xml file will
   * be loaded into memory if it exists.
   */
  GadgetsMetadata();
  ~GadgetsMetadata();

  /** Initialize this object. Mainly for unittest. */
  void Init();

  /**
   * Temporarily free the memory used to store metadata to save memory.
   * Any later call to @c Init(), @c UpdateFromServer() or
   * @c GetAllGadgetInfo() will read the metadata back into memory.
   */
  void FreeMemory();

 public:
  /**
   * Asynchronously updates gadget metadata from the server. After a successful
   * download, the updated data will be saved into the local plugins.xml file.
   * @param full_download if @c true, a full download will be performed.
   * @param request a newly created XMLHttpRequestInterface instance. This
   *     parameter is provided to ease unittest.
   * @param on_done (optional) will be called when this request is done.
   *     If this parameter is provided, the caller must ensure the slot is
   *     available during the request or the life of this GadgetsMetadata
   *     object. The first bool parameter of this slot indicates whether the
   *     download request is successful, the second bool parameter indicates
   *     whether the result is successfully parsed.
   */
  void UpdateFromServer(bool full_download, XMLHttpRequestInterface *request,
                        Slot2<void, bool, bool> *on_done);

  /**
   * Returns a map from gadget id (which for now is the shortname attribute) to
   * GadgetInfo. The returned value is not const, because we allow
   * @c GadgetManager to update some fields of @c GadgetInfo.
   */
  GadgetInfoMap *GetAllGadgetInfo();
  const GadgetInfoMap *GetAllGadgetInfo() const;

  /**
   * Adds the metadata of a local gadget, so that the user can view and add
   * the gadget in the gadget browser.
   * Note: if FreeMemory() is called, the added local gadget info will be
   * cleared and won't be refreshed by GetAllGadgetInfo(), so the caller must
   * add them again to make them available.
   */
  const GadgetInfo *AddLocalGadgetInfo(const char *path);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(GadgetsMetadata);
};

} // namespace google
} // namespace ggadget

#endif // GGADGET_GOOGLE_GADGETS_METADATA_H__
