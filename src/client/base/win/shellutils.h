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

#ifndef GOOPY_BASE_WIN_SHELLUTILS_H_
#define GOOPY_BASE_WIN_SHELLUTILS_H_

#ifdef OS_WINDOWS
#include <windows.h>
#include <AccCtrl.h>
#else
#include "base/windows_types.h"
#endif

namespace base {
class ShellUtils {
 public:
  enum OS {
    OS_NOTSUPPORTED = 0,
    OS_WINDOWSXPSP2 = 1,
    OS_WINDOWSVISTA = 2,
    OS_WINDOWS7 = 3,
    OS_WINDOWS8 = 4,
  };
#ifdef OS_WINDOWS
  // A multithread version of ShellExcute
  // This execute will not block system if is_block == FALSE
  // but you cannot know whether it runs successfully or not
  static BOOL Execute(const WCHAR* file, const WCHAR* cmdline,
                      DWORD show_cmd, BOOL is_block);
  // Launch a process with specified parameters, if wait is true, the caller
  // process will block until the child process exits.
  static void LaunchProcess(const WCHAR *path,
                            const WCHAR *params,
                            bool wait);
  // Deprecated.
#endif
  static BOOL CheckWindowsVista();
  static BOOL CheckWindowsXPOrLater();
  static BOOL CheckWindows8();
  // Returns the OS information. Caller should use this method instead of
  // CheckWindowXXX().
  static OS GetOS();
  static bool IsCurrentUserAdmin();
  static bool IsCurrentProcessAdmin();
  static bool IsSystemAccount();
  static bool Is64BitOS();
  static bool SetHandleLowIntegrity(HANDLE handle, SE_OBJECT_TYPE type);
  static int GetPageSize();
  static int NumCPUs();
  // A wrapper of system API Wow64DisableWow64FsRedirection.
  // The param |old_value| is used in RevertWow64FsRedirection function to
  // re-enable wow64 file system redirection, do not change the value.
  static bool DisableWow64FsRedirection(PVOID* old_value);
  // A wrapper of system API Wow64RevertWow64FsRedirection.
  // NOTICE: On 64bit window xp, loading dlls in system32 to 32bit process will
  // failed when WOW64 redirection is disabled because it will try to load the
  // 64bit version in system32 instead of being redirected to sysWOW64. So we
  // need to re-enable the redirection as soon as possible in case some api may
  // try to load some dll from system32 especially when 360 exists.
  static bool RevertWow64FsRedirection(PVOID old_value);
};
}  // namespace base
#endif  // GOOPY_BASE_WIN_SHELLUTILS_H_
