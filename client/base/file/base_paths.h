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

#ifndef GOOPY_BASE_FILE_BASE_PATHS_H_
#define GOOPY_BASE_FILE_BASE_PATHS_H_
#pragma once

// This file declares path keys for the base module.  These can be used with
// the PathService to access various special directories and files.

#if defined(OS_WIN)
#include "base/file/base_paths_win.h"
#elif defined(OS_MACOSX)
#include "base/file/base_paths_mac.h"
#endif

namespace base {

enum BasePathKey {
  PATH_START = 0,

  DIR_CURRENT,  // current directory
  DIR_EXE,      // directory containing FILE_EXE
  DIR_MODULE,   // directory containing FILE_MODULE
  DIR_TEMP,     // temporary directory
  FILE_EXE,     // Path and filename of the current executable.
  FILE_MODULE,  // Path and filename of the module containing the code for the
                // PathService (which could differ from FILE_EXE if the
                // PathService were compiled into a shared object, for example).
  DIR_SOURCE_ROOT,  // Returns the root of the source tree.  This key is useful
                    // for tests that need to locate various resources.  It
                    // should not be used outside of test code.
#if defined(OS_POSIX)
  DIR_CACHE,    // Directory where to put cache data.  Note this is
                // *not* where the browser cache lives, but the
                // browser cache can be a subdirectory.
                // This is $XDG_CACHE_HOME on Linux and
                // ~/Library/Caches on Mac.
#endif
#if defined(OS_ANDROID)
  DIR_ANDROID_APP_DATA,  // Directory where to put android app's data.
#endif

  PATH_END
};

}  // namespace base

#endif  // GOOPY_BASE_FILE_BASE_PATHS_H_
