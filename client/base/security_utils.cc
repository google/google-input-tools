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

#include "i18n/input/engine/lib/public/win/security_utils.h"

#include <Aclapi.h>
#include <Sddl.h>

#include "base/logging.h"

namespace i18n_input {
namespace engine {

// Util to set low integrity to an object handle.
//
// NOTE(ygwang): this is similar to common/vistautil LowIntegritySecurityDesc
// class but without atl dependency, used in several cases (like sandbox,
// sharedfile, sharedmem) which does not work well with ATL classes.
//
// See http://msdn.microsoft.com/en-us/library/bb250462(VS.85).aspx for more
// details on setting low integrity to system objects.
bool SetHandleLowIntegrity(HANDLE handle, SE_OBJECT_TYPE type) {
  static LPCWSTR kLowIntegritySddlSacl = L"S:(ML;;NW;;;LW)";
  PSECURITY_DESCRIPTOR security_desc = NULL;
  PACL acl = NULL;
  BOOL acl_present = FALSE;
  BOOL acl_defaulted = FALSE;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
      kLowIntegritySddlSacl, SDDL_REVISION_1, &security_desc, NULL)) {
    DLOG(ERROR) << "fail to convert security descriptor: " << GetLastError();
    return false;
  }
  bool ret = true;
  if (!GetSecurityDescriptorSacl(security_desc, &acl_present,
                                 &acl, &acl_defaulted)) {
    DLOG(ERROR) << "fail to get security descriptor sacl: " << GetLastError();
    ret = false;
  }
  if (ret && SetSecurityInfo(handle, type, LABEL_SECURITY_INFORMATION,
                             NULL, NULL, NULL, acl) != ERROR_SUCCESS) {
    DLOG(ERROR) << "fail to set security info: " << GetLastError();
    ret = false;
  }
  LocalFree(security_desc);
  return ret;
}

}  // namespace engine
}  // namespace i18n_input
