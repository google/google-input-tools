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

#include <shlwapi.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "appsensorapi/appsensor_helper.h"
#include "appsensorapi/common.h"
#include "appsensorapi/handlers/cs16_handler.h"
#include "base/logging.h"
#include "common/framework_interface.h"
#include "common/shellutils.h"
#include "imm/common.h"
#include "imm/context.h"
#include "imm/context_manager.h"
#include "imm/ui_window.h"
#include "imm/candidate_info.h"
#include "imm/composition_string.h"
#include "imm/debug.h"
#include "imm/immdev.h"
#include "imm/input_context.h"

using ime_goopy::InputMethod;
using ime_goopy::imm::Context;
using ime_goopy::imm::ContextManager;
using ime_goopy::UIManagerInterface;
using ime_goopy::imm::UIWindow;
using ime_goopy::imm::Debug;
using ime_goopy::imm::MessageQueue;

// A guard variable, it should be set to true when the dll is unloading.
bool _goopy_exiting = false;
bool _disable_ime = false;

static bool ShouldDisableIMEForApp() {
  if (_disable_ime)
    return true;
  // Disable IME for specific applications like cs1.6, because application
  // supports english/native input only.
  if (ime_goopy::AppSensorHelper::Instance()->Init()) {
    ime_goopy::AppSensorHelper::Instance()->RegisterHandler(
        new ime_goopy::handlers::CS16Handler);
    bool should_disable_ime_for_app = false;
    ime_goopy::AppSensorHelper::Instance()->HandleCommand(
        ime_goopy::CMD_SHOULD_DISABLE_IME,
        &should_disable_ime_for_app);
    if (should_disable_ime_for_app)
      return true;
  }
  return false;
}

// Handles initialization of the IME.
// ImeInquire is called to initialize the IME for every thread of the
// application process.
BOOL WINAPI ImeInquire(LPIMEINFO ime_info, LPTSTR ui_class, DWORD flags) {
  DVLOG(2) << __FUNCTION__
           << L" flag: "
           << Debug::IME_SYSINFO_String(flags);

  // Fail immediately in safe mode.
  // 0: Normal boot
  // 1: Fail-safe boot
  // 2: Fail-safe with network boot
  if (::GetSystemMetrics(SM_CLEANBOOT) > 0)
    return FALSE;

  ime_info->dwPrivateDataSize = 0;
  ime_info->fdwProperty = InputMethod::kImmProperty;
  if (!ime_goopy::ShellUtils::CheckWindowsVista()) {
    // Clear IME_PROP_AT_CARET and set IME_PROP_SPECIAL_UI in windows system.
    // See http://b.corp.google.com/issue?id=7208182
    // Excel seems cannot deal with inline composition correctly in XP,
    // Unfortunalty the OS will cache the ime_info, so that we can not set this
    // flag specially for excel, so we choose to disable inline composition on
    // XP, we will draw fake inline composition in this case.
    ime_info->fdwProperty = ime_info->fdwProperty & ~IME_PROP_AT_CARET;
    ime_info->fdwProperty = ime_info->fdwProperty | IME_PROP_SPECIAL_UI;
  }
  // Conversion mode capability.
  ime_info->fdwConversionCaps = InputMethod::kConversionModeMask;

  // Sentence mode capability.
  ime_info->fdwSentenceCaps = InputMethod::kSentenceModeMask;

  // User interface capability.
  ime_info->fdwUICaps = UI_CAP_SOFTKBD;

  // Composition string capability.
  ime_info->fdwSCSCaps = 0;

  // Capability to inherit input contexts.
  ime_info->fdwSelectCaps = SELECT_CAP_CONVERSION;

  // Registered classname.
  StrCpyN(ui_class,
          InputMethod::kUIClassName,
          arraysize(InputMethod::kUIClassName));

  if (!UIWindow::RegisterClass(InputMethod::kUIClassName))
    return FALSE;

  // Don't active our ime in winlogon process. We still need to set ime_info
  // even if we don't want to activate our ime because in windows xp, if
  // winlogon calls ImeInquire and got an invalid ime_info when the system is
  // starting up, the system will take our ime as invalid and never load our ime
  // again. And simply return FALSE in ImeInquire will not prevent the
  // application from calling ImeSelect, so we need to set _disable_ime and
  // check it in ImeSelect.
  if ((flags & IME_SYSINFO_WINLOGON) != 0) {
    _disable_ime = true;
    return FALSE;
  }

  return TRUE;
}

