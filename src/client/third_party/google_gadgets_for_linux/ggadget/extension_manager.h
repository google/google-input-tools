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

#ifndef GGADGET_EXTENSION_MANAGER_H__
#define GGADGET_EXTENSION_MANAGER_H__

#include <string>
#include <ggadget/slot.h>

namespace ggadget {

class ElementFactory;
class FileManagerWrapper;
class ScriptContextInterface;
class ScriptableInterface;
class ScriptRuntimeManager;
class Module;
class GadgetInterface;

/**
 * @defgroup Extension Extension
 * @ingroup CoreLibrary
 * Extension related classes.
 * @{
 */

static const char kElementExtensionSymbolName[] = "RegisterElementExtension";
static const char kScriptExtensionSymbolName[] = "RegisterScriptExtension";
static const char kFrameworkExtensionSymbolName[] =
    "RegisterFrameworkExtension";
static const char kScriptRuntimeExtensionSymbolName[] =
    "RegisterScriptRuntimeExtension";
static const char kFileManagerExtensionSymbolName[] =
    "RegisterFileManagerExtension";

/**
 * Interface of real ExtensionRegister classes.
 *
 * An ExtensionRegister class is used by ExtensionManager class when
 * registering an extension. an ExtensionRegister class is in charge of
 * resolving necessary symbols for registering a certain type of extension and
 * call them to register the extension module.
 *
 * Different extension modules are distinguished by using different register
 * symbols (might be function, or data structure).
 *
 * Currently following different kind of extension modules are defined:
 *  - Element extension module
 *    An Element extension module can be used to provide additional element
 *    classes, such as platform dependent elements or gadget specific elements.
 *    This kind of extension modules must provide following symbol:
 *    - bool RegisterElementExtension(ElementFactory *factory);
 *    This function shall register factory functions of all elements provided by
 *    the module into the specified ElementFactory instance.
 *
 *  - Script extension module
 *    A Script extension module can be used to provide additional script
 *    classes or objects which can be used by gadgets in their script code.
 *    This kind of extension modules must provide following symbol:
 *    - bool RegisterScriptExtension(ScriptContextInterface *context,
 *                                   GadgetInterface *gadget);
 *    This function shall register all script classes or objects provided by
 *    the module into the specified ScriptContext instance.
 *
 *  - Framework extension module
 *    A framework extension module can be used to provide additional script
 *    objects under framework namespace. All platform dependent classes or
 *    objects under framework namespace shall be provided through framework
 *    extensions.
 *    This kind of extension modules must provide following symbol:
 *    - bool RegisterFrameworkExtension(ScriptableInterface *framework,
 *                                      GadgetInterface *gadget);
 *    The specified @c framework object will be exported as framework script
 *    object. All objects and methods belong to framework namespace must be
 *    registered to it.
 *    The objects and methods under framework.system must be registered to the
 *    "system" property of specified framework scriptable instance.
 *    The specified @c gadget object is the Gadget instance which owns the
 *    framework and system objects. All per gadget framework objects shall bind
 *    to the @c gadget object and be destroyed upon destroying the gadget.
 *    Currently only two objects are per gadget:
 *    - ScriptableAudio, which needs per gadget FileManager to load audio file.
 *    - ScriptablePerfmon, which needs gadget's main View to send event.
 *    Please refer to extensions/default_framework/default_framework.cc for
 *    how to implement a framework extension.
 *
 *  - ScriptRuntime extension module
 *    A ScriptRuntime extension module can be used to provide specified
 *    implementation of ScriptRuntimeInterface.
 *    This kind of extension modules must provide following symbol:
 *    - bool RegisterScriptRuntimeExtension(ScriptRuntimeManager *manager);
 *    The extension module shall register its static allocated ScriptRuntime
 *    object to the specified ScriptRuntimeManager object.
 *    The ScriptRuntime object shall be singleton and shared by all gadgets.
 *    So each ScriptRuntime extension shall only be registered one time.
 *
 *  - FileManager extension module
 *    A FileManager extension module can add file manager into the gadget
 *    file manager.
 *    This kind of extension modules must provide following symbol:
 *    - bool RegisterFileManagerExtension(FileManagerWrapper *fm_wrapper);
 *
 *  - Extension modules that registers global resources in Initialize()
 *    Some extension modules don't need to provide additional symbols, but
 *    register global resources with certain global functions provided by
 *    the main library:
 *    - XMLHttpRequest extension module: SetXMLHttpRequestFactory()
 *    - XMLParser extension module: SetXMLParser()
 *    - Options extension module: SetOptionsFactory()
 *    - Encryptor extension module: SetEncryptor()
 *
 * The register function provided by an extension module may be called multiple
 * times for different gadgets during the life time of the extension module.
 * A symbol prefix modulename_LTX_ shall be used to avoid possible symbol
 * conflicts. So the real function name shall look like:
 * modulename_LTX_RegisterElementExtension
 * modulename is the name of the extension module, with all characters
 * other than [0-9a-zA-Z_] replaced to '_' (underscore).
 */
class ExtensionRegisterInterface {
 public:
  virtual ~ExtensionRegisterInterface() {}

