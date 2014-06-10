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
// This helper classes provide convienient functions to search on google.

#ifndef GOOPY_FRONTEND_GOOGLE_SEARCH_UTILS_H_
#define GOOPY_FRONTEND_GOOGLE_SEARCH_UTILS_H_
#include <string>
#include "base/basictypes.h"

namespace ime_goopy {
class GoogleSearchUtils {
 public:
  static std::string GoogleHomepageUrl();
  // Generates search url for utf8 encoded string.
  static std::string GenerateSearchUrl(const std::string& query);
  // Opens google search for keyword |query|.
  static void Search(const std::string& query);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GoogleSearchUtils);
};
}  // namespace ime_goopy
#endif  // GOOPY_FRONTEND_GOOGLE_SEARCH_UTILS_H_
