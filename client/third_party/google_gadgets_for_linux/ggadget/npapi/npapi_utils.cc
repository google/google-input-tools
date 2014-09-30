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

#include "npapi_utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <cstring>

#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>

namespace ggadget {
namespace npapi {

PluginLibraryInfo::PluginLibraryInfo()
    : ref_count(0), handle(NULL), shutdown_proc(NULL) {
  memset(&plugin_funcs, 0, sizeof(plugin_funcs));
  plugin_funcs.size = sizeof(plugin_funcs);
}

static const char kEnvBrowserPluginsDir[] = "BROWSER_PLUGINS_DIR";

static void ScanDirsForPlugins(const std::vector<std::string> &dirs,
                               std::vector<std::string> *paths) {
  for (size_t i = 0; i < dirs.size(); i++) {
    struct stat state_value;
    if(::stat(dirs[i].c_str(), &state_value) != 0)
      continue;
    ::DIR *dp = ::opendir(dirs[i].c_str());
    if (dp) {
      struct dirent *dr;
      std::string lib;
      while ((dr = ::readdir(dp))) {
        lib = dr->d_name;
        std::string::size_type pos = lib.find_last_of(".");
        if (pos != lib.npos && lib.compare(pos, 3, ".so") == 0) {
          lib = dirs[i] + "/" + lib;
          paths->push_back(lib);
          DLOG("Add plugin: %s", lib.c_str());
        }
      }
      ::closedir(dp);
    }
  }
}

// TODO: Cross-platform.
static void GetPluginPaths(std::vector<std::string> *paths) {
  // Get paths of all NPAPI-compatible plugins. Check the environment
  // variable first. And then the default directory user specified during
  // compiling.
  std::vector<std::string> dirs;
  char *env_paths = getenv(kEnvBrowserPluginsDir);
  DLOG("Search plugins in dirs: %s", env_paths);
  if (env_paths)
    SplitStringList(env_paths, ":", &dirs);

  // Search for user installed plugins first.
  dirs.insert(dirs.begin(), BuildFilePath(GetHomeDirectory().c_str(),
                                          ".mozilla", "plugins", NULL));
  ScanDirsForPlugins(dirs, paths);

#ifdef GGL_DEFAULT_BROWSER_PLUGINS_DIR
  DLOG("And in dirs: %s", GGL_DEFAULT_BROWSER_PLUGINS_DIR);
  dirs.clear();
  SplitStringList(GGL_DEFAULT_BROWSER_PLUGINS_DIR, ":", &dirs);
  ScanDirsForPlugins(dirs, paths);
#endif
}

typedef std::vector<PluginLibraryInfo> PluginLibraries;
static PluginLibraries g_plugin_libraries;
// The keys are MIME types, and the values are the indexes into
// g_plugin_libraries, or kInvalidIndex if no plugin can be successfully
// loaded for the MIME type.
typedef LightMap<std::string, size_t> MIMEPluginMap;
static MIMEPluginMap g_mime_plugin_map;

static void EnsureLoadPluginsInfo() {
  if (!g_mime_plugin_map.empty())
    return;

  std::vector<std::string> paths;
  GetPluginPaths(&paths);
  for (size_t i = 0; i < paths.size(); i++) {
    void *handle = dlopen(paths[i].c_str(), RTLD_LAZY);
    if (!handle) {
      LOG("Failed to open library: %s", dlerror());
    } else {
      NP_GetMIMEDescriptionUPP get_mime_description_proc =
          (NP_GetMIMEDescriptionUPP)dlsym(handle, "NP_GetMIMEDescription");
      if (get_mime_description_proc) {
        char *mime_descriptions = get_mime_description_proc();
        if (mime_descriptions && *mime_descriptions) {
          std::vector<std::string> types;
          SplitStringList(mime_descriptions, ";", &types);
          if (!types.empty()) {
            size_t index = g_plugin_libraries.size();
            g_plugin_libraries.resize(index + 1);
            PluginLibraryInfo *info = &g_plugin_libraries[index];
            info->path = paths[i];
            for (size_t j = 0; j < types.size(); j++) {
              std::string type = types[j];
              std::string::size_type pos = type.find(':');
              if (pos != std::string::npos)
                type.erase(pos);
              info->mime_types.push_back(type);
            }
          }
        }
      }
      dlclose(handle);
    }
  }
}

static bool InitLibrary(PluginLibraryInfo *info,
                        const NPNetscapeFuncs *container_funcs) {
  info->handle = dlopen(info->path.c_str(), RTLD_LAZY);
  if (info->handle) {
    NP_InitializeUPP initialize_proc =
        (NP_InitializeUPP)dlsym(info->handle, "NP_Initialize");
    if (!initialize_proc) {
      // Some old plugins use NP_PluginInit as the name of the init function.
      initialize_proc = (NP_InitializeUPP)dlsym(info->handle, "NP_PluginInit");
    }
    if (!initialize_proc) {
      LOG("Failed to find NPAPI entry point from library: %s", dlerror());
    } else {
      memset(&info->plugin_funcs, 0, sizeof(NPPluginFuncs));
      info->plugin_funcs.size = sizeof(NPPluginFuncs);
      NPError ret = initialize_proc(container_funcs,
                                    &info->plugin_funcs);
      if (ret != NPERR_NO_ERROR) {
        LOG("Failed to initialize plugin %s - NPError code = %d",
            info->path.c_str(), ret);
      } else {
        info->shutdown_proc = (NP_ShutdownUPP)dlsym(info->handle,
                                                    "NP_Shutdown");
        NP_GetValueUPP get_value_proc = (NP_GetValueUPP)dlsym(info->handle,
                                                              "NP_GetValue");
        if (get_value_proc) {
          char *name = NULL, *description = NULL;
          get_value_proc(NULL, NPPVpluginNameString, &name);
          get_value_proc(NULL, NPPVpluginDescriptionString, &description);
          if (name)
            info->name = name;
          if (description)
            info->description = description;
        }

        DLOG("Successfully loaded plugin %s, name: %s, description: %s",
             info->path.c_str(), info->name.c_str(), info->description.c_str());
        return true;
      }
    }

    dlclose(info->handle);
    info->handle = NULL;
  }
  return false;
}

static bool ReferenceLibrary(PluginLibraryInfo *info,
                             const NPNetscapeFuncs *container_funcs) {
  if (info->ref_count < 0) {
    // This is a bad library.
    return false;
  }
  if (info->ref_count > 0) {
    // The library has already been successfully loaded.
    info->ref_count++;
    return true;
  }

  // The library needs to be loaded.
  if (InitLibrary(info, container_funcs)) {
    info->ref_count = 1;
    return true;
  }

  info->handle = NULL;
  info->ref_count = -1;
  return false;
}

static PluginLibraryInfo *LoadLibraryForMIMEType(
    const std::string mime_type, const NPNetscapeFuncs *container_funcs) {
  for (size_t i = 0; i < g_plugin_libraries.size(); i++) {
    PluginLibraryInfo *info = &g_plugin_libraries[i];
    if (std::find(info->mime_types.begin(), info->mime_types.end(),
                  mime_type) != info->mime_types.end() &&
        ReferenceLibrary(info, container_funcs)) {
      g_mime_plugin_map[mime_type] = i;
      return info;
    }
  }
  LOG("Failed to find plugin for MIME type %s", mime_type.c_str());
  g_mime_plugin_map[mime_type] = kInvalidIndex;
  return NULL;
}

PluginLibraryInfo *GetPluginLibrary(const char *mime_type,
                                    const NPNetscapeFuncs *container_funcs) {
  if (!mime_type || !*mime_type)
    return NULL;

  EnsureLoadPluginsInfo();
  PluginLibraryInfo *info = NULL;
  std::string mime_type_str(mime_type);
  MIMEPluginMap::const_iterator it = g_mime_plugin_map.find(mime_type_str);
  if (it == g_mime_plugin_map.end()) {
    return LoadLibraryForMIMEType(mime_type, container_funcs);
  }
  if (it->second == kInvalidIndex) {
    // We have known no plugin for this MIME type.
    return NULL;
  }

  info = &g_plugin_libraries[it->second];
  if (ReferenceLibrary(info, container_funcs))
    return info;

  // The library had been successfully loaded, and then released because no
  // one needed it. Now it failed to be loaded, we should rescan the libraries
  // for next usable library for the MIME type.
  return LoadLibraryForMIMEType(mime_type, container_funcs);
}

void ReleasePluginLibrary(PluginLibraryInfo *info) {
  ASSERT(info && info->ref_count > 0);
  if (!info || info->ref_count <= 0)
    return;

  if (--info->ref_count == 0) {
    if (info->shutdown_proc) {
      NPError ret = info->shutdown_proc();
      if (ret != NPERR_NO_ERROR) {
        LOG("Failed to shutdown plugin %s - nperror code %d",
            info->path.c_str(), ret);
      }
    }
    dlclose(info->handle);
    info->handle = NULL;
  }
}

NPObject *CreateNPObject(NPP instance, NPClass *a_class) {
  ENSURE_MAIN_THREAD(NULL);
  if (!a_class)
    return NULL;
  NPObject *obj;
  if (a_class->allocate) {
    obj = a_class->allocate(instance, a_class);
  } else {
    obj = new NPObject;
    memset(obj, 0, sizeof(NPObject));
    obj->_class = a_class;
  }
  obj->referenceCount = 1;
  return obj;
}

NPObject *RetainNPObject(NPObject *npobj) {
  ENSURE_MAIN_THREAD(NULL);
  if (npobj)
    npobj->referenceCount++;
  return npobj;
}

void ReleaseNPObject(NPObject *npobj) {
  ENSURE_MAIN_THREAD_VOID();
  if (npobj) {
    ASSERT(npobj->referenceCount > 0);
    if (--npobj->referenceCount == 0) {
      if (npobj->_class->deallocate) {
        npobj->_class->deallocate(npobj);
      } else {
        if (npobj->_class->invalidate)
          npobj->_class->invalidate(npobj);
        delete npobj;
      }
    }
  }
}

void NewNPVariantString(const std::string &str, NPVariant *variant) {
  ENSURE_MAIN_THREAD_VOID();
  size_t size = str.size();
  char *new_s = new char [size + 1];
  str.copy(new_s, size);
  new_s[size] = '\0';
  STRINGN_TO_NPVARIANT(new_s, static_cast<uint32_t>(size), *variant);
}

void ReleaseNPVariant(NPVariant *variant) {
  ENSURE_MAIN_THREAD_VOID();
  switch (variant->type) {
    case NPVariantType_String: {
      NPString *s = &NPVARIANT_TO_STRING(*variant);
      if (s->utf8characters)
        delete [] s->utf8characters;
      break;
    }
    case NPVariantType_Object: {
      NPObject *npobj = NPVARIANT_TO_OBJECT(*variant);
      if (npobj)
        ReleaseNPObject(npobj);
      break;
    }
    default:
      break;
  }
}

struct Identifier {
  enum IdType{
    TYPE_INT,
    TYPE_STRING,
  };

