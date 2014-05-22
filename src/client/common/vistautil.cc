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

#include "base/logging.h"
#include "common/vistautil.h"
#include "common/registry.h"

#include <accctrl.h>
#include <Aclapi.h>
#include <Sddl.h>

namespace vista_util {

static SID_IDENTIFIER_AUTHORITY mandatory_label_auth =
    SECURITY_MANDATORY_LABEL_AUTHORITY;


static HRESULT GetSidIntegrityLevel(PSID sid, MANDATORY_LEVEL* level) {
  if (!IsValidSid(sid))
    return E_FAIL;

  SID_IDENTIFIER_AUTHORITY* authority = GetSidIdentifierAuthority(sid);
  if (!authority)
    return E_FAIL;

  if (memcmp(authority, &mandatory_label_auth,
      sizeof(SID_IDENTIFIER_AUTHORITY)))
    return E_FAIL;

  PUCHAR count = GetSidSubAuthorityCount(sid);
  if (!count || *count != 1)
    return E_FAIL;

  DWORD* rid = GetSidSubAuthority(sid, 0);
  if (!rid)
    return E_FAIL;

  if ((*rid & 0xFFF) != 0 || *rid > SECURITY_MANDATORY_PROTECTED_PROCESS_RID)
    return E_FAIL;

  *level = static_cast<MANDATORY_LEVEL>(*rid >> 12);
  return S_OK;
}

// Will return S_FALSE and MandatoryLevelMedium if the acl is NULL
static HRESULT GetAclIntegrityLevel(PACL acl, MANDATORY_LEVEL* level,
    bool* and_children) {
  *level = MandatoryLevelMedium;
  if (and_children)
    *and_children = false;
  if (!acl) {
    // This is the default label value if the acl was empty
    return S_FALSE;
  }

  SYSTEM_MANDATORY_LABEL_ACE* mandatory_label_ace;
  if (!GetAce(acl, 0, reinterpret_cast<void**>(&mandatory_label_ace)))
    return S_FALSE;

  if (mandatory_label_ace->Header.AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE)
    return S_FALSE;

  if (!(mandatory_label_ace->Mask & SYSTEM_MANDATORY_LABEL_NO_WRITE_UP)) {
    // I have found that if this flag is not set, a low integrity label doesn't
    // prevent writes from being virtualized.  MS provides zero documentation.
    // I just did an MSDN search, a Google search, and a search of the Beta
    // vista SDKs, and no docs. TODO(tw): Check docs again periodically
    // For now, act as if no label was set, and default to medium
    return S_FALSE;
  }

  if (and_children) {
    *and_children = ((mandatory_label_ace->Header.AceFlags &
        (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE))
        == (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE));
  }

  return GetSidIntegrityLevel(reinterpret_cast<SID*>(&mandatory_label_ace->
      SidStart), level);
}

// If successful, the caller needs to free the ACL using LocalFree()
// on failure, returns NULL
static ACL* CreateMandatoryLabelAcl(MANDATORY_LEVEL level, bool and_children) {
  int ace_size = sizeof(SYSTEM_MANDATORY_LABEL_ACE)
      - sizeof(DWORD) + GetSidLengthRequired(1);
  int acl_size = sizeof(ACL) + ace_size;

  ACL* acl = reinterpret_cast<ACL*>(LocalAlloc(LPTR, acl_size));
  if (!acl)
    return NULL;

  bool failed = true;
  if (InitializeAcl(acl, acl_size, ACL_REVISION)) {
    if (level > 0) {
      SYSTEM_MANDATORY_LABEL_ACE* ace = reinterpret_cast<
          SYSTEM_MANDATORY_LABEL_ACE*>(LocalAlloc(LPTR, ace_size));
      if (ace) {
        ace->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
        ace->Header.AceFlags = and_children ?
            (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE) : 0;
        ace->Header.AceSize = ace_size;
        ace->Mask = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP;

        SID* sid = reinterpret_cast<SID*>(&ace->SidStart);

        if (InitializeSid(sid, &mandatory_label_auth, 1)) {
          *GetSidSubAuthority(sid, 0) = static_cast<DWORD>(level) << 12;
          failed = !AddAce(acl, ACL_REVISION, 0, ace, ace_size);
        }
        LocalFree(ace);
      }
    }
  }
  if (failed) {
    LocalFree(acl);
    acl = NULL;
  }
  return acl;
}


TCHAR* AllocFullRegPath(HKEY root, const TCHAR* subkey) {
  if (!subkey)
    return NULL;

  const TCHAR* root_string;

  if (root == HKEY_CURRENT_USER)
    root_string = _T("CURRENT_USER\\");
  else if (root == HKEY_LOCAL_MACHINE)
    root_string = _T("MACHINE\\");
  else if (root == HKEY_CLASSES_ROOT)
    root_string = _T("CLASSES_ROOT\\");
  else if (root == HKEY_USERS)
    root_string = _T("USERS\\");
  else
    return NULL;

  size_t root_size = _tcslen(root_string);
  size_t size = root_size + _tcslen(subkey) + 1;
  TCHAR* result = reinterpret_cast<TCHAR*>(LocalAlloc(LPTR,
      size * sizeof(TCHAR)));
  if (!result)
    return NULL;

  memcpy(result, root_string, size * sizeof(TCHAR));
  memcpy(result + root_size, subkey, (1 + size - root_size) * sizeof(TCHAR));
  return result;
}




bool IsVistaOrLater()
{
  static bool known = false;
  static bool is_vista = false;
  if (!known) {
    OSVERSIONINFOEX osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    DWORDLONG conditional = 0;
    VER_SET_CONDITION(conditional, VER_MAJORVERSION, VER_GREATER_EQUAL);
    is_vista = !!VerifyVersionInfo(&osvi, VER_MAJORVERSION, conditional);
    // When the module is loaded in Visual Studio 6.0 SP6 in XP, GetVersion
    // API family doesn't work. (dwMajorVersion returned is 0x8f). So we need
    // to check the boundary in a reasonable range in case of such bug in VS6.
    osvi.dwMajorVersion = 10;
    conditional = 0;
    VER_SET_CONDITION(conditional, VER_MAJORVERSION, VER_LESS);
    is_vista =
        is_vista && !!VerifyVersionInfo(&osvi, VER_MAJORVERSION, conditional);
    known = true;
  }
  return is_vista;
}


bool IsUACDisabled() {
  ime_goopy::RegistryKey key;
  key.Open(
      HKEY_LOCAL_MACHINE,
      _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
      KEY_READ | KEY_WOW64_64KEY);
  DWORD value;
  LONG result = key.QueryDWORDValue(_T("EnableLUA"), value);
  return (result == ERROR_SUCCESS) && (value == 0);
}


HRESULT RunElevated(const TCHAR* file_path, const TCHAR* parameters,
                    HWND window, int show_window, DWORD* exit_code) {
  SHELLEXECUTEINFO shell_execute_info;
  shell_execute_info.cbSize = sizeof(SHELLEXECUTEINFO);
  shell_execute_info.fMask = SEE_MASK_FLAG_NO_UI;
  if (exit_code != NULL)
    shell_execute_info.fMask |= SEE_MASK_NOCLOSEPROCESS;
  shell_execute_info.hProcess = NULL;
  shell_execute_info.hwnd = window;
  shell_execute_info.lpVerb = L"runas";
  shell_execute_info.lpFile = file_path;
  shell_execute_info.lpParameters = parameters;
  shell_execute_info.lpDirectory = NULL;
  shell_execute_info.nShow = show_window;
  shell_execute_info.hInstApp = NULL;

  if (!ShellExecuteEx(&shell_execute_info))
    return AtlHresultFromLastError();

  // Wait for the end of the spawned process, if needed
  if (exit_code != NULL) {
    WaitForSingleObject(shell_execute_info.hProcess, INFINITE);
    DCHECK(0 != GetExitCodeProcess(shell_execute_info.hProcess, exit_code));
    CloseHandle(shell_execute_info.hProcess);
  }
  return S_OK;
}


HRESULT GetProcessIntegrityLevel(DWORD process_id, MANDATORY_LEVEL* level) {
  if (!IsVistaOrLater())
    return E_NOTIMPL;

  if (process_id == 0)
    process_id = GetCurrentProcessId();

  HRESULT result = E_FAIL;
  HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
  if (process != NULL) {
    HANDLE current_token;
    if (OpenProcessToken(process, TOKEN_QUERY | TOKEN_QUERY_SOURCE, &current_token)) {
      DWORD label_size = 0;
      TOKEN_MANDATORY_LABEL* label;
      GetTokenInformation(current_token, TokenIntegrityLevel,
          NULL, 0, &label_size);
      if (label_size && (label = reinterpret_cast<TOKEN_MANDATORY_LABEL*>
          (LocalAlloc(LPTR, label_size))) != NULL) {
        if (GetTokenInformation(current_token, TokenIntegrityLevel,
            label, label_size, &label_size)) {
          result = GetSidIntegrityLevel(label->Label.Sid, level);
        }
        LocalFree(label);
      }
      CloseHandle(current_token);
    }
    CloseHandle(process);
  }
  return result;
}


HRESULT GetFileOrFolderIntegrityLevel(const TCHAR* file,
    MANDATORY_LEVEL* level, bool* and_children) {
  if (!IsVistaOrLater())
    return E_NOTIMPL;

  PSECURITY_DESCRIPTOR descriptor;
  PACL acl = NULL;

  DWORD result = GetNamedSecurityInfo(const_cast<TCHAR*>(file), SE_FILE_OBJECT,
      LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, &acl, &descriptor);
  if (result != ERROR_SUCCESS)
    return HRESULT_FROM_WIN32(result);

#if 0 // TODO(Tw): Remove this -- I currently use if for debugging sometimes
  LPTSTR string = NULL;
  ULONG string_len = 0;
  ConvertSecurityDescriptorToStringSecurityDescriptor(descriptor,
     1, SACL_SECURITY_INFORMATION, &string, &string_len);
  if (string) {
     MessageBox(0, string, file, 0);
     LocalFree(string);
  }
#endif

  HRESULT hr = GetAclIntegrityLevel(acl, level, and_children);
  LocalFree(descriptor);
  return hr;
}


HRESULT SetFileOrFolderIntegrityLevel(const TCHAR* file,
    MANDATORY_LEVEL level, bool and_children) {
  if (!IsVistaOrLater())
    return E_NOTIMPL;

  ACL* acl = CreateMandatoryLabelAcl(level, and_children);
  if (!acl)
    return E_FAIL;

  DWORD result = SetNamedSecurityInfo(const_cast<TCHAR*>(file), SE_FILE_OBJECT,
      LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, acl);
  LocalFree(acl);
  return HRESULT_FROM_WIN32(result);
}


HRESULT GetRegKeyIntegrityLevel(HKEY root, const TCHAR* subkey,
    MANDATORY_LEVEL* level, bool* and_children) {
  if (!IsVistaOrLater())
    return E_NOTIMPL;

  TCHAR* reg_path = AllocFullRegPath(root, subkey);
  if (!reg_path)
    return E_FAIL;

  PSECURITY_DESCRIPTOR descriptor;
  PACL acl = NULL;

  DWORD result = GetNamedSecurityInfo(reg_path, SE_REGISTRY_KEY,
      LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, &acl, &descriptor);
  if (result != ERROR_SUCCESS) {
    LocalFree(reg_path);
    return HRESULT_FROM_WIN32(result);
  }

  HRESULT hr = GetAclIntegrityLevel(acl, level, and_children);
  LocalFree(descriptor);
  LocalFree(reg_path);
  return hr;
}


HRESULT SetRegKeyIntegrityLevel(HKEY root, const TCHAR* subkey,
    MANDATORY_LEVEL level, bool and_children) {
  if (!IsVistaOrLater())
    return E_NOTIMPL;

  TCHAR* reg_path = AllocFullRegPath(root, subkey);
  if (!reg_path)
    return E_FAIL;

  ACL* acl = CreateMandatoryLabelAcl(level, and_children);
  if (!acl) {
    LocalFree(reg_path);
    return E_FAIL;
  }

  DWORD result = SetNamedSecurityInfo(reg_path, SE_REGISTRY_KEY,
      LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, acl);
  LocalFree(acl);
  LocalFree(reg_path);
  return HRESULT_FROM_WIN32(result);
}


void GetSecurityAttributesWithEmptyAcl(SECURITY_ATTRIBUTES* attributes,
                                       SECURITY_DESCRIPTOR* descriptor,
                                       bool inherit_handle) {
  InitializeSecurityDescriptor(descriptor, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(descriptor, TRUE, NULL, FALSE);
  attributes->nLength = sizeof(*attributes);
  attributes->lpSecurityDescriptor = descriptor;
  attributes->bInheritHandle = inherit_handle;
}


LowIntegritySecurityDesc::LowIntegritySecurityDesc(ACCESS_MASK mask)
    : is_valid_(true) {
  // In the case of pre-vista, leave as the default (NULL) descriptor.
  if (!IsVistaOrLater())
    return;

  // Create this descriptor with a system access control list for the low
  // integrity level mandatory label and above. SACLs are normally used for
  // auditing, but Vista also uses them to determine integrity levels.
  // For more info, http://www.google.com/search?q=SDDL+for+Mandatory+Labels
  // SL = SACL
  // ML = Mandatory label (aka integrity level)
  // NW = No write up (integrity levels less than low cannot gain access)
  // LW = Low Integrity Level (What IE normally runs in)
  FromString(_T("S:(ML;;NW;;;LW)"));

  // Fill out the rest of the security descriptor from the process token.
  CAccessToken token;
  token.GetProcessToken(TOKEN_QUERY);

  // The owner.
  CSid sid_owner;
  if (!token.GetOwner(&sid_owner)) {
    is_valid_ = false;
    return;
  }
  SetOwner(sid_owner);

  // The group.
  CSid sid_group;
  token.GetPrimaryGroup(&sid_group);
  SetGroup(sid_group);

  // The discretionary access control list.
  CDacl dacl;
  token.GetDefaultDacl(&dacl);

  // Add an access control entry mask for the current user.
  // This is what grants this user access from lower integrity levels.
  CSid sid_user;
  token.GetUser(&sid_user);
  dacl.AddAllowedAce(sid_user, mask);

  // Lastly, save the dacl to this descriptor.
  SetDacl(dacl);
};

}; // namespace vista_util
