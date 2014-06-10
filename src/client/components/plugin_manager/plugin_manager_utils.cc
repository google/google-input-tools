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

#include "components/plugin_manager/plugin_manager_utils.h"

#ifdef OS_WINDOWS
#include <shlwapi.h>
#include <string.h>
#else
#include "common/windows_types.h"
#endif
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/string_utils_win.h"
#include "common/app_utils.h"

namespace ime_goopy {
namespace components {
namespace {

static const wchar_t kPluginFileExtension[] = L".dll";
inline bool IsPluginFile(const std::wstring& name) {
#ifdef OS_WINDOWS
  wchar_t* extension = PathFindExtension(name.c_str());
  return extension != NULL && _wcsicmp(extension, kPluginFileExtension) == 0;
#else
  // We don't use DLL extension for other platforms.
  // Leave it here and return False for now
  return false;
#endif
}
}  // namespace

bool PluginManagerUtils::ListPluginFile(const std::string& path,
                                        std::vector<std::string>* files) {
  DCHECK(files);
  files->clear();
  std::vector<std::wstring> files_utf16;
  std::wstring path_utf16 = Utf8ToWide(path);
  if (!AppUtils::GetFileList(path_utf16.c_str(), &files_utf16, IsPluginFile))
    return false;
  for (int i = 0; i < files_utf16.size(); ++i)
    files->push_back(WideToUtf8(files_utf16[i]));
  return true;
}

}  // namespace components
}  // namespace ime_goopy
