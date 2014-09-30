/*
  Copyright 2008 Google Inc.

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

#include "runtime.h"
#include <ggadget/sysdeps.h>
#include <ggadget/logger.h>
#include <sys/utsname.h>

namespace ggadget {
namespace framework {
namespace linux_system {

Runtime::Runtime() {
  struct utsname uts;
  if (uname(&uts)) {
    DLOG("Failed to get the system information.");
    os_name_ = GGL_PLATFORM;
  } else {
    os_name_ = uts.sysname;
    os_version_ = uts.release;
  }
}

Runtime::~Runtime() {
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
