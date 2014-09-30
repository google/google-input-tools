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

#ifndef GGADGET_MODULE_H__
#define GGADGET_MODULE_H__

#include <string>
#include <ggadget/slot.h>

namespace ggadget {

/**
 * @ingroup Extension
 *
 * This class is used to load a dynamic module into the main process.
 * A dynamic module can provide additional components to extend the core
 * functionalities of libggadget.
 * For example, a dynamic module can provide additional Elements and
 * JavaScript objects which are not included in libggadget.
 *
 * A valid module must provide two public C functions:
 *
 * - bool modulename_LTX_Initialize();
 *   A function to initialize the module. This function will be called
 *   immediately when the module is loaded successfully. This function shall
 *   return true if the module is initialized correctly, otherwise false shall
 *   be returned. If it returns false, then the module will be unloaded
 *   immediately without any further operation.
 *
 * - void modulename_LTX_Finalize(void);
 *   A function to finalize the module. this function will be called just
 *   before unloading the module. All resources allocated inside the module
 *   must be freed here.
 *
 * The modulename_LTX_ prefix in above function names is used to avoid symbol
 * conflicts when loading multiple modules at the same time.
 * 'modulename' is the name of the module.
 *
 * All modules shall be installed into a predefined directory, usually it would
 * be $(libdir)/ggl-1.0/
 *
 * Environment variable GGL_MODULE_PATH can be used to specify additional
 * module search path. Multiple paths can be delimited by colon.
 */
class Module {
 public:
  /** Default constructor */
  Module();

  /**
   * Constructor to load a specified module directly.
   * This constructor will call @c Load() to load the module. Call @c IsValid()
   * to check if the module is loaded successfully before using it.
   * @sa Load().
   */
  explicit Module(const char *name);

  /**
   * Destructor. If the module was loaded, then it'll be unloaded. Destroying a
   * resident module may cause undefined behavior.
   */
  ~Module();

  /**
   * Loads a specified module.
   * @param name Name of the module file, the file extension can be omitted.
   *        If the specified name is not an absolute path, then the module
   *        will be searched in the default module path, and the paths
   *        specified by GGL_MODULE_PATH environment variable.
   * @return true if the module was loaded and initialized successfully.
   */
  bool Load(const char *name);

  /**
   * Unloads a loaded module.
   * A resident module can't be unloaded.
   *
   * @return true if the module was unloaded successfully.
   */
  bool Unload();

  /**
   * Check if the module has been loaded and initialized correctly.
   */
  bool IsValid() const;

  /**
   * Convert a module into a resident module, so that it can't be unloaded.
   *
   * @return true if success.
   */
  bool MakeResident();

  /**
   * Check if the module is resident. A resident module can't be unloaded. So
   * destroying a resident module may cause undefined behavior.
   */
  bool IsResident() const;

  /**
   * Get file path of the module.
   */
  std::string GetPath() const;

  /**
   * Get normalized module name.
   * The returned module name is same as the module file name without suffix,
   * but all characters other than [0-9a-zA-Z_] will be replaced with '_'.
   */
  std::string GetName() const;

  /**
   * Get a function pointer of a specified symbol.
   *
   * @return pointer to the symbol or NULL if the symbol is not available.
   */
  void *GetSymbol(const char *symbol_name) const;

 public:
  /**
   * Enumerate all predefined module search paths, including the paths
   * specified in the GGL_MODULE_PATH environment variable and built-in paths.
   * The specified callback will be called for each module search path until
   * the callback returns false.
   *
   * @param callback A slot to be called back for each module search path.
   * If the callback returns false, then the enumeration process will be
   * stopped. The slot will be deleted by this function before it returns.
   *
   * @return true if one or more search paths were found and all of them were
   * enumerated.
   */
  static bool EnumerateModulePaths(Slot1<bool, const char *> *callback);

  /**
   * Enumerate all modules in a specified module path. The specified callback
   * will be called for each module found in the specified module path, until
   * the callback returns false.
   *
   * @param path A specified searching path. Can be an absolute path or a
   * relative path. If it's a relative path or empty string, the default module
   * path and the paths specified by GGL_MODULE_PATH environment variable will
   * be searched with the specified relative path appended.
   *
   * @param callback A slot to be called back for each module found in the
   * path. If the callback returns false, then the enumeration process will be
   * stopped. The slot will be deleted by this function before it returns.
   * The parameter string of the callback is the full path of the module, so
   * that it can be used directly to load the module.
   *
   * @return true if some modules were found and all of them were enumerated.
   */
  static bool EnumerateModuleFiles(const char *path,
                                   Slot1<bool, const char *> *callback);

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(Module);
};

} // namespace ggadget

#endif // GGADGET_MODULE_H__
