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

#include "system_file_functions.h"

#include <string.h>
#include "common.h"
#include "unicode_utils.h"

namespace ggadget {

int access(const char *pathname, int mode) {
  UTF16String str;
  ConvertStringUTF8ToUTF16(pathname, strlen(pathname), &str);
  return ::_waccess(str.c_str(), mode);
}

FILE *fopen(const char *path, const char *mode) {
  UTF16String wpath;
  UTF16String wmode;
  ConvertStringUTF8ToUTF16(path, strlen(path), &wpath);
  ConvertStringUTF8ToUTF16(mode, strlen(mode), &wmode);
  return ::_wfopen(wpath.c_str(), wmode.c_str());
}

int mkdir(const char *pathname, mode_t mode) {
  GGL_UNUSED(mode);
  UTF16String str;
  ConvertStringUTF8ToUTF16(pathname, strlen(pathname), &str);
  return ::_wmkdir(str.c_str());
}

int stat(const char *path, StatStruct *buf) {
  UTF16String str;
  ConvertStringUTF8ToUTF16(path, strlen(path), &str);
  return ::_wstat(str.c_str(), buf);
}

mode_t umask(mode_t mask) {
  return ::_umask(mask);
}

int unlink(const char *pathname) {
  UTF16String str;
  ConvertStringUTF8ToUTF16(pathname, strlen(pathname), &str);
  return ::_wunlink(str.c_str());
}

}  // namespace ggadget
