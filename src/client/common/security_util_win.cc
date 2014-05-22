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

#include "common/security_util_win.h"
#include <aclapi.h>
#include <sddl.h>
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/win/windows_version.h"
#include "common/shellutils.h"
// Define scoped string allocated by |LocalAlloc|.
DEFINE_SCOPED_HANDLE(ScopedLocalString, LPWSTR, LocalFree);

// Define scoped security descriptor allocated by |LocalAlloc|.
DEFINE_SCOPED_HANDLE(ScopedSecurityDescriptor, PSECURITY_DESCRIPTOR, LocalFree);

// Define scoped buffer allocated by |HeapAlloc(::GetProcessHeap(), 0, ...)|.
static inline void FreeProcessHeapBuffer(LPVOID buffer) {
  ::HeapFree(::GetProcessHeap(), 0, buffer);
}
DEFINE_SCOPED_HANDLE(ScopedProcessHeapBuffer, LPVOID, FreeProcessHeapBuffer);

// String format ACE item privilege.
static wchar_t kAllPrivilegesMask[] = L"0x1fffff";
static wchar_t kReadOnlyFileMappingPrivilegeMask[] = L"0x1ffff4";
static wchar_t kLowIntegrityPrivilege[] = L"S:(ML;;NWNR;;;LW)";

namespace ime_goopy {

bool SetPrivilege(HANDLE token_handle, LPCTSTR privilege, bool enable) {
  TOKEN_PRIVILEGES privileges;
  LUID luid;

  if (!::LookupPrivilegeValue(NULL, privilege, &luid))
    return false;

  privileges.PrivilegeCount = 1;
  privileges.Privileges[0].Luid = luid;
  privileges.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

  if (!::AdjustTokenPrivileges(
      token_handle,
      false,
      &privileges,
      sizeof(privileges),
      static_cast<PTOKEN_PRIVILEGES>(NULL),
      static_cast<PDWORD>(NULL))) {
    return false;
  }

  return true;
}

bool GetProcessSecurityInformation(LPTSTR* owner_sid_str,
                                   LPTSTR* grp_sid_str,
                                   LPTSTR* security_descriptor_str) {
  DCHECK(owner_sid_str);
  DCHECK(grp_sid_str);

  *owner_sid_str = NULL;
  *grp_sid_str = NULL;
  if (security_descriptor_str)
    *security_descriptor_str = NULL;

  SECURITY_INFORMATION security_information =
      OWNER_SECURITY_INFORMATION |
      GROUP_SECURITY_INFORMATION |
      DACL_SECURITY_INFORMATION;

  PSID psid_owner = NULL;
  PSID psid_group = NULL;
  PACL pdacl = NULL;

  ScopedSecurityDescriptor psecurity_descriptor;
  {
    PSECURITY_DESCRIPTOR psd = NULL;
    DWORD ret = ::GetSecurityInfo(::GetCurrentProcess(),
                                  SE_KERNEL_OBJECT,
                                  security_information,
                                  &psid_owner,
                                  &psid_group,
                                  &pdacl,
                                  NULL,
                                  &psd);
    if (ret != ERROR_SUCCESS) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << " GetSecurityInfo failed error = " << error;
      return false;
    }
    psecurity_descriptor.Reset(psd);
  }

  ScopedLocalString local_owner_sid;
  {
    LPWSTR sid_owner_str = NULL;
    if (!::ConvertSidToStringSid(psid_owner, &sid_owner_str)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << " ConvertSidToStringSid failed error = " << error;
      return false;
    }
    local_owner_sid.Reset(sid_owner_str);
  }

  ScopedLocalString local_group_sid;
  {
    LPWSTR sid_group_str = NULL;
    if (!::ConvertSidToStringSid(psid_group, &sid_group_str)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << " ConvertSidToStringSid failed error = " << error;
      return false;
    }
    local_group_sid.Reset(sid_group_str);
  }

  if (security_descriptor_str != NULL) {
    if (!::ConvertSecurityDescriptorToStringSecurityDescriptor(
        psecurity_descriptor,
        SDDL_REVISION_1,
        security_information,
        security_descriptor_str,
        NULL)) {
      DWORD error = ::GetLastError();
      DLOG(ERROR) << " ConvertSecurityDescriptorToStringSecurityDescriptor"
                  << " failed error = " << error;
      return false;
    }
  }
  *owner_sid_str = local_owner_sid.Detach();
  *grp_sid_str = local_group_sid.Detach();
  return true;
}

bool GetLogonSID(HANDLE token_handle, PSID *ppsid) {
  // Verify the parameter passed in is not NULL.
  if (!ppsid || token_handle == INVALID_HANDLE_VALUE)
    return false;
  *ppsid = NULL;

  HANDLE process_heap = ::GetProcessHeap();
  // Get required buffer size and allocate the TOKEN_GROUPS buffer.

  DWORD length = 0;
  ScopedProcessHeapBuffer local_token_groups;
  {
    LPVOID token_groups = NULL;
    if (!::GetTokenInformation(
        token_handle,
        TokenGroups,
        token_groups,
        0,
        &length)) {
      if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return false;

      token_groups =
          ::HeapAlloc(process_heap, HEAP_ZERO_MEMORY, length);
      if (!token_groups)
        return false;

      local_token_groups.Reset(token_groups);
    }
  }

  // Get the token group information from the access token.
  if (!::GetTokenInformation(
      token_handle,
      TokenGroups,
      local_token_groups.Get(),
      length,
      &length)) {
    return false;
  }

  // Loop through the groups to find the logon SID.
  TOKEN_GROUPS* token_groups =
      reinterpret_cast<TOKEN_GROUPS*>(local_token_groups.Get());
  for (DWORD index = 0; index < token_groups->GroupCount; ++index) {
    if ((token_groups->Groups[index].Attributes & SE_GROUP_LOGON_ID) ==
        SE_GROUP_LOGON_ID) {
      // Found the logon SID; make a copy of it.
      length = ::GetLengthSid(token_groups->Groups[index].Sid);

      ScopedProcessHeapBuffer psid(
          ::HeapAlloc(process_heap, HEAP_ZERO_MEMORY, length));
      if (!psid.Get())
        return false;

      if (!::CopySid(length, psid.Get(), token_groups->Groups[index].Sid))
        return false;

      *ppsid = psid.Detach();
      break;
    }
  }
  return true;
}