// Provides a dialog box to request optional information for an IME.
BOOL WINAPI ImeConfigure(HKL hkl, HWND parent, DWORD mode, LPVOID data) {
  DVLOG(2) << __FUNCTION__
           << L" hkl: " << hkl
           << L" parent: " << parent
           << L" mode: " << Debug::IME_CONFIG_String(mode);
  if (!InputMethod::ShowConfigureWindow(parent)) return FALSE;
  return TRUE;
}

// Obtains the list of FE characters or strings from one character or string.
// The return value is the number of bytes in the result string list.
DWORD WINAPI ImeConversionList(HIMC himc,
                               LPCTSTR source,
                               LPCANDIDATELIST dest,
                               DWORD length,
                               UINT flag) {
  DVLOG(2) << __FUNCTION__
           << L" himc: " << himc
           << L" source: " << source
           << L" flag: " << flag;
  // We don't support this function at present.
  return 0;
}

// Terminates the IME.
BOOL WINAPI ImeDestroy(UINT reserved) {
  DVLOG(1) << __FUNCTION__;
  // According to DDK doc, we should return FALSE if reserved is not 0.
  if (reserved) return FALSE;
  return TRUE;
}

// Other capabilities that are not directly available though other IMM
// functions.
LRESULT WINAPI ImeEscape(HIMC himc, UINT escape, LPVOID data) {
  DVLOG(2) << __FUNCTION__
           << L" himc: " << himc
           << L" escape: " << Debug::IME_ESC_String(escape);
  if (escape == IME_ESC_QUERY_SUPPORT) {
    if (!data) return 0;
    UINT query_escape = *reinterpret_cast<LPUINT>(data);
    switch (query_escape) {
      case IME_ESC_IME_NAME:
        return TRUE;
        break;
      case IME_ESC_IMPORT_DICTIONARY:
        return TRUE;
        break;
      default:
        return FALSE;
        break;
    }
  }

  switch (escape) {
    case IME_ESC_IME_NAME:
      if (!data) return 0;
      StringCchCopy(reinterpret_cast<wchar_t*>(data),
                    InputMethod::kMaxDisplayNameLength,
                    InputMethod::kDisplayName);
      return TRUE;
      break;
    case IME_ESC_IMPORT_DICTIONARY:
      if (!data) return 0;
      MessageQueue* message_queue = new MessageQueue(himc);
      Context* context = new Context(himc, message_queue);
      ime_goopy::EngineInterface* engine = InputMethod::CreateEngine(context);
      if (!engine) {
        delete context;
        return FALSE;
      }

      BOOL success = engine->ImportDictionary(reinterpret_cast<wchar_t*>(data));
      delete engine;
      delete context;
      return success;
      break;
  }
  return 0;
}

// Preprocesses keystrokes given through IMM.
// The return value is TRUE if a key is necessary for the IME with the given
// input context. Otherwise, it is FALSE.
BOOL WINAPI ImeProcessKey(HIMC himc,
                          UINT virtual_key,
                          LPARAM lparam,
                          CONST LPBYTE key_state) {
  DVLOG(2) << __FUNCTION__
           << L" himc: " << himc
           << L" virtual_key: " << virtual_key
           << L" lparam: " << lparam;
  Context* context = ContextManager::Instance().Get(himc);
  if (!context) return FALSE;
  // A short path for win key. Seem that if we took too long to process win key,
  // windows will discard it.
  if (virtual_key == VK_LWIN || virtual_key == VK_RWIN)
    return false;
  return context->OnProcessKey(virtual_key, lparam, key_state);
}

// Initializes and uninitializes the IME's private context.
BOOL WINAPI ImeSelect(HIMC himc, BOOL select) {
  DVLOG(1) << __FUNCTION__
           << L" himc: " << himc
           << L" select: " << select;
  if (ShouldDisableIMEForApp())
    return FALSE;

  if (select) {
    // TODO(ybo): Now the private context is created when the system call
    // ImeSelect. Actually, the system may call ImeSelect on many HIMC and
    // never use them. It's a waste of resource if we create private context
    // here. We may move the creation to WM_IME_SELECT in UI window.

    // message_queue and engine will be deleted in class Context.
    MessageQueue* message_queue = new MessageQueue(himc);
    Context* context = new Context(himc, message_queue);
    ime_goopy::EngineInterface* engine = InputMethod::CreateEngine(context);
    if (!engine) {
      delete context;
      return FALSE;
    }
    bool added = ContextManager::Instance().Add(himc, context);
    if (!added) {
      delete context;
      delete engine;
      return FALSE;
    }

    return context->Initialize(engine);
  } else {
    Context* context = ContextManager::Instance().Get(himc);
    InputMethod::DestroyEngineOfContext(context);
    // TODO(ybo): If goopy is active when the application is existing, the
    // system won't call ImeSelect to select goopy out. We should have a
    // object pool to make sure all context is freed at the end.
    ContextManager::Instance().Destroy(himc);
  }
  return TRUE;
}

