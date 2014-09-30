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
#include "tsf/tsf_utils.h"

#include <windows.h>
#include <msctf.h>
#include "common/framework_interface.h"
#include "common/smart_com_ptr.h"
namespace ime_goopy {
namespace tsf {

void TSFUtils::SwitchToTIP(DWORD lang, const std::wstring& profile) {
  SmartComPtr<ITfInputProcessorProfileMgr> profiles;
  HRESULT hr = profiles.CoCreate(CLSID_TF_InputProcessorProfiles,
                                 NULL, CLSCTX_ALL);
  if (FAILED(hr)) return;
  GUID profile_guid = { 0 };
  ::CLSIDFromString(profile.c_str(), &profile_guid);
  hr = profiles->ActivateProfile(
      TF_PROFILETYPE_INPUTPROCESSOR,
      lang,
      InputMethod::kTextServiceClsid,
      profile_guid,
      NULL,
      // Use TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE to allow switching between
      // profiles with different languages.
      TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
}

std::wstring TSFUtils::GetCurrentLanguageProfile() {
  wchar_t layout[MAX_PATH] = { 0 };
  SmartComPtr<ITfInputProcessorProfiles> profiles;
  HRESULT hr = profiles.CoCreate(CLSID_TF_InputProcessorProfiles,
                                 NULL, CLSCTX_ALL);
  if (FAILED(hr)) return false;
  LANGID langid = 0;
  GUID profile = { 0 };
  profiles->GetActiveLanguageProfile(InputMethod::kTextServiceClsid, &langid,
                                     &profile);
  ::StringFromGUID2(profile, layout, MAX_PATH);
  return layout;
}

DWORD TSFUtils::GetCurrentLanguageId() {
  SmartComPtr<ITfInputProcessorProfiles> profiles;
  HRESULT hr = profiles.CoCreate(CLSID_TF_InputProcessorProfiles,
                                 NULL, CLSCTX_ALL);
  if (FAILED(hr)) return false;
  LANGID langid = 0;
  GUID profile = { 0 };
  profiles->GetActiveLanguageProfile(InputMethod::kTextServiceClsid, &langid,
                                     &profile);
  return langid;
}

}  // namespace tsf
}  // namespace ime_goopy
