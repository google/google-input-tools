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

#include "base/file/base_paths_win.h"

#include <windows.h>
#include <shlobj.h>
#include <string>

#include "base/file/base_paths.h"
#include "base/file/file_path.h"
#include "base/file/path_service.h"
#include "base/win/shellutils.h"
#include "base/win/windows_version.h"

// http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace base {

bool PathProviderWin(int key, FilePath* result) {
  // We need to go compute the value. It would be nice to support paths with
  // names longer than MAX_PATH, but the system functions don't seem to be
  // designed for it either, with the exception of GetTempPath (but other
  // things will surely break if the temp path is too long, so we don't bother
  // handling it.
  wchar_t system_buffer[MAX_PATH] = {0};
  system_buffer[0] = 0;

  FilePath cur;
  switch (key) {
    case base::FILE_EXE:
      if (!GetModuleFileName(NULL, system_buffer, MAX_PATH))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::FILE_MODULE: {
      // the resource containing module is assumed to be the one that
      // this code lives in, whether that's a dll or exe
      HMODULE this_module = reinterpret_cast<HMODULE>(&__ImageBase);
      if (!GetModuleFileName(this_module, system_buffer, MAX_PATH))
        return false;
      cur = FilePath(system_buffer);
      break;
    }
    case base::DIR_WINDOWS:
      if (!GetWindowsDirectory(system_buffer, MAX_PATH))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SYSTEM:
      if (!GetSystemDirectory(system_buffer, MAX_PATH))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SYSTEMX86:
      if (base::ShellUtils::Is64BitOS() &&
          !base::ShellUtils::CheckWindowsVista()) {
        // Fix bug 5811480 to overcome a bug in 64bit XP.
        if (!GetSystemWow64Directory(system_buffer, MAX_PATH))
          return false;
      } else {
        if (FAILED(SHGetFolderPath(NULL, CSIDL_SYSTEMX86, NULL,
                                   SHGFP_TYPE_CURRENT, system_buffer)))
          return false;
      }
      cur = FilePath(system_buffer);
      break;
    case base::DIR_PROGRAM_FILESX86:
      if (base::ShellUtils::Is64BitOS()) {
        if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL,
                                   SHGFP_TYPE_CURRENT, system_buffer)))
          return false;
        cur = FilePath(system_buffer);
        break;
      }
      // Fall through to base::DIR_PROGRAM_FILES if we're on an X86 machine.
    case base::DIR_PROGRAM_FILES:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_IE_INTERNET_CACHE:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_COMMON_START_MENU:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_START_MENU:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_PROFILE:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_LOCAL_APP_DATA_LOW:
      if (::base::win::GetVersion() < ::base::win::VERSION_VISTA) {
        return false;
      }
      // TODO: We should use SHGetKnownFolderPath instead.
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer).DirName()
                                   .Append(FILE_PATH_LITERAL("LocalLow"));
      break;
    case base::DIR_LOCAL_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SOURCE_ROOT: {
      FilePath executableDir;
      // On Windows, unit tests execute two levels deep from the source root.
      // For example:  chrome/{Debug|Release}/ui_tests.exe
      PathService::Get(base::DIR_EXE, &executableDir);
      cur = executableDir.DirName().DirName();
      break;
    }
    case base::DIR_TEMP:
      if (!GetTempPath(MAX_PATH, system_buffer))
        return false;
      cur = FilePath(system_buffer);
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  // namespace base
