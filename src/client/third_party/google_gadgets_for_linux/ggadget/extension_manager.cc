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

#include <algorithm>
#include <utility>
#include <vector>
#include <cstdlib>
#include "logger.h"
#include "module.h"
#include "common.h"
#include "extension_manager.h"
#include "small_object.h"

namespace ggadget {

ElementExtensionRegister::ElementExtensionRegister(ElementFactory *factory)
  : factory_(factory) {
}

bool ElementExtensionRegister::RegisterExtension(const Module *extension) {
  ASSERT(extension);
  // reinterpret_cast<> doesn't work on gcc 3.x
  RegisterElementExtensionFunc func = (RegisterElementExtensionFunc)(
      extension->GetSymbol(kElementExtensionSymbolName));
  return func ? func(factory_) : false;
}

ScriptExtensionRegister::ScriptExtensionRegister(
    ScriptContextInterface *context, GadgetInterface *gadget)
  : context_(context), gadget_(gadget) {
}

bool ScriptExtensionRegister::RegisterExtension(const Module *extension) {
  ASSERT(extension);
  // reinterpret_cast<> doesn't work on gcc 3.x
  RegisterScriptExtensionFunc func = (RegisterScriptExtensionFunc)(
      extension->GetSymbol(kScriptExtensionSymbolName));
  return func ? func(context_, gadget_) : false;
}

FrameworkExtensionRegister::FrameworkExtensionRegister(
    ScriptableInterface *framework, GadgetInterface *gadget)
  : framework_(framework), gadget_(gadget) {
}

bool FrameworkExtensionRegister::RegisterExtension(const Module *extension) {
  ASSERT(extension);
  // reinterpret_cast<> doesn't work on gcc 3.x
  RegisterFrameworkExtensionFunc func = (RegisterFrameworkExtensionFunc)(
      extension->GetSymbol(kFrameworkExtensionSymbolName));
  return func ? func(framework_, gadget_) : false;
}

ScriptRuntimeExtensionRegister::ScriptRuntimeExtensionRegister(
    ScriptRuntimeManager *manager)
  : manager_(manager) {
}

bool
ScriptRuntimeExtensionRegister::RegisterExtension(const Module *extension) {
  ASSERT(extension);
  // reinterpret_cast<> doesn't work on gcc 3.x
  RegisterScriptRuntimeExtensionFunc func =
      (RegisterScriptRuntimeExtensionFunc)(
          extension->GetSymbol(kScriptRuntimeExtensionSymbolName));
  return func ? func(manager_) : false;
}

FileManagerExtensionRegister::FileManagerExtensionRegister(
    FileManagerWrapper *fm_wrapper)
  : fm_wrapper_(fm_wrapper) {
}

bool FileManagerExtensionRegister::RegisterExtension(const Module *extension) {
  ASSERT(extension);
  // reinterpret_cast<> doesn't work on gcc 3.x
  RegisterFileManagerExtensionFunc func =
      (RegisterFileManagerExtensionFunc)(
          extension->GetSymbol(kFileManagerExtensionSymbolName));
  return func ? func(fm_wrapper_) : false;
}

class MultipleExtensionRegisterWrapper::Impl : public SmallObject<> {
 public:
  typedef std::vector<ExtensionRegisterInterface *> ExtRegisterVector;
  ExtRegisterVector ext_registers_;
};

MultipleExtensionRegisterWrapper::MultipleExtensionRegisterWrapper()
  : impl_(new Impl()) {
}

MultipleExtensionRegisterWrapper::~MultipleExtensionRegisterWrapper() {
  delete impl_;
}

bool MultipleExtensionRegisterWrapper::RegisterExtension(
    const Module *extension) {
  ASSERT(extension);

  bool result = false;

  Impl::ExtRegisterVector::iterator it = impl_->ext_registers_.begin();
  for (;it != impl_->ext_registers_.end(); ++it) {
    if ((*it)->RegisterExtension(extension))
      result = true;
  }

  return result;
}

void MultipleExtensionRegisterWrapper::AddExtensionRegister(
    ExtensionRegisterInterface *ext_register) {
  ASSERT(ext_register);
  impl_->ext_registers_.push_back(ext_register);
}

class ExtensionManager::Impl : public SmallObject<> {
  typedef std::vector<std::pair<std::string, Module*> > ExtensionVector;

 public:
  Impl() : readonly_(false) {
  }

  ~Impl() {
    // Destroy extensions in reverse order.
    for (ExtensionVector::reverse_iterator it = extensions_.rbegin();
         it != extensions_.rend(); ++it) {
      delete it->second;
    }
  }

  ExtensionVector::iterator FindExtension(const std::string &name) {
    ExtensionVector::iterator it = extensions_.begin();
    for (; it != extensions_.end(); ++it) {
      if (it->first == name)
        break;
    }
    return it;
  }