  Identifier(int32_t int_id)
      : type(TYPE_INT), int_id(int_id) { }
  Identifier(const std::string &name)
      : type(TYPE_STRING), int_id(-1), name(name) { }

  IdType type;
  int32_t int_id;
  std::string name;
};

LightMap<std::string, Identifier> g_string_identifiers;
LightMap<int32_t, Identifier> g_int_identifiers;

NPIdentifier GetStringIdentifier(const NPUTF8 *name) {
  ENSURE_MAIN_THREAD(NULL);
  if (!name)
    return NULL;

  std::string name_str(name);
  return &(g_string_identifiers.insert(std::make_pair(
      name_str, Identifier(name_str))).first->second);
}

NPIdentifier GetIntIdentifier(int32_t int_id) {
  ENSURE_MAIN_THREAD(NULL);
  return &(g_int_identifiers.insert(std::make_pair(
      int_id, Identifier(int_id))).first->second);
}

bool IdentifierIsString(NPIdentifier identifier) {
  ENSURE_MAIN_THREAD(false);
  if (!identifier)
    return false;
  return reinterpret_cast<Identifier *>(identifier)->type ==
         Identifier::TYPE_STRING;
}

NPUTF8 *UTF8FromIdentifier(NPIdentifier identifier) {
  ENSURE_MAIN_THREAD(NULL);
  Identifier *id = reinterpret_cast<Identifier *>(identifier);
  if (!id || id->type != Identifier::TYPE_STRING)
    return NULL;
  size_t size = id->name.size();
  NPUTF8 *buf = new NPUTF8[size + 1];
  buf[size] = '\0';
  id->name.copy(buf, size);
  return buf;
}

std::string GetIdentifierName(NPIdentifier identifier) {
  Identifier *id = reinterpret_cast<Identifier *>(identifier);
  return id && id->type == Identifier::TYPE_STRING ? id->name : "";
}

int32_t IntFromIdentifier(NPIdentifier identifier) {
  ENSURE_MAIN_THREAD(-1);
  Identifier *id = reinterpret_cast<Identifier *>(identifier);
  if (!id || id->type != Identifier::TYPE_INT)
    return -1; // The behaviour is undefined by NPAPI.
  return id->int_id;
}

void *MemAlloc(uint32 size) {
  CHECK_MAIN_THREAD();
  return new char[size];
}

void MemFree(void *ptr) {
  CHECK_MAIN_THREAD();
  delete [] reinterpret_cast<char*>(ptr);
}

} // namespace npapi
} // namespace ggadget
