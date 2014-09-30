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

#include "base/file/base_paths.h"

#include "base/file/file_path.h"
#include "base/file/path_service.h"

namespace base {

bool PathProvider(int key, FilePath* result) {
  // NOTE: DIR_CURRENT is a special cased in PathService::Get

  FilePath cur;
  switch (key) {
    case base::DIR_EXE:
      PathService::Get(base::FILE_EXE, &cur);
      cur = cur.DirName();
      break;
    case base::DIR_MODULE:
      PathService::Get(base::FILE_MODULE, &cur);
      cur = cur.DirName();
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  // namespace base