// Activates or deactivates an input context and notifies the IME of the newly
// active input context. The IME can use the notification for initialization.
// The IME is informed by this function about a newly selected Input Context.
// The IME can carry out initialization, but it is not required.
BOOL WINAPI ImeSetActiveContext(HIMC himc, BOOL flag) {
  DVLOG(2) << __FUNCTION__
           << L" himc: " << himc
           << L" flag: " << flag;
  Context* context = ContextManager::Instance().Get(himc);
  if (!flag) {
    UIManagerInterface* ui_manager =
        ContextManager::Instance().DisassociateUIManager(context);
    if (ui_manager) {
      ui_manager->SetContext(NULL);
    }
  } else if (context) {
    context->SetOpenStatus(TRUE);
  }
  return TRUE;
}

// Translates messages using the IME conversion engine associated with the
// given input context.
// The return value is the number of messages.
UINT WINAPI ImeToAsciiEx(UINT virtual_key,
                         UINT scan_code,
                         CONST LPBYTE key_state,
                         LPTRANSMSGLIST trans_list,
                         UINT state,
                         HIMC himc) {
  DVLOG(2) << __FUNCTION__
           << L" virtual_key: " << virtual_key
           << L" scan_code: " << scan_code
           << L" state: " << state
           << L" himc: " << himc;
  Context* context = ContextManager::Instance().Get(himc);
  if (!context) return FALSE;

  return context->OnToAsciiEx(
    virtual_key, scan_code, key_state, trans_list, state);
}

// Changes the status of the IME according to the given parameters.
BOOL WINAPI NotifyIME(HIMC himc, DWORD action, DWORD index, DWORD value) {
#ifdef _DEBUG
  switch (action) {
    case NI_CONTEXTUPDATED:
      DVLOG(1) << __FUNCTION__
               << L" himc: " << himc
               << L" action: " << Debug::NI_String(action)
               << L" index: " << index
               << L" value: " << Debug::IMC_String(value);
      break;
    case NI_COMPOSITIONSTR:
      DVLOG(1) << __FUNCTION__
               << L" himc: " << himc
               << L" action: " << Debug::NI_String(action)
               << L" index: " << Debug::CPS_String(index)
               << L" value: " << value;
      break;
    default:
      DVLOG(1) << __FUNCTION__
               << L" himc: " << himc
               << L" action: " << Debug::NI_String(action)
               << L" index: " << index
               << L" value: " << value;
      break;
  }
#endif  // _DEBUG
  Context* context = ContextManager::Instance().Get(himc);
  if (!context) return FALSE;
  return context->OnNotifyIME(action, index, value);
}

// Adds a new string to the dictionary of this IME.
BOOL WINAPI ImeRegisterWord(LPCTSTR reading, DWORD style, LPCTSTR value) {
  DVLOG(2) << __FUNCTION__
           << L" reading: " << reading
           << L" value: " << value;
  // Not implemented in current version.
  return FALSE;
}

// Removes a string from the dictionary of this IME.
BOOL WINAPI ImeUnregisterWord(LPCTSTR reading, DWORD style, LPCTSTR value) {
  DVLOG(2) << __FUNCTION__
           << L" reading: " << reading
           << L" value: " << value;
  // Not implemented in current version.
  return FALSE;
}

// Retrieves the available styles in this IME.
UINT WINAPI ImeGetRegisterWordStyle(UINT item, LPSTYLEBUF style_buffer) {
  DVLOG(2) << __FUNCTION__
           << L" item: " << item;
  // Not implemented in current version.
  return 0;
}

// Enumerates all strings that match the given reading string, style, or
// registered string.
UINT WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC enum_proc,
                                LPCTSTR reading,
                                DWORD style,
                                LPCTSTR value,
                                LPVOID data) {
  DVLOG(2) << __FUNCTION__;
  // Not implemented in current version.
  return 0;
}

// Causes the IME to arrange the composition string structure with the given
// data.
BOOL WINAPI ImeSetCompositionString(HIMC himc,
                                    DWORD index,
                                    LPVOID composition,
                                    DWORD composition_length,
                                    LPVOID reading,
                                    DWORD reading_length) {
  DVLOG(2) << __FUNCTION__;
  // Not implemented in current version.
  return FALSE;
}
