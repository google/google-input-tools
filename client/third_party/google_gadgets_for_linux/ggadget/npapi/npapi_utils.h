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

#ifndef GGADGET_NPAPI_NPAPI_H__
#define GGADGET_NPAPI_NPAPI_H__

#include <string>
#include <vector>

#include <ggadget/npapi/npapi_wrapper.h>

namespace ggadget {
namespace npapi {

#define NOT_IMPLEMENTED() LOG("Unimplemented function")

#define CHECK_MAIN_THREAD() do {                \
    if (!GetGlobalMainLoop()->IsMainThread())   \
      LOG("Called from wrong thread");          \
  } while (0)

#define ENSURE_MAIN_THREAD(retval) do {         \
    if (!GetGlobalMainLoop()->IsMainThread()) { \
      LOG("Called from wrong thread");          \
      return (retval);                          \
    }                                           \
  } while (0)

#define ENSURE_MAIN_THREAD_VOID() do {          \
    if (!GetGlobalMainLoop()->IsMainThread()) { \
      LOG("Called from wrong thread");          \
      return;                                   \
    }                                           \
  } while (0)

typedef char *(*NP_GetMIMEDescriptionUPP)(void);
typedef NPError (*NP_GetValueUPP)(void *instance, NPPVariable variable,
                                  void *value);
typedef NPError (*NP_InitializeUPP)(const NPNetscapeFuncs *netscape_funcs,
                                    NPPluginFuncs *plugin_funcs);
typedef NPError (*NP_ShutdownUPP)(void);

/**
 * Information structure for a plugin library.
 */
struct PluginLibraryInfo {
  PluginLibraryInfo();
  std::string path;
  std::string name;
  std::string description;
  std::vector<std::string> mime_types;
  int ref_count;
  /** The handle of the loaded library. */
  void *handle;
  NP_ShutdownUPP shutdown_proc;
  NPPluginFuncs plugin_funcs;
};

/**
 * Gets the information structure of the plugin library for the MIME type.
 * Returns NULL if the plugin for the MIME type can't be found or loaded.
 * If the library has already been loaded, the function simply add a reference
 * to the library.
 */
PluginLibraryInfo *GetPluginLibrary(const char *mime_type,
                                    const NPNetscapeFuncs *container_funcs);

/**
 * Releases the reference to a plugin library, and if the reference count
 * reaches zero, the library will be closed.
 */
void ReleasePluginLibrary(PluginLibraryInfo *info);

/** NPObject routines. */
NPObject *CreateNPObject(NPP instance, NPClass *a_class);
NPObject *RetainNPObject(NPObject *npobj);
void ReleaseNPObject(NPObject *npobj);

/** Converts a native string to NPVariant. */
void NewNPVariantString(const std::string &str, NPVariant *variant);
/** Releases a NPVariant value. */
void ReleaseNPVariant(NPVariant *variant);

/** Identifier management. */
NPIdentifier GetStringIdentifier(const NPUTF8 *name);
NPIdentifier GetIntIdentifier(int32_t int_id);
bool IdentifierIsString(NPIdentifier identifier);
NPUTF8 *UTF8FromIdentifier(NPIdentifier identifier);
std::string GetIdentifierName(NPIdentifier identifier);
int32_t IntFromIdentifier(NPIdentifier identifier);

void *MemAlloc(uint32 size);
void MemFree(void *ptr);

} // namespace npapi
} // namespace ggadget

#endif // GGADGET_NPAPI_NPAPI_H__
