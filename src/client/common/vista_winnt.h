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

// This file contains snippets borrowed from the Vista SDK version of
// WinNT.h, (c) Microsoft (2006)

#ifndef BAR_COMMON_VISTA_WINNT_H__
#define BAR_COMMON_VISTA_WINNT_H__

//#include <windows.h>
//#include <tchar.h>
//#include <accctrl.h>
//#include <Aclapi.h>
//#include <Sddl.h>
//#include <WinNT.h>

// If no Vista SDK yet, borrow these from Vista's version of WinNT.h
#ifndef SE_GROUP_INTEGRITY

// TOKEN_MANDATORY_LABEL.Label.Attributes = SE_GROUP_INTEGRITY
#define SE_GROUP_INTEGRITY                 (0x00000020L)

typedef struct _TOKEN_MANDATORY_LABEL {
    SID_AND_ATTRIBUTES Label;
} TOKEN_MANDATORY_LABEL, *PTOKEN_MANDATORY_LABEL;

// These are 2 extra enums for TOKEN_INFORMATION_CLASS
// TokenIntegrityLevel is the proces's privilege level, low, med, or high
// TokenIntegrityLevelDeasktop is an alternate level used for access apis
// (screen readers, imes)
#define TokenIntegrityLevel static_cast<TOKEN_INFORMATION_CLASS>(25)
#define TokenIntegrityLevelDesktop static_cast<TOKEN_INFORMATION_CLASS>(26)

// This is a new flag to pass to GetNamedSecurityInfo or SetNamedSecurityInfo
// that puts the mandatory level label info in an access control list (ACL)
// structure in the parameter normally used for system acls (SACL)
#define LABEL_SECURITY_INFORMATION       (0x00000010L)

// The new Access Control Entry type identifier for mandatory labels
#define SYSTEM_MANDATORY_LABEL_ACE_TYPE         (0x11)

// The structure of mandatory label acess control binary entry
typedef struct _SYSTEM_MANDATORY_LABEL_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    DWORD SidStart;
} SYSTEM_MANDATORY_LABEL_ACE, *PSYSTEM_MANDATORY_LABEL_ACE;

// Masks for ACCESS_MASK above
#define SYSTEM_MANDATORY_LABEL_NO_WRITE_UP         0x1
#define SYSTEM_MANDATORY_LABEL_NO_READ_UP          0x2
#define SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP       0x4
#define SYSTEM_MANDATORY_LABEL_VALID_MASK \
    (SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | \
     SYSTEM_MANDATORY_LABEL_NO_READ_UP | \
     SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP)

// The SID authority for mandatory labels
#define SECURITY_MANDATORY_LABEL_AUTHORITY          {0,0,0,0,0,16}

// the RID values (sub authorities) that define mandatory label levels
#define SECURITY_MANDATORY_UNTRUSTED_RID            (0x00000000L)
#define SECURITY_MANDATORY_LOW_RID                  (0x00001000L)
#define SECURITY_MANDATORY_MEDIUM_RID               (0x00002000L)
#define SECURITY_MANDATORY_HIGH_RID                 (0x00003000L)
#define SECURITY_MANDATORY_SYSTEM_RID               (0x00004000L)
#define SECURITY_MANDATORY_UI_ACCESS_RID            (0x00004100L)
#define SECURITY_MANDATORY_PROTECTED_PROCESS_RID    (0x00005000L)

// Vista's mandatory labels, enumerated
typedef enum _MANDATORY_LEVEL {
    MandatoryLevelUntrusted = 0,
    MandatoryLevelLow,
    MandatoryLevelMedium,
    MandatoryLevelHigh,
    MandatoryLevelSystem,
    MandatoryLevelSecureProcess,
    MandatoryLevelCount
} MANDATORY_LEVEL, *PMANDATORY_LEVEL;

#endif // #ifndef SE_GROUP_INTEGRITY

#endif // #ifndef BAR_COMMON_VISTA_WINNT_H__
