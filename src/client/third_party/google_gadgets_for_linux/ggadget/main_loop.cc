
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

#include <cstdio>

#include "build_config.h"
#include "common.h"
#include "main_loop_interface.h"

#if defined(OS_WIN)
#include "win32/thread_local_singleton_holder.h"
#endif // OS_WIN

namespace ggadget {

#if !defined(OS_WIN)
static MainLoopInterface *ggl_global_main_loop_ = NULL;
#endif // OS_WIN

bool SetGlobalMainLoop(MainLoopInterface *main_loop) {
#if defined(OS_WIN)
  MainLoopInterface *old_main_loop =
      win32::ThreadLocalSingletonHolder<MainLoopInterface>::GetValue();
  if (old_main_loop && main_loop)
    return false;
  return win32::ThreadLocalSingletonHolder<MainLoopInterface>::SetValue(
      main_loop);
#else // OS_WIN
  ASSERT(!ggl_global_main_loop_ && main_loop);
  if (!ggl_global_main_loop_ && main_loop) {
    ggl_global_main_loop_ = main_loop;
    return true;
  }
  return false;
#endif // OS_WIN
}

MainLoopInterface *GetGlobalMainLoop() {
#if defined(OS_WIN)
  return win32::ThreadLocalSingletonHolder<MainLoopInterface>::GetValue();
#else // OS_WIN
  return ggl_global_main_loop_;
#endif // OS_WIN
}

} // namespace ggadget
