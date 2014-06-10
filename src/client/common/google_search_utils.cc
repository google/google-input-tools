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

#include "base/string_utils_win.h"
#include "common/charsetutils.h"
#include "common/shellutils.h"

namespace ime_goopy {
static const char kUrl[] = "http://www.google.com/search?q=";

// |query|: UTF8 encoded string.
std::string GoogleSearchUtils::GenerateSearchUrl(const std::string& query) {
  return kUrl +
      WideToUtf8(CharsetUtils::UTF8ToWstringEscaped(query.c_str(), query.length())) +
      "&sourceid=ime-win&ie=UTF-8&hl=zh-CN";
}

std::string GoogleSearchUtils::GoogleHomepageUrl() {
  return "http://www.google.com/";
}

void GoogleSearchUtils::Search(const std::string& query) {
  // It's not allowed to open explorer with a system account.
  if (ShellUtils::IsSystemAccount())
    return;
  std::string& url = GoogleSearchUtils::GenerateSearchUrl(query);
  ::ShellExecute(NULL, NULL, Utf8ToWide(url).c_str(), NULL, NULL, SW_SHOW);
}

}  // namespace ime_goopy
