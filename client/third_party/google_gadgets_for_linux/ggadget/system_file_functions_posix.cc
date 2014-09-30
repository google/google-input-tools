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

namespace ggadget {

int access(const char *pathname, int mode) {
  return ::access(pathname, mode);
}

FILE *fopen(const char *path, const char *mode) {
  return ::fopen(path, mode);
}

int mkdir(const char *pathname, mode_t mode) {
  return ::mkdir(pathname, mode);
}

int stat(const char *path, StatStruct *buf) {
  return ::stat(path, buf);
}

mode_t umask(mode_t mask) {
  return ::umask(mask);
}

int unlink(const char *pathname) {
  return ::unlink(pathname);
}

}  // namespace ggadget
