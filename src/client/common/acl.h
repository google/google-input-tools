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

#ifndef GOOPY_COMMON_ACL_H__
#define GOOPY_COMMON_ACL_H__

#include <windows.h>
#include "base/scoped_ptr.h"
#include "common/sid.h"

namespace ime_goopy {

// Returns the default dacl from the token passed in.
bool GetDefaultDacl(HANDLE token, scoped_ptr<TOKEN_DEFAULT_DACL>* default_dacl);

// Appends an ACE represented by |sid| and |access| to |old_dacl|. If the
// function succeeds, new_dacl contains the new dacl and must be freed using
// LocalFree.
bool AddSidToDacl(const Sid& sid, ACL* old_dacl, ACCESS_MASK access,
                  ACL** new_dacl);

// Adds and ACE represented by |sid| and |access| to the default dacl present
// in the token.
bool AddSidToDefaultDacl(HANDLE token, const Sid& sid, ACCESS_MASK access);

// Adds an ACE represented by the user sid and |access| to the default dacl
// present in the token.
bool AddUserSidToDefaultDacl(HANDLE token, ACCESS_MASK access);

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_ACL_H__
