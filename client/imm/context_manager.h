/*
  Copyright 2014 Google Inc.

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

#ifndef GOOPY_IMM_CONTEXT_MANAGER_H__
#define GOOPY_IMM_CONTEXT_MANAGER_H__

#include <atlbase.h>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "imm/context.h"
#include "imm/candidate_info.h"
#include "imm/composition_string.h"
#include "imm/context_locker.h"
#include "imm/debug.h"
#include "imm/immdev.h"

namespace ime_goopy {
namespace imm {
template <class ContextType>
class ContextManagerT {
 public:
  ~ContextManagerT() {
    DestroyAll();
  }

  // Create a context in the pool. The caller should call Destroy to delete the
  // context if user is switching to another input method.
  bool Add(HIMC himc, ContextType* context) {
    assert(himc);
    if (!context) return false;

    CComCritSecLock<CComAutoCriticalSection> lock(critical_section_);
    ContextMap::const_iterator iter = context_map_.find(himc);
    if (iter != context_map_.end()) return iter->second;

    context_map_[himc] = context;
    return true;
  }

  ContextType* Get(HIMC himc) {
    if (!himc) return NULL;
    CComCritSecLock<CComAutoCriticalSection> lock(critical_section_);
    ContextMap::const_iterator iter = context_map_.find(himc);
    if (iter == context_map_.end()) return NULL;
    return iter->second;
  }

  // Save the mapping of context <-> ui_manager
  bool AssociateUIManager(ContextInterface* context,
                          UIManagerInterface* ui_manager) {
    if (!context || !ui_manager) return false;
    CComCritSecLock<CComAutoCriticalSection> lock(critical_section_);
    active_ui_manager_map_[context] = ui_manager;
    return true;
  }

  UIManagerInterface* DisassociateUIManager(ContextInterface* context) {
    if (!context) return NULL;
    CComCritSecLock<CComAutoCriticalSection> lock(critical_section_);
    ContextUIManagerMap::iterator iter =
        active_ui_manager_map_.find(context);
    if (iter == active_ui_manager_map_.end()) return NULL;
    UIManagerInterface* ui_manager = iter->second;
    active_ui_manager_map_.erase(iter);
    return ui_manager;
  }

  ContextType* GetFromWindow(HWND hwnd) {
    assert(hwnd);
    // There is a bug in the type definition of GetWindowLongPtr, which cause
    // the warning 4312. So we disable this warning just for the following
    // line. More information at
    // http://kernelmustard.com/2006/09/16/when-you-cant-follow-the-api/
    // ImmGetContext can't be used here, because when the focus changed to an
    // empty context, ImmGetContext() still get the old context handler, not the
    // expected NULL value.
#pragma warning(suppress:4312)
    HIMC himc = reinterpret_cast<HIMC>(GetWindowLongPtr(hwnd, IMMGWLP_IMC));
    return Get(himc);
  }

  bool Destroy(HIMC himc) {
    assert(himc);
    CComCritSecLock<CComAutoCriticalSection> lock(critical_section_);
    ContextMap::iterator iter = context_map_.find(himc);
    if (iter == context_map_.end()) return false;

    delete iter->second;
    context_map_.erase(iter);
    return true;
  }

  void DestroyAll() {
    DVLOG(1) << __FUNCTION__;
    CComCritSecLock<CComAutoCriticalSection> lock(critical_section_);
    for (ContextMap::const_iterator iter = context_map_.begin();
         iter != context_map_.end();
         iter++) {
      delete iter->second;
    }
    context_map_.clear();
  }

  static ContextManagerT& Instance() {
    static ContextManagerT manager;
    return manager;
  }

 private:
  typedef map<HIMC, ContextType*> ContextMap;
  typedef map<ContextInterface*, UIManagerInterface*> ContextUIManagerMap;

  ContextManagerT() {}

  ContextMap context_map_;
  CComAutoCriticalSection critical_section_;
  // Bug fix for 4505900(IME v2.4 crash in explorer.exe process under WindowsXP)
  // There will be mutiple UIWindow objects and UIManager objects in
  // explorer.exe process at the same time. The mapping from context to
  // active ui_manager should be saved. So that, while a context is switched
  // out, the according ui_manager can be deactivated.
  ContextUIManagerMap active_ui_manager_map_;
  DISALLOW_EVIL_CONSTRUCTORS(ContextManagerT);
};

typedef ContextManagerT<Context> ContextManager;
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_CONTEXT_MANAGER_H__