  /**
   * Function to register an extension module. It'll be called by
   * ExtensionManager class.
   *
   * Derived classes shall implement this function to do the real register
   * task, such as resolve necessary symbols and call them.
   *
   * @param extension The Module instance of the extension.
   * @return true if the extension has been registered successfully.
   */
  virtual bool RegisterExtension(const Module *extension) = 0;
};

/**
 * Register class for Element extension modules.
 */
class ElementExtensionRegister : public ExtensionRegisterInterface {
 public:
  explicit ElementExtensionRegister(ElementFactory *factory);
  virtual bool RegisterExtension(const Module *extension);

  typedef bool (*RegisterElementExtensionFunc)(ElementFactory *);
 private:
  ElementFactory *factory_;
  DISALLOW_EVIL_CONSTRUCTORS(ElementExtensionRegister);
};

/**
 * Register class for Script extension modules.
 */
class ScriptExtensionRegister : public ExtensionRegisterInterface {
 public:
  explicit ScriptExtensionRegister(ScriptContextInterface *context,
                                   GadgetInterface *gadget);
  virtual bool RegisterExtension(const Module *extension);

  typedef bool (*RegisterScriptExtensionFunc)(ScriptContextInterface *,
                                              GadgetInterface *);
 private:
  ScriptContextInterface *context_;
  GadgetInterface *gadget_;
  DISALLOW_EVIL_CONSTRUCTORS(ScriptExtensionRegister);
};

/**
 * Register class for framework extension modules.
 */
class FrameworkExtensionRegister : public ExtensionRegisterInterface {
 public:
  /**
   * @param framework The root scriptable object for framework namespace.
   * @param file_manager The FileManager instance used by a specified gadget.
   */
  FrameworkExtensionRegister(ScriptableInterface *framework,
                             GadgetInterface *gadget);
  virtual bool RegisterExtension(const Module *extension);

  typedef bool (*RegisterFrameworkExtensionFunc)(ScriptableInterface *,
                                                 GadgetInterface *);
 private:
  ScriptableInterface *framework_;
  GadgetInterface *gadget_;
  DISALLOW_EVIL_CONSTRUCTORS(FrameworkExtensionRegister);
};

/**
 * Register class for ScriptRuntime extension modules.
 *
 * This class shall only be used in main program to initialize all available
 * ScriptRuntime extensions at the very beginning.
 */
class ScriptRuntimeExtensionRegister : public ExtensionRegisterInterface {
 public:
  ScriptRuntimeExtensionRegister(ScriptRuntimeManager *manager);
  virtual bool RegisterExtension(const Module *extension);

  typedef bool (*RegisterScriptRuntimeExtensionFunc)(ScriptRuntimeManager *);

 private:
  ScriptRuntimeManager *manager_;
  DISALLOW_EVIL_CONSTRUCTORS(ScriptRuntimeExtensionRegister);
};

/**
 * Register class for FileManager extension modules.
 */
class FileManagerExtensionRegister : public ExtensionRegisterInterface {
 public:
  FileManagerExtensionRegister(FileManagerWrapper *fm_wrapper);
  virtual bool RegisterExtension(const Module *extension);

  typedef bool (*RegisterFileManagerExtensionFunc)(FileManagerWrapper *);

 private:
  FileManagerWrapper *fm_wrapper_;
  DISALLOW_EVIL_CONSTRUCTORS(FileManagerExtensionRegister);
};

/**
 * A wrapper register class which can hold multiple real extension register
 * instances. This class can be used with ExtensionManager to register all
 * extensions with different types altogether.
 */
class MultipleExtensionRegisterWrapper : public ExtensionRegisterInterface {
 public:
  MultipleExtensionRegisterWrapper();
  virtual ~MultipleExtensionRegisterWrapper();

  /**
   * This function will register the specified extension module by all
   * extension register instances added to this wrapper.
   *
   * @return true if the specified extension module is registered by at least
   * one extension register instance.
   */
  virtual bool RegisterExtension(const Module *extension);

