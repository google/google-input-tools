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

#include <atlstr.h>
#include <shlwapi.h>
#include "common/exception.h"

#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/registry.h"
#include "third_party/breakpad/src/client/windows/handler/exception_handler.h"

namespace ime_goopy {
static const WCHAR kLastReport[] = L"LastReport";
static const HINSTANCE kShellExecuteSucceeded =
  reinterpret_cast<HINSTANCE>(32);

// Callback from Breakpad when a crash occurs.
static bool MinidumpCallback(const wchar_t *minidump_path,
                             const wchar_t *minidump_id,
                             void *context,
                             EXCEPTION_POINTERS *exinfo,
                             MDRawAssertionInfo *assertion,
                             bool succeeded) {
  wstring reporter = AppUtils::GetBinaryFilePath(kReporterFilename);
  if (reporter.empty()) return false;
  // When user upgrades Goopy to a newer version while the IME is in use, some
  // files will not be updated until next reboot. In this case, the crashed
  // binary will send its own version number, instead of the upgraded product
  // version.
  CString command;
  command.Format(L"--file_version=%s %s",
                 RC_VERSION_STRING,
                 minidump_id);

  if (ShellExecute(
      NULL, L"open", reporter.c_str(),
      command, NULL, SW_SHOW) < kShellExecuteSucceeded) {
    return false;
  }
  return true;
}

// get the handle to the module containing the given executing address
static HMODULE GetModuleHandleFromAddress(void * address) {
  // address may be NULL
  MEMORY_BASIC_INFORMATION mbi;
  SIZE_T result = VirtualQuery(address, &mbi, sizeof(mbi));
  if (!result) return NULL;

  return static_cast<HMODULE>(mbi.AllocationBase);
}

// get the handle to the currently executing module
static HMODULE GetCurrentModuleHandle() {
  return GetModuleHandleFromAddress(GetCurrentModuleHandle);
}

// Check to see if an address is in the current module.
inline bool IsAddressInCurrentModule(void * address) {
  // address may be NULL
  return GetCurrentModuleHandle() == GetModuleHandleFromAddress(address);
}

bool IsCurrentModuleInStack(PCONTEXT context) {
  STACKFRAME64 stack;
  ZeroMemory(&stack, sizeof(stack));
#if defined(_M_IX86)
  stack.AddrPC.Offset = context->Eip;
  stack.AddrPC.Mode = AddrModeFlat;
  stack.AddrStack.Offset = context->Esp;
  stack.AddrStack.Mode = AddrModeFlat;
  stack.AddrFrame.Offset = context->Ebp;
  stack.AddrFrame.Mode = AddrModeFlat;
#elif defined(_M_X64)
  stack.AddrPC.Offset = context->Rip;
  stack.AddrPC.Mode = AddrModeFlat;
  stack.AddrStack.Offset = context->Rsp;
  stack.AddrStack.Mode = AddrModeFlat;
  stack.AddrFrame.Offset = context->Rbp;
  stack.AddrFrame.Mode = AddrModeFlat;
#else
#error "unkown platform"
#endif

  while (StackWalk64(IMAGE_FILE_MACHINE_I386,
                     GetCurrentProcess(),
                     GetCurrentThread(),
                     &stack,
                     context,
                     0,
                     SymFunctionTableAccess64,
                     SymGetModuleBase64,
                     0)) {
    if (IsAddressInCurrentModule(
            reinterpret_cast<void*>(stack.AddrPC.Offset))) return true;
  }
  return false;
}

static bool FilterHandler(void *context, EXCEPTION_POINTERS *exinfo,
                          MDRawAssertionInfo *assertion) {
  // Make sure we at most report crash information once per day.
  SYSTEMTIME systime;
  GetSystemTime(&systime);
  DWORD current_date =
      systime.wYear * 10000 + systime.wMonth * 100 + systime.wDay;
  DWORD last_date = 0;
  scoped_ptr<RegistryKey> registry(AppUtils::OpenUserRegistry());
  if (registry.get()) {
    if (registry->QueryDWORDValue(kLastReport, last_date) == ERROR_SUCCESS) {
      if (last_date == current_date) return false;
    }
  }

  // Make sure it's our module which cause the crash.
  if (IsAddressInCurrentModule(exinfo->ExceptionRecord->ExceptionAddress)) {
    return true;
  }

  if (IsCurrentModuleInStack(exinfo->ContextRecord)) return true;

  return false;
}

bool ExceptionHandler::Init() {
  wstring dir = AppUtils::GetDumpPath();
  if (dir.empty()) return false;

  if (handler_ == NULL)
    handler_ = new google_breakpad::ExceptionHandler(
        dir.c_str(), FilterHandler, MinidumpCallback,
        NULL, google_breakpad::ExceptionHandler::HANDLER_ALL);

  return true;
}

void ExceptionHandler::Release() {
  if (handler_) delete handler_;
  handler_ = NULL;
}

google_breakpad::ExceptionHandler* ExceptionHandler::handler_ = NULL;
}  // namespace ime_goopy
