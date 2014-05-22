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

#include "components/common/mock_data_manager.h"

#include "i18n/input/engine/t13n/public/custom_token_dictionary_interface.h"
#include "i18n/input/engine/t13n/public/language_alphabet_interface.h"
#include "i18n/input/engine/t13n/public/user_cache_interface.h"
#include "i18n/input/engine/t13n/public/user_dictionary_interface.h"

#include "base/logging.h"

namespace {
using i18n_input::engine::t13n::UserDictionaryInterface;
using i18n_input::engine::t13n::CustomTokenDictionaryInterface;
using i18n_input::engine::t13n::UserCacheInterface;
using i18n_input::engine::t13n::LanguageAlphabetInterface;
using i18n_input::engine::t13n::CanonicalSchemeLoadResult;
}  // namespace

namespace ime_goopy {
namespace components {

MockDataManager::MockDataManager() {
}

MockDataManager::~MockDataManager() {
}

bool MockDataManager::EnrollData(const string& decoder_id,
                                 const string& model_id,
                                 const string& model_location) {
  return true;
}

bool MockDataManager::EnrollDataByFileDescriptor(const string& decoder_id,
                                                 const string& model_id,
                                                 int file_descriptor,
                                                 int offset,
                                                 int length) {
  return false;
}

CustomTokenDictionaryInterface* MockDataManager::GetCustomTokenDictionary(
    const string& decoder_id) {
  return NULL;
}

UserCacheInterface* MockDataManager::GetUserCache(const string& decoder_id) {
  return NULL;
}

UserDictionaryInterface* MockDataManager::GetUserDictionary(
    const string& decoder_id) {
  return NULL;
}

const LanguageAlphabetInterface*
MockDataManager::GetLanguageAlphabetForTargetLanguage(
    const string& decoder_id) {
  return NULL;
}

bool MockDataManager::LoadCanonicalScheme(const string& decoder_id,
                                          const string& model_location,
                                          CanonicalSchemeLoadResult* result) {
  result->scheme_id = model_location;
  result->scheme_display_name = model_location;
  return true;
}

}  // namespace components
}  // namespace ime_goopy
