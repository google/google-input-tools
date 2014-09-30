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
//
// Based on the source code of Chromium.
// Original code: chromimutrunk/sandbox/src/acl.*

#include "common/acl.h"
#include <aclapi.h>
#include <sddl.h>
#include "base/logging.h"

namespace ime_goopy {

bool GetDefaultDacl(HANDLE token,
                    scoped_ptr<TOKEN_DEFAULT_DACL>* default_dacl) {
  if (token == NULL)
    return false;

  DCHECK(default_dacl != NULL);

  unsigned long length = 0;
  ::GetTokenInformation(token, TokenDefaultDacl, NULL, 0, &length);
  if (length == 0) {
    NOTREACHED();
    return false;
  }

  TOKEN_DEFAULT_DACL* acl =
      reinterpret_cast<TOKEN_DEFAULT_DACL*>(new char[length]);
  default_dacl->reset(acl);

  if (!::GetTokenInformation(token, TokenDefaultDacl, default_dacl->get(),
                             length, &length))
      return false;

  return true;
}

bool AddSidToDacl(const Sid& sid, ACL* old_dacl, ACCESS_MASK access,
                  ACL** new_dacl) {
  EXPLICIT_ACCESS new_access = {0};
  new_access.grfAccessMode = GRANT_ACCESS;
  new_access.grfAccessPermissions = access;
  new_access.grfInheritance = NO_INHERITANCE;

  new_access.Trustee.pMultipleTrustee = NULL;
  new_access.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  new_access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
  new_access.Trustee.ptstrName = reinterpret_cast<LPWSTR>(
                                    const_cast<SID*>(sid.GetPSID()));

  if (ERROR_SUCCESS != ::SetEntriesInAcl(1, &new_access, old_dacl, new_dacl))
    return false;

  return true;
}

bool AddSidToDefaultDacl(HANDLE token, const Sid& sid, ACCESS_MASK access) {
  if (token == NULL)
    return false;

  scoped_ptr<TOKEN_DEFAULT_DACL> default_dacl;
  if (!GetDefaultDacl(token, &default_dacl))
    return false;

  ACL* new_dacl = NULL;
  if (!AddSidToDacl(sid, default_dacl->DefaultDacl, access, &new_dacl))
    return false;

  TOKEN_DEFAULT_DACL new_token_dacl = {0};
  new_token_dacl.DefaultDacl = new_dacl;

  BOOL ret = ::SetTokenInformation(token, TokenDefaultDacl, &new_token_dacl,
                                   sizeof(new_token_dacl));
  ::LocalFree(new_dacl);
  return (TRUE == ret);
}

bool AddUserSidToDefaultDacl(HANDLE token, ACCESS_MASK access) {
  DWORD size = sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE;
  TOKEN_USER* token_user = reinterpret_cast<TOKEN_USER*>(new BYTE[size]);

  scoped_ptr<TOKEN_USER> token_user_ptr(token_user);

  if (!::GetTokenInformation(token, TokenUser, token_user, size, &size))
    return false;

  return AddSidToDefaultDacl(token,
                             reinterpret_cast<SID*>(token_user->User.Sid),
                             access);
}

}  // namespace ime_goopy