  Module *LoadExtension(const char *name, bool resident) {
    ASSERT(name && *name);
    if (readonly_) {
      LOG("Can't load extension %s, into a readonly ExtensionManager.",
          name);
      return NULL;
    }

    if (name && *name) {
      std::string name_str(name);

      // If the module has already been loaded, then just return it.
      ExtensionVector::iterator it = FindExtension(name_str);
      if (it != extensions_.end()) {
        if (!it->second->IsResident() && resident)
          it->second->MakeResident();
        return it->second;
      }

      Module *extension = new Module(name);
      if (!extension->IsValid()) {
        delete extension;
        return NULL;
      }

      if (resident)
        extension->MakeResident();

      extensions_.push_back(std::make_pair(name_str, extension));
      DLOG("Extension %s was loaded successfully.", name);
      return extension;
    }
    return NULL;
  }

  bool UnloadExtension(const char *name) {
    ASSERT(name && *name);
    if (readonly_) {
      LOG("Can't unload extension %s, from a readonly ExtensionManager.", name);
      return false;
    }

    if (name && *name) {
      std::string name_str(name);

      ExtensionVector::iterator it = FindExtension(name_str);
      if (it != extensions_.end()) {
        if (it->second->IsResident()) {
          LOG("Can't unload extension %s, it's resident.", name);
          return false;
        }
        delete it->second;
        extensions_.erase(it);
        return true;
      }
    }
    return false;
  }

  bool EnumerateLoadedExtensions(
      Slot2<bool, const char *, const char *> *callback) {
    ASSERT(callback);

    bool result = false;
    for (ExtensionVector::const_iterator it = extensions_.begin();
         it != extensions_.end(); ++it) {
      result = (*callback)(it->first.c_str(), it->second->GetName().c_str());
      if (!result) break;
    }

    delete callback;
    return result;
  }

  bool RegisterExtension(const char *name, ExtensionRegisterInterface *reg) {
    ASSERT(name && *name && reg);
    Module *extension = LoadExtension(name, false);
    if (extension && extension->IsValid()) {
      return reg->RegisterExtension(extension);
    }
    return false;
  }

  bool RegisterLoadedExtensions(ExtensionRegisterInterface *reg) {
    ASSERT(reg);
    if (extensions_.size()) {
      bool ret = true;
      for (ExtensionVector::const_iterator it = extensions_.begin();
           it != extensions_.end(); ++it) {
        if (!reg->RegisterExtension(it->second))
          ret = false;
      }
      return ret;
    }
    return false;
  }

  void SetReadonly() {
    // Don't make extensions resident, so that they can be unloaded when exit.
    readonly_ = true;
  }

 public:
  static void ExitHandler() {
    // Inform the logger not to use contexts any more, because the destruction
    // of the global manager will unload the module that log contexts might
    // depend on.
    FinalizeLogger();
    if (global_manager_) {
      DLOG("Destroy global extension manager.");
      delete global_manager_;
      global_manager_ = NULL;
    }
  }

  static bool SetGlobalExtensionManager(ExtensionManager *manager) {
    if (!global_manager_ && manager) {
      global_manager_ = manager;
      atexit(ExitHandler);
      return true;
    }
    return false;
  }

  static ExtensionManager *GetGlobalExtensionManager() {
    return global_manager_;
  }

  static ExtensionManager *global_manager_;

 private:
  ExtensionVector extensions_;
  bool readonly_;
};

ExtensionManager* ExtensionManager::Impl::global_manager_ = NULL;

ExtensionManager::ExtensionManager()
  : impl_(new Impl()) {
}

ExtensionManager::~ExtensionManager() {
  delete impl_;
}

bool ExtensionManager::Destroy() {
  if (this && this != Impl::global_manager_) {
    delete this;
    return true;
  }

  DLOG("Try to destroy %s ExtensionManager object.",
       (this == NULL ? "an invalid" : "the global"));

  return false;
}

bool ExtensionManager::LoadExtension(const char *name, bool resident) {
  return impl_->LoadExtension(name, resident) != NULL;
}

bool ExtensionManager::UnloadExtension(const char *name) {
  return impl_->UnloadExtension(name);
}

bool ExtensionManager::EnumerateLoadedExtensions(
    Slot2<bool, const char *, const char *> *callback) const {
  return impl_->EnumerateLoadedExtensions(callback);
}

bool ExtensionManager::RegisterExtension(const char *name,
                                         ExtensionRegisterInterface *reg) const {
  return impl_->RegisterExtension(name, reg);
}

bool ExtensionManager::RegisterLoadedExtensions(
    ExtensionRegisterInterface *reg) const {
  return impl_->RegisterLoadedExtensions(reg);
}

void ExtensionManager::SetReadonly() {
  impl_->SetReadonly();
}

const ExtensionManager *ExtensionManager::GetGlobalExtensionManager() {
  return Impl::GetGlobalExtensionManager();
}

bool ExtensionManager::SetGlobalExtensionManager(ExtensionManager *manager) {
  return Impl::SetGlobalExtensionManager(manager);
}

ExtensionManager *
ExtensionManager::CreateExtensionManager() {
  ExtensionManager *manager = new ExtensionManager();
  return manager;
}

} // namespace ggadget
