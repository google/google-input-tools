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
#ifndef GOOPY_BASE_FILE_FILE_UTIL_H_
#define GOOPY_BASE_FILE_FILE_UTIL_H_

class FilePath;
namespace file_util {
bool GetCurrentDirectory(FilePath* dir);
bool DirectoryExists(const FilePath& path);
bool CreateDirectory(const FilePath& full_path);
bool PathExists(const FilePath& path);
bool AbsolutePath(FilePath* path);
}  // namespace file_util

#endif  // GOOPY_BASE_FILE_FILE_UTIL_H_
