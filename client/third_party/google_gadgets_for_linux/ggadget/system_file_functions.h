/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_SYSTEM_FILE_FUNCTIONS_H__
#define GGADGET_SYSTEM_FILE_FUNCTIONS_H__

#include <ggadget/build_config.h>  // It defines OS_WIN

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(OS_WIN)
#include <io.h>
#include <direct.h>
#include <ggadget/win32/port.h>
#elif defined(OS_POSIX)
#include <unistd.h>
#endif

namespace ggadget {

#if defined(OS_WIN)
typedef struct _stat StatStruct;
#elif defined(OS_POSIX)
typedef struct stat StatStruct;
#endif

int access(const char *pathname, int mode);
FILE *fopen(const char *path, const char *mode);
int mkdir(const char *pathname, mode_t mode);
int stat(const char *path, StatStruct *buf);
mode_t umask(mode_t mask);
int unlink(const char *pathname);

}  // namespace

#endif  // GGADGET_SYSTEM_FILE_FUNCTIONS_H__
