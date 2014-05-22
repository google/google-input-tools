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

#include <comutil.h>
#include <iphlpapi.h>
#include <msxml2.h>
#include <shlwapi.h>
#include <wincrypt.h>

#include "shared/closed/financial/rlz/win/lib/rlz_lib.h"
#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/branding.h"
#include "common/registry.h"
#include "common/shellutils.h"

namespace ime_goopy {
static const int kMaxNICCount = 16;
static const int kMaxMACStringLength = 256;
static const int kHashedMacLength = 16;
static const TCHAR* kDefaultBrandCode = _T("GGPY");
static const int kGuidStringLength = 32;

static CString CreateStringGuid() {
  GUID guid;
  CoCreateGuid(&guid);

  CString id;
  id.Format(_T("%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X"),
      guid.Data1, guid.Data2, guid.Data3,
      guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
      guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
  return id;
}

Branding::Branding()
    : user_registry_(AppUtils::OpenUserRegistry()),
      system_registry_(AppUtils::OpenSystemRegistry(false)) {
  // Failed to open system registry as writable, try readonly.
  if (!system_registry_.get())
    system_registry_.reset(AppUtils::OpenSystemRegistry(true));
}

Branding::~Branding() {
}


CString Branding::GetUserId() {
  if (!user_registry_.get()) return L"";
  wstring id;
  if (user_registry_->QueryStringValue(kUserIdName, &id) == ERROR_SUCCESS)
    return id.c_str();

  // guid didn't exist, first time to use, generate a guid for user.
  CString new_id = CreateStringGuid();
  user_registry_->SetStringValue(kUserIdName, new_id);
  return new_id;
}

CString Branding::GetMachineGuid() {
  if (!system_registry_.get()) return L"";
  wstring guid;
  if (system_registry_->QueryStringValue(kMachineGuidName,
                                         &guid) == ERROR_SUCCESS)
    if (guid.size() == kGuidStringLength) return guid.c_str();

  CString new_id = CreateStringGuid();
  // This only success when running as administrator.
  if (system_registry_->SetStringValue(kMachineGuidName,
                                       new_id) == ERROR_SUCCESS) {
    return new_id;
  }
  return L"";
}

CString Branding::GetBrandCode() {
  if (!system_registry_.get()) return kDefaultBrandCode;
  wstring value;
  if (system_registry_->QueryStringValue(kBrandCodeName,
                                         &value) == ERROR_SUCCESS)
    return value.c_str();
  return kDefaultBrandCode;
}

CString Branding::GetOSVersion() {
  OSVERSIONINFO version_info;
  version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!GetVersionEx(&version_info)) return _T("");
  CString version;
  version.Format(_T("%d.%d"),
      version_info.dwMajorVersion, version_info.dwMinorVersion);

  // Appends CPU arch info to the OS version info.
  if (ShellUtils::Is64BitOS())
    version += _T("_x64");
  else
    version += _T("_x86");
  return version;
}

CString Branding::GetRlzCode() {
  WCHAR rlz_string[rlz_lib::kMaxRlzLength + 1] = { L'\0' };
  if (rlz_lib::GetAccessPointRlz(rlz_lib::GOOPY_IME,
                                 rlz_string, ARRAYSIZE(rlz_string)) &&
      rlz_string[0] != L'\0') {
    return CString(rlz_string);
  }
  return L"";
}

CString Branding::GetLegacyRlzCode() {
  if (!system_registry_.get()) return L"";
  wstring rlz_base;
  system_registry_->QueryStringValue(kRlzName, &rlz_base);
  CString rlz = rlz_base.c_str();
  if (rlz.IsEmpty()) {
    return rlz;
  }
  int rlz_weeks = GetRlzWeeks();
  if (rlz_weeks > 0)
    rlz.AppendFormat(L"%d", rlz_weeks);
  return rlz;
}

int Branding::GetRlzWeeks() {
  if (!system_registry_.get()) return -1;
  // Secs of Feb 3, 2003, used by GetRlzWeeks. See
  // http://wiki.corp.google.com/twiki/bin/view/Pso/RlzHumanReadable
  static const int kRlzWeeksBaseSecs = 1044201600;
  // Total secs per week.
  static const int kSecsPerWeek = 3600 * 24 * 7;
  DWORD value = 0;
  if (system_registry_->QueryDWORDValue(kInstallTimeName,
                                        value) != ERROR_SUCCESS)
    return -1;
  int secs = static_cast<int>(value) - kRlzWeeksBaseSecs;
  if (secs < 0)
    return -1;
  else
    return secs / kSecsPerWeek;
}

CString Branding::GetHashedMacAddress() {
  char hashed_mac[kMaxMACStringLength] = { 0 };
  IP_ADAPTER_INFO adapter_info[kMaxNICCount];
  DWORD buf_len = sizeof(adapter_info);
  // Get the first NIC mac address.
  if (GetAdaptersInfo(adapter_info, &buf_len) == ERROR_SUCCESS) {
    PIP_ADAPTER_INFO adapter_info_ptr = adapter_info;
    char mac_address[kMaxMACStringLength] = { 0 };
    for (UINT i = 0; i < adapter_info_ptr->AddressLength; i++) {
      sprintf(mac_address + i * 2, "%02x", adapter_info_ptr->Address[i]);
    }

    // Hash the mac address.
    HCRYPTPROV crypt_provider = NULL;
    if(CryptAcquireContext(&crypt_provider, NULL, MS_DEF_PROV,
                           PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
      HCRYPTHASH hash = NULL;
      if (CryptCreateHash(crypt_provider, CALG_SHA1, NULL, 0, &hash)) {
        if (CryptHashData(hash, reinterpret_cast<BYTE*>(mac_address),
                          sizeof(mac_address), 0)) {
          DWORD size = 0;
          BYTE buffer[kMaxMACStringLength] = { 0 };
          // Get hash size.
          if (CryptGetHashParam(hash, HP_HASHVAL, NULL, &size, 0)) {
            // Get hash value.
            if (CryptGetHashParam(hash, HP_HASHVAL, buffer, &size, 0)) {
              for (int i = 0; i < kHashedMacLength; i++)
                sprintf(hashed_mac + i * 2, "%02x", buffer[i]);
            }
          }
        }
      }
      if (hash) CryptDestroyHash(hash);
    }
    if (crypt_provider) CryptReleaseContext(crypt_provider, 0);
  }
  return hashed_mac;
}
}   // namespace ime_goopy