static bool GetIPCSecurityDescriptor(
    SECURITY_DESCRIPTOR** psd, bool filemap_readonly) {
  DCHECK(psd && !*psd);
  *psd = NULL;

  // Sid of logon sid of security object.
  ScopedLocalString local_logon_sid;
  {
    // Get process token with TOKEN_QUERY privilege.
    ScopedHandle local_token_handle;
    {
      HANDLE token_handle = NULL;
      if (!::OpenProcessToken(
          ::GetCurrentProcess(), TOKEN_QUERY, &token_handle)) {
        return false;
      }
      local_token_handle.Reset(token_handle);
    }

    // Query logoin sid from process token.
    ScopedProcessHeapBuffer local_psid;
    {
      PSID psid = NULL;
      if (!GetLogonSID(local_token_handle.Get(), &psid))
        return false;
      local_psid.Reset(psid);
    }

    LPWSTR logon_sid_str = NULL;
    if (!::ConvertSidToStringSid(local_psid.Get(), &logon_sid_str))
      return false;

    local_logon_sid.Reset(logon_sid_str);
  }

  // Sid of owner of security object.
  ScopedLocalString local_owner_sid;
  // Sid of group of security object.
  ScopedLocalString local_group_sid;
  {
    LPWSTR owner_sid_str = NULL;
    LPWSTR grp_sid_str = NULL;

    if (!GetProcessSecurityInformation(&owner_sid_str, &grp_sid_str, NULL))
      return false;
    local_owner_sid.Reset(owner_sid_str);
    local_group_sid.Reset(grp_sid_str);
  }

  std::wstring security_descriptor_str;

  // Add owner sid information.
  security_descriptor_str += L"O:";
  security_descriptor_str += local_owner_sid.Get();
  // Add group sid information.
  security_descriptor_str += L"G:";
  security_descriptor_str += local_group_sid.Get();
  security_descriptor_str += L"D:";
  if (ime_goopy::ShellUtils::CheckWindows8()) {
    // - Deny Remote Acccess
    // - Allow general access to LocalSystem
    // - Allow general access to Built-in Administorators
    // - Allow general access to the current user
    // - Allow general access to ALL APPLICATION PACKAGES
    // - Allow read/write access to low integrity
    security_descriptor_str += L"(D;;GA;;;NU)(A;;GA;;;SY)(A;;GA;;;BA)";
    security_descriptor_str += L"(A;;GA;;;AC)";
  }
  // Add generic & standand and all other possible object specific access
  // right to logon user.
  // set file mapping object specific
  // privilege to read-only if caller explicitly want a read-only file mapping.
  security_descriptor_str += L"(A;;";
  security_descriptor_str +=
      filemap_readonly ? kReadOnlyFileMappingPrivilegeMask : kAllPrivilegesMask;
  security_descriptor_str += L";;;";

  security_descriptor_str += local_logon_sid.Get();
  security_descriptor_str += L")";

  // Add low integrity label to support application environment like IE
  // protective mode if system supports(vista or latter).
  if (base::win::GetVersion() >= base::win::VERSION_VISTA)
    security_descriptor_str += kLowIntegrityPrivilege;

  // Convert the string to a security descriptor.
  if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(
      security_descriptor_str.c_str(),
      SDDL_REVISION_1,
      reinterpret_cast<PSECURITY_DESCRIPTOR*>(psd),
      NULL)) {
    DWORD error = ::GetLastError();
    DLOG(ERROR) << "ConvertStringSecurityDescriptorToSecurityDescriptor"
                << "failed error = " << error;
    return false;
  }
  return true;
}

bool GetIPCSecurityAttributes(SECURITY_ATTRIBUTES* security_attributes) {
  security_attributes->bInheritHandle = false;
  security_attributes->nLength = sizeof(*security_attributes);
  return GetIPCSecurityDescriptor(reinterpret_cast<SECURITY_DESCRIPTOR**>(
      &security_attributes->lpSecurityDescriptor), false);
}

bool GetIPCFileMapReadOnlySecurityAttributes(
    SECURITY_ATTRIBUTES* security_attributes) {
  security_attributes->bInheritHandle = false;
  security_attributes->nLength = sizeof(*security_attributes);
  return GetIPCSecurityDescriptor(reinterpret_cast<SECURITY_DESCRIPTOR**>(
          &security_attributes->lpSecurityDescriptor), true);
}

void ReleaseIPCSecurityAttributes(SECURITY_ATTRIBUTES* security_attributes) {
  if (security_attributes && security_attributes->lpSecurityDescriptor)
    ::LocalFree(security_attributes->lpSecurityDescriptor);
}

}  // namespace ime_goopy
