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

#include "common/google_search_utils.h"

#include "common/search_setting/ie_reg_keys.h"
#include "common/branding.h"
#include "common/charsetutils.h"
#include "common/shellutils.h"

namespace ime_goopy {

std::wstring GoogleSearchUtils::GenerateSearchUrl(const std::wstring& query) {
  Branding branding;
  return search_setting::kIE6SearchAssistantValueDataGoogleCN +
      CharsetUtils::Unicode2UTF8Escaped(query) +
      L"&sourceid=ime-win&ie=UTF-8&hl=zh-CN" +
      L"&brand=" + std::wstring(branding.GetBrandCode()) +
      L"&rlz=" + std::wstring(branding.GetRlzCode());
}

// |query|: UTF8 encoded string.
std::wstring GoogleSearchUtils::GenerateSearchUrl(const std::string& query) {
  Branding branding;
  return search_setting::kIE6SearchAssistantValueDataGoogleCN +
      CharsetUtils::UTF8ToWstringEscaped(query.c_str(), query.length()) +
      L"&sourceid=ime-win&ie=UTF-8&hl=zh-CN" +
      L"&brand=" + std::wstring(branding.GetBrandCode()) +
      L"&rlz=" + std::wstring(branding.GetRlzCode());
}

std::wstring GoogleSearchUtils::GoogleHomepageUrl() {
  return search_setting::kIEHomepageValueDataGoogleCN;
}

void GoogleSearchUtils::Search(const std::string& query) {
  // It's not allowed to open explorer with a system account.
  if (ShellUtils::IsSystemAccount())
    return;
  std::wstring& url = GoogleSearchUtils::GenerateSearchUrl(query);
  ::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOW);
}

}  // namespace ime_goopy