 public:
  /**
   * Adds an extension register instance into this wrapper.
   *
   * @param ext_register The extension register instance. The wrapper doesn't
   * own this register instance. The caller shall destroy it afterwards,
   * if necessary.
   */
  void AddExtensionRegister(ExtensionRegisterInterface *ext_register);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(MultipleExtensionRegisterWrapper);
};

/**
 * class ExtensionManager is used to manage extension modules.
 *
 * An extension module can be used to provide additional Elements and Script
 * objects to gadgets.
 *
 * In addition to Initialize() and Finalize(), an extension module shall
 * provide additional symbols to identify its type.
 *
 * @sa ExtensionRegisterInterface
 */
class ExtensionManager {
 public:

  /**
   * Destroy a ExtensionManager object.
   *
   * @return true upon success. It's only possible to return false when trying
   * to destroy the global ExtensionManager instance, which can't be destroyed.
   */
  bool Destroy();

  /**
   * Loads a specified extension module.
   * @param name Name of the extension module (may or may not include the
   *     extension name). Can be a full path to the module file.
   * @param resident Indicates if the extension should be resident in memory.
   *     A resident extension module can't be unloaded.
   * @return true if the extension was loaded and initialized successfully.
   */
  bool LoadExtension(const char *name, bool resident);

  /**
   * Unloads a loaded extension.
   * A resident extension can't be unloaded.
   *
   * @param name The exactly same name used to load the module.
   *
   * @return true if the extension was unloaded successfully.
   */
  bool UnloadExtension(const char *name);

  /**
   * Enumerate all loaded extensions.
   *
   * The caller shall not unload any extension during enumeration.
   *
   * @param callback A slot which will be called for each loaded extension,
   * the first parameter is the name used to load the extension. the second
   * parameter is the normalized name of the extension, without any file path
   * information. If the callback returns false, then the enumeration process
   * will be interrupted.
   *
   * @return true if all extensions have been enumerated. If there is no
   * loaded extension or the callback returns false, then returns false.
   */
  bool EnumerateLoadedExtensions(
      Slot2<bool, const char *, const char *> *callback) const;

  /**
   * Registers Element classes and Script objects provided by a specified
   * extension.
   *
   * If the specified extension has not been loaded, then try to load it first.
   *
   * @param name The name of the extension to be registered. It must be the
   * same name used to load the extension.
   * @param ext_register An extension register instance which will be called to
   * register the specified extension. A MultipleExtensionRegisterWrapper
   * instance can be used, which might contain multiple register instances of
   * different extension types, in case the extension module may provides
   * multiple extensions with different types.
   *
   * @return true if the extension has been loaded and registered successfully.
   */
  bool RegisterExtension(const char *name,
                         ExtensionRegisterInterface *ext_register) const;

  /**
   * Registers Element classes and Script objects provided by all loaded
   * extensions.
   *
   * @param ext_register An extension register instance which will be called to
   * register the loaded extension modules. A MultipleExtensionRegisterWrapper
   * instance can be used, which might contain multiple register instances of
   * different extension types, in case there are multiple extension modules
   * with different types.
   *
   * @return true if all loaded extensions have been registered successfully.
   */
  bool RegisterLoadedExtensions(ExtensionRegisterInterface *ext_register) const;

  /**
   * Sets this extension manager readonly. After this, no more LoadExtension()
   * or UnloadExtension() is allowed, and all loaded extension modules are made
   * resident.
   */
  void SetReadonly();

 public:
  /**
   * Set an ExtensionManager instance as the global singleton for all gadgets.
   *
   * The global ExtensionManager instance can only be set once, and after
   * setting a manager as the global singleton, no extensions can be added or
   * removed from the manager anymore.
   *
   * @return true upon success. If the global ExtensionManager is already set,
   * returns false.
   */
  static bool SetGlobalExtensionManager(ExtensionManager *manager);

  /**
   * Returns the global ExtensionManager singleton previously set by calling
   * SetGlobalExtensionManager().
   * The global ExtensionManager instance is is shared by all gadgets.
   * The caller shall not destroy it.
   */
  static const ExtensionManager *GetGlobalExtensionManager();

  static ExtensionManager *CreateExtensionManager();

 private:
  class Impl;
  Impl *impl_;

  /**
   * Private constructor to prevent creating ExtensionManager object directly.
   */
  ExtensionManager();

  /**
   * Private destructor to prevent deleting ExtensionManager object directly.
   */
  ~ExtensionManager();

  DISALLOW_EVIL_CONSTRUCTORS(ExtensionManager);
};

/** @} */

} // namespace ggadget

#endif // GGADGET_EXTENSION_MANAGER_H__
