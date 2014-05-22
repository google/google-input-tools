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

#ifndef GOOPY_BASE_FILE_BASE_PATHS_WIN_H__
#define GOOPY_BASE_FILE_BASE_PATHS_WIN_H__
#pragma once

// This file declares windows-specific path keys for the base module.
// These can be used with the PathService to access various special
// directories and files.

namespace base {

enum {
  PATH_WIN_START = 100,

  DIR_WINDOWS,  // Windows directory, usually "c:\windows"
  DIR_SYSTEM,   // Usually c:\windows\system32"
  DIR_SYSTEMX86,          // Usually c:\windows\syswow64
  DIR_PROGRAM_FILES,      // Usually c:\program files
  DIR_PROGRAM_FILESX86,   // Usually c:\program files or c:\program files (x86)

  DIR_IE_INTERNET_CACHE,  // Temporary Internet Files directory.
  DIR_COMMON_START_MENU,  // Usually "C:\Documents and Settings\All Users\
                          // Start Menu\Programs"
  DIR_START_MENU,         // Usually "C:\Documents and Settings\<user>\
                          // Start Menu\Programs"
  DIR_APP_DATA,           // Application Data directory under the user profile.
  DIR_PROFILE,            // Usually "C:\Documents and settings\<user>.
  DIR_LOCAL_APP_DATA_LOW,  // Local AppData directory for low integrity level.
  DIR_LOCAL_APP_DATA,  // "Local Settings\Application Data" directory under the
                       // user profile.
  PATH_WIN_END
};

}  // namespace base

#endif  // GOOPY_BASE_FILE_BASE_PATHS_WIN_H__
