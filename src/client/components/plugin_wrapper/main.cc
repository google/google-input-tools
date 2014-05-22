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

#include <atlbase.h>
#include <shlwapi.h>

#include "base/logging.h"
#include "common/exception.h"
#include "common/framework_interface.h"

class DllModule : public CAtlDllModuleT<DllModule> {
};

DllModule _AtlModule;

// DLL Entry Point
BOOL APIENTRY DllMain(HINSTANCE instance,
                      DWORD reason,
                      LPVOID reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
#ifdef DEBUG
      {
        // Log path is $TEMP\googleinputtools\module-pid.log
        char path[MAX_PATH] = {0};
        ::GetTempPathA(MAX_PATH, path);
        ::PathAppendA(path, "googleinputtools");
        ::CreateDirectoryA(path, NULL);
        char module[MAX_PATH] = {0};
        if (::GetModuleFileNameA(NULL, module, MAX_PATH) == 0) {
          module[0] = 0;
        }
        char* module_basename = ::PathFindFileNameA(module);
        CStringA logfile;
        logfile.Format("%s-%d.log", module_basename, GetCurrentProcessId());
        ::PathAppendA(path, logfile);
        logging::InitLogging(path,
                             logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                             logging::DONT_LOCK_LOG_FILE,
                             logging::DELETE_OLD_LOG_FILE);
        logging::SetLogItems(false, true, true, false);
        ime_shared::VLog::SetFromEnvironment(L"INPUT_TOOLS_VMODULE",
                                             L"INPUT_TOOLS_VLEVEL");
      }
#endif
      break;
    case DLL_PROCESS_DETACH:
      // Don't use google3 logging here, because some application may already
      // unintialize CRT before calling our DllMain, and logging functions
      // require CRT.
      ::OutputDebugString(L"DllMain DLL_PROCESS_DETACH\n");
      break;
  }
  return _AtlModule.DllMain(reason, reserved);
}
