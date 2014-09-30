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

#include "utilities.h"
#include <string>
#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {
namespace win32 {

namespace {

// The win32 API ShellExecute returns a HINSTANCE greater than 32 if successful.
static const HINSTANCE kValidInstance = reinterpret_cast<HINSTANCE>(32);

}  // namespace

bool OpenURL(const GadgetInterface* gadget, const char* url) {
  ASSERT(url && *url);
  if (!gadget)
    return false;
  const Permissions* permissions = gadget->GetPermissions();
  std::string path;

  // If url does not points to a web url, ask for ALL_ACCESS.
  if (!IsValidWebURL(url)) {
    if (!permissions->IsRequiredAndGranted(Permissions::ALL_ACCESS)) {
      LOG("No permission to open a local file: %s", url);
      return false;
    }
  } else {  // If url points to an web url, ask for NETWORK permission.
    if (!permissions->IsRequiredAndGranted(Permissions::NETWORK)) {
      LOG("No permission to open URL\n");
      return false;
    }
  }

  if (ggadget::IsValidFileURL(url))
    path = ggadget::DecodeURL(url + arraysize(kFileUrlPrefix) - 1);
  else
    path = url;

  UTF16String url_utf16;
  ConvertStringUTF8ToUTF16(path, &url_utf16);
  HINSTANCE instance = ::ShellExecute(
      NULL, L"open", reinterpret_cast<const wchar_t*>(url_utf16.c_str()), NULL,
      NULL, SW_SHOWNORMAL);
  return instance > kValidInstance;
}

}  // namespace win32
}  // namespace ggadget
