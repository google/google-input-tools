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

#ifndef GOOPY_BASE_WIN_SECURITY_UTIL_WIN_H_
#define GOOPY_BASE_WIN_SECURITY_UTIL_WIN_H_

#include <windows.h>

namespace base {

// Enable/Disable the privilege of token_handle.
//
// token_handle -> in, access_token to set privilege.
//
// privilege -> required|in, privilege to set.
//
// enable -> in, true to enable the privilege.
bool SetPrivilege(HANDLE token_handle,
                  LPCTSTR privilege,
                  bool enable);

// Query logon sid of an access token.
//
// token_handle -> in, access_token to query.
//
// ppsid -> required|output, contains a pointer to SID object.
// the returned sid should be deconstructed by calling
//
//   HeapFree(::GetProcessHeap(), 0, reinterpret_cast<LPVOID>(*ppsid))
//
// Return false if failed.
bool GetLogonSID(HANDLE token_handle, PSID* ppsid);

// Get string format owner sid, group sid and security descriptor of current
// process.
//
// owner_sid_str -> required|out, a pointer to LPTSTR type, should be
// deconstructed by calling |LocalFree| after usage.
//
// grp_sid_str -> required|out, a pointer to LPTSTR type,
// should be deconstructed by calling |LocalFree| after usage.
//
// security_descriptor_str -> optional|out, should be deconstructed by calling
// |LocalFree| after usage.
//
// Return false if failed.
bool GetProcessSecurityInformation(LPTSTR* owner_sid_str,
                                   LPTSTR* grp_sid_str,
                                   LPTSTR* security_descriptor_str);

// Set the security descriptor with privileges:
//   1) current logon user have generic access right.
//   2) low integrity on vista platform or latter.
//
// psa -> required|out, a pointer to in-stack structure of type
// SECURITY_ATTRIBUTES.
//
// Return false if failed.
bool GetIPCSecurityAttributes(SECURITY_ATTRIBUTES* psa);

// Set the security descriptor for creating a file mapping object with
// privileges:
// 1) Any sharing view (calling OpenFileMapping) has only read-only access to
// the object.
// 2) low integrity on vista platform or latter.
//
// psa -> required|out, a pointer to in-stack structure of type
// SECURITY_ATTRIBUTES.
//
// Return false if failed.
bool GetIPCFileMapReadOnlySecurityAttributes(SECURITY_ATTRIBUTES* psa);

// Release the resource of security attributes allocated in previous call of
//
// psa -> required|out, a pointer to in-stack structure of type
// SECURITY_ATTRIBUTES.
//
void ReleaseIPCSecurityAttributes(SECURITY_ATTRIBUTES* psa);

}  // namespace base

#endif  // GOOPY_BASE_WIN_SECURITY_UTIL_WIN_H_
