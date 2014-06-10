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

#include "tsf/registrar.h"

#include <msctf.h>

#include "base/stringprintf.h"
#include "common/atl.h"
#include "common/framework_interface.h"
#include "common/goopy_utils.h"
#include "common/shellutils.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
namespace tsf {

static const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT = {
    0x13A016DF, 0x560B, 0x46CD, {0x94, 0x7A, 0x4C,0x3A,0xF1,0xE0,0xE3,0x5D}};

static const GUID kCategories[] = {
  GUID_TFCAT_TIP_KEYBOARD,              // It's a keyboard input method.
  GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,  // It supports inline input.
  GUID_TFCAT_TIPCAP_UIELEMENTENABLED,   // It supports UI less mode.
  // COM less is required for some applications, like WOW. With this category,
  // we are not allowed to use CoCreateInstance in our ime.
  GUID_TFCAT_TIPCAP_COMLESS,
};

Registrar::Registrar(const CLSID& clsid) : clsid_(clsid) {
}

HRESULT Registrar::Register(const wstring& base_filename,
                            LANGID language_id,
                            const GUID& profile_guid,
                            const wstring& display_name,
                            HKL hkl) {
  wchar_t icon_path[MAX_PATH] = {0}, system_path[MAX_PATH] = {0};
  GetSystemDirectory(system_path, MAX_PATH);
  PathCombine(icon_path, system_path, base_filename.c_str());

  if (ShellUtils::GetOS() == ShellUtils::OS_WINDOWSVISTA &&
      ShellUtils::Is64BitOS()) {
    // Fix bug http://b.corp.google.com/issue?id=6830794
    // In windows vista, when we register COM by setting registry value:
    //   HKEY_CLASS_ROOT\\kTextServiceRegistryKey\\InprocServer32\\
    // to the path of ime file:
    //   c:\\windows\\system32\\GooglePinyin2.ime
    // The system will automatically replace "system32" to "syswow64" as our
    // installer is 32bit. We need to fix this path here.
    DWORD flags = KEY_READ | KEY_WRITE | KEY_WOW64_64KEY;
    CString subkey;
    wchar_t guid[MAX_PATH] = {0};
    ::StringFromGUID2(clsid_, guid, MAX_PATH);
    subkey.Format(L"CLSID\\%s\\%s", guid, L"InprocServer32");
    CRegKey registry;
    if (registry.Open(HKEY_CLASSES_ROOT, subkey, flags) == ERROR_SUCCESS) {
      registry.SetStringValue(NULL, icon_path);
      registry.Close();
    }
  }
  // Register categories.
  // Due to GUID_TFCAT_TIPCAP_COMLESS, we can not use CoCreateInstance in the
  // text service, but we can use it here because this function will only be
  // called from regsvr32.exe.
  SmartComPtr<ITfCategoryMgr> category_manager;
  HRESULT hr = category_manager.CoCreate(CLSID_TF_CategoryMgr,
                                         NULL, CLSCTX_ALL);
  if (FAILED(hr)) return hr;
  for (int i = 0; i < arraysize(kCategories); i++) {
    hr = category_manager->RegisterCategory(clsid_, kCategories[i], clsid_);
    if (FAILED(hr)) return hr;
  }
  if (ShellUtils::GetOS() == ShellUtils::OS_WINDOWS8) {
    hr = category_manager->RegisterCategory(
        clsid_, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT, clsid_);
    if (FAILED(hr)) return hr;
    SmartComPtr<ITfInputProcessorProfileMgr> profile_manager;
    hr = profile_manager.CoCreate(CLSID_TF_InputProcessorProfiles,
                                  NULL, CLSCTX_ALL);
    if (FAILED(hr)) return hr;
    hr = profile_manager->RegisterProfile(
        clsid_,
        language_id,
        profile_guid,
        display_name.c_str(),
        display_name.length(),
        icon_path,
        wcslen(icon_path),
        0,                            // uIconIndex
        hkl,                          // hklsubstitue
        0,                            // dwPreferredLayout
        TRUE,                         // bEnabledByDefault
        0);                           // dwFlags
    return hr;
  } else {
    SmartComPtr<ITfInputProcessorProfiles> profiles;
    HRESULT hr = profiles.CoCreate(CLSID_TF_InputProcessorProfiles,
                                   NULL, CLSCTX_ALL);
    if (FAILED(hr)) return hr;

    // Register the text service.
    hr = profiles->Register(clsid_);
    if (FAILED(hr)) return hr;


    // Register profiles.
    hr = profiles->AddLanguageProfile(
        clsid_, language_id, profile_guid,
        display_name.c_str(), -1, icon_path, -1, 0);
    if (FAILED(hr)) return hr;
    if (hkl) {
      hr = profiles->SubstituteKeyboardLayout(
          clsid_, language_id, profile_guid, hkl);
      if (FAILED(hr)) return hr;
    }
    return hr;
  }
}

HRESULT Registrar::Unregister(LANGID language_id,
                              const GUID& profile_guid) {
  // Unregister categories.
  SmartComPtr<ITfCategoryMgr> category_manager;
  HRESULT hr = category_manager.CoCreate(CLSID_TF_CategoryMgr,
                                         NULL, CLSCTX_ALL);
  if (SUCCEEDED(hr)) {
    SmartComPtr<IEnumGUID> guids;
    hr = category_manager->EnumCategoriesInItem(clsid_, guids.GetAddress());
    GUID guid = {0};
    ULONG fetched = 0;
    if (SUCCEEDED(hr)) {
      while (guids->Next(1, &guid, &fetched) == S_OK) {
        category_manager->UnregisterCategory(clsid_, guid, clsid_);
      }
    }
  }

  // Unregister profiles and text service.
  SmartComPtr<ITfInputProcessorProfiles> profiles;
  hr = profiles.CoCreate(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_ALL);
  if (SUCCEEDED(hr)) {
    profiles->SubstituteKeyboardLayout(clsid_, language_id, profile_guid, NULL);
    profiles->Unregister(clsid_);
  }

  return hr;
}

bool Registrar::IsInstalled() {
  SmartComPtr<ITfInputProcessorProfiles> profiles;
  HRESULT hr = profiles.CoCreate(CLSID_TF_InputProcessorProfiles,
                                 NULL, CLSCTX_ALL);
  if (FAILED(hr)) return false;
  SmartComPtr<IEnumGUID> guids;
  hr = profiles->EnumInputProcessorInfo(guids.GetAddress());
  if (FAILED(hr)) return false;
  GUID guid = {0};
  ULONG fetched = 0;
  while (guids->Next(1, &guid, &fetched) == S_OK) {
    if (guid == clsid_) return true;
  }
  return false;
}

}  // namespace tsf
}  // namespace ime_goopy
