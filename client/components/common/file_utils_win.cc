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

#include <shlobj.h>
#include <shlwapi.h>

#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/scoped_ptr.h"
#include "base/string_utils_win.h"
#include "common/app_const.h"
#include "common/app_utils.h"

namespace ime_goopy {
namespace {
const size_t kMaxFileSize = 20 * 1024 * 1024;
DEFINE_SCOPED_HANDLE(ScopedFileHandle, FILE*, fclose);
}  // namespace

// TODO(synch): the path definition in this file is currently temporary.
//   We need to remove the dependency of AppUtils when the directory structure
//   of new goopy is defined and rewrite this file.
std::string FileUtils::GetSystemDataPath() {
  return WideToUtf8(AppUtils::GetSystemDataFilePath(L""));
}

std::string FileUtils::GetSystemPluginPath() {
  return GetSystemDataPath() + "/" + kPluginsSubFolder;
}

std::string FileUtils::GetUserDataPath() {
  return WideToUtf8(AppUtils::GetUserDataFilePath(L""));
}

std::string FileUtils::GetDataPathForComponent(const std::string& component) {
  return GetSystemDataPath() + "/" + component;
}

std::string FileUtils::GetUserDataPathForComponent(
    const std::string& component) {
  return GetUserDataPath() + "/" + component;
}

std::string FileUtils::GetDataPathForComponentAndLanguage(
    const std::string& component,
    const std::string& language) {
  return GetSystemDataPath() + "/" + component + "/" + language;
}

std::string FileUtils::GetDataPathForLanguage(const std::string& language) {
  return GetSystemDataPath() + "/" + language;
}

bool FileUtils::ReadFileContent(const std::string& path, std::string* content) {
  DCHECK(content);
  if (content == NULL)
    return false;
  content->clear();
  ScopedFileHandle fp(_wfopen(Utf8ToWide(path).c_str(), L"rb"));
  if (!fp.Get())
    return false;
  fseek(fp.Get(), 0, SEEK_END);
  int size = ftell(fp.Get());
  DCHECK_LT(size, kMaxFileSize);
  if (size >= kMaxFileSize) {
    DLOG(ERROR) << L"File is too big (>" << kMaxFileSize << ") : " << path;
    return false;
  }
  fseek(fp.Get(), 0, SEEK_SET);
  scoped_array<char> buffer(new char[size]);
  int size_loaded = fread(buffer.get(), 1, size, fp.Get());
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
  int ret = SHCreateDirectory(NULL, Utf8ToWide(dir).c_str());
  return ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS;
}
}  // namespace ime_goopy
