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

#include "components/common/file_utils.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/scoped_ptr.h"
#include "common/app_const.h"
#include "common/app_utils_posix.h"
#include "common/string_utils.h"

namespace ime_goopy {
namespace {
const size_t kMaxFileSize = 20 * 1024 * 1024;
// DEFINE_SCOPED_HANDLE(ScopedFileHandle, FILE*, fclose);
}  // namespace

std::string FileUtils::GetSystemDataPath() {
  return "";
}

std::string FileUtils::GetSystemPluginPath() {
  return "";
}

std::string FileUtils::GetUserDataPath() {
  return "";
}

std::string FileUtils::GetDataPathForComponent(const std::string& component) {
  return "";
}

std::string FileUtils::GetUserDataPathForComponent(
    const std::string& component) {
  return "";
}

std::string FileUtils::GetDataPathForComponentAndLanguage(
    const std::string& component,
    const std::string& language) {
  return "";
}

std::string FileUtils::GetDataPathForLanguage(const std::string& language) {
  return "";
}

bool FileUtils::ReadFileContent(const std::string& path, std::string* content) {
  DCHECK(content);
  if (content == NULL)
    return false;
  content->clear();
  FILE * fp = fopen(path.c_str(), "rb");
  if (!fp)
    return false;
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  DCHECK_LT(size, kMaxFileSize);
  if (size >= kMaxFileSize) {
    DLOG(ERROR) << L"File is too big (>" << kMaxFileSize << ") : " << path;
    return false;
  }
  fseek(fp, 0, SEEK_SET);
  scoped_array<char> buffer(new char[size]);
  int size_loaded = fread(buffer.get(), 1, size, fp);
  DCHECK_EQ(size, size_loaded);
  if (size != size_loaded) {
    DLOG(ERROR) << size_loaded << L" of " << size << L" Bytes Loaded from file"
                << path;
    return false;
  }
  content->append(buffer.get(), size_loaded);
  return true;
}

bool FileUtils::CreateDirectoryRecuisively(const std::string& dir) {
  int ret = mkdir(dir.c_str(), 0755);
  return ret == 0 || ret == -1;
}
}  // namespace ime_goopy
